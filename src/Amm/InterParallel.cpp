#include "Amm/InterParallel.hpp"
#include "Amm/Single.hpp"
#include "Utils/Logger.hpp"

using namespace GAMM;

void InterParallel::reduce() {
  auto t = getT();

  std::vector<LockedMatrices> temp{getT()};
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
  if (t * l > (size_t)d) {
    INTELLI_WARNING("Not running inter-parallelism due to small size: "
                    << t << " * " << l << " > " << d);
    return;
  }

  BS::multi_future<void> tasks(t - 1);

  for (size_t i = 1; i < t; ++i) {
    tasks[i - 1] = pool->submit([this, i]() { workerTask(i); });
  }
  workerTask(0);
  tasks.wait();

  matrices.clear();
}

void InterParallel::workerTask(size_t workerId) {
  auto d = x.value().cols();
  auto t = getT();
  auto nruns = UtilityFunctions::trailingZeros(
                   workerId | UtilityFunctions::nextPowerOfTwo(t)) +
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
      if (otherId >= t) {
        break;
      }

      otherGuard.emplace(matrices[otherId].mtx);

      // leftCols is used create a block pointing to the whole matrix
      xcols = matrices[otherId].bx->leftCols(l);
      ycols = matrices[otherId].by->leftCols(l);
    } else {
      auto [startCol, numCols] = UtilityFunctions::unevenDivide(workerId, d, t);

      *ownMatrices.bx = x.value().middleCols(startCol, l);
      *ownMatrices.by = y.value().middleCols(startCol, l);

      // We need nested expression to make the types match
      xcols =
          x.value().nestedExpression().middleCols(startCol + l, numCols - l);
      ycols =
          y.value().nestedExpression().middleCols(startCol + l, numCols - l);
    }

    Single bamm(l, beta);

    bamm.Bamm::reduce(std::move(xcols.value()), std::move(ycols.value()),
                      ownMatrices.bx, ownMatrices.by);
  }
}
