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
  INTELLI_TRACE("ParameterizedReduceRank using delta = " << delta);
  diagonal -= delta * attenuateVec;
  diagonal = diagonal.cwiseMax(0.0).cwiseSqrt();
  INTELLI_TRACE("Finished ParameterizedReduceRank");
}

void Bamm::reductionStepSetup() {
  auto end = std::min(xi + zeroedColumns.nzeroed(), (size_t)x.value()->cols());

  INTELLI_DEBUG("Copying " << (end - xi) << " columns into bx and by");

  for (size_t i = xi; i < end; ++i) {
    auto zeroCol = zeroedColumns.getNextZeroed();
    // INTELLI_WARNING("Copying column " << i << " to column " << zeroCol);
    bx.value()->col(zeroCol) = x.value()->col(i);
    by.value()->col(zeroCol) = y.value()->col(i);
  }
  xi = end;

  INTELLI_TRACE("PERFORMING QR");

  Matrix rx = Matrix::Zero(l, l);
  Matrix ry_t = Matrix::Zero(l, l);

  UtilityFunctions::qr(*bx.value(), rx);
  UtilityFunctions::qr(*by.value(), ry_t, true);

  INTELLI_DEBUG("Qx: \n" << (*bx.value()));
  INTELLI_DEBUG("Qy: \n" << (*by.value()));
  INTELLI_DEBUG("Rx: \n" << rx);
  INTELLI_DEBUG("Ry^T: \n" << ry_t);

  rx *= ry_t;

  INTELLI_TRACE("svdMatrix size (" << rx.rows() << ", " << rx.cols() << ")");
  INTELLI_DEBUG("SVD Matrix: \n" << rx);

  INTELLI_TRACE("STARTING SVD");
  svd->startSvd(std::move(rx));
}

bool Bamm::reductionStepSvdStep(size_t nsteps) { return svd->svdStep(nsteps); }

void Bamm::reductionStepFinish() {
  svd->finishSvd();

  auto &sv = svd->singularValues();
  auto &u = svd->matrixU();
  auto &v = svd->matrixV();

  INTELLI_DEBUG("SVD performed, \nU:\n"
                << u << "\nV:" << v << "\nSV:" << (sv.diagonal()));

  parameterizedReduceRank(sv);
  // std::cout << (sv.diagonal()) << "\n";

  INTELLI_TRACE("Scaling " << sv.cols() << " values");

  for (int i = 0; i < sv.cols(); ++i) {
    const auto &value = sv.diagonal()[i];

    INTELLI_TRACE("Singular Value " << i << " has value " << value);

    if (UtilityFunctions::isZero(value)) {
      zeroedColumns.setZeroed(i);
    }

    u.col(i) *= value;
    v.col(i) *= value;
  }

  *bx.value() *= u;
  *by.value() *= v;
}
