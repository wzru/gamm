// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_AMM_SINGLE_HPP_
#define IntelliStream_SRC_AMM_SINGLE_HPP_

#include <cstddef>
#include <memory>

#include <Eigen/Dense>
#include <optional>

#include "Amm/Bamm.hpp"
#include "Svd/SequentialJTS.hpp"
#include "Utils/ZeroedColumns.hpp"

namespace GAMM {

class Single : public Bamm {
public:
  Single(size_t l, scalar_t beta)
      : Bamm(l, beta, std::make_unique<SequentialJTS>()) {}
  void reduce() override;
};
} // namespace GAMM
#endif
