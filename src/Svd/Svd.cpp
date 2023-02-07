#include "Utils/Logger.hpp"
#include <Svd/Svd.hpp>
#include <iterator>
#include <vector>

using namespace GAMM;

void Svd::sortSingularValues() noexcept {
  const long size = sv.cols();
  INTELLI_TRACE("Sorting " << size << " singular values");

  // INTELLI_DEBUG("Pre sorted SVD performed, \nU:\n"
  //               << u << "\nV:" << v << "\nSV:" << (sv.diagonal()));

  // (singular value, index)
  std::vector<std::pair<scalar_t, long>> svIndices;
  svIndices.reserve(size);

  auto svStorage = sv.diagonal().data();

  for (long i = 0; i < size; ++i) {
    svIndices.emplace_back(svStorage[i], i);
  }

  INTELLI_TRACE("Sorting");
  std::sort(svIndices.begin(), svIndices.end(),
            std::greater<std::pair<scalar_t, long>>());

  // for (const auto &val : svIndices) {
  //   std::cout << '(' << val.first << ", " << val.second << "), ";
  // }
  // std::cout << '\n';

  Eigen::VectorXi indices{size};

  INTELLI_TRACE("Creating permutation matrix");
  for (int i = 0; i < size; ++i) {
    const auto &value = svIndices[i];
    // Update the singularValues storage to reflect sorted order
    sv.diagonal()(i) = value.first;
    // And keep track of the permuted matrices
    indices[i] = value.second;
  }

  // std::copy(indices.begin(), indices.end(),
  //           std::ostream_iterator<int>(std::cout, ","));
  // std::cout << "\n";

  Eigen::PermutationMatrix<Eigen::Dynamic> permutation{indices};

  INTELLI_TRACE("Applying permutation");
  u.applyOnTheRight(permutation);
  v.applyOnTheRight(permutation);
  INTELLI_TRACE("Sorted singular values");
}

Svd::JacobiRotation::JacobiRotation(ColumnPair columnPair, const Matrix &mat) {
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
}

void Svd::JacobiRotation::applyTo(Matrix &matrix) const {
  auto a = matrix.col(j);
  auto b = matrix.col(k);
  applyTo(a, b);
}

void Svd::JacobiRotation::applyTo(Matrix::ColXpr col_j,
                                  Matrix::ColXpr col_k) const {
  col_j *= c;
  col_j -= s * col_k;

  col_k *= inv_c;
  col_k += t * col_j;
}
