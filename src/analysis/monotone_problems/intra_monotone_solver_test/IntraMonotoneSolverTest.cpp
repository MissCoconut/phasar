/*
 * MonotoneSolverTest.cpp
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#include "IntraMonotoneSolverTest.hh"

IntraMonotoneSolverTest::IntraMonotoneSolverTest(LLVMBasedCFG &Cfg,
                                                 const llvm::Function *F)
    : IntraMonotoneProblem<const llvm::Instruction *, const llvm::Value *,
                           const llvm::Function *, LLVMBasedCFG &>(Cfg, F) {}

MonoSet<const llvm::Value *>
IntraMonotoneSolverTest::join(const MonoSet<const llvm::Value *> &Lhs,
                              const MonoSet<const llvm::Value *> &Rhs) {
  cout << "MonotoneSolverTest::join()\n";
  MonoSet<const llvm::Value *> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  return Result;
}

bool IntraMonotoneSolverTest::sqSubSetEqual(
    const MonoSet<const llvm::Value *> &Lhs,
    const MonoSet<const llvm::Value *> &Rhs) {
  cout << "MonotoneSolverTest::sqSubSetEqual()\n";
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
}

MonoSet<const llvm::Value *>
IntraMonotoneSolverTest::flow(const llvm::Instruction *S,
                              const MonoSet<const llvm::Value *> &In) {
  cout << "MonotoneSolverTest::flow()\n";
  MonoSet<const llvm::Value *> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Store = llvm::dyn_cast<llvm::StoreInst>(S)) {
    Result.insert(Store);
  }
  return Result;
}

MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
IntraMonotoneSolverTest::initialSeeds() {
  cout << "MonotoneSolverTest::initialSeeds()\n";
  return { { &Function->front().front(), MonoSet<const llvm::Value *>{} } };
}

string IntraMonotoneSolverTest::D_to_string(const llvm::Value* d) {
  return llvmIRToString(d);
}
