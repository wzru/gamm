#include "Amm/Single.hpp"
#include "Svd/SequentialJTS.hpp"
#include "Utils/Logger.hpp"

using namespace GAMM;

void Single::reduce() {
  auto maxSweeps = dynamic_cast<SequentialJTS &>(*svd).getOptions().maxSweeps;

  while (!reductionStepSetup()) {
    INTELLI_VERIFY(reductionStepSvdStep(maxSweeps),
                   "Running maxSweeps number of steps");
    reductionStepFinish();
  }
}
