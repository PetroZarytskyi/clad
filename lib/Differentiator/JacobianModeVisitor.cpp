#include "clad/Differentiator/JacobianModeVisitor.h"

#include "ConstantFolder.h"
#include "clad/Differentiator/CladUtils.h"

#include "llvm/Support/SaveAndRestore.h"

using namespace clang;

namespace clad {
JacobianModeVisitor::JacobianModeVisitor(DerivativeBuilder& builder,
                                         const DiffRequest& request)
    : VectorPushForwardModeVisitor(builder, request) {}

DiffMode JacobianModeVisitor::GetPushForwardMode() {
  return DiffMode::jacobian;
}

std::string JacobianModeVisitor::GetPushForwardFunctionSuffix() {
  return "_jac";
}

DerivativeAndOverload JacobianModeVisitor::DeriveJacobian() {
  return DerivePushforward();
}

StmtDiff JacobianModeVisitor::VisitReturnStmt(const clang::ReturnStmt* RS) {
  // If there is no return value, we must not attempt to differentiate
  if (!RS->getRetValue())
    return nullptr;

  StmtDiff retValDiff = Visit(RS->getRetValue());
  // return StmtDiff(retValDiff.getExpr_dx());
  // This can instantiate as part of the move or copy initialization and
  // needs a fake source location.
  SourceLocation fakeLoc = utils::GetValidSLoc(m_Sema);
  Stmt* returnStmt =
      m_Sema
          .ActOnReturnStmt(fakeLoc, retValDiff.getExpr_dx(), getCurrentScope())
          .get();
  return StmtDiff(returnStmt);
}
} // end namespace clad
