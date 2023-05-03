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

bool Bamm::reductionStepSetup() {
  auto end = std::min(xi + zeroedColumns.nzeroed(), (size_t)x.value().cols());

  INTELLI_TRACE("Copying " << (end - xi) << " columns into bx and by");
  for (; xi < end; ++xi) {

    if (x.value().col(xi).unaryExpr(std::ref(UtilityFunctions::isZero)).all()) {

      INTELLI_TRACE("SKIPPING copying zero col " << xi);
      continue;
    }

    auto zeroCol = zeroedColumns.getNextZeroed();

    bx.value()->col(zeroCol) = x.value().col(xi);
    by.value()->col(zeroCol) = y.value().col(xi);
  }

  // If there are no more columns to copy then exit early
  if (xi >= (size_t)x.value().cols())
    return true;

  Matrix rx = Matrix::Zero(l, l);
  Matrix ry_t = Matrix::Zero(l, l);

  UtilityFunctions::qr(*bx.value(), rx);
  UtilityFunctions::qr(*by.value(), ry_t, true);

  rx *= ry_t;

  svd->startSvd(std::move(rx));

  return false;
}

bool Bamm::reductionStepSvdStep(size_t nsteps) { return svd->svdStep(nsteps); }

bool Bamm::reductionStepFinish() {
  auto svd_future = pool->submit([&]() { svd->finishSvd(); });
  // svd->finishSvd();

  svd_future.get();

  auto &sv = svd->singularValues();
  auto &u = svd->matrixU();
  auto &v = svd->matrixV();

  auto prr_future = pool->submit([&]() { parameterizedReduceRank(sv); });
  // parameterizedReduceRank(sv);
  prr_future.get();

  auto end_future = pool->submit([&]() {
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
  });

  end_future.get();

  return true;
}
