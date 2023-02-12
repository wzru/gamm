// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_SVD_AbstractJTS_HPP_
#define IntelliStream_SRC_SVD_AbstractJTS_HPP_

#include <Eigen/Dense>
#include <compare>
#include <memory>

#include "Svd/Svd.hpp"
#include "Utils/UtilityFunctions.hpp"

namespace GAMM {
class AbstractJTS : public Svd {
public:
  struct Options {
    size_t tau = 32, maxSweeps = 100;
    scalar_t tol = 1e-7;
  };

  AbstractJTS() : options{} {}
  AbstractJTS(Options options) : options{std::move(options)} {}

  virtual void finishSvd() override;

  const Options &getOptions() const { return options; }

protected:
  struct ColumnPair {
    ColumnPair() {}

    ColumnPair(size_t j, size_t k, scalar_t d) : j{j}, k{k}, d{d} {}
    std::partial_ordering operator<=>(const ColumnPair &other) const {
      return d <=> other.d;
    }

    size_t j, k;
    scalar_t d;
  };

  struct JacobiRotation {
    scalar_t c, s, inv_c, t;
    size_t j, k;

    JacobiRotation() {}
    JacobiRotation(ColumnPair columnPair, const Matrix &mat);

    void applyTo(Matrix &mat) const;
    void applyTo(Matrix::ColXpr col_j, Matrix::ColXpr col_k) const;
  };

  constexpr size_t nColumnPairs() const noexcept {
    return u.cols() * (u.cols() - 1) / 2;
  }
  constexpr size_t npivots() const noexcept {
    return nColumnPairs() / options.tau;
  }

  Options options;

  size_t iterNumber{0};
  scalar_t delta;
};

typedef std::unique_ptr<AbstractJTS> AbstractJTSUPtr;
} // namespace GAMM
#endif
