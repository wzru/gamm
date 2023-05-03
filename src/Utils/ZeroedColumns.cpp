#include <Utils/Logger.hpp>
#include <Utils/ZeroedColumns.hpp>
#include <cstddef>

using namespace GAMM;

void ZeroedColumns::resizeFilled(size_t size) {
  // Empty nextZeroed and then fill it with NON_ZERO
  nextZeroed.clear();
  nextZeroed.resize(size, NON_ZERO);
  head = size;
}

void ZeroedColumns::resizeEmpty(size_t size) {
  resizeFilled(size);
  for (size_t i = 0; i < size; ++i) {
    nextZeroed[i] = i + 1;
  }
  head = 0;
  zeroedCount = size;
}

void ZeroedColumns::fromMatrix(const Matrix &matrix) {
  resizeFilled(matrix.cols());
  for (int i = 0; i < matrix.cols(); ++i) {
    auto isZero =
        matrix.col(i).unaryExpr(std::ref(UtilityFunctions::isZero)).all();
    // std::cout << "Index " << i << " is zero? " << isZero << "\n";
    if (isZero) {
      setZeroed(i);
    }
  }
}

void ZeroedColumns::setZeroed(size_t index) {
  if (nextZeroed[index] != NON_ZERO) {
    // INTELLI_WARNING("Setting already zeroed column (" << index << ") to zero");
    // Reduce zeroedCount so that increment later will restore it to the same
    // count
    zeroedCount--;
  }

  nextZeroed[index] = head;
  head = index;
  zeroedCount++;
}

size_t ZeroedColumns::getNextZeroed() {
  INTELLI_ASSERT(zeroedCount != 0, "Cannot get a zeroed column");

  auto oldHead = head;
  head = nextZeroed[head];
  nextZeroed[oldHead] = NON_ZERO;
  zeroedCount--;
  return oldHead;
}
