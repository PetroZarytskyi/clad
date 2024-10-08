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

void JacobianModeVisitor::ExecuteInsidePushforwardFunctionBlock() {
  SetIndependentVarsExpr();

  // Expression for maintaining the number of independent variables processed
  // till now present as array elements. This will be sum of sizes of all such
  // arrays.
  Expr* arrayIndVarCountExpr = nullptr;

  // Number of non-array independent variables processed till now.
  size_t nonArrayIndVarCount = 0;

  // Current Index of independent variable in the param list of the function.
  size_t independentVarIndex = 0;

  size_t numParamsOriginalFn = m_DiffReq->getNumParams();
  SourceLocation loc{m_DiffReq->getLocation()};

  for (size_t i = 0; i < numParamsOriginalFn; ++i) {
    bool is_array =
        utils::isArrayOrPointerType(m_DiffReq->getParamDecl(i)->getType());
    VarDecl* param = m_Derivative->getParamDecl(i);
    QualType dParamType = clad::utils::GetValueType(param->getType());

    Expr* dVectorParam = nullptr;
    if (m_DiffReq.DVI.size() > independentVarIndex &&
        m_DiffReq.DVI[independentVarIndex].param == m_DiffReq->getParamDecl(i)) {

      // Current offset for independent variable.
      Expr* offsetExpr = arrayIndVarCountExpr;
      Expr* nonArrayIndVarCountExpr = ConstantFolder::synthesizeLiteral(
          m_Context.UnsignedLongTy, m_Context, nonArrayIndVarCount);
      if (!offsetExpr)
        offsetExpr = nonArrayIndVarCountExpr;
      else if (nonArrayIndVarCount != 0)
        offsetExpr = BuildOp(BinaryOperatorKind::BO_Add, offsetExpr,
                             nonArrayIndVarCountExpr);

      if (is_array) {
        Expr* matrix = m_Variables[m_Derivative->getParamDecl(i)];
        Expr* numRows = BuildCallExprToMemFn(matrix, /*MemberFunctionName=*/"rows", {});

        // Create an identity matrix for the parameter,
        // with number of rows equal to the size of the array,
        // and number of columns equal to the number of independent variables
        llvm::SmallVector<Expr*, 3> args = {numRows, m_IndVarCountExpr,
                                            offsetExpr};
        dVectorParam = BuildIdentityMatrixExpr(dParamType, args, loc);

        // Update the array independent expression.
        if (!arrayIndVarCountExpr) {
          arrayIndVarCountExpr = numRows;
        } else {
          arrayIndVarCountExpr = BuildOp(BinaryOperatorKind::BO_Add,
                                         arrayIndVarCountExpr, numRows);
        }
      } else {
        // Create a one hot vector for the parameter.
        llvm::SmallVector<Expr*, 2> args = {m_IndVarCountExpr, offsetExpr};
        dVectorParam = BuildCallExprToCladFunction("one_hot_vector", args,
                                                   {dParamType}, loc);
        ++nonArrayIndVarCount;
      }
      ++independentVarIndex;
    } else {
      // We cannot initialize derived variable for pointer types because
      // we do not know the correct size.
      if (is_array)
        continue;
      // This parameter is not an independent variable.
      // Initialize by all zeros.
      dVectorParam = BuildCallExprToCladFunction(
          "zero_vector", {m_IndVarCountExpr}, {dParamType}, loc);
    }

    // For each function arg to be differentiated, create a variable
    // _d_vector_arg to store the vector of derivatives for that arg.
    // for ex: double f(double x, double y, double z);
    // and we want to differentiate w.r.t. x and z, then we will have
    // -> _d_vector_x = {1, 0};
    // -> _d_vector_y = {0, 0};
    // -> _d_vector_z = {0, 1};
    VarDecl* paramDiff = m_Derivative->getParamDecl(numParamsOriginalFn + i);
    Expr* paramAssignment = BuildOp(BO_Assign, BuildDeclRef(paramDiff), dVectorParam);
    addToCurrentBlock(paramAssignment);
  }

  BaseForwardModeVisitor::ExecuteInsidePushforwardFunctionBlock();
}

DerivativeAndOverload JacobianModeVisitor::DeriveJacobian() {
  return DerivePushforward();
}

StmtDiff JacobianModeVisitor::VisitReturnStmt(const clang::ReturnStmt* RS) {
  // If there is no return value, we must not attempt to differentiate
  if (!RS->getRetValue())
    return nullptr;

  StmtDiff retValDiff = Visit(RS->getRetValue());
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
