#include "Utils/Logger.hpp"
#include "Utils/UtilityFunctions.hpp"
#include <Svd/SequentialJTS.hpp>

using namespace GAMM;

void SequentialJTS::startSvd(Matrix matrix) {
  INTELLI_ASSERT(matrix.rows() == matrix.cols(),
                 "Only square matrices supported");

  u = std::move(matrix);
  v = Matrix::Identity(u.rows(), u.cols());
  iterNumber = 0;
  delta = options.tol * u.squaredNorm();

  p.reserve(nColumnPairs());
  q.reserve(npivots());
}

bool SequentialJTS::svdStep() {
  size_t n = u.cols();

  p.clear();
  // ====== Phase 1 ======
  // Generate columns on which rotations will be applied
  for (size_t j = 0; j < n - 1; ++j) {
    for (size_t k = j + 1; k < n; ++k) {
      auto d = u.col(j).dot(u.col(k));
      // if (UtilityFunctions::isZero(d)) {
      //   d = 0.0;
      // }
      p.emplace_back(j, k, d);
    }
  }

  std::sort(p.begin(), p.end(), std::greater<ColumnPair>());

  p.resize(npivots());

  if (p[0].d < delta) {
    return true;
  }
  // =====================

  // ====== Phase 2 ======
  // Generate rotations based on the top 1/tau fracti  on of column-pairs

  q.clear();
  for (auto columnPair : p) {
    q.emplace_back(columnPair, u);
  }
  // =====================

  // ====== Phase 3 ======
  // Apply Jacobi rotations

  for (auto rotation : q) {
    rotation.applyTo(u);
    rotation.applyTo(v);
  }
  // =====================

  // Check if maximum iterations has been reached
  return ++iterNumber == options.maxSweeps;
}
