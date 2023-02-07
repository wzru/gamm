#include "Utils/Logger.hpp"
#include "Utils/UtilityFunctions.hpp"
#include <Svd/SequentialJTS.hpp>

using namespace GAMM;

void SequentialJTS::startSvd(Matrix matrix) {
  INTELLI_TRACE("SequentialJTS BEGIN SVD");
  INTELLI_ASSERT(matrix.rows() == matrix.cols(),
                 "Only square matrices supported");

  u = std::move(matrix);

  iterNumber = 0;

  INTELLI_TRACE("U norm " << u.squaredNorm());

  delta = options.tol * u.squaredNorm();
  v = Matrix::Identity(u.rows(), u.cols());

  INTELLI_TRACE("p reserve " << nColumnPairs());
  p.reserve(nColumnPairs());
  INTELLI_TRACE("q reserve " << npivots());
  q.reserve(npivots());
}

bool SequentialJTS::svdStep() {
  INTELLI_TRACE("SequentialJTS SVD STEP " << iterNumber);
  INTELLI_TRACE("b size (" << u.rows() << ", " << u.cols() << ")");

  size_t n = u.cols();

  p.clear();
  // ====== Phase 1 ======
  // Generate columns on which rotations will be applied
  for (size_t j = 0; j < n - 1; ++j) {
    for (size_t k = j + 1; k < n; ++k) {
      auto d = u.col(j).dot(u.col(k));
      if (UtilityFunctions::isZero(d)) {
        d = 0.0;
      }
      p.emplace_back(j, k, d);
    }
  }

  INTELLI_TRACE("Generated p");
  std::sort(p.begin(), p.end(), std::greater<ColumnPair>());
  INTELLI_TRACE("Sorted p");

  INTELLI_TRACE("p has size " << p.size());
  p.resize(npivots());

  INTELLI_TRACE("Checking " << p[0].d << " < " << delta << "? "
                            << (p[0].d <= delta));
  if (p[0].d <= delta) {
    return true;
  }
  // =====================

  // ====== Phase 2 ======
  // Generate rotations based on the top 1/tau fracti  on of column-pairs

  q.clear();
  for (auto columnPair : p) {
    INTELLI_TRACE("Generating rotation from column pair ("
                  << columnPair.j << ", " << columnPair.k << ", "
                  << columnPair.d << ")");
    q.emplace_back(columnPair, u);
  }
  // =====================

  INTELLI_TRACE("Generated " << q.size() << " rotations");

  // ====== Phase 3 ======
  // Apply Jacobi rotations

  for (auto rotation : q) {
    INTELLI_TRACE("Applying rotation between "
                  << rotation.j << " and " << rotation.k << " (" << rotation.c
                  << ", " << rotation.s << ", " << rotation.inv_c << ", "
                  << rotation.t << ")");
    rotation.applyTo(u);
    rotation.applyTo(v);
  }
  // =====================
  INTELLI_TRACE("Applied rotations");

  // Check if maximum iterations has been reached
  return ++iterNumber == options.maxSweeps;
}

void SequentialJTS::finishSvd() {
  INTELLI_TRACE("SequentialJTS FINISH SVD");
  size_t n = u.cols();

  INTELLI_TRACE("Number of singular values: " << n);
  sv.resize(n);

  // INTELLI_DEBUG("Pre scaled SVD performed, \nU:\n"
  //               << u << "\nV:" << v << "\nSV:" << (sv.diagonal()));

  for (size_t i = 0; i < n; ++i) {
    auto &value = sv.diagonal()[i];
    value = u.col(i).norm();
    if (value < options.tol) {
      value = 0.0;
      u.col(i) *= 0;
    } else {
      u.col(i) /= value;
    }
  }

  // INTELLI_DEBUG("Post scaled SVD performed, \nU:\n"
  //               << u << "\nV:" << v << "\nSV:" << (sv.diagonal()));

  sortSingularValues();
}
