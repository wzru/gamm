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
#include <Amm/CombinedParallel.hpp>
#include <Amm/InterParallel.hpp>
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
                 std::optional<GAMM::EnergyMeterPtr> energyMeter,
                 const std::optional<std::string> &energyCSVPath);

int main(int argc, char **argv) {
  // Setup Logs.
  setupLogging("benchmark.log", LOG_TRACE);

  const GAMM::Config config{argc, argv};

  INTELLI_INFO(config);

  auto res = config.loadMatrices();
  if (!res.has_value()) {
    INTELLI_FATAL_ERROR("Error loading matrices in config. " << argc << argv);
    return 1;
  }

  auto [x, y] = res.value();

  INTELLI_INFO("Loaded matrices x(" << x->rows() << ", " << x->cols()
                                    << ") and y(" << y->rows() << ", "
                                    << y->cols() << ')');

  INTELLI_INFO("x:\n" << (x->block<2, 2>(0, 0)));
  INTELLI_INFO("y:\n" << (y->block<2, 2>(0, 0)));

  BS::timer tmr;
  tmr.start();
  const GAMM::Matrix z = *x * y->transpose();
  tmr.stop();

  std::cout << "Lib-MM " << tmr.ms() << "ms\n";
  INTELLI_INFO("z:\n" << (z.block<2, 2>(0, 0)));

  std::optional<GAMM::EnergyMeterPtr> energyMeter{};

  if (config.measureEnergy) {
    if (GAMM::JetsonEnergyMeter::canBeUsed()) {
      INTELLI_INFO("Using Jetson Energy Meter");
      energyMeter = std::make_shared<GAMM::JetsonEnergyMeter>();
    } else {
      INTELLI_FATAL_ERROR("No energy meter");
      return 1;
    }
  }

  if (config.bins.single) {
    runFunction("single-threaded",
                std::make_unique<GAMM::Single>(config.l, config.beta), x, y, z,
                energyMeter, config.energyCSVPath);
  }

  if (config.bins.intra) {
    runFunction(
        "intra-parallel",
        std::make_unique<GAMM::IntraParallel>(config.l, config.beta, config.t),
        x, y, z, energyMeter, config.energyCSVPath);
  }

  if (config.bins.inter) {
    runFunction(
        "inter-parallel",
        std::make_unique<GAMM::InterParallel>(config.l, config.beta, config.t),
        x, y, z, energyMeter, config.energyCSVPath);
  }

  if (config.bins.combined) {
    for (size_t p = 1; p <= config.t; ++p) {
      std::string s{"combined-parallel-"};
      s += std::to_string(p);
      runFunction(s,
                  std::make_unique<GAMM::CombinedParallel>(
                      config.l, config.beta, config.t, p),
                  x, y, z, energyMeter, config.energyCSVPath);
    }
  }
}

void runFunction(std::string_view name, GAMM::BammUPtr bamm,
                 const GAMM::MatrixPtr &x, const GAMM::MatrixPtr &y,
                 const GAMM::Matrix &z,
                 std::optional<GAMM::EnergyMeterPtr> energyMeter,
                 const std::optional<std::string> &energyCSVPath) {

  INTELLI_INFO("Running " << name << " with energyMeter? "
                          << energyMeter.has_value());
  if (energyMeter.has_value()) {
    energyMeter.value()->startSampling();
  }

  std::optional<GAMM::AbstractEnergyMeter::Readings> energyReadings{};

  BS::timer tmr;
  tmr.start();
  auto z_amm = bamm->multiply(*x, *y);
  tmr.stop();

  if (energyMeter.has_value()) {
    energyReadings = energyMeter.value()->stopSampling();
  }

  INTELLI_INFO(name << " z_amm:\n" << (z_amm->block<2, 2>(0, 0)));

  *z_amm -= z;

  auto err = GAMM::UtilityFunctions::spectralNorm(*z_amm);

  std::cout << ' ' << std::setw(23) << name << ":  Time - " << std::setw(8)
            << std::setprecision(4) << ((double)tmr.ms() / 1000.0) << "s;  ";

  if (energyReadings.has_value()) {
    if (energyCSVPath.has_value()) {
      energyReadings.value().writeCSV(energyCSVPath.value().c_str());
    }

    std::cout << "Energy - " << std::setw(8) << std::setprecision(4)
              << (energyReadings.value().energyConsumed()) << "J;  ";
  }

  std::cout << "Error - " << std::setprecision(4) << err << std::endl;
}
