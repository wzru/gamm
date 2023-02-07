#include <Utils/Logger.hpp>
#include <Utils/ZeroedColumns.hpp>
#include <cstddef>

using namespace GAMM;

void ZeroedColumns::setZeroed(size_t index) {
  INTELLI_TRACE("SetZeroed " << index);
  INTELLI_ASSERT(nextZeroed[index] == NON_ZERO,
                 "Setting zero column to non_zero");

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
