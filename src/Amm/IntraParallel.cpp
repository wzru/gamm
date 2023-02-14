#include "Amm/IntraParallel.hpp"
#include "Svd/ParallelJTS.hpp"
#include "Utils/Logger.hpp"

using namespace GAMM;

void IntraParallel::reduce() {
  auto maxSweeps = dynamic_cast<ParallelJTS &>(*svd).getOptions().maxSweeps;
  while ((long int)xi < x.value().cols()) {
    reductionStepSetup();
    INTELLI_ASSERT(reductionStepSvdStep(maxSweeps),
                   "Running maxSweeps number of steps");
    reductionStepFinish();
  }
}
