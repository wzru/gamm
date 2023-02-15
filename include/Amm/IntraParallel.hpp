// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_AMM_INTRA_HPP_
#define IntelliStream_SRC_AMM_INTRA_HPP_

#include <cstddef>
#include <memory>
#include <optional>

#include <Eigen/Dense>

#include "Amm/Bamm.hpp"
#include "BS_thread_pool.hpp"
#include "Svd/ParallelJTS.hpp"
#include "Utils/ZeroedColumns.hpp"

namespace GAMM {

class IntraParallel : public Bamm {
public:
  IntraParallel(size_t l, scalar_t beta, size_t t)
      : Bamm(l, beta, std::make_unique<ParallelJTS>(t)) {}
  IntraParallel(size_t l, scalar_t beta, BS::thread_pool_ptr pool, size_t t)
      : Bamm(l, beta, std::make_unique<ParallelJTS>(pool, t)) {}
  void reduce() override;
};
} // namespace GAMM
#endif
