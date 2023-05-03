// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_AMM_BAMM_HPP_
#define IntelliStream_SRC_AMM_BAMM_HPP_

#include <cstddef>
#include <memory>
#include <optional>

#include <Eigen/Dense>

#include "BS_thread_pool.hpp"
#include "Svd/Svd.hpp"
#include "Utils/Logger.hpp"
#include "Utils/ZeroedColumns.hpp"

namespace GAMM {

class Bamm {
public:
  struct result;

  Bamm(size_t l, scalar_t beta, SvdUPtr svd)
      : l{l}, beta{beta}, svd{std::move(svd)} {
    attenuateVec.resize(l);
    for (size_t i = 0; i < l; ++i) {
      attenuateVec[i] = std::expm1((scalar_t)i * beta / ((scalar_t)l - 1.0)) /
                        std::expm1(beta);
    }
  }

  Bamm(size_t l, scalar_t beta, SvdUPtr svd, BS::thread_pool_ptr pool)
      : l{l}, beta{beta}, svd{std::move(svd)}, pool(pool) {
    attenuateVec.resize(l);
    for (size_t i = 0; i < l; ++i) {
      attenuateVec[i] = std::expm1((scalar_t)i * beta / ((scalar_t)l - 1.0)) /
                        std::expm1(beta);
    }
  }

  MatrixPtr multiply(Matrix &x, Matrix &y) {
    bx = std::make_shared<Matrix>(x.rows(), l);
    by = std::make_shared<Matrix>(y.rows(), l);

    auto d = x.cols();
    this->x = x.leftCols(d);
    this->y = y.leftCols(d);

    zeroedColumns.resizeEmpty(l);
    xi = 0;

    reduce();

    *bx.value() *= by.value()->transpose();

    return std::move(*bx);
  }

  void reduce(MatrixRef x, MatrixRef y, MatrixPtr bx, MatrixPtr by) {
    this->bx = std::move(bx);
    this->by = std::move(by);
    this->x = std::move(x);
    this->y = std::move(y);

    zeroedColumns.fromMatrix(*this->bx.value());
    xi = 0;

    reduce();
  }

  void setMatrices(MatrixRef x, MatrixRef y, MatrixPtr bx, MatrixPtr by) {
    this->bx = std::move(bx);
    this->by = std::move(by);
    this->x = std::move(x);
    this->y = std::move(y);

    zeroedColumns.fromMatrix(*this->bx.value());
    xi = 0;
  }

  struct result {
    MatrixPtr bx, by;
  };

  bool reductionStepSetup();

  bool reductionStepFinish();

  bool reductionStepSvdStep(size_t nsteps = 1);

protected:
  virtual void reduce() = 0;

  void parameterizedReduceRank(DiagonalMatrix &sv) const;

  size_t l;
  scalar_t beta;
  size_t xi;
  std::optional<MatrixRef> x, y;
  std::optional<MatrixPtr> bx, by;
  SvdUPtr svd;
  ZeroedColumns zeroedColumns;
  BS::thread_pool_ptr pool;

private:
  Vector attenuateVec;
};

typedef std::unique_ptr<Bamm> BammUPtr;
} // namespace GAMM
#endif
