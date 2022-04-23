#ifndef LEANMD_HPP
#define LEANMD_HPP

#include <ck/ck.hpp>

#include "physics.hpp"

class Cell : public ck::chare<Cell, ck::array<CkIndex3D>> {
 private:
  std::vector<Particle> particles;  // list of atoms
  int **computesList;               // my compute locations
  int stepCount;   // to count the number of steps, and decide when to stop
  int myNumParts;  // number of atoms in my cell
  int inbrs;       // number of interacting neighbors
  int updateCount;
  int forceCount;

  void migrateToCell(Particle p, int &px, int &py, int &pz);
  void updateProperties();  // updates properties after receiving forces from
                            // computes
  void limitVelocity(Particle &p);  // limit velcities to an upper limit
  Particle &wrapAround(
      Particle &p);  // particles going out of right enters from left

 public:
  using parent_t = ck::chare<Cell, ck::array<CkIndex3D>>;

  Cell();
  Cell(CkMigrateMessage *msg) : parent_t(msg) {}

  void pup(PUP::er &p);
  void createComputes();  // add my computes
  void migrateParticles();
  void sendPositions();
  void addForces(vec3 *forces);
};

class Compute : public ck::chare<Compute, ck::array<CkIndex6D>> {
 private:
  int stepCount;  // current step number

 public:
  using parent_t = ck::chare<Compute, ck::array<CkIndex6D>>;
  Compute();
  Compute(CkMigrateMessage *msg) : parent_t(msg) {}
  void pup(PUP::er &p);
};

class Main : public ck::chare<Main, ck::main_chare> {
 public:
  using parent_t = ck::chare<Main, ck::main_chare>;
  Main(CkArgMsg *msg);
  Main(CkMigrateMessage *msg) : parent_t(msg) {}
  void pup(PUP::er &p);
};

CK_READONLY(int, cellArrayDimX);
CK_READONLY(int, cellArrayDimY);
CK_READONLY(int, cellArrayDimZ);
CK_READONLY(int, finalStepCount);

#endif
