// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_SVD_SequentialJTS_HPP_
#define IntelliStream_SRC_SVD_SequentialJTS_HPP_

#include <Eigen/Dense>
#include <compare>
#include <memory>

#include "Svd/Svd.hpp"
#include "Utils/UtilityFunctions.hpp"

namespace GAMM {
class SequentialJTS : public Svd {
public:
  struct Options {
    size_t tau = 32, maxSweeps = 100;
    scalar_t tol = 1e-7;
  };

  SequentialJTS() : options{} {}
  SequentialJTS(Options options) : options{std::move(options)} {}

  virtual void startSvd(Matrix matrix) override;
  virtual bool svdStep() override;
  virtual void finishSvd() override;

private:
  constexpr size_t nColumnPairs() const noexcept {
    return u.cols() * (u.cols() - 1) / 2;
  }
  constexpr size_t npivots() const noexcept {
    return nColumnPairs() / options.tau;
  }

  Options options;

  size_t iterNumber{0};
  scalar_t delta;
  std::vector<ColumnPair> p{};
  std::vector<JacobiRotation> q{};
};

typedef std::unique_ptr<SequentialJTS> SequentialJTSUPtr;
} // namespace GAMM
#endif
