clad_compat__find_first_existing_vc_file("${CLANG_SOURCE_DIR}" clang_vc)
clad_compat__find_first_existing_vc_file("${CLAD_SOURCE_DIR}" clad_vc)

# The VC revision include that we want to generate.
set(version_inc "${CMAKE_CURRENT_BINARY_DIR}/VCSVersion.inc")
clad_compat__DefineCustomCommandVCS("${version_inc}" "${LLVM_CMAKE_DIR}" "${CLANG_SOURCE_DIR}" "${CLAD_SOURCE_DIR}" "${clang_vc}" "${cald_vc}")

# Mark the generated header as being generated.
set_source_files_properties("${version_inc}"
  PROPERTIES GENERATED TRUE
             HEADER_FILE_ONLY TRUE)

set_property(SOURCE Version.cpp APPEND PROPERTY
             COMPILE_DEFINITIONS "HAVE_VCS_VERSION_INC")

# (Ab)use llvm facilities for adding libraries.
llvm_add_library(cladDifferentiator
  STATIC
  ActivityAnalyzer.cpp
  BaseForwardModeVisitor.cpp
  CladUtils.cpp
  ConstantFolder.cpp
  DerivativeBuilder.cpp
  DerivedFnCollector.cpp
  DerivedFnInfo.cpp
  DiffPlanner.cpp
  ErrorEstimator.cpp
  EstimationModel.cpp
  JacobianModeVisitor.cpp
  HessianModeVisitor.cpp
  MultiplexExternalRMVSource.cpp
  PushForwardModeVisitor.cpp
  ReverseModeForwPassVisitor.cpp
  ReverseModeVisitor.cpp
  TBRAnalyzer.cpp
  Timers.cpp
  StmtClone.cpp
  UsefulAnalyzer.cpp
  VectorForwardModeVisitor.cpp
  VectorPushForwardModeVisitor.cpp
  Version.cpp
  VisitorBase.cpp
  ${version_inc}
  )
