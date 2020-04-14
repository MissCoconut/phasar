/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureMemoryDescription.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDETSAnalysisOpenSSLSecureMemoryTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/openssl/secure_memory/";
  const std::set<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  LLVMPointsToInfo *PT;
  OpenSSLSecureMemoryDescription *Desc;
  IDETypeStateAnalysis *TSProblem;
  IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
            IDETypeStateAnalysis::f_t, IDETypeStateAnalysis::t_t,
            IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
            IDETypeStateAnalysis::i_t> *Llvmtssolver = 0;

  enum OpenSSLSecureMemoryState {
    TOP = 42,
    BOT = 0,
    ZEROED = 1,
    FREED = 2,
    ERROR = 3,
    ALLOCATED = 4
  };
  IDETSAnalysisOpenSSLSecureMemoryTest() = default;
  ~IDETSAnalysisOpenSSLSecureMemoryTest() override = default;

  void initialize(const std::vector<std::string> &IRFiles) {
    IRDB = new ProjectIRDB(IRFiles, IRDBOptions::WPA);
    TH = new LLVMTypeHierarchy(*IRDB);
    PT = new LLVMPointsToInfo(*IRDB);
    ICFG = new LLVMBasedICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, TH,
                             PT);
    Desc = new OpenSSLSecureMemoryDescription();
    TSProblem =
        new IDETypeStateAnalysis(IRDB, TH, ICFG, PT, *Desc, EntryPoints);
    Llvmtssolver =
        new IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
                      IDETypeStateAnalysis::f_t, IDETypeStateAnalysis::t_t,
                      IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
                      IDETypeStateAnalysis::i_t>(*TSProblem);

    Llvmtssolver->solve();
  }

  void SetUp() override {
    boost::log::core::get()->set_logging_enabled(false);
    ValueAnnotationPass::resetValueID();
  }

  void TearDown() override {
    delete IRDB;
    delete TH;
    delete ICFG;
    delete TSProblem;
    delete Llvmtssolver;
  }

  /**
   * We map instruction id to value for the ground truth. ID has to be
   * a string since Argument ID's are not integer type (e.g. main.0 for
   argc).
   * @param groundTruth results to compare against
   * @param solver provides the results
   */
  void compareResults(
      const std::map<std::size_t, std::map<std::string, int>> &GroundTruth) {
    for (auto InstToGroundTruth : GroundTruth) {
      auto *Inst = IRDB->getInstruction(InstToGroundTruth.first);
      auto GT = InstToGroundTruth.second;
      std::map<std::string, int> Results;
      for (auto Result : Llvmtssolver->resultsAt(Inst, true)) {
        if (GT.find(getMetaDataID(Result.first)) != GT.end()) {
          Results.insert(std::pair<std::string, int>(
              getMetaDataID(Result.first), Result.second));
        } // else {
        //   std::cout << "Unused result at " << InstToGroundTruth.first << ": "
        //             << llvmIRToShortString(Result.first) << " => "
        //             << Result.second << std::endl;
        // }
      }
      EXPECT_EQ(Results, GT) << "at inst " << llvmIRToShortString(Inst);
    }
  }
}; // Test Fixture

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory1) {
  initialize({PathToLlFiles + "memory1_c.ll"});
  // llvmtssolver->printReport();
  std::map<std::size_t, std::map<std::string, int>> Gt;
  // TODO add GT values
  Gt[10] = {{"8", OpenSSLSecureMemoryState::ALLOCATED},
            {"3", OpenSSLSecureMemoryState::ALLOCATED}};
  Gt[20] = Gt[10];
  Gt[29] = {{"3", OpenSSLSecureMemoryState::ERROR}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory2) {
  initialize({PathToLlFiles + "memory2_c.ll"});
  std::map<size_t, std::map<std::string, int>> Gt;
  Gt[10] = {{"8", OpenSSLSecureMemoryState::ALLOCATED},
            {"3", OpenSSLSecureMemoryState::ALLOCATED}};
  Gt[20] = Gt[10];
  Gt[30] = {{"8", OpenSSLSecureMemoryState::ZEROED},
            {"3", OpenSSLSecureMemoryState::ZEROED},
            {"27", OpenSSLSecureMemoryState::ZEROED}};
  Gt[32] = {{"8", OpenSSLSecureMemoryState::FREED},
            {"3", OpenSSLSecureMemoryState::FREED},
            {"27", OpenSSLSecureMemoryState::FREED},
            {"30", OpenSSLSecureMemoryState::FREED}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory3) {
  // boost::log::core::get()->set_logging_enabled(true);
  initialize({PathToLlFiles + "memory3_c.ll"});
  // llvmtssolver->printReport();
  std::map<size_t, std::map<std::string, int>> Gt;
  Gt[15] = {{"13", OpenSSLSecureMemoryState::ZEROED},
            {"6", OpenSSLSecureMemoryState::ZEROED}};

  // Imprecision of the analysis => write into buffer kills it permanently
  // instead of degrading the typestate

  // gt[34] = {{"6", OpenSSLSecureMemoryState::ALLOCATED}};
  // gt[49] = {{"6", OpenSSLSecureMemoryState::ALLOCATED},
  //          {"48", OpenSSLSecureMemoryState::ALLOCATED}};
  // gt[50] = {{"6", OpenSSLSecureMemoryState::ERROR},
  //          {"48", OpenSSLSecureMemoryState::ERROR}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory4) {
  initialize({PathToLlFiles + "memory4_c.ll"});
  std::map<size_t, std::map<std::string, int>> Gt;
  Gt[15] = {{"13", OpenSSLSecureMemoryState::ZEROED},
            {"6", OpenSSLSecureMemoryState::ZEROED}};

  // Imprecision of the analysis => write into buffer kills it permanently
  // instead of degrading the typestate (as for Memory3)

  // gt[34] = {{"6", OpenSSLSecureMemoryState::ALLOCATED}};
  // gt[49] = {{"6", OpenSSLSecureMemoryState::ZEROED},
  //           {"48", OpenSSLSecureMemoryState::ZEROED}};
  // gt[50] = {{"6", OpenSSLSecureMemoryState::FREED},
  //           {"48", OpenSSLSecureMemoryState::FREED}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory5) {
  initialize({PathToLlFiles + "memory5_c.ll"});
  std::map<size_t, std::map<std::string, int>> Gt;
  Gt[10] = {{"8", OpenSSLSecureMemoryState::ALLOCATED},
            {"3", OpenSSLSecureMemoryState::ALLOCATED}};

  // Imprecision of the analysis => write into buffer kills it permanently
  // instead of degrading the typestate (as for Memory3)
  // For whatever reason it nevertheless works

  Gt[22] = {{"3", OpenSSLSecureMemoryState::ALLOCATED}};
  Gt[31] = {{"3", OpenSSLSecureMemoryState::ALLOCATED},
            {"30", OpenSSLSecureMemoryState::ALLOCATED}};
  Gt[32] = {{"3", OpenSSLSecureMemoryState::ERROR},
            {"30", OpenSSLSecureMemoryState::ERROR}};
  compareResults(Gt);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
