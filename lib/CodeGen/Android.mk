LOCAL_PATH:= $(call my-dir)

codegen_SRC_FILES := \
  AggressiveAntiDepBreaker.cpp \
  AllocationOrder.cpp \
  Analysis.cpp \
  BranchFolding.cpp \
  CalcSpillWeights.cpp \
  CallingConvLower.cpp \
  CodeGen.cpp \
  CodePlacementOpt.cpp \
  CriticalAntiDepBreaker.cpp \
  DeadMachineInstructionElim.cpp \
  DwarfEHPrepare.cpp \
  EdgeBundles.cpp \
  ELFCodeEmitter.cpp \
  ELFWriter.cpp \
  ExecutionDepsFix.cpp \
  ExpandISelPseudos.cpp \
  ExpandPostRAPseudos.cpp \
  GCMetadata.cpp \
  GCMetadataPrinter.cpp \
  GCStrategy.cpp \
  IfConversion.cpp \
  InlineSpiller.cpp \
  InterferenceCache.cpp \
  IntrinsicLowering.cpp \
  LLVMTargetMachine.cpp \
  LatencyPriorityQueue.cpp \
  LexicalScopes.cpp \
  LiveDebugVariables.cpp \
  LiveInterval.cpp \
  LiveIntervalAnalysis.cpp \
  LiveIntervalUnion.cpp \
  LiveStackAnalysis.cpp \
  LiveVariables.cpp \
  LiveRangeCalc.cpp \
  LiveRangeEdit.cpp \
  LocalStackSlotAllocation.cpp \
  MachineBasicBlock.cpp \
  MachineBlockFrequencyInfo.cpp \
  MachineBranchProbabilityInfo.cpp \
  MachineCSE.cpp \
  MachineDominators.cpp \
  MachineFunction.cpp \
  MachineFunctionAnalysis.cpp \
  MachineFunctionPass.cpp \
  MachineFunctionPrinterPass.cpp \
  MachineInstr.cpp \
  MachineLICM.cpp \
  MachineLoopInfo.cpp \
  MachineLoopRanges.cpp \
  MachineModuleInfo.cpp \
  MachineModuleInfoImpls.cpp \
  MachinePassRegistry.cpp \
  MachineRegisterInfo.cpp \
  MachineSSAUpdater.cpp \
  MachineSink.cpp \
  MachineVerifier.cpp \
  ObjectCodeEmitter.cpp \
  OcamlGC.cpp \
  OptimizePHIs.cpp \
  PHIElimination.cpp \
  PHIEliminationUtils.cpp \
  Passes.cpp \
  PeepholeOptimizer.cpp \
  PostRASchedulerList.cpp \
  ProcessImplicitDefs.cpp \
  PrologEpilogInserter.cpp \
  PseudoSourceValue.cpp \
  RegAllocBasic.cpp \
  RegAllocFast.cpp \
  RegAllocGreedy.cpp \
  RegAllocLinearScan.cpp \
  RegAllocPBQP.cpp \
  RegisterCoalescer.cpp \
  RegisterClassInfo.cpp \
  RegisterScavenging.cpp \
  RenderMachineFunction.cpp \
  ScheduleDAG.cpp \
  ScheduleDAGEmit.cpp \
  ScheduleDAGInstrs.cpp \
  ScheduleDAGPrinter.cpp \
  ScoreboardHazardRecognizer.cpp \
  ShadowStackGC.cpp \
  ShrinkWrapping.cpp \
  SjLjEHPrepare.cpp \
  SlotIndexes.cpp \
  Spiller.cpp \
  SpillPlacement.cpp \
  SplitKit.cpp \
  Splitter.cpp \
  StackProtector.cpp \
  StackSlotColoring.cpp \
  StrongPHIElimination.cpp \
  TailDuplication.cpp \
  TargetInstrInfoImpl.cpp \
  TargetLoweringObjectFileImpl.cpp \
  TwoAddressInstructionPass.cpp \
  UnreachableBlockElim.cpp \
  VirtRegMap.cpp \
  VirtRegRewriter.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(codegen_SRC_FILES)
LOCAL_MODULE:= libLLVMCodeGen

LOCAL_MODULE_TAGS := optional

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(codegen_SRC_FILES)
LOCAL_MODULE:= libLLVMCodeGen

LOCAL_MODULE_TAGS := optional

include $(LLVM_DEVICE_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_STATIC_LIBRARY)
