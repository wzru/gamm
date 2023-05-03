#include <boost/program_options.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <Utils/Config.hpp>
#include <Utils/Logger.hpp>

using namespace GAMM;
namespace po = boost::program_options;

void trySetBin(std::string_view binString, Config::Bins &bins) {
  if (binString == "all") {
    bins = Config::RUN_ALL;
  } else if (binString == "parallel") {
    bins = Config::RUN_ALL;
    bins.single = 0;
  } else if (binString == "single") {
    bins.single = 1;
  } else if (binString == "intra") {
    bins.intra = 1;
  } else if (binString == "inter") {
    bins.inter = 1;
  } else if (binString == "combined") {
    bins.combined = 1;
  } else if (binString == "dynamic") {
    bins.dynamic = 1;
  } else {
    INTELLI_WARNING("Unknown strategy " << binString);
  }
}

void Config::useConfigFile(std::string_view path) noexcept {
  auto res = toml::parse_file(path);

  if (res.failed()) {
    INTELLI_WARNING("Failed to parse config file '" << path << "':");
    INTELLI_WARNING(res.error());
    INTELLI_WARNING("Using default configuration");
  }

  auto tbl = res.table();

  auto tbl_x = tbl["x"];
  if (tbl_x.is_string()) {
    x = tbl_x.value<std::string>().value();
  }

  auto tbl_y = tbl["y"];
  if (tbl_y.is_string()) {
    y = tbl_y.value<std::string>().value();
  }

  auto tbl_l = tbl["l"];
  if (tbl_l.is_integer()) {
    l = tbl_l.value<size_t>().value();
  }

  auto tbl_t = tbl["t"];
  if (tbl_t.is_integer()) {
    t = tbl_t.value<size_t>().value();
  }

  auto tbl_beta = tbl["beta"];
  if (tbl_beta.is_number()) {
    beta = tbl_beta.value<scalar_t>().value();
  }

  auto tbl_bin = tbl["bin"];
  if (tbl_bin.is_string()) {
    trySetBin(tbl_bin.value<std::string_view>().value(), bins);
  } else if (toml::array *arr = tbl_bin.as_array()) {
    arr->for_each([this](auto &&el) {
      if constexpr (toml::is_string<decltype(el)>) {
        trySetBin(*el, bins);
      }
    });
  }
}

Config::Config(int argc, const char *const argv[]) {
  // clang-format off
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("t,t", po::value<size_t>(), "set number of threads to be spawned")
    ("l,l", po::value<size_t>(), "set value for l used in beta-AMM")
    ("beta", po::value<scalar_t>(), "set value for beta used in beta-AMM")
    ("x,x", po::value<std::string>(), "path to the matrix X")
    ("y,y", po::value<std::string>(), "path to the matrix Y")
    ("bin,b", po::value<std::vector<std::string>>(), "binaries to run")
    ("defaults,d", po::value<std::string>(), "config file to use for defaults")
    ("measure-energy,e", "whether to measure the energy consumed by each amm")
    ("energy-csv,c", po::value<std::string>(), "path to a csv to write detailed energy readings. "
                                               "Automatically enables measure-energy");
  // clang-format on

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("defaults")) {
    useConfigFile(vm["defaults"].as<std::string>());
  }

  // Exit early if the user just wants to ask for help
  if (vm.count("help")) {
    std::cout << desc << "\n";
    std::exit(0);
  }

  if (vm.count("t")) {
    t = vm["t"].as<size_t>();
  }

  if (vm.count("l")) {
    l = vm["l"].as<size_t>();
  }

  if (vm.count("beta")) {
    beta = vm["beta"].as<scalar_t>();
  }

  if (vm.count("x")) {
    x = vm["x"].as<std::string>();
  }

  if (vm.count("y")) {
    y = vm["y"].as<std::string>();
  }

  if (vm.count("bin")) {
    for (const auto &bin : vm["bin"].as<std::vector<std::string>>()) {
      trySetBin(bin, bins);
    }
  }

  if (vm.count("measure-energy")) {
    measureEnergy = true;
  }

  if (vm.count("energy-csv")) {
    energyCSVPath = vm["energy-csv"].as<std::string>();
    measureEnergy = true;
  }

  if (bins.isEmpty()) {
    bins = RUN_ALL;
  }
}

uint64_t readInt(std::ifstream &f) {
  uint8_t buf[8];
  f.read((char *)buf, 8);
  auto ret{0};
  for (int i = 0; i < 8; ++i) {
    ret = ret | (buf[i] << (8 * i));
  }
  return ret;
}

float readFloat(std::ifstream &f) {
  float val;
  f.read((char *)&val, sizeof(val));
  return val;
}

std::optional<std::shared_ptr<Matrix>> loadMatrix(const char *file) {
  std::ifstream f(file, std::ios::in | std::ios::binary);
  auto n = readInt(f);
  auto m = readInt(f);

  auto mat = std::make_shared<Matrix>(n, m);

  for (uint64_t i = 0; i != n; ++i) {
    for (uint64_t j = 0; j != m; ++j) {
      (*mat)(i, j) = (scalar_t)readFloat(f);
    }
  }

  if (f.fail()) {
    return {};
  }

  return mat;
}

std::optional<Config::matrices> Config::loadMatrices() const noexcept {
  auto x = loadMatrix(this->x.c_str());
  auto y = loadMatrix(this->y.c_str());

  if (!x.has_value() || !y.has_value()) {
    return {};
  }

  return matrices{std::move(x.value()), std::move(y.value())};
}

std::ostream &GAMM::operator<<(std::ostream &o, Config::Bins const &bins) {
  int nbins{0};

  if (bins.single) {
    if (nbins++) {
      o << " | ";
    }
    o << "single";
  }
  if (bins.intra) {
    if (nbins++) {
      o << " | ";
    }
    o << "intra";
  }
  if (bins.inter) {
    if (nbins++) {
      o << " | ";
    }
    o << "inter";
  }
  if (bins.combined) {
    if (nbins++) {
      o << " | ";
    }
    o << "combined";
  }
  if (bins.dynamic) {
    if (nbins++) {
      o << " | ";
    }
    o << "dynamic";
  }

  if (nbins == 0) {
    o << "none";
  }

  return o;
}

std::ostream &GAMM::operator<<(std::ostream &o, Config const &config) {
  return o << "Config { x: " << config.x << ", y: " << config.y
           << ", l: " << config.l << ", t: " << config.t
           << ", beta: " << config.beta << ", bins: " << config.bins
           << ", measure-energy: " << (config.measureEnergy ? "true" : "false")
           << ", energy-csv-file: "
           << (config.energyCSVPath.has_value() ? config.energyCSVPath.value()
                                                : "none")
           << " }";
}
