#include "clad/Differentiator/VectorPushForwardModeVisitor.h"

#include "ConstantFolder.h"
#include "clad/Differentiator/CladUtils.h"

#include "llvm/Support/SaveAndRestore.h"

using namespace clang;

namespace clad {
VectorPushForwardModeVisitor::VectorPushForwardModeVisitor(
    DerivativeBuilder& builder, const DiffRequest& request)
    : VectorForwardModeVisitor(builder, request) {}

VectorPushForwardModeVisitor::~VectorPushForwardModeVisitor() = default;

void VectorPushForwardModeVisitor::ExecuteInsidePushforwardFunctionBlock() {
  SetIndependentVarsExpr();
  BaseForwardModeVisitor::ExecuteInsidePushforwardFunctionBlock();
}

StmtDiff
VectorPushForwardModeVisitor::VisitReturnStmt(const clang::ReturnStmt* RS) {
  // If there is no return value, we must not attempt to differentiate
  if (!RS->getRetValue())
    return nullptr;

  StmtDiff retValDiff = Visit(RS->getRetValue());
  llvm::SmallVector<Expr*, 2> returnValues = {retValDiff.getExpr(),
                                              retValDiff.getExpr_dx()};
  // This can instantiate as part of the move or copy initialization and
  // needs a fake source location.
  SourceLocation fakeLoc = utils::GetValidSLoc(m_Sema);
  Expr* initList = m_Sema.ActOnInitList(fakeLoc, returnValues, noLoc).get();
  Stmt* returnStmt =
      m_Sema.ActOnReturnStmt(fakeLoc, initList, getCurrentScope()).get();
  return StmtDiff(returnStmt);
}

} // end namespace clad
