// Copyright (C) 2023 by the INTELLI team (https://github.com/intellistream)

#ifndef IntelliStream_SRC_SVD_SequentialJTS_HPP_
#define IntelliStream_SRC_SVD_SequentialJTS_HPP_

#include <Eigen/Dense>
#include <compare>
#include <memory>

#include "Svd/AbstractJTS.hpp"
#include "Utils/UtilityFunctions.hpp"

namespace GAMM {
class SequentialJTS : public AbstractJTS {
public:
  SequentialJTS() : AbstractJTS() {}
  SequentialJTS(Options options) : AbstractJTS(options) {}

  virtual void startSvd(Matrix matrix) override;
  virtual bool svdStep() override;

private:
  std::vector<ColumnPair> p{};
  std::vector<JacobiRotation> q{};
};

typedef std::unique_ptr<SequentialJTS> SequentialJTSUPtr;
} // namespace GAMM
#endif
