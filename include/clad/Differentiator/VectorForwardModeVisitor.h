#ifndef CLAD_VECTOR_FORWARD_MODE_VISITOR_H
#define CLAD_VECTOR_FORWARD_MODE_VISITOR_H

#include "BaseForwardModeVisitor.h"
#include "DerivativeBuilder.h"

#include <unordered_map>

namespace clad {
/// A visitor for processing the function code in vector forward mode.
/// Used to compute derivatives by clad::vector_forward_differentiate.
class VectorForwardModeVisitor : public BaseForwardModeVisitor {
protected:
  llvm::SmallVector<const clang::ValueDecl*, 16> m_IndependentVars;
  /// Map used to keep track of parameter variables w.r.t which the
  /// the derivative is being computed. This is separate from the
  /// m_Variables map because all other intermediate variables will have
  /// derivatives as vectors.
  std::unordered_map<const clang::ValueDecl*, clang::Expr*> m_ParamVariables;
  /// Expression for total number of independent variables. This also includes
  /// the size of array independent variables which will be inferred from the
  /// size of the corresponding clad array they provide at runtime for storing
  /// the derivatives.
  clang::Expr* m_IndVarCountExpr;

public:
  VectorForwardModeVisitor(DerivativeBuilder& builder,
                           const DiffRequest& request);
  ~VectorForwardModeVisitor();

  ///\brief Produces the first derivative of a given function with
  /// respect to multiple parameters.
  ///
  ///\returns The differentiated and potentially created enclosing
  /// context.
  ///
  DerivativeAndOverload Derive() override;

  /// Builds an overload for the vector mode function that has derived params
  /// for all the arguments of the requested function and it calls the original
  /// gradient function internally.
  /// For ex.: if the original function is: double foo(double x, double y)
  /// , then the generated vector mode overload will be:
  /// double foo(double x, double y, void*, void*), irrespective of the
  /// what parameters are requested to be differentiated w.r.t.
  /// Inside it, we will call the original vector mode function with the
  /// original parameters and the derived parameters.
  clang::FunctionDecl* CreateVectorModeOverload();

  /// Builds and returns the sequence of derived function parameters for
  //  vectorized forward mode.
  ///
  /// Information about the original function, derived function, derived
  /// function parameter types and the differentiation mode are implicitly
  /// taken from the data member variables.
  llvm::SmallVector<clang::ParmVarDecl*, 8>
  BuildVectorModeParams(DiffParams& diffParams);

  /// Get an expression used to initialize the one-hot vector for the
  /// given index and size. A one-hot vector is a vector with all elements
  /// set to 0 except for one element which is set to 1.
  ///
  /// For example: for index = 2 and size = 4, the returned expression
  /// is: {0, 0, 1, 0}
  clang::Expr* getOneHotInitExpr(size_t index, size_t size,
                                 clang::QualType type);

  /// Get an expression used to initialize a zero vector of the given size.
  ///
  /// For example: for size = 4, the returned expression is: {0, 0, 0, 0}
  clang::Expr* getZeroInitListExpr(size_t size, clang::QualType type);

  StmtDiff VisitFloatingLiteral(const clang::FloatingLiteral* FL) override;
  StmtDiff VisitIntegerLiteral(const clang::IntegerLiteral* IL) override;
  StmtDiff
  VisitArraySubscriptExpr(const clang::ArraySubscriptExpr* ASE) override;
  StmtDiff VisitReturnStmt(const clang::ReturnStmt* RS) override;
  // Decl is not Stmt, so it cannot be visited directly.
  DeclDiff<clang::VarDecl>
  DifferentiateVarDecl(const clang::VarDecl* VD) override;

  std::string GetPushForwardFunctionSuffix() override;
  DiffMode GetPushForwardMode() override;

  // Function for setting the independent variables for vector mode.
  void SetIndependentVarsExpr(clang::Expr* IndVarCountExpr);
};
} // end namespace clad

#endif // CLAD_VECTOR_FORWARD_MODE_VISITOR_H
