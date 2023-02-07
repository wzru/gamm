// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_AMM_BAMM_HPP_
#define IntelliStream_SRC_AMM_BAMM_HPP_

#include <cstddef>
#include <memory>
#include <optional>

#include <Eigen/Dense>

#include "Svd/Svd.hpp"
#include "Utils/Logger.hpp"
#include "Utils/ZeroedColumns.hpp"

namespace GAMM {

class Bamm {
public:
  struct result;

  Bamm(size_t l, scalar_t beta, SvdUPtr svd) : svd{std::move(svd)}, l{l} {
    attenuateVec.resize(l);
    for (size_t i = 0; i < l; ++i) {
      attenuateVec[i] = std::expm1((scalar_t)i * beta / ((scalar_t)l - 1.0)) /
                        std::expm1(beta);
    }
  }

  MatrixPtr multiply(MatrixPtr x, MatrixPtr y) {
    bx = std::make_shared<Matrix>(x->rows(), l);
    by = std::make_shared<Matrix>(y->rows(), l);
    this->x = x;
    this->y = y;

    zeroedColumns.resizeEmpty(l);

    INTELLI_TRACE("x size (" << x->rows() << ", " << x->cols() << ")");
    INTELLI_TRACE("y size (" << y->rows() << ", " << y->cols() << ")");

    INTELLI_TRACE("bx size (" << bx.value()->rows() << ", "
                              << bx.value()->cols() << ")");
    INTELLI_TRACE("by size (" << by.value()->rows() << ", "
                              << by.value()->cols() << ")");

    INTELLI_DEBUG("Going to start reducing");
    reduce();
    INTELLI_DEBUG("Reduced");

    *bx.value() *= by.value()->transpose();

    return std::move(*bx);
  }

  void reduce(MatrixPtr x, MatrixPtr y, MatrixPtr bx, MatrixPtr by) {
    this->bx = std::move(bx);
    this->by = std::move(by);
    this->x = std::move(x);
    this->y = std::move(y);

    // TODO, resize zeroedColumns
    INTELLI_NOT_IMPLEMENTED();

    reduce();
  }

  struct result {
    MatrixPtr bx, by;
  };

protected:
  virtual void reduce() = 0;

  void parameterizedReduceRank(DiagonalMatrix &sv) const;

  void reductionStepSetup();
  bool reductionStepSvdStep();
  void reductionStepFinish();

  size_t xi;
  std::optional<MatrixPtr> x, y, bx, by;
  SvdUPtr svd;
  ZeroedColumns zeroedColumns;

private:
  size_t l;
  Vector attenuateVec;
};
typedef std::unique_ptr<Bamm> BammUPtr;
} // namespace GAMM
#endif
