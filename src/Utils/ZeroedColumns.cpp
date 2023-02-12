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

void ZeroedColumns::setZeroed(size_t index) {
  INTELLI_TRACE("SetZeroed " << index);
  if (nextZeroed[index] != NON_ZERO) {
    INTELLI_WARNING("Setting already zeroed column (" << index << ") to zero");
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
