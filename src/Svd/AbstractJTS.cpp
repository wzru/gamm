#include "Utils/Logger.hpp"
#include "Utils/UtilityFunctions.hpp"
#include <Svd/AbstractJTS.hpp>

using namespace GAMM;

void AbstractJTS::finishSvd() {
  size_t n = u.cols();

  sv.resize(n);

  for (size_t i = 0; i < n; ++i) {
    auto &value = sv.diagonal()[i];
    value = u.col(i).norm();
    if (UtilityFunctions::isZero(value)) {
      value = 0.0;
      u.col(i) *= 0;
    } else {
      u.col(i) /= value;
    }
  }

  sortSingularValues();
}

AbstractJTS::JacobiRotation::JacobiRotation(ColumnPair columnPair,
                                            const Matrix &mat) {
  j = columnPair.j;
  k = columnPair.k;

  auto gamma = (mat.col(k).squaredNorm() - mat.col(j).squaredNorm()) /
               (2.0 * columnPair.d);
  INTELLI_TRACE("GAMMA " << gamma);
  t = std::copysign(1.0 / (std::abs(gamma) + std::sqrt(1 + gamma * gamma)),
                    gamma);
  inv_c = std::sqrt(t * t + 1.0);
  c = 1.0 / inv_c;
  s = t * c;

  if (std::isnan(s)) {
    INTELLI_ERROR("Found NaN");
  }
}

void AbstractJTS::JacobiRotation::applyTo(Matrix &matrix) const {
  auto a = matrix.col(j);
  auto b = matrix.col(k);
  applyTo(a, b);
}

void AbstractJTS::JacobiRotation::applyTo(Matrix::ColXpr col_j,
                                          Matrix::ColXpr col_k) const {
  col_j *= c;
  col_j -= s * col_k;

  col_k *= inv_c;
  col_k += t * col_j;
}
