// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_SVD_SVD_HPP_
#define IntelliStream_SRC_SVD_SVD_HPP_

#include "Utils/UtilityFunctions.hpp"
#include <Eigen/Dense>
#include <Eigen/src/Core/Diagonal.h>
#include <Eigen/src/Core/DiagonalMatrix.h>
#include <Eigen/src/Core/util/Constants.h>
#include <memory>

namespace GAMM {
class Svd {
public:
  const Matrix &matrixU() const noexcept { return u; }
  const Matrix &matrixV() const noexcept { return v; }
  const DiagonalMatrix &singularValues() const noexcept { return sv; }

  Matrix &matrixU() noexcept { return u; }
  Matrix &matrixV() noexcept { return v; }
  DiagonalMatrix &singularValues() noexcept { return sv; }

  virtual void startSvd(Matrix matrix) = 0;
  // Returns whether to stop iteration
  virtual bool svdStep() = 0;
  virtual bool svdStep(size_t nsteps) {
    for (size_t i = 0; i < nsteps; ++i) {
      if (svdStep()) {
        return true;
      }
    }
    return false;
  }
  virtual void finishSvd() = 0;

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

  void sortSingularValues() noexcept;

  Matrix u;
  Matrix v;
  DiagonalMatrix sv;
};

typedef std::unique_ptr<Svd> SvdUPtr;
} // namespace GAMM
#endif
