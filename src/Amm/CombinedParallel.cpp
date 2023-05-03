#include <cmath>

#include "Amm/CombinedParallel.hpp"
#include "Amm/IntraParallel.hpp"
#include "Utils/Logger.hpp"

using namespace GAMM;

void CombinedParallel::reduce() {
  std::vector<LockedMatrices> temp{p};
  matrices.swap(temp);

  // The bx,by given to worker 1 are the final bx,by computed once
  // inter-parallel bamm is compelete. So we use the bx and by created by the
  // Bamm super class
  matrices[0].bx = bx.value();
  matrices[0].by = by.value();
  for (auto i = matrices.begin() + 1; i != matrices.end(); ++i) {
    i->bx = std::make_shared<Matrix>(x.value().rows(), l);
    i->by = std::make_shared<Matrix>(y.value().rows(), l);
  }

  auto d = x.value().cols();
  if (p * l > (size_t)d) {
    INTELLI_WARNING("Not running inter-parallelism due to small size: "
                    << p << " * " << l << " > " << d);
    return;
  }

  BS::multi_future<void> tasks(p - 1);

  auto subpool = std::make_shared<BS::thread_pool>(8);

  for (size_t i = 1; i < p; ++i) {
    tasks[i - 1] =
        pool->submit([this, i, subpool]() { workerTask(i, subpool); });
  }
  workerTask(0, subpool);
  tasks.wait();

  matrices.clear();
}

void CombinedParallel::workerTask(size_t workerId,
                                  BS::thread_pool_ptr subpool) {
  auto d = x.value().cols(); // 10000
  auto nruns = UtilityFunctions::trailingZeros(
                   workerId | UtilityFunctions::nextPowerOfTwo(p)) +
               1;

  auto &ownMatrices = matrices[workerId];
  const std::lock_guard<std::mutex> ownGuard{ownMatrices.mtx};

  // Wait for all threads to have their own lock first
  barrier.arrive_and_wait();

  for (size_t i = 0; i < nruns; ++i) {
    std::optional<MatrixRef> xcols, ycols;
    std::optional<std::lock_guard<std::mutex>> otherGuard;
    if (i > 0) {
      auto otherId = workerId + (1 << (i - 1));
      if (otherId >= p) {
        break;
      }

      otherGuard.emplace(matrices[otherId].mtx);

      // leftCols is used create a block pointing to the whole matrix
      xcols = matrices[otherId].bx->leftCols(l);
      ycols = matrices[otherId].by->leftCols(l);
    } else {
      auto [startCol, numCols] = UtilityFunctions::unevenDivide(workerId, d, p);

      *ownMatrices.bx = x.value().middleCols(startCol, l);
      *ownMatrices.by = y.value().middleCols(startCol, l);

      // We need nested expression to make the types match
      xcols =
          x.value().nestedExpression().middleCols(startCol + l, numCols - l);
      ycols =
          y.value().nestedExpression().middleCols(startCol + l, numCols - l);
    }

    auto nthreads = getNumIntraThreads(workerId, (int)i);

    IntraParallel bamm(l, beta, subpool, nthreads);

    bamm.Bamm::reduce(std::move(xcols.value()), std::move(ycols.value()),
                      ownMatrices.bx, ownMatrices.by);
  }
}

size_t CombinedParallel::getNumIntraThreads(size_t workerId, int i) {
  auto numInterThreads = (size_t)std::ceil((double)p / std::pow(2.0, i) - 0.5);
  return UtilityFunctions::unevenDivide(workerId, getT(), numInterThreads)
      .length;
}
