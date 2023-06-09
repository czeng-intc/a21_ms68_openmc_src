#include "openmc/urr.h"

#include <iostream>

namespace openmc {

UrrData::UrrData(hid_t group_id)
{
  // Read interpolation and other flags
  int interp_temp;
  read_attribute(group_id, "interpolation", interp_temp);
  interp_ = static_cast<Interpolation>(interp_temp);

  // read the metadata
  read_attribute(group_id, "inelastic", inelastic_flag_);
  read_attribute(group_id, "absorption", absorption_flag_);
  int temp_multiply_smooth;
  read_attribute(group_id, "multiply_smooth", temp_multiply_smooth);
  multiply_smooth_ = (temp_multiply_smooth == 1);

  // read the energies at which tables exist
  read_dataset(group_id, "energy", energy_);

  // Set n_energy_
  n_energy_ = energy_.shape()[0];

  // Read URR tables
  read_dataset(group_id, "table", prob_);
}

void UrrData::flatten_urr_data()
{
  device_energy_ = energy_.data();
  device_prob_ = prob_.data();
  n_bands_ = prob_.shape(2);
  n_total_prob_ = n_energy_ * 6 * n_bands_;
}

double UrrData::prob(int i_energy, URRTableParam i_tableparam, int band) const
{
  return device_prob_[i_energy * 6 * n_bands_ + static_cast<int>(i_tableparam) * n_bands_ + band];
}

}
