//
// Created by LinjianLi on 2019/10/7.
//

#include <iostream>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "tinyxml2.h"

#include "Dataset.h"
#include "Network.h"
#include "CustomNetwork.h"
#include "BNSLComparison.h"
#include "Inference.h"
#include "ExactInference.h"
#include "ApproximateInference.h"
#include "StructureLearning.h"
#include "ParameterLearning.h"
#include "ChowLiuTree.h"
#include "PCStable.h"
#include "JunctionTree.h"
#include "CustomNetwork.h"
#include "ScoreFunction.h"
#include "gadget.h"


//class ExperimentOnA1a : public ::testing::Test {
//protected:
//
//    void SetUp() override {
//        trainer = new Dataset();
//        tester = new Dataset();
//        network = new ChowLiuTree(true);
//
//        string train_set_file_path = "../../data/dataset/a1a.txt",
//                test_set_file_path = "../../data/dataset/a1a.test.txt";
//
//        trainer->LoadLIBSVMData(train_set_file_path);
//        tester->LoadLIBSVMData(test_set_file_path);
//        network->StructLearnCompData(trainer, true);
//        network->LearnParamsKnowStructCompData(trainer, 2, true);
//    }
//
//    Dataset *trainer;
//    Dataset *tester;
//    Network *network;
//};
//
//TEST_F(ExperimentOnA1a, ve) {
//    Inference *inference = new ExactInference(network);
//    double accuracy = inference->EvaluateAccuracy(tester, -1, "ve", true);
//    EXPECT_GT(accuracy, 0.8230);
//}
//
//TEST_F(ExperimentOnA1a, ve_partial) {
//    Inference *inference = new ExactInference(network);
//    double accuracy = inference->EvaluateAccuracy(tester, -1, "ve", false);
//    EXPECT_GT(accuracy, 0.8230);
//}
//
//TEST_F(ExperimentOnA1a, brute_force) {
//    Inference *inference = new ExactInference(network);
//    double accuracy = inference->EvaluateAccuracy(tester, -1, "direct", true);
//    EXPECT_GT(accuracy, 0.8230);
//}
//
//TEST_F(ExperimentOnA1a, likelihood_weighing) {
//    Inference *inference = new ApproximateInference(network);
//    double accuracy = inference->EvaluateAccuracy(tester, 50, "likelihood", true);
//    EXPECT_GT(accuracy, 0.8150);
//}
//
//TEST_F(ExperimentOnA1a, approx) {
//    Inference *inference = new ApproximateInference(network);
//    double accuracy = inference->EvaluateAccuracy(tester, 50, "emm", true);
//    EXPECT_GT(accuracy, 0.650);
//}
//
//
//TEST_F(ExperimentOnA1a, junction_tree) {
//    auto *jt = new JunctionTree(network, false);
//    double accuracy = jt->EvaluateJTAccuracy(0, tester);
//    delete jt;
//    EXPECT_GT(accuracy,0.8150);
//}




//class ExperimentBNSLOnAlarm : public ::testing::Test {
//protected:
//
//    void SetUp() override {
//        trainer = new Dataset();
//        network = new Network(true);
//        string train_set_file_path = "../../../dataset/alarm/alarm_s10000.txt";
//        trainer->LoadCSVData(train_set_file_path, true, true, 0);
//    }
//    Dataset *trainer;
//    Network *network;
//};
//
//TEST_F(ExperimentBNSLOnAlarm, pc_stable) {
//    StructureLearning *bnsl = new PCStable(network, 0.05);
//    bnsl->StructLearnCompData(trainer, 1, 1, false, false);
//    delete trainer;
//
//    CustomNetwork *ref_net = new CustomNetwork();
//    ref_net->LoadBIFFile("../../../dataset/alarm/alarm.bif");
//
//    BNSLComparison comp(ref_net, network);
//    int shd = comp.GetSHD();
//    cout << "SHD = " << shd << endl;
//    delete network;
//    delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnAlarm, pc_stable2) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 2, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/alarm/alarm.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnAlarm, pc_stable4) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 4, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/alarm/alarm.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnAlarm, pc_stable8) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 8, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/alarm/alarm.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnAlarm, pc_stable16) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 16, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/alarm/alarm.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnAlarm, pc_stable32) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 32, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/alarm/alarm.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}




//class ExperimentBNSLOnInsurance : public ::testing::Test {
//protected:
//
//    void SetUp() override {
//        trainer = new Dataset();
//        network = new Network(true);
//        string train_set_file_path = "../../../dataset/insurance/insurance_s10000.txt";
//        trainer->LoadCSVData(train_set_file_path, true, true, 0);
//    }
//    Dataset *trainer;
//    Network *network;
//};

//TEST_F(ExperimentBNSLOnInsurance, pc_stable) {
//    StructureLearning *bnsl = new PCStable(network, 0.05);
//    bnsl->StructLearnCompData(trainer, 1, 1, false, false);
//    delete trainer;
//
//    CustomNetwork *ref_net = new CustomNetwork();
//    ref_net->LoadBIFFile("../../../dataset/insurance/insurance.bif");
//
//    BNSLComparison comp(ref_net, network);
//    int shd = comp.GetSHD();
//    cout << "SHD = " << shd << endl;
//    delete network;
//    delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnInsurance, pc_stable2) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 2, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/insurance/insurance.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnInsurance, pc_stable4) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 4, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/insurance/insurance.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnInsurance, pc_stable8) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 8, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/insurance/insurance.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnInsurance, pc_stable16) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 16, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/insurance/insurance.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnInsurance, pc_stable32) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 32, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/insurance/insurance.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}




//class ExperimentBNSLOnHepar2 : public ::testing::Test {
//protected:
//
//    void SetUp() override {
//        trainer = new Dataset();
//        network = new Network(true);
//        string train_set_file_path = "../../../dataset/hepar2/hepar2_s15000.txt";
//        trainer->LoadCSVData(train_set_file_path, true, true, 0);
//    }
//    Dataset *trainer;
//    Network *network;
//};
//
//TEST_F(ExperimentBNSLOnHepar2, pc_stable) {
//    StructureLearning *bnsl = new PCStable(network, 0.05);
//    bnsl->StructLearnCompData(trainer, 1, 1, false, false);
//    delete trainer;
//
//    CustomNetwork *ref_net = new CustomNetwork();
//    ref_net->LoadBIFFile("../../../dataset/hepar2/hepar2.bif");
//
//    BNSLComparison comp(ref_net, network);
//    int shd = comp.GetSHD();
//    cout << "SHD = " << shd << endl;
//    delete network;
//    delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnHepar2, pc_stable2) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 2, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/hepar2/hepar2.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnHepar2, pc_stable4) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 4, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/hepar2/hepar2.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnHepar2, pc_stable8) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 8, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/hepar2/hepar2.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnHepar2, pc_stable16) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 16, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/hepar2/hepar2.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnHepar2, pc_stable32) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 32, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/hepar2/hepar2.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}



//class ExperimentBNSLOnWin95pts : public ::testing::Test {
//protected:
//
//    void SetUp() override {
//        trainer = new Dataset();
//        network = new Network(true);
//        string train_set_file_path = "../../../dataset/win95pts/win95pts_s15000.txt";
//        trainer->LoadCSVData(train_set_file_path, true, true, 0);
//    }
//    Dataset *trainer;
//    Network *network;
//};
//
//TEST_F(ExperimentBNSLOnWin95pts, pc_stable) {
//    StructureLearning *bnsl = new PCStable(network, 0.05);
//    bnsl->StructLearnCompData(trainer, 1, 1, false, false);
//    delete trainer;
//
//    CustomNetwork *ref_net = new CustomNetwork();
//    ref_net->LoadBIFFile("../../../dataset/win95pts/win95pts.bif");
//
//    BNSLComparison comp(ref_net, network);
//    int shd = comp.GetSHD();
//    cout << "SHD = " << shd << endl;
//    delete network;
//    delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnWin95pts, pc_stable2) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 2, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/win95pts/win95pts.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnWin95pts, pc_stable4) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 4, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/win95pts/win95pts.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnWin95pts, pc_stable8) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 8, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/win95pts/win95pts.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnWin95pts, pc_stable16) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 16, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/win95pts/win95pts.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnWin95pts, pc_stable32) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 32, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/win95pts/win95pts.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}


//class ExperimentBNSLOnAndes : public ::testing::Test {
//protected:
//
//    void SetUp() override {
//        trainer = new Dataset();
//        network = new Network(true);
//        string train_set_file_path = "../../../dataset/andes/andes_s10000.txt";
//        trainer->LoadCSVData(train_set_file_path, true, true, 0);
//    }
//    Dataset *trainer;
//    Network *network;
//};
//
//TEST_F(ExperimentBNSLOnAndes, pc_stable) {
//    StructureLearning *bnsl = new PCStable(network, 0.05);
//    bnsl->StructLearnCompData(trainer, 1, 1, false, false);
//    delete trainer;
//
//    CustomNetwork *ref_net = new CustomNetwork();
//    ref_net->LoadBIFFile("../../../dataset/andes/andes.bif");
//
//    BNSLComparison comp(ref_net, network);
//    int shd = comp.GetSHD();
//    cout << "SHD = " << shd << endl;
//    delete network;
//    delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnAndes, pc_stable2) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 2, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/andes/andes.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnAndes, pc_stable4) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 4, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/andes/andes.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnAndes, pc_stable8) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 8, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/andes/andes.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnAndes, pc_stable16) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 16, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/andes/andes.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnAndes, pc_stable32) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 32, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/andes/andes.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}



//class ExperimentBNSLOnMunin1 : public ::testing::Test {
//protected:
//
//    void SetUp() override {
//        trainer = new Dataset();
//        network = new Network(true);
//        string train_set_file_path = "../../../dataset/munin1/munin1_s15000.txt";
//        trainer->LoadCSVData(train_set_file_path, true, true, 0);
//    }
//    Dataset *trainer;
//    Network *network;
//};
//
//TEST_F(ExperimentBNSLOnMunin1, pc_stable) {
//    StructureLearning *bnsl = new PCStable(network, 0.05);
//    bnsl->StructLearnCompData(trainer, 1, 1, false, false);
//    delete trainer;
//
//    CustomNetwork *ref_net = new CustomNetwork();
//    ref_net->LoadBIFFile("../../../dataset/munin1/munin1.bif");
//
//    BNSLComparison comp(ref_net, network);
//    int shd = comp.GetSHD();
//    cout << "SHD = " << shd << endl;
//    delete network;
//    delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnMunin1, pc_stable2) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 2, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/munin1/munin1.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnMunin1, pc_stable4) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 4, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/munin1/munin1.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnMunin1, pc_stable8) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 8, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/munin1/munin1.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnMunin1, pc_stable16) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 16, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/munin1/munin1.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}
//
//TEST_F(ExperimentBNSLOnMunin1, pc_stable32) {
//StructureLearning *bnsl = new PCStable(network, 0.05);
//bnsl->StructLearnCompData(trainer, 1, 32, false, false);
//delete trainer;
//
//CustomNetwork *ref_net = new CustomNetwork();
//ref_net->LoadBIFFile("../../../dataset/munin1/munin1.bif");
//
//BNSLComparison comp(ref_net, network);
//int shd = comp.GetSHD();
//cout << "SHD = " << shd << endl;
//delete network;
//delete ref_net;
//}


class ExperimentBNSLOnPathfinder : public ::testing::Test {
protected:

    void SetUp() override {
        trainer = new Dataset();
        network = new Network(true);

        string train_set_file_path = "../../../dataset/pathfinder/pathfinder_s5000.txt";

        trainer->LoadCSVData(train_set_file_path, true, true, 0);
    }

    Dataset *trainer;
    Network *network;
};

TEST_F(ExperimentBNSLOnPathfinder, pc_stable) {
    StructureLearning *bnsl = new PCStable(network, 0.05);
    bnsl->StructLearnCompData(trainer, 1, 1, false, false);
    delete trainer;

    CustomNetwork *ref_net = new CustomNetwork();
    ref_net->LoadBIFFile("../../../dataset/pathfinder/pathfinder.bif");

    BNSLComparison comp(ref_net, network);
    int shd = comp.GetSHD();
    cout << "SHD = " << shd << endl;
    delete network;
    delete ref_net;
}


class ExperimentBNSLOnDiabetes : public ::testing::Test {
protected:

    void SetUp() override {
        trainer = new Dataset();
        network = new Network(true);

        string train_set_file_path = "../../../dataset/diabetes/diabetes_s5000.txt";

        trainer->LoadCSVData(train_set_file_path, true, true, 0);
    }

    Dataset *trainer;
    Network *network;
};

TEST_F(ExperimentBNSLOnDiabetes, pc_stable) {
    StructureLearning *bnsl = new PCStable(network, 0.05);
    bnsl->StructLearnCompData(trainer, 1, 1, false, false);
    delete trainer;

    CustomNetwork *ref_net = new CustomNetwork();
    ref_net->LoadBIFFile("../../../dataset/diabetes/diabetes.bif");

    BNSLComparison comp(ref_net, network);
    int shd = comp.GetSHD();
    cout << "SHD = " << shd << endl;
    delete network;
    delete ref_net;
}


class ExperimentBNSLOnLink : public ::testing::Test {
protected:

    void SetUp() override {
        trainer = new Dataset();
        network = new Network(true);
        string train_set_file_path = "../../../dataset/link/link_s5000.txt";
        trainer->LoadCSVData(train_set_file_path, true, true, 0);
    }

    Dataset *trainer;
    Network *network;
};

TEST_F(ExperimentBNSLOnLink, pc_stable) {
    StructureLearning *bnsl = new PCStable(network, 0.05);
    bnsl->StructLearnCompData(trainer, 1, 1, false, false);
    delete trainer;

    CustomNetwork *ref_net = new CustomNetwork();
    ref_net->LoadBIFFile("../../../dataset/link/link.bif");

    BNSLComparison comp(ref_net, network);
    int shd = comp.GetSHD();
    cout << "SHD = " << shd << endl;
    delete network;
    delete ref_net;
}


class ExperimentBNSLOnPigs : public ::testing::Test {
protected:

    void SetUp() override {
        trainer = new Dataset();
        network = new Network(true);
        string train_set_file_path = "../../../dataset/pigs/pigs_s5000.txt";
        trainer->LoadCSVData(train_set_file_path, true, true, 0);
    }

    Dataset *trainer;
    Network *network;
};

TEST_F(ExperimentBNSLOnPigs, pc_stable) {
    StructureLearning *bnsl = new PCStable(network, 0.05);
    bnsl->StructLearnCompData(trainer, 1, 1, false, false);
    delete trainer;

    CustomNetwork *ref_net = new CustomNetwork();
    ref_net->LoadBIFFile("../../../dataset/pigs/pigs.bif");

    BNSLComparison comp(ref_net, network);
    int shd = comp.GetSHD();
    cout << "SHD = " << shd << endl;
    delete network;
    delete ref_net;
}









