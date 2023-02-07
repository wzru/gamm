#include <Utils/Meter/JetsonEnergyMeter.hpp>
#include <atomic>
#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <memory>

using namespace GAMM;

constexpr auto SYS_BASE = "/sys/bus/i2c/drivers/ina3221/1-0040/hwmon";

void JetsonEnergyMeter::init() {
  if (!std::filesystem::is_directory(SYS_BASE)) {
    throw "No energy meter found";
  }
  auto entry = *std::filesystem::directory_iterator{SYS_BASE};

  auto voltagePath = entry.path() / "in2_input";
  if (!std::filesystem::exists(voltagePath)) {
    throw "Unable to open voltage file";
  };
  voltageFd = open(voltagePath.c_str(), O_RDONLY);

  auto currentPath = entry.path() / "curr2_input";
  if (!std::filesystem::exists(voltagePath)) {
    throw "Unable to open current file";
  };
  currentFd = open(currentPath.c_str(), O_RDONLY);
}

bool JetsonEnergyMeter::canBeUsed() {
  return std::filesystem::is_directory(SYS_BASE);
}

uint32_t read(int fd) {
  char buff[256];
  int nread;
  nread = read(fd, buff, 256);
  buff[nread] = 0;
  double ru = std::atof(buff);
  // printf("%s\r\n",buff);
  return ru;
}

void JetsonEnergyMeter::collectData(
    std::shared_ptr<std::atomic_bool> shouldStop, JetsonEnergyMeter *meter) {
  auto samplingInterval = meter->samplingInterval;
  auto voltageFd = meter->voltageFd;
  auto currentFd = meter->currentFd;

  JetsonEnergyMeter::Readings readings{samplingInterval};

  while (!shouldStop->load(std::memory_order_relaxed)) {
    auto voltageReading = read(voltageFd);
    auto currentReading = read(currentFd);

    readings.voltage.push_back(voltageReading);
    readings.current.push_back(currentReading);

    std::this_thread::sleep_for(samplingInterval);
  }

  meter->readings = std::move(readings);
}

void JetsonEnergyMeter::startSampling() {
  if (samplingThread.has_value()) {
    return;
  }
  shouldStop->store(false, std::memory_order_relaxed);
  samplingThread.emplace(std::thread(collectData, shouldStop, this));
}

JetsonEnergyMeter::Readings JetsonEnergyMeter::stopSampling() {
  if (!samplingThread.has_value()) {
    return {samplingInterval};
  }
  shouldStop->store(true, std::memory_order_relaxed);
  samplingThread.value().join();
  samplingThread.reset();

  return readings;
}
