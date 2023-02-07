// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_UTILS_METER_AbstractEnergyMeter_HPP_
#define IntelliStream_SRC_UTILS_METER_AbstractEnergyMeter_HPP_
#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace GAMM {
class AbstractEnergyMeter {
public:
  struct Readings;

  using Duration = std::chrono::milliseconds;

  AbstractEnergyMeter(Duration duration) : samplingInterval{duration} {}
  AbstractEnergyMeter(const AbstractEnergyMeter &) = delete;
  ~AbstractEnergyMeter() {}

  virtual void startSampling() = 0;
  virtual Readings stopSampling() = 0;

  struct Readings {
    std::vector<uint32_t> current, voltage;
    Duration samplingInterval;

    Readings(Duration samplingInterval) : samplingInterval{samplingInterval} {}

    double energyConsumed() const noexcept;
    void writeCSV(const char *path) const;
  };

  Duration samplingInterval;
};

typedef std::shared_ptr<AbstractEnergyMeter> EnergyMeterPtr;
} // namespace GAMM
#endif
