#include "Utils/Logger.hpp"
#include <Svd/Svd.hpp>
#include <iterator>
#include <vector>

using namespace GAMM;

void Svd::sortSingularValues() noexcept {
  const long size = sv.cols();

  // (singular value, index)
  std::vector<std::pair<scalar_t, long>> svIndices;
  svIndices.reserve(size);

  auto svStorage = sv.diagonal().data();

  for (long i = 0; i < size; ++i) {
    svIndices.emplace_back(svStorage[i], i);
  }

  std::sort(svIndices.begin(), svIndices.end(),
            std::greater<std::pair<scalar_t, long>>());

  Eigen::VectorXi indices{size};

  for (int i = 0; i < size; ++i) {
    const auto &value = svIndices[i];
    // Update the singularValues storage to reflect sorted order
    sv.diagonal()(i) = value.first;
    // And keep track of the permuted matrices
    indices[i] = value.second;
  }

  Eigen::PermutationMatrix<Eigen::Dynamic> permutation{indices};

  u.applyOnTheRight(permutation);
  v.applyOnTheRight(permutation);
}
