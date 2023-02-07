#include "Amm/Single.hpp"
#include "Utils/Logger.hpp"

using namespace GAMM;

void Single::reduce() {
  INTELLI_TRACE("SINGLE REDUCE");
  INTELLI_DEBUG("X has NaN? " << x.value()->array().abs().isNaN().any());
  INTELLI_DEBUG("Y has NaN? " << y.value()->array().abs().isNaN().any());

  int i = 0;
  while ((long int)xi < x.value()->cols()) {
    INTELLI_TRACE("STARTING REDUCTION STEP");
    auto bxHasNaN = bx.value()->array().abs().isNaN().any();
    INTELLI_DEBUG("BX has NaN? " << bxHasNaN);
    INTELLI_DEBUG("BY has NaN? " << by.value()->array().abs().isNaN().any());
    if (bxHasNaN) {
      INTELLI_FATAL_ERROR("BX HAS NAN. EXITING");
      return;
    }

    reductionStepSetup();
    while (!reductionStepSvdStep()) {
      INTELLI_TRACE("SVD STEP");
    }
    INTELLI_TRACE("FINISHING REDUCTION STEP");
    reductionStepFinish();
    INTELLI_DEBUG("Iter " << i);
    INTELLI_DEBUG("BX:\n" << (*bx.value()));
    INTELLI_DEBUG("BY:\n" << (*by.value()));
    ++i;
  }
}
