#include "Amm/IntraParallel.hpp"
#include "Svd/ParallelJTS.hpp"
#include "Utils/Logger.hpp"

using namespace GAMM;

void IntraParallel::reduce() {
  auto maxSweeps = dynamic_cast<ParallelJTS &>(*svd).getOptions().maxSweeps;
  while (!reductionStepSetup()) {
    INTELLI_VERIFY(reductionStepSvdStep(maxSweeps),
                   "Running maxSweeps number of steps");
    reductionStepFinish();
  }
}
