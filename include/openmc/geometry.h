#ifndef OPENMC_GEOMETRY_H
#define OPENMC_GEOMETRY_H

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "openmc/particle.h"


namespace openmc {

//==============================================================================
// Global variables
//==============================================================================

namespace model {

#pragma omp declare target
extern int root_universe;  //!< Index of root universe
#pragma omp end declare target
extern "C" int n_coord_levels; //!< Number of CSG coordinate levels

extern std::vector<int64_t> overlap_check_count;

} // namespace model

//==============================================================================
//! Check two distances by coincidence tolerance
//==============================================================================

#pragma omp declare target
inline bool coincident(double d1, double d2) {
  return std::abs(d1 - d2) < FP_COINCIDENT;
}
#pragma omp end declare target

//==============================================================================
//! Check for overlapping cells at a particle's position.
//==============================================================================

bool check_cell_overlap(Particle& p, bool error=true);

//==============================================================================
//! Locate a particle in the geometry tree and set its geometry data fields.
//!
//! \param p A particle to be located.  This function will populate the
//!   geometry-dependent data fields of the particle.
//! \return True if the particle's location could be found and ascribed to a
//!   valid geometry coordinate stack.
//==============================================================================
#pragma omp declare target
bool exhaustive_find_cell(Particle& p);
bool neighbor_list_find_cell(Particle& p); // Only usable on surface crossings
#pragma omp end declare target

//==============================================================================
//! Move a particle into a new lattice tile.
//==============================================================================

#pragma omp declare target
void cross_lattice(Particle& p, const BoundaryInfo& boundary);
#pragma omp end declare target

//==============================================================================
//! Find the next boundary a particle will intersect.
//==============================================================================

#pragma omp declare target
BoundaryInfo distance_to_boundary(Particle& p);
#pragma omp end declare target

} // namespace openmc

#endif // OPENMC_GEOMETRY_H
