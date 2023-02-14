// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_AMM_INTER_HPP_
#define IntelliStream_SRC_AMM_INTER_HPP_

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

class InterParallel : public Bamm {
  struct LockedMatrices {
    std::mutex mtx;
    MatrixPtr bx, by;
  };

  BS::thread_pool_ptr pool;
  std::vector<LockedMatrices> matrices;
  std::barrier<> barrier;

  size_t getT() const { return pool->get_thread_count() + 1; }
  void workerTask(size_t workerId);

public:
  InterParallel(size_t l, scalar_t beta, size_t t)
      : InterParallel(l, beta, std::make_shared<BS::thread_pool>(t - 1)) {}

  InterParallel(size_t l, scalar_t beta, BS::thread_pool_ptr pool)
      : Bamm(l, beta, std::make_unique<SequentialJTS>()), pool{pool},
        barrier{pool->get_thread_count() + 1} {}
  void reduce() override;
};
} // namespace GAMM
#endif
