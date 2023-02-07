// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_UTILS_ZeroedColumns_HPP_
#define IntelliStream_SRC_UTILS_ZeroedColumns_HPP_

#include <Eigen/Dense>
#include <cstddef>
#include <iterator>
#include <limits>
#include <vector>

#include "Utils/UtilityFunctions.hpp"

namespace GAMM {
class ZeroedColumns {
public:
  ZeroedColumns() : nextZeroed{} {}
  ZeroedColumns(size_t size)
      : head{size}, nextZeroed(std::vector{size, NON_ZERO}) {}

  ZeroedColumns(const Matrix &matrix)
      : nextZeroed(std::vector{(size_t)matrix.cols(), NON_ZERO}) {
    for (int i = 0; i < matrix.cols(); ++i) {
      if (matrix.col(i).unaryExpr(std::ref(UtilityFunctions::isZero)).all()) {
        setZeroed(i);
      }
    }
  }

  void resizeFilled(size_t size) {
    // Empty nextZeroed and then fill it with NON_ZERO
    nextZeroed.clear();
    nextZeroed.resize(size, NON_ZERO);
    head = size;
  }
  void resizeEmpty(size_t size) {
    resizeFilled(size);
    for (int i = size - 1; i >= 0; --i) {
      setZeroed(i);
    }
  }

  size_t nzeroed() const noexcept { return zeroedCount; }
  void setZeroed(size_t index);
  size_t getNextZeroed();

private:
  static constexpr size_t NON_ZERO = -1;

  size_t head{0};
  size_t zeroedCount{0};
  std::vector<size_t> nextZeroed;
};
} // namespace GAMM
#endif
