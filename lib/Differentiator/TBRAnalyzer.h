#ifndef CLAD_DIFFERENTIATOR_TBRANALYZER_H
#define CLAD_DIFFERENTIATOR_TBRANALYZER_H

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Analysis/CFG.h"

#include "clad/Differentiator/CladUtils.h"
#include "clad/Differentiator/Compatibility.h"


#include <map>
#include <unordered_map>

using namespace clang;

namespace clad {

/// Gradient computation requres reversal of the control flow of the original
/// program becomes necessary. To guarantee correctness, certain values that are
/// computed and overwritten in the original program must be made available in
/// the adjoint program. They can be determined by performing a static data flow
/// analysis, the so-called To-Be-Recorded (TBR) analysis. Overestimation of
/// this set must be kept minimal to get efficient adjoint codes.
///
/// This class implements this to-be-recorded analysis.
class TBRAnalyzer : public clang::RecursiveASTVisitor<TBRAnalyzer> {
  /// ProfileID is the key type for ArrMap used to represent array indices
  /// and object fields.
  using ProfileID = clad_compat::FoldingSetNodeID;
  using hash_code = clad_compat::hash_code;
  using HashSequence = llvm::SmallVector<hash_code, 1>;

  ProfileID getProfileID(const IntegerLiteral* IL) const {
    ProfileID profID;
    IL->Profile(profID, m_Context, /* Canonical */ true);
    return profID;
  }

  static ProfileID getProfileID(const Decl* D) {
    ProfileID profID;
    profID.AddPointer(D);
    return profID;
  }


  void getProfileIDHash(const Expr* E, HashSequence& hashSequence) {
    E = E->IgnoreImplicit();
    if (const auto* DRE = dyn_cast<DeclRefExpr>(E)) {
      hash_code hash = getProfileID(E->getDecl()).ComputeHash();
      hashSequence.push_back(hash);
    } else if (const auto* ME = dyn_cast<MemberExpr>(E)) {
      getProfileIDHash(ME->getBase(), hashSequence);
      const auto* FD = cast<clang::FieldDecl>(ME->getMemberDecl());
      hash_code fieldHash = getProfileID(FD).ComputeHash();
      hashSequence.push_back(fieldHash);
    } else if (const auto* ASE = dyn_cast<ArraySubscriptExpr>(E)) {
      getProfileIDHash(ASE->getBase(), hashSequence);
      hash_code IndexHash = 0;
      if (const auto* IL = dyn_cast<clang::IntegerLiteral>(ASE->getIdx()))
        IndexHash = getProfileID(IL).ComputeHash();
      hashSequence.push_back(IndexHash);
    }
  }

  /// Whenever an array element with a non-constant index is set to required
  /// this function is used to set to required all the array elements that
  /// could match that element (e.g. set 'a[1].y' and 'a[6].y' to required
  /// when 'a[k].y' is set to required). Takes unwrapped sequence of
  /// indices/members of the expression being overlaid and the index of of the
  /// current index/member.
  // void overlay(VarData& targetData, llvm::SmallVector<ProfileID, 2>& IDSequence,
  //              size_t i);
  /// Returns true if there is at least one required to store node among
  /// child nodes.
  bool findReq(const HashSequence& varData);
  /// Used to merge together VarData for one variable from two branches
  /// (e.g. after an if-else statements). Look at the Control Flow section for
  /// more information.
  void merge(TBRStatus& targetData, const TBRStatus& mergeData);
  /// Used to recursively copy VarData when separating into different branches
  /// (e.g. when entering an if-else statements). Look at the Control Flow
  /// section for more information.
  VarData copy(VarData& copyData);

  clang::CFGBlock* getCFGBlockByID(unsigned ID);


  /// Whenever an array element with a non-constant index is set to required
  /// this function is used to set to required all the array elements that
  /// could match that element (e.g. set 'a[1].y' and 'a[6].y' to required when
  /// 'a[k].y' is set to required). Unwraps the a given expression into a
  /// sequence of indices/members of the expression being overlaid and calls
  /// VarData::overlay() recursively.
  // void overlay(const clang::Expr* E);

  enum TBRStatus {
    USEFUL,
    USELESS,
    UNDEFINED
  };

  /// Used to store all the necessary information about variables at a
  /// particular moment.
  /// Note: the VarsData of one CFG block only stores information specific
  /// to that block and relies on the VarsData of it's predecessors
  /// for old information. This is done to avoid excessive copying
  /// memory and usage.
  /// Note: 'this' pointer does not have a declaration so nullptr is used as
  /// its key instead.
  struct VarsData {
    std::unordered_map<const HashSequence, TBRStatus> m_Data;
    VarsData* m_Prev = nullptr;

    VarsData() = default;
    VarsData(const VarsData& other) = default;
    ~VarsData() = default;
    VarsData(VarsData&& other) noexcept
        : m_Data(std::move(other.m_Data)), m_Prev(other.m_Prev) {}
    VarsData& operator=(const VarsData& other) = delete;
    VarsData& operator=(VarsData&& other) noexcept {
      if (&m_Data == &other.m_Data) {
        m_Data = std::move(other.m_Data);
        m_Prev = other.m_Prev;
      }
      return *this;
    }

    using iterator =
        std::unordered_map<const HashSequence, TBRStatus>::iterator;
    iterator begin() { return m_Data.begin(); }
    iterator end() { return m_Data.end(); }
    TBRStatus& operator[](const clang::VarDecl* VD) { return m_Data[VD]; }
    iterator find(const clang::VarDecl* VD) { return m_Data.find(VD); }
    void clear() { m_Data.clear(); }
  };

  /// Collects the data from 'varsData' and its predecessors until
  /// 'limit' into one map ('limit' VarsData is not included).
  /// If 'limit' is 'nullptr', data is collected starting with
  /// the entry CFG block.
  VarsData static collectDataFromPredecessors(VarsData* varsData,
                                                   VarsData* limit = nullptr);

  /// Finds the lowest common ancestor of two VarsData
  /// (based on the m_Prev field in VarsData).
  static VarsData* findLowestCommonAncestor(VarsData* varsData1,
                                            VarsData* varsData2);

  /// Merges mergeData into targetData. Should be called
  /// after mergeData is passed and the corresponding CFG
  /// block is one of the predecessors of targetData's CFG block
  /// (e.g. instance when merging if- and else- blocks).
  /// Note: The first predecessor (targetData->m_Prev) does NOT have
  /// to be merged to targetData.
  void merge(VarsData* targetData, VarsData* mergeData);

  /// Used to find DeclRefExpr's that will be used in the backwards pass.
  /// In order to be marked as required, a variables has to appear in a place
  /// where it would have a differential influence and will appear non-linearly
  /// (e.g. for 'x = 2 * y;', y will not appear in the backwards pass). Hence,
  /// markingMode and nonLinearMode.
  enum Mode { kMarkingMode = 1, kNonLinearMode = 2 };
  /// Tells if the variable at a given location is required to store. Basically,
  /// is the result of analysis.
  std::set<clang::SourceLocation> m_TBRLocs;

  /// Stores modes in a stack (used to retrieve the old mode after entering
  /// a new one).
  std::vector<int> m_ModeStack;

  ASTContext& m_Context;

  /// clang::CFG of the function being analysed.
  std::unique_ptr<clang::CFG> m_CFG;

  /// Stores VarsData structures for CFG blocks (the indices in
  /// the vector correspond to CFG blocks' IDs)
  std::vector<std::unique_ptr<VarsData>> m_BlockData;

  /// Stores the number of performed passes for a given CFG block index.
  std::vector<short> m_BlockPassCounter;

  /// ID of the CFG block being visited.
  unsigned m_CurBlockID{};

  /// The set of IDs of the CFG blocks that should be visited.
  std::set<unsigned> m_CFGQueue;

  //// Setters
  /// Creates VarData for a new VarDecl*.
  void addVar(const clang::VarDecl* VD);
  void copyVarToCurBlock(const clang::VarDecl* VD);
  /// Marks the SourceLocation of E if it is required to store.
  /// E could be DeclRefExpr*, ArraySubscriptExpr* or MemberExpr*.
  void markLocation(const clang::Expr* E);
  /// Sets E's corresponding VarData (or all its child nodes) to
  /// required/not required. For isReq==true, checks if the current mode is
  /// markingMode and nonLinearMode. E could be DeclRefExpr*,
  /// ArraySubscriptExpr* or MemberExpr*.
  void setIsRequired(const clang::Expr* E, bool isReq = true);

  /// Returns the VarsData of the CFG block being visited.
  VarsData& getCurBlockVarsData() { return *m_BlockData[m_CurBlockID]; }

  //// Modes Setters
  /// Sets the mode manually
  void setMode(int mode) { m_ModeStack.push_back(mode); }
  /// Sets nonLinearMode but leaves markingMode just as it was.
  void startNonLinearMode() {
    m_ModeStack.push_back(m_ModeStack.back() | Mode::kNonLinearMode);
  }
  /// Sets markingMode but leaves nonLinearMode just as it was.
  void startMarkingMode() {
    m_ModeStack.push_back(Mode::kMarkingMode | m_ModeStack.back());
  }
  /// Removes the last mode in the stack (retrieves the previous one).
  void resetMode() { m_ModeStack.pop_back(); }

public:
  /// Constructor
  TBRAnalyzer(ASTContext& Context) : m_Context(Context) {
    m_ModeStack.push_back(0);
  }

  /// Destructor
  ~TBRAnalyzer() = default;

  /// Delete copy/move operators and constructors.
  TBRAnalyzer(const TBRAnalyzer&) = delete;
  TBRAnalyzer& operator=(const TBRAnalyzer&) = delete;
  TBRAnalyzer(const TBRAnalyzer&&) = delete;
  TBRAnalyzer& operator=(const TBRAnalyzer&&) = delete;

  /// Returns the result of the whole analysis
  std::set<clang::SourceLocation> getResult() { return m_TBRLocs; }

  /// Visitors
  void Analyze(const clang::FunctionDecl* FD);

  void VisitCFGBlock(const clang::CFGBlock& block);

  bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr* ASE);
  bool VisitBinaryOperator(clang::BinaryOperator* BinOp);
  bool VisitCallExpr(clang::CallExpr* CE);
  bool VisitConditionalOperator(clang::ConditionalOperator* CO);
  bool VisitCXXConstructExpr(clang::CXXConstructExpr* CE);
  bool VisitDeclRefExpr(clang::DeclRefExpr* DRE);
  bool VisitDeclStmt(clang::DeclStmt* DS);
  bool VisitInitListExpr(clang::InitListExpr* ILE);
  bool VisitMemberExpr(clang::MemberExpr* ME);
  bool VisitUnaryOperator(clang::UnaryOperator* UnOp);
};

} // end namespace clad
#endif // CLAD_DIFFERENTIATOR_TBRANALYZER_H
