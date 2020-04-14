#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "gtest/gtest.h"

using namespace std;
using namespace psr;

class LLVMBasedICFG_DTATest : public ::testing::Test {
protected:
  const std::string PathToLlFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/";
};

TEST_F(LLVMBasedICFG_DTATest, VirtualCallSite_5) {
  ProjectIRDB IRDB({PathToLlFiles + "call_graphs/virtual_call_5_cpp.ll"},
                   IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::DTA, {"main"}, &TH);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *FuncA = IRDB.getFunctionDefinition("_ZN1A4funcEv");
  const llvm::Function *VFuncA = IRDB.getFunctionDefinition("_ZN1A5VfuncEv");
  const llvm::Function *VFuncB = IRDB.getFunctionDefinition("_ZN1B5VfuncEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FuncA);
  ASSERT_TRUE(VFuncA);
  ASSERT_TRUE(VFuncB);

  const llvm::Instruction *I = getNthInstruction(F, 16);
  if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
    set<const llvm::Function *> Callees = ICFG.getCalleesOfCallAt(I);

    ASSERT_TRUE(ICFG.isVirtualFunctionCall(I));
    ASSERT_EQ(Callees.size(), 2);
    ASSERT_TRUE(Callees.count(VFuncB));
    ASSERT_TRUE(Callees.count(VFuncA));
    ASSERT_TRUE(ICFG.getCallersOf(VFuncA).count(I));
    ASSERT_TRUE(ICFG.getCallersOf(VFuncB).count(I));
  }
}

TEST_F(LLVMBasedICFG_DTATest, VirtualCallSite_6) {
  ProjectIRDB IRDB({PathToLlFiles + "call_graphs/virtual_call_6_cpp.ll"},
                   IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::DTA, {"main"}, &TH);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *VFuncA = IRDB.getFunctionDefinition("_ZN1A5VfuncEv");
  const llvm::Function *VFuncB = IRDB.getFunctionDefinition("_ZN1B5VfuncEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(VFuncA);
  ASSERT_TRUE(VFuncB);

  const llvm::Instruction *I = getNthInstruction(F, 6);
  set<const llvm::Instruction *> Callers = ICFG.getCallersOf(VFuncA);
  llvm::ImmutableCallSite CS(I);
  ASSERT_EQ(Callers.size(), 1);
  ASSERT_TRUE(Callers.count(I));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
