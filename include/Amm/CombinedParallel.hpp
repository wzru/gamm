// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_AMM_COMBINED_HPP_
#define IntelliStream_SRC_AMM_COMBINED_HPP_

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>

#include <Eigen/Dense>

#include "Amm/Bamm.hpp"
#include "BS_thread_pool.hpp"
#include "Svd/SequentialJTS.hpp"
#include "Utils/ZeroedColumns.hpp"

namespace GAMM {

class CombinedParallel : public Bamm {
  struct LockedMatrices {
    std::mutex mtx;
    MatrixPtr bx, by;
  };

  size_t p;
  BS::thread_pool_ptr pool;
  std::vector<LockedMatrices> matrices;
  std::barrier<> barrier;

  // The way CombinedParallel works is that at each level of inter-parallelism,
  // each thread gets equal number of threads to use for intra-parallelism. This
  // works fine with t threads if all the inter-parallel workers are in the same
  // level at the same time. Due to inconsistent execution time however, some
  // worker may proceed to the next level and consume all available threads
  // before a previous level is finished for another worker. By giving p extra
  // threads, there are more than enough threads to execute on.
  size_t getT() const { return pool->get_thread_count() - p + 1; }
  void workerTask(size_t workerId);

  size_t getNumIntraThreads(size_t workerId, int i);

public:
  // See getT() for the reason for t+p-1 threads being spawned
  CombinedParallel(size_t l, scalar_t beta, size_t t, size_t p)
      : CombinedParallel(l, beta, std::make_shared<BS::thread_pool>(t + p - 1),
                         p) {}

  CombinedParallel(size_t l, scalar_t beta, BS::thread_pool_ptr pool, size_t p)
      : Bamm(l, beta, std::make_unique<SequentialJTS>()), p{p}, pool{pool},
        barrier(p) {}

  void reduce() override;
};
} // namespace GAMM
#endif
