#ifndef LEANMD_HPP
#define LEANMD_HPP

#include <ck/ck.hpp>

#include "physics.hpp"

class Cell : public ck::chare<Cell, ck::array<CkIndex3D>> {
 private:
  std::vector<Particle> particles;      // list of atoms
  std::vector<CkIndex6D> computesList;  // my compute locations
  int stepCount;   // to count the number of steps, and decide when to stop
  int myNumParts;  // number of atoms in my cell
  int inbrs;       // number of interacting neighbors
  int updateCount;
  int forceCount;

  void migrateToCell(Particle p, int &px, int &py, int &pz);
  // updates properties after receiving forces from computes
  void updateProperties(void);
  void limitVelocity(Particle &p);  // limit velcities to an upper limit
  // particles going out of right enters from left
  Particle &wrapAround(Particle &p);

 public:
  using parent_t = ck::chare<Cell, ck::array<CkIndex3D>>;

  Cell(void);
  Cell(CkMigrateMessage *msg) : parent_t(msg) {}

  void pup(PUP::er &p);
  void createComputes(void);  // add my computes
  void migrateParticles(void);
  void sendPositions(void);
  void addForces(vec3 *forces);
  void receiveParticles(std::vector<Particle> &&particles);
};

class Compute : public ck::chare<Compute, ck::array<CkIndex6D>> {
 private:
  int stepCount;  // current step number

 public:
  using parent_t = ck::chare<Compute, ck::array<CkIndex6D>>;
  Compute(void);
  Compute(CkMigrateMessage *msg) : parent_t(msg) {}
  void calculateForces(const CkIndex3D &index, int stepCount,
                       ck::span<vec3> &&positions);
  void pup(PUP::er &p);
};

class Main : public ck::chare<Main, ck::main_chare> {
 public:
  using parent_t = ck::chare<Main, ck::main_chare>;
  Main(CkArgMsg *msg);
  Main(CkMigrateMessage *msg) : parent_t(msg) {}
  void pup(PUP::er &p);
  void computesCreated(void);
};

CK_READONLY(int, cellArrayDimX);
CK_READONLY(int, cellArrayDimY);
CK_READONLY(int, cellArrayDimZ);
CK_READONLY(int, finalStepCount);

CK_READONLY(ck::chare_proxy<Main>, mainProxy);
CK_READONLY(ck::array_proxy<Cell>, cellArray);
CK_READONLY(ck::array_proxy<Compute>, computeArray);

#endif
