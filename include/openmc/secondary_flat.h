//! \file secondary_flat.h
//! Unified angle-energy distribution

#ifndef OPENMC_SECONDARY_FLAT_H
#define OPENMC_SECONDARY_FLAT_H

#include "openmc/angle_energy.h"
#include "openmc/serialize.h"

namespace openmc {

enum class AngleEnergyType {
  UNCORRELATED,
  KALBACH_MANN,
  CORRELATED,
  NBODY,
  COHERENT_ELASTIC,
  INCOHERENT_ELASTIC,
  INCOHERENT_ELASTIC_DISCRETE,
  INCOHERENT_INELASTIC,
  INCOHERENT_INELASTIC_DISCRETE
};

class AngleEnergyFlat {
public:
  // Constructors
  explicit AngleEnergyFlat(const AngleEnergy& dist);
  explicit AngleEnergyFlat(DataBuffer buffer);

  //! Sample distribution for an angle and energy
  //! \param[in] E_in Incoming energy in [eV]
  //! \param[out] E_out Outgoing energy in [eV]
  //! \param[out] mu Outgoing cosine with respect to current direction
  //! \param[inout] seed Pseudorandom seed pointer
  #pragma omp declare target
  void sample(double E_in, double& E_out, double& mu, uint64_t* seed) const;
  #pragma omp end declare target

  void copy_to_device() const;
  void release_device() const;

  const uint8_t* data() const { return buffer_.data_; }
  AngleEnergyType type() const { return type_; }
private:
  // Data members
  AngleEnergyType type_;
  DataBuffer buffer_;
};

} // namespace openmc

#endif // OPENMC_SECONDARY_FLAT_H