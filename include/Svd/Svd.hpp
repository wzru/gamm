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
  void sortSingularValues() noexcept;

  Matrix u;
  Matrix v;
  DiagonalMatrix sv;
};

typedef std::unique_ptr<Svd> SvdUPtr;
} // namespace GAMM
#endif
