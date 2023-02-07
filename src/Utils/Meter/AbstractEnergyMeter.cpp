#include "Utils/Logger.hpp"
#include <Utils/Meter/AbstractEnergyMeter.hpp>
#include <cassert>
#include <cstddef>
#include <fstream>

using namespace GAMM;

double AbstractEnergyMeter::Readings::energyConsumed() const noexcept {
  assert(current.size() == voltage.size());
  double cumulative_energy{};
  for (auto i = std::begin(current), v = std::begin(voltage);
       i != std::end(current); ++i, ++v) {
    cumulative_energy += (double)(*i * *v * samplingInterval.count()) / 1e9;
  }
  return cumulative_energy;
}

void AbstractEnergyMeter::Readings::writeCSV(const char *path) const {
  std::ofstream f{path, std::ios::out};

  if (f.bad()) {
    INTELLI_ERROR("Failed to open file " << path);
    return;
  }

  assert(current.size() == voltage.size());

  double cumulative_energy{};

  f << "Time (ms), Voltage (mV), Current (mA), Power (W),"
       "Cumulative Energy (J)\n";

  for (size_t i = 0; i != current.size(); ++i) {
    size_t time_ms = i * samplingInterval.count();
    auto v = voltage[i];
    auto c = current[i];
    auto power = (double)(v * c) / 1e6;
    cumulative_energy += power * (double)samplingInterval.count() / 1e3;
    f << time_ms << ", " << v << ", " << c << ", " << power << ", "
      << cumulative_energy << std::endl;
  }
}
