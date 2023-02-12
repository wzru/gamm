// Copyright (C) 2021 by the IntelliStream team
// (https://github.com/intellistream)

/**
 * @brief This is the main entry point of the entire program.
 * We use this as the entry point for benchmarking.
 */
#include <BS_thread_pool.hpp>
#include <iomanip>
#include <memory>
#include <optional>

#include <Amm/Bamm.hpp>
#include <Amm/IntraParallel.hpp>
#include <Amm/Single.hpp>
#include <Utils/Config.hpp>
#include <Utils/Logger.hpp>
#include <Utils/Meter/AbstractEnergyMeter.hpp>
#include <Utils/Meter/JetsonEnergyMeter.hpp>
#include <Utils/UtilityFunctions.hpp>

void runFunction(std::string_view name, GAMM::BammUPtr bamm,
                 const GAMM::MatrixPtr &x, const GAMM::MatrixPtr &y,
                 const GAMM::Matrix &z,
                 std::optional<GAMM::EnergyMeterPtr> energyMeter);

int main(int argc, char **argv) {
  // Setup Logs.
  setupLogging("benchmark.log", LOG_INFO);

  const GAMM::Config config{argc, argv};

  INTELLI_INFO(config);

  auto res = config.loadMatrices();
  if (!res.has_value()) {
    INTELLI_INFO("Error loading matrices in config. " << argc << argv);
  }

  auto [x, y] = res.value();

  INTELLI_INFO("Loaded matrices x(" << x->rows() << ", " << x->cols()
                                    << ") and y(" << y->rows() << ", "
                                    << y->cols() << ')');

  INTELLI_INFO("x: " << (x->block<2, 2>(0, 0)));
  INTELLI_INFO("y: " << (y->block<2, 2>(0, 0)));

  BS::timer tmr;
  tmr.start();
  const GAMM::Matrix z = *x * y->transpose();
  tmr.stop();

  std::cout << "Lib-MM " << tmr.ms() << "ms\n";
  INTELLI_INFO("z: " << (z.block<2, 2>(0, 0)));

  std::optional<GAMM::EnergyMeterPtr> energyMeter{};

  // TODO have a measure energy flag
  if (GAMM::JetsonEnergyMeter::canBeUsed() && false) {
    INTELLI_INFO("Using Jetson Energy Meter");
    energyMeter = std::make_shared<GAMM::JetsonEnergyMeter>();
  } else {
    INTELLI_INFO("No energy meter");
  }

  // if (config.bins.single) {
  //   runFunction("single-threaded",
  //               std::make_unique<GAMM::Single>(config.l, config.beta), x, y,
  //               z, energyMeter);
  // }

  if (config.bins.intra) {
    runFunction(
        "intra-parallel",
        std::make_unique<GAMM::IntraParallel>(config.l, config.beta, config.t),
        x, y, z, energyMeter);
  }
}

void runFunction(std::string_view name, GAMM::BammUPtr bamm,
                 const GAMM::MatrixPtr &x, const GAMM::MatrixPtr &y,
                 const GAMM::Matrix &z,
                 std::optional<GAMM::EnergyMeterPtr> energyMeter) {

  INTELLI_DEBUG("Running " << name << " with energyMeter? "
                           << energyMeter.has_value());
  INTELLI_DEBUG("Bamm ptr " << bamm);
  if (energyMeter.has_value()) {
    energyMeter.value()->startSampling();
  }

  BS::timer tmr;
  tmr.start();
  auto z_amm = bamm->multiply(x, y);
  tmr.stop();

  std::optional<GAMM::AbstractEnergyMeter::Readings> energyReadings{};

  if (energyMeter.has_value()) {
    energyReadings = energyMeter.value()->stopSampling();
  }

  std::cout << z_amm->block<2, 2>(0, 0) << std::endl;

  *z_amm -= z;

  auto err = GAMM::UtilityFunctions::spectralNorm(*z_amm);

  std::cout << ' ' << std::setw(20) << name << ":  Time - " << std::setw(8)
            << std::setprecision(4) << ((double)tmr.ms() / 1000.0) << "s;  ";

  // TODO check whether the energy readings should be written to CSV
  if (energyReadings.has_value()) {
    std::cout << "Energy - " << std::setw(8) << std::setprecision(4)
              << (energyReadings.value().energyConsumed()) << "J;  ";
  }

  std::cout << "Error - " << std::setprecision(4) << err << std::endl;
}
