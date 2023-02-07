// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_UTILS_METER_JetsonEnergyMeter_HPP_
#define IntelliStream_SRC_UTILS_METER_JetsonEnergyMeter_HPP_
#include "Utils/Meter/AbstractEnergyMeter.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <thread>
#include <vector>

namespace GAMM {
class JetsonEnergyMeter : public AbstractEnergyMeter {
public:
  JetsonEnergyMeter(Duration samplingInterval = Duration(5))
      : AbstractEnergyMeter(samplingInterval), readings{samplingInterval} {
    init();
  }

  ~JetsonEnergyMeter() { stopSampling(); }

  virtual void startSampling() override;
  virtual Readings stopSampling() override;

  static bool canBeUsed();

private:
  void init();
  static void collectData(std::shared_ptr<std::atomic_bool>,
                          JetsonEnergyMeter *);

  int currentFd, voltageFd;
  Readings readings;
  std::optional<std::thread> samplingThread;
  std::shared_ptr<std::atomic<bool>> shouldStop;
};
} // namespace GAMM
#endif
