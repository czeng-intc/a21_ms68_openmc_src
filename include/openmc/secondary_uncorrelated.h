//! \file secondary_uncorrelated.h
//! Uncorrelated angle-energy distribution

#ifndef OPENMC_SECONDARY_UNCORRELATED_H
#define OPENMC_SECONDARY_UNCORRELATED_H

#include <memory>
#include <vector>

#include "hdf5.h"

#include "openmc/angle_energy.h"
#include "openmc/distribution_angle.h"
#include "openmc/distribution_energy.h"
#include "openmc/secondary_flat.h"

namespace openmc {

//==============================================================================
//! Uncorrelated angle-energy distribution. This corresponds to when an energy
//! distribution is given in ENDF File 5/6 and an angular distribution is given
//! in ENDF File 4.
//==============================================================================

class UncorrelatedAngleEnergy : public AngleEnergy {
public:
  explicit UncorrelatedAngleEnergy(hid_t group);

  //! Sample distribution for an angle and energy
  //! \param[in] E_in Incoming energy in [eV]
  //! \param[out] E_out Outgoing energy in [eV]
  //! \param[out] mu Outgoing cosine with respect to current direction
  //! \param[inout] seed Pseudorandom seed pointer
  void sample(double E_in, double& E_out, double& mu, uint64_t* seed) const override;

  void serialize(DataBuffer& buffer) const override;

  // Accessors
  AngleDistribution& angle() { return angle_; }
  bool& fission() { return fission_; }
private:
  AngleDistribution angle_; //!< Angle distribution
  std::unique_ptr<EnergyDistribution> energy_; //!< Energy distribution
  bool fission_ {false}; //!< Whether distribution is use for fission
};

class UncorrelatedAngleEnergyFlat {
public:
  #pragma omp declare target
  explicit UncorrelatedAngleEnergyFlat(const uint8_t* data) : data_(data) { }

  //! Sample distribution for an angle and energy
  //! \param[in] E_in Incoming energy in [eV]
  //! \param[out] E_out Outgoing energy in [eV]
  //! \param[out] mu Outgoing cosine with respect to current direction
  //! \param[inout] seed Pseudorandom seed pointer
  void sample(double E_in, double& E_out, double& mu, uint64_t* seed) const;
  #pragma omp end declare target

  AngleDistributionFlat angle() const;
  EnergyDistributionFlat energy() const;
  bool fission() const;
  void set_fission(bool fission);
private:
  const uint8_t* data_;
};

} // namespace openmc

#endif // OPENMC_SECONDARY_UNCORRELATED_H
