#ifndef CLAD_DIFFERENTIATOR_JACOBIANMODEVISITOR_H
#define CLAD_DIFFERENTIATOR_JACOBIANMODEVISITOR_H

#include "VectorPushForwardModeVisitor.h"

namespace clad {
class JacobianModeVisitor : public VectorPushForwardModeVisitor {

public:
  JacobianModeVisitor(DerivativeBuilder& builder, const DiffRequest& request);

  DiffMode GetPushForwardMode() override;

  std::string GetPushForwardFunctionSuffix() override;

  DerivativeAndOverload DeriveJacobian();

  StmtDiff VisitReturnStmt(const clang::ReturnStmt* RS) override;
};
} // end namespace clad

#endif // CLAD_DIFFERENTIATOR_JACOBIANMODEVISITOR_H
