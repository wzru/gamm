#include "Amm/IntraParallel.hpp"
#include "Svd/ParallelJTS.hpp"
#include "Utils/Logger.hpp"

using namespace GAMM;

void IntraParallel::reduce() {
  while ((long int)xi < x.value()->cols()) {
    reductionStepSetup();
    INTELLI_ASSERT(
        reductionStepSvdStep(
            dynamic_cast<ParallelJTS &>(*svd).getOptions().maxSweeps),
        "Running maxSweeps number of steps");
    reductionStepFinish();
  }
}
