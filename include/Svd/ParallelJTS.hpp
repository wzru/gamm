// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_SVD_SequentialJTS_HPP_
#define IntelliStream_SRC_SVD_SequentialJTS_HPP_

#include <Eigen/Dense>
#include <atomic>
#include <barrier>
#include <compare>

#include <BS_thread_pool.hpp>
#include <limits>

#include "Svd/Svd.hpp"
#include "Utils/UtilityFunctions.hpp"

namespace GAMM {
class ParallelJTS : public Svd {
public:
  struct Options {
    size_t tau = 32, maxSweeps = 100, t;
    scalar_t tol = 1e-7;
    BS::thread_pool_ptr pool;

    Options(BS::thread_pool_ptr pool)
        : t{pool->get_thread_count() + 1}, pool{pool} {}
  };

  ParallelJTS(Options options)
      : options{std::move(options)}, barrier{
                                         static_cast<ptrdiff_t>(options.t)} {}

  virtual void startSvd(Matrix matrix) override;
  virtual bool svdStep() override;
  virtual bool svdStep(size_t nsteps) override;
  virtual void finishSvd() override;

private:
  static constexpr size_t COMPLETED = std::numeric_limits<size_t>::max();

  constexpr size_t nColumnPairs() const noexcept {
    return b.cols() * (b.cols() - 1) / 2;
  }
  constexpr size_t npivots() const noexcept {
    return nColumnPairs() / options.tau;
  }

  auto &p() { return pToUse ? p2 : p1; }

  bool workerTask(size_t workerId, size_t nsteps);

  void workerTaskPhase1(size_t workerId);
  void workerTaskPhase2(size_t workerId);
  void workerTaskPhase3(size_t workerId);

  Options options;

  size_t iterNumber;
  Matrix b;
  scalar_t delta;

  std::vector<ColumnPair> p1;
  std::vector<ColumnPair> p2;
  int pToUse{0};
  std::vector<JacobiRotation> q;

  std::barrier<> barrier;
  std::atomic_size_t counter;
  // (lock, data guarded)
  //
  // The data guarded is the length of the slice which has been sorted by the
  // worker who had that lock
  std::vector<std::pair<std::mutex, size_t>> sortLocks;
};

typedef std::unique_ptr<ParallelJTS> ParallelJTSUPtr;
} // namespace GAMM
#endif
