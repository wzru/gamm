// Copyright (C) 2021 by the IntelliStream team
// (https://github.com/intellistream)

#include <Eigen/SVD>
#include <Utils/Logger.hpp>
#include <Utils/UtilityFunctions.hpp>
#include <cmath>
#include <cstdint>
#include <limits>

using namespace GAMM;

static unsigned long mt[UTIL_N]; /* the array for the state vector  */
static int mti;                  /* mti==N+1 means mt[N] is not initialized */

long UtilityFunctions::genrand_int31() { return long(genrand_int32() >> 1); }
unsigned long UtilityFunctions::genrand_int32() {
  unsigned long y;
  static unsigned long mag01[2] = {0x0UL, UTIL_MATRIX_A};
  /* mag01[x] = x * MATRIX_A  for x=0,1 */

  if (mti >= UTIL_N) { /* generate N words at one time */
    int kk;

    if (mti == UTIL_N + 1)  /* if init_genrand() has not been called, */
      init_genrand(5489UL); /* a default initial seed is used */

    for (kk = 0; kk < UTIL_N - UTIL_M; kk++) {
      y = (mt[kk] & UTIL_UPPER_MASK) | (mt[kk + 1] & UTIL_LOWER_MASK);
      mt[kk] = mt[kk + UTIL_M] ^ (y >> 1) ^ mag01[y & 0x1UL];
    }
    for (; kk < UTIL_N - 1; kk++) {
      y = (mt[kk] & UTIL_UPPER_MASK) | (mt[kk + 1] & UTIL_LOWER_MASK);
      mt[kk] = mt[kk + (UTIL_M - UTIL_N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
    }
    y = (mt[UTIL_N - 1] & UTIL_UPPER_MASK) | (mt[0] & UTIL_LOWER_MASK);
    mt[UTIL_N - 1] = mt[UTIL_M - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];

    mti = 0;
  }

  y = mt[mti++];

  /* Tempering */
  y ^= (y >> 11);
  y ^= (y << 7) & 0x9d2c5680UL;
  y ^= (y << 15) & 0xefc60000UL;
  y ^= (y >> 18);

  return y;
}
UtilityFunctions::UtilityFunctions() {
  mti = UTIL_N + 1; /* mti==N+1 means mt[N] is not initialized */
}

/**
 * initializes mt[N] with a seed
 * @param s
 */
void UtilityFunctions::init_genrand(unsigned long s) {
  /* initializes mt[N] with a seed */
  mt[0] = s & 0xffffffffUL;
  for (mti = 1; mti < UTIL_N; mti++) {
    mt[mti] = (1812433253UL * (mt[mti - 1] ^ (mt[mti - 1] >> 30)) + mti);
    /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
    /* In the previous versions, MSBs of the seed affect   */
    /* only MSBs of the array mt[].                        */
    /* 2002/01/09 modified by Makoto Matsumoto             */
    mt[mti] &= 0xffffffffUL;
    /* for >32 bit machines */
  }
}

/**
 *  generates a random number on (0,1)-real-interval
 * @return
 */
double UtilityFunctions::genrand_real3() {
  return (((double)genrand_int32()) + 0.5) * (1.0 / 4294967296.0);
  /* divided by 2^32 */
}
std::shared_ptr<std::barrier<>> UtilityFunctions::createBarrier(int count) {
  return std::make_shared<std::barrier<>>(count);
}

bool UtilityFunctions::closeTo(scalar_t a, scalar_t b) noexcept {
  return std::abs(a - b) < std::numeric_limits<scalar_t>::epsilon();
}

UtilityFunctions::divideResult
UtilityFunctions::unevenDivide(size_t i, size_t m, size_t n) noexcept {

  auto base = m / n;
  auto extra = m % n;

  if (i < extra) {
    return divideResult{i * (base + 1), base + 1};
  } else {
    return divideResult{i * base + extra, base};
  }
}

scalar_t UtilityFunctions::spectralNorm(const Matrix &mat) {
  const Eigen::BDCSVD<Matrix> svd{mat};

  return svd.singularValues().diagonal()[0];
}

#if defined(__GNUC__) && defined(__cplusplus)
uint64_t UtilityFunctions::trailingZeros(uint64_t n) {
  return __builtin_ctzll(n);
}

uint64_t UtilityFunctions::leadingZeros(uint64_t n) {
  return __builtin_clzll(n);
}
#endif

uint64_t UtilityFunctions::nextPowerOfTwo(uint64_t n) {
  return (n <= 1)
             ? 1
             : (std::numeric_limits<uint64_t>::max() >> leadingZeros(n - 1)) +
                   1;
}

void UtilityFunctions::qr(Matrix &q, Matrix &r, bool transposed) {
  auto n = q.cols();
  INTELLI_ASSERT(r.rows() == n, "r is not (n, n) matrix");
  INTELLI_ASSERT(r.cols() == n, "r is not (n, n) matrix");

  for (int k = 0; k < n; ++k) {
    for (int i = 0; i < k; ++i) {
      auto [r_row, r_col] = transposed ? std::array{k, i} : std::array{i, k};
      auto d = q.col(i).dot(q.col(k));

      if (isZero(d)) {
        d = 0.0;
      }

      if (std::isinf(d)) {
        INTELLI_ERROR("Found inf");
      }
      if (std::isnan(d)) {
        INTELLI_ERROR("Found NaN");
      }
      r(r_row, r_col) = d;
      q.col(k).noalias() -= d * q.col(i);
    }

    auto norm = q.col(k).norm();
    if (isZero(norm)) {
      norm = 0.0;
    } else {
      q.col(k) /= norm;
    }

    r(k, k) = norm;
  }
}
