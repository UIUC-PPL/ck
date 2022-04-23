#ifndef LEANMD_PHYSICS_HPP
#define LEANMD_PHYSICS_HPP

#include <ck/ck.hpp>

#define BLOCK_SIZE 512

#define HYDROGEN_MASS (1.67 * pow(10.0, -24))
#define VDW_A (1.60694452 * pow(10.0, -134))
#define VDW_B (1.031093844 * pow(10.0, -77))

#define ENERGY_VAR (1.0 * pow(10.0, -5))
// average of next two should be what you want as you atom density
#define PARTICLES_PER_CELL_START 300
#define PARTICLES_PER_CELL_END 300

#define DEFAULT_DELTA 1  // in femtoseconds

#define DEFAULT_FIRST_LDB 20
#define DEFAULT_LDB_PERIOD 20
#define DEFAULT_FT_PERIOD 100000

#define KAWAY_X 1
#define KAWAY_Y 1
#define KAWAY_Z 1
#define NBRS_X (2 * KAWAY_X + 1)
#define NBRS_Y (2 * KAWAY_Y + 1)
#define NBRS_Z (2 * KAWAY_Z + 1)
#define NUM_NEIGHBORS (NBRS_X * NBRS_Y * NBRS_Z)

#define CELLARRAY_DIM_X 3
#define CELLARRAY_DIM_Y 3
#define CELLARRAY_DIM_Z 3
#define PTP_CUT_OFF 12  // cut off for atom to atom interactions
#define CELL_MARGIN 4   // constant diff between cutoff and cell size
#define CELL_SIZE_X (PTP_CUT_OFF + CELL_MARGIN) / KAWAY_X
#define CELL_SIZE_Y (PTP_CUT_OFF + CELL_MARGIN) / KAWAY_Y
#define CELL_SIZE_Z (PTP_CUT_OFF + CELL_MARGIN) / KAWAY_Z
#define CELL_ORIGIN_X 0
#define CELL_ORIGIN_Y 0
#define CELL_ORIGIN_Z 0

#define MIGRATE_STEPCOUNT 20
#define DEFAULT_FINALSTEPCOUNT 1001
#define MAX_VELOCITY 30.0

#define WRAP_X(a) (((a) + cellArrayDimX) % cellArrayDimX)
#define WRAP_Y(a) (((a) + cellArrayDimY) % cellArrayDimY)
#define WRAP_Z(a) (((a) + cellArrayDimZ) % cellArrayDimZ)

CK_EXTERN_READONLY(int, cellArrayDimX);
CK_EXTERN_READONLY(int, cellArrayDimY);
CK_EXTERN_READONLY(int, cellArrayDimZ);
CK_EXTERN_READONLY(int, finalStepCount);

struct vec3 {
  double x, y, z;

  vec3(double d = 0.0) : x(d), y(d), z(d) {}
  vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

  inline vec3 &operator+=(const vec3 &rhs) {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
  }
  inline vec3 &operator-=(const vec3 &rhs) { return *this += (rhs * -1.0); }
  inline vec3 operator*(const double d) const {
    return vec3(d * x, d * y, d * z);
  }
  inline vec3 operator-(const vec3 &rhs) const {
    return vec3(x - rhs.x, y - rhs.y, z - rhs.z);
  }
};

inline double dot(const vec3 &a, const vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

struct Particle {
  double mass;
  vec3 pos, acc, vel, force;
};

PUPbytes(vec3);
PUPbytes(Particle);

void sendForces(const CkIndex3D &index, int stepCount,
                ck::span<vec3> &&positions);

// function to calculate forces among 2 lists of atoms
inline void calcPairForces(int stepCount, const CkIndex3D &first,
                           const ck::span<vec3> &firstPos,
                           const CkIndex3D &second,
                           const ck::span<vec3> &secondPos) {
  int i, j, jpart, ptpCutOffSqd, diff;
  int firstLen = firstPos.size();
  int secondLen = secondPos.size();
  double powTwenty, powTen, r, rsqd, f, fr;
  vec3 separation, force;
  double rSix, rTwelve;

  vec3 *firstmsg = new vec3[firstLen];
  vec3 *secondmsg = new vec3[secondLen];
  // check for wrap around and adjust locations accordingly
  if (abs(first.x - second.x) > 1) {
    diff = CELL_SIZE_X * cellArrayDimX;
    if (second.x < first.x) diff = -1 * diff;
    for (i = 0; i < firstLen; i++) firstPos[i].x += diff;
  }
  if (abs(first.y - second.y) > 1) {
    diff = CELL_SIZE_Y * cellArrayDimY;
    if (second.y < first.y) diff = -1 * diff;
    for (i = 0; i < firstLen; i++) firstPos[i].y += diff;
  }
  if (abs(first.z - second.z) > 1) {
    diff = CELL_SIZE_Z * cellArrayDimZ;
    if (second.z < first.z) diff = -1 * diff;
    for (i = 0; i < firstLen; i++) firstPos[i].z += diff;
  }
  ptpCutOffSqd = PTP_CUT_OFF * PTP_CUT_OFF;
  powTen = pow(10.0, -10);
  powTwenty = pow(10.0, -20);

  int i1, j1;
  for (i1 = 0; i1 < firstLen; i1 = i1 + BLOCK_SIZE)
    for (j1 = 0; j1 < secondLen; j1 = j1 + BLOCK_SIZE)
      for (i = i1; i < i1 + BLOCK_SIZE && i < firstLen; i++) {
        for (jpart = j1; jpart < j1 + BLOCK_SIZE && jpart < secondLen;
             jpart++) {
          separation = firstPos[i] - secondPos[jpart];
          rsqd = dot(separation, separation);
          if (rsqd >= 0.001 && rsqd < ptpCutOffSqd) {
            rsqd = rsqd * powTwenty;
            r = sqrt(rsqd);
            rSix = ((double)rsqd) * rsqd * rsqd;
            rTwelve = rSix * rSix;
            f = (double)(VDW_A / rTwelve - VDW_B / rSix);
            fr = f / r;
            force = separation * (fr * powTen);
            firstmsg[i] += force;
            secondmsg[jpart] -= force;
          }
        }
      }

  sendForces(first, stepCount, ck::span<vec3>(firstmsg, firstLen));
  sendForces(second, stepCount, ck::span<vec3>(secondmsg, secondLen));
}

// function to calculate forces among atoms in a single list
inline void calcInternalForces(int stepCount, const CkIndex3D &first,
                               const ck::span<vec3> &firstPos) {
  int i, j, ptpCutOffSqd;
  int firstLen = firstPos.size();
  double powTwenty, powTen, firstx, firsty, firstz, rx, ry, rz, r, rsqd, fx, fy,
      fz, f, fr;
  vec3 firstpos, separation, force;
  double rSix, rTwelve;
  vec3 *firstmsg = new vec3[firstLen];

  ptpCutOffSqd = PTP_CUT_OFF * PTP_CUT_OFF;
  powTen = pow(10.0, -10);
  powTwenty = pow(10.0, -20);
  for (i = 0; i < firstLen; i++) {
    firstpos = firstPos[i];
    for (j = i + 1; j < firstLen; j++) {
      // computing base values
      separation = firstpos - firstPos[j];
      rsqd = dot(separation, separation);
      if (rsqd >= 0.001 && rsqd < ptpCutOffSqd) {
        rsqd = rsqd * powTwenty;
        r = sqrt(rsqd);
        rSix = ((double)rsqd) * rsqd * rsqd;
        rTwelve = rSix * rSix;
        f = (double)(VDW_A / rTwelve - VDW_B / rSix);
        fr = f / r;
        force = separation * (fr * powTen);
        firstmsg[j] += force;
        firstmsg[i] -= force;
      }
    }
  }
  sendForces(first, stepCount, ck::span<vec3>(firstmsg, firstLen));
}

#endif
