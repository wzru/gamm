// Copyright (C) 2021 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_UTILS_UTILITYFUNCTIONS_HPP_
#define IntelliStream_SRC_UTILS_UTILITYFUNCTIONS_HPP_
#include <Eigen/Dense>
#include <barrier>
#include <experimental/filesystem>
#include <functional>
#include <memory>
#include <string>
/* Period parameters */
#define UTIL_N 624
#define UTIL_M 397
#define UTIL_MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UTIL_UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define UTIL_LOWER_MASK 0x7fffffffUL /* least significant r bits */
#define TRUE 1
#define FALSE 0

namespace GAMM {
typedef float scalar_t;
typedef std::shared_ptr<std::barrier<>> BarrierPtr;
typedef Eigen::Matrix<scalar_t, Eigen::Dynamic, Eigen::Dynamic> Matrix;
typedef std::shared_ptr<Matrix> MatrixPtr;
typedef Eigen::Block<Matrix, Eigen::Dynamic, Eigen::Dynamic, true> MatrixRef;
typedef Eigen::DiagonalMatrix<scalar_t, Eigen::Dynamic> DiagonalMatrix;
typedef Eigen::Matrix<scalar_t, Eigen::Dynamic, 1> Vector;

class UtilityFunctions {
public:
  UtilityFunctions();
  static void init_genrand(unsigned long s);
  static double genrand_real3();
  static long genrand_int31(void);
  static unsigned long genrand_int32(void);
  static std::shared_ptr<std::barrier<>> createBarrier(int count);
  static bool closeTo(scalar_t a, scalar_t b) noexcept;
  static bool isZero(scalar_t a) noexcept { return closeTo(a, 0.0); }
  struct divideResult {
    size_t start_index, length;
  };
  static divideResult unevenDivide(size_t i, size_t m, size_t n) noexcept;

  static scalar_t spectralNorm(const Matrix &mat);

  static uint64_t trailingZeros(uint64_t n);
  static uint64_t leadingZeros(uint64_t n);
  static uint64_t nextPowerOfTwo(uint64_t n);
  // Performs thin QR, Q is stored in mat itself. If transposed is set to to
  // true, the `r` computed is transposed
  static void qr(Matrix &mat, Matrix &r, bool transposed = false);
};
} // namespace GAMM
#endif // IntelliStream_SRC_UTILS_UTILITYFUNCTIONS_HPP_
