// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_SVD_ParallelJTS_HPP_
#define IntelliStream_SRC_SVD_ParallelJTS_HPP_

#include <Eigen/Dense>
#include <atomic>
#include <barrier>
#include <compare>

#include <BS_thread_pool.hpp>
#include <limits>

#include "Svd/AbstractJTS.hpp"
#include "Utils/UtilityFunctions.hpp"

namespace GAMM {
class ParallelJTS : public AbstractJTS {
public:
  ParallelJTS(size_t t)
      : AbstractJTS(), pool{std::make_shared<BS::thread_pool>(t - 1)},
        barrier(t) {}
  ParallelJTS(Options options, BS::thread_pool_ptr pool)
      : AbstractJTS(options), pool{pool},
        barrier(pool->get_thread_count() + 1) {}

  virtual void startSvd(Matrix matrix) override;
  virtual bool svdStep() override;
  virtual bool svdStep(size_t nsteps) override;

private:
  static constexpr size_t COMPLETED = std::numeric_limits<size_t>::max();

  auto &getP() noexcept { return pToUse ? p2 : p1; }
  size_t getT() const noexcept { return pool->get_thread_count() + 1; }

  bool workerTask(size_t workerId, size_t nsteps);

  void workerTaskPhase1(size_t workerId);
  void workerTaskPhase2(size_t workerId);
  void workerTaskPhase3(size_t workerId);

  BS::thread_pool_ptr pool;

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
