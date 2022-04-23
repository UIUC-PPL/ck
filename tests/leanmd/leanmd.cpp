#include "leanmd.hpp"

#include "rand48_replacement.h"

Main::Main(CkArgMsg* m) {
  CkPrintf("\nLENNARD JONES MOLECULAR DYNAMICS START UP ...\n");

  // set variable values to a default set
  finalStepCount = DEFAULT_FINALSTEPCOUNT;
  cellArrayDimX = CELLARRAY_DIM_X;
  cellArrayDimY = CELLARRAY_DIM_Y;
  cellArrayDimZ = CELLARRAY_DIM_Z;

  mainProxy = thisProxy;

  int numPes = CkNumPes();
  int cur_arg = 1;

  CkPrintf("\nInput Parameters...\n");

  // read user parameters
  // number of cells in each dimension
  if (m->argc > cur_arg) {
    cellArrayDimX = atoi(m->argv[cur_arg++]);
    cellArrayDimY = atoi(m->argv[cur_arg++]);
    cellArrayDimZ = atoi(m->argv[cur_arg++]);
    CkPrintf("Cell Array Dimension X:%d Y:%d Z:%d of size %d %d %d\n",
             cellArrayDimX, cellArrayDimY, cellArrayDimZ, CELL_SIZE_X,
             CELL_SIZE_Y, CELL_SIZE_Z);
  }

  // number of steps in simulation
  if (m->argc > cur_arg) {
    finalStepCount = atoi(m->argv[cur_arg++]);
    CkPrintf("Final Step Count:%d\n", finalStepCount);
  }

  // initializing the 6D compute array
  computeArray = ck::array_proxy<Compute>::create();

  // intialize the options for the cell array
  ck::constructor_options<Cell> c_opts(
      CkArrayIndex3D(cellArrayDimX, cellArrayDimY, cellArrayDimZ));
  CkEntryOptions e_opts;
  e_opts.addGroupDepID((CkGroupID)computeArray.ckGetArrayID());
  c_opts.e_opts = &e_opts;

  // initializing the 3D cell array
  cellArray = ck::array_proxy<Cell>::create(c_opts);

  CkPrintf("\nCells: %d X %d X %d .... created\n", cellArrayDimX, cellArrayDimY,
           cellArrayDimZ);
}

void Main::computesCreated(void) {
  computeArray.doneInserting();
  CkPrintf("Starting simulation .... \n\n");
  // cellArray.run();
  // computeArray.run();
}

Compute::Compute() {
  stepCount = 1;
  usesAtSync = true;
}

Cell::Cell(void) {
  int i;
  inbrs = NUM_NEIGHBORS;
  // load balancing to be called when AtSync is called
  usesAtSync = true;

  int myid = thisIndex.x + thisIndex.y * cellArrayDimX +
             thisIndex.z * cellArrayDimX * cellArrayDimY;
  myNumParts = PARTICLES_PER_CELL_START +
               (myid * (PARTICLES_PER_CELL_END - PARTICLES_PER_CELL_START)) /
                   (cellArrayDimX * cellArrayDimY * cellArrayDimZ);

  // starting random generator
  srand48(thisIndex.x +
          cellArrayDimX * (thisIndex.y + thisIndex.z * cellArrayDimY));

  // Particle initialization
  for (i = 0; i < myNumParts; i++) {
    particles.push_back(Particle());
    particles[i].mass = HYDROGEN_MASS;

    // give random values for position and velocity
    particles[i].pos.x = drand48() * CELL_SIZE_X + thisIndex.x * CELL_SIZE_X;
    particles[i].pos.y = drand48() * CELL_SIZE_Y + thisIndex.y * CELL_SIZE_Y;
    particles[i].pos.z = drand48() * CELL_SIZE_Z + thisIndex.z * CELL_SIZE_Z;
    particles[i].vel.x = (drand48() - 0.5) * .2 * MAX_VELOCITY;
    particles[i].vel.y = (drand48() - 0.5) * .2 * MAX_VELOCITY;
    particles[i].vel.z = (drand48() - 0.5) * .2 * MAX_VELOCITY;
    particles[i].force.x = 0.0;
    particles[i].force.y = 0.0;
    particles[i].force.z = 0.0;
  }

  stepCount = 1;
  updateCount = 0;
  forceCount = 0;

  this->createComputes();
}

// function to create my computes
void Cell::createComputes() {
  int x = thisIndex.x;
  int y = thisIndex.y;
  int z = thisIndex.z;

  // For Round Robin insertion
  int numPes = CkNumPes();
  int currPe = CkMyPe();

  computesList.resize(inbrs);

  /*  The computes X are inserted by a given cell:
   *
   *	^  X  X  X
   *	|  0  X  X
   *	y  0  0  0
   *	   x ---->
   */

  for (auto num = 0; num < inbrs; num++) {
    auto dx = num / (NBRS_Y * NBRS_Z) - NBRS_X / 2;
    auto dy = (num % (NBRS_Y * NBRS_Z)) / NBRS_Z - NBRS_Y / 2;
    auto dz = num % NBRS_Z - NBRS_Z / 2;

    auto construct = [](auto& idx, int x1, int y1, int z1, int dx, int dy,
                        int dz) {
      new (&idx) CkIndex6D{
          .x1 = (short)x1,
          .y1 = (short)y1,
          .z1 = (short)z1,
          .x2 = (short)(x1 + dx),
          .y2 = (short)(y1 + dy),
          .z2 = (short)(z1 + dz),
      };
    };

    if (num >= inbrs / 2) {
      auto& idx = computesList[num];
      construct(idx, x + 2, y + 2, z + 2, dx, dy, dz);
      computeArray[idx].insert();  // TODO ( pass PE as argument )
    } else {
      // these computes will be created by pairing cells
      construct(computesList[num], WRAP_X(x + dx) + 2, WRAP_Y(y + dy) + 2,
                WRAP_Z(z + dz) + 2, -dx, -dy, -dz);
    }
  }  // end of for loop
  auto cb = ck::make_callback<&Main::computesCreated>(mainProxy);
  contribute(cb);
}

void Cell::sendPositions() {
  // create the particle and control message to be sent to computes
  int len = particles.size();
  std::shared_ptr<vec3[]> positions(new vec3[len]);
  std::transform(particles.begin(), particles.end(), positions.get(),
                 [](const Particle& particle) { return particle.pos; });
  ck::span<vec3> span(std::move(positions), len);

  for (int num = 0; num < inbrs; num++) {
    computeArray[computesList[num]].send<&Compute::calculateForces>(
        thisIndex, stepCount, span);
  }
}

// send the atoms that have moved beyond my cell to neighbors
void Cell::migrateParticles() {
  std::vector<std::vector<Particle>> outgoing(inbrs);

  for (int i = 0; i < particles.size(); i++) {
    int x1, y1, z1;
    migrateToCell(particles[i], x1, y1, z1);
    if (x1 != 0 || y1 != 0 || z1 != 0) {
      outgoing[(x1 + 1) * NBRS_Y * NBRS_Z + (y1 + 1) * NBRS_Z + (z1 + 1)]
          .push_back(wrapAround(particles[i]));
      particles.erase(particles.begin() + i);
    }
  }

  for (int num = 0; num < inbrs; num++) {
    auto x1 = num / (NBRS_Y * NBRS_Z) - NBRS_X / 2;
    auto y1 = (num % (NBRS_Y * NBRS_Z)) / NBRS_Z - NBRS_Y / 2;
    auto z1 = num % NBRS_Z - NBRS_Z / 2;
    cellArray[CkArrayIndex3D(WRAP_X(thisIndex.x + x1), WRAP_Y(thisIndex.y + y1),
                             WRAP_Z(thisIndex.z + z1))]
        .send<&Cell::receiveParticles>(outgoing[num]);
  }
}

// check if the particle is to be moved
void Cell::migrateToCell(Particle p, int& px, int& py, int& pz) {
  // currently this is assuming that particles are
  // migrating only to the immediate neighbors
  int x = thisIndex.x * CELL_SIZE_X + CELL_ORIGIN_X;
  int y = thisIndex.y * CELL_SIZE_Y + CELL_ORIGIN_Y;
  int z = thisIndex.z * CELL_SIZE_Z + CELL_ORIGIN_Z;
  px = py = pz = 0;

  if (p.pos.x < x)
    px = -1;
  else if (p.pos.x > x + CELL_SIZE_X)
    px = 1;

  if (p.pos.y < y)
    py = -1;
  else if (p.pos.y > y + CELL_SIZE_Y)
    py = 1;

  if (p.pos.z < z)
    pz = -1;
  else if (p.pos.z > z + CELL_SIZE_Z)
    pz = 1;
}

// Function to update properties (i.e. acceleration, velocity and position) in
// particles
void Cell::updateProperties() {
  int i;
  double powTen, powFteen, realTimeDelta, invMassParticle;
  powTen = pow(10.0, -10);
  powFteen = pow(10.0, -15);
  realTimeDelta = DEFAULT_DELTA * powFteen;
  for (i = 0; i < particles.size(); i++) {
    // applying kinetic equations
    invMassParticle = 1 / particles[i].mass;
    particles[i].acc = particles[i].force * invMassParticle;
    particles[i].vel += particles[i].acc * realTimeDelta;

    limitVelocity(particles[i]);

    particles[i].pos += particles[i].vel * realTimeDelta;
    particles[i].force.x = 0.0;
    particles[i].force.y = 0.0;
    particles[i].force.z = 0.0;
  }
  forceCount = 0;
}

void Cell::addForces(vec3* forces) {
  // updating force information
  for (int i = 0; i < particles.size(); i++) {
    particles[i].force += forces[i];
  }
}

inline double velocityCheck(double inVelocity) {
  if (fabs(inVelocity) > MAX_VELOCITY) {
    if (inVelocity < 0.0)
      return -MAX_VELOCITY;
    else
      return MAX_VELOCITY;
  } else {
    return inVelocity;
  }
}

void Cell::limitVelocity(Particle& p) {
  p.vel.x = velocityCheck(p.vel.x);
  p.vel.y = velocityCheck(p.vel.y);
  p.vel.z = velocityCheck(p.vel.z);
}

Particle& Cell::wrapAround(Particle& p) {
  if (p.pos.x < CELL_ORIGIN_X) p.pos.x += CELL_SIZE_X * cellArrayDimX;
  if (p.pos.y < CELL_ORIGIN_Y) p.pos.y += CELL_SIZE_Y * cellArrayDimY;
  if (p.pos.z < CELL_ORIGIN_Z) p.pos.z += CELL_SIZE_Z * cellArrayDimZ;
  if (p.pos.x > CELL_ORIGIN_X + CELL_SIZE_X * cellArrayDimX)
    p.pos.x -= CELL_SIZE_X * cellArrayDimX;
  if (p.pos.y > CELL_ORIGIN_Y + CELL_SIZE_Y * cellArrayDimY)
    p.pos.y -= CELL_SIZE_Y * cellArrayDimY;
  if (p.pos.z > CELL_ORIGIN_Z + CELL_SIZE_Z * cellArrayDimZ)
    p.pos.z -= CELL_SIZE_Z * cellArrayDimZ;
  return p;
}

void Cell::receiveParticles(std::vector<Particle>&& particles) {}

void Compute::calculateForces(const CkIndex3D& index, int stepCount,
                              ck::span<vec3>&& positions) {}

void Cell::pup(PUP::er& p) {
  p | particles;
  p | computesList;
  p | stepCount;
  p | myNumParts;
  p | inbrs;
  p | updateCount;
  p | forceCount;
}

void Compute::pup(PUP::er& p) { p | stepCount; }

void Main::pup(PUP::er& p) {}

extern "C" void CkRegisterMainModule(void) { ck::__register(); }
