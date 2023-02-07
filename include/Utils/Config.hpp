// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_UTILS_CONFIG_HPP_
#define IntelliStream_SRC_UTILS_CONFIG_HPP_

#include <Eigen/Dense>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string_view>

#define TOML_EXCEPTIONS 0

#include "toml.hpp"

#include "Utils/UtilityFunctions.hpp"

namespace GAMM {
class Config {
public:
  struct Bins;
  struct matrices;

  Config() {}

  // Construct from a config file
  Config(std::string_view configPath) noexcept { useConfigFile(configPath); }

  // Construct using cli arguments
  Config(int argc, const char *const *argv) noexcept;

  struct Bins {
    std::uint8_t single : 1, intra : 1, inter : 1, combined : 1, dynamic : 1;
  };

  static constexpr Bins NONE = {0, 0, 0, 0, 0};
  static constexpr Bins RUN_ALL = {1, 1, 1, 1, 1};

  std::string x{"./benchmark/datasets/x.dat"}, y{"./benchmark/datasets/y.dat"};
  size_t l{400}, t{std::thread::hardware_concurrency()};
  scalar_t beta{28.0};
  Bins bins{RUN_ALL};

  std::optional<matrices> loadMatrices() const noexcept;

  struct matrices {
    MatrixPtr x, y;

    matrices(MatrixPtr x, MatrixPtr y) : x{x}, y{y} {}
  };

private:
  void useConfigFile(std::string_view path) noexcept;
};

std::ostream &operator<<(std::ostream &o, Config::Bins const &bins);
std::ostream &operator<<(std::ostream &o, Config const &config);
} // namespace GAMM
#endif
