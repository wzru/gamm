#include "Amm/Bamm.hpp"
#include "Utils/Logger.hpp"
#include "Utils/UtilityFunctions.hpp"

using namespace GAMM;

void Bamm::parameterizedReduceRank(DiagonalMatrix &sv) const {
  auto &diagonal = sv.diagonal();

  // Incorrect version
  //
  auto delta = diagonal[0];

  // auto delta = diagonal[l / 2 - 1];
  diagonal -= delta * attenuateVec;
  diagonal = diagonal.cwiseMax(0.0).cwiseSqrt();
}

void Bamm::reductionStepSetup() {
  auto end = std::min(xi + zeroedColumns.nzeroed(), (size_t)x.value().cols());

  INTELLI_DEBUG("Copying " << (end - xi) << " columns into bx and by");

  for (size_t i = xi; i < end; ++i) {
    if (x.value().col(i).unaryExpr(std::ref(UtilityFunctions::isZero)).all()) {

      INTELLI_TRACE("SKIPPING copying zero col " << i);
      continue;
    }

    auto zeroCol = zeroedColumns.getNextZeroed();

    bx.value()->col(zeroCol) = x.value().col(i);
    by.value()->col(zeroCol) = y.value().col(i);
  }
  xi = end;

  Matrix rx = Matrix::Zero(l, l);
  Matrix ry_t = Matrix::Zero(l, l);

  UtilityFunctions::qr(*bx.value(), rx);
  UtilityFunctions::qr(*by.value(), ry_t, true);

  rx *= ry_t;

  svd->startSvd(std::move(rx));
}

bool Bamm::reductionStepSvdStep(size_t nsteps) { return svd->svdStep(nsteps); }

void Bamm::reductionStepFinish() {
  svd->finishSvd();

  auto &sv = svd->singularValues();
  auto &u = svd->matrixU();
  auto &v = svd->matrixV();

  parameterizedReduceRank(sv);

  for (int i = 0; i < sv.cols(); ++i) {
    const auto &value = sv.diagonal()[i];

    if (UtilityFunctions::isZero(value)) {
      zeroedColumns.setZeroed(i);
    }

    u.col(i) *= value;
    v.col(i) *= value;
  }

  *bx.value() *= u;
  *by.value() *= v;
}
