//
// Created by jjt on 2021/6/15.
//

#ifndef BAYESIANNETWORK_INFERENCE_H
#define BAYESIANNETWORK_INFERENCE_H

#include "Network.h"
#include "Dataset.h"
#include "Node.h"
#include "DiscreteNode.h"
#include "ContinuousNode.h"
#include "PotentialTable.h"
#include "Timer.h"
#include <iostream>
#include <sys/time.h>
#include <locale.h>
#include "omp.h"

/**
 * abstract class
 */
class Inference {
public:
    Network *network; // the learned network which can be used for inference
    int num_instances; // number of instances in the testing set
    /**
     * the index of the variable we are interested in (for each instance in the testing set)
     * for the classification problem, typically we have only one class variable to infer, so query_index is only one index
     * but actually for inference problem on BNs, there are commonly a set of variables interested
     * meanwhile, we commonly compute/estimate the probability of every non-evidence variable in the network
     */
    int query_index;

    vector<DiscreteConfig> evidences; // the evidences of each instance in the testing set
    vector<int> ground_truths; // the ground truths of each class variable
    vector<vector<vector<double>>> ground_truth_probability_tables; // the probabilities for each state of each node, computed by exact inference algorithms

    Inference(Network *net, Dataset *dts, bool is_dense);
    Inference(Network *net); // it is used only in LBP for EPIS-BN

    virtual ~Inference() {};

    virtual double EvaluateAccuracy(string path, int num_threads)= 0;

    double Accuracy(vector<int> predictions);
    DiscreteConfig Sparse2Dense(DiscreteConfig evidence, int num_nodes);
    int ArgMax(const vector<double> &array);

    void LoadGroundTruthProbabilityTable(string file_path);
    double CalculateMSE(const vector<vector<double>> &approximate_distribution, int instance_index);
    double CalculateHellingerDistance(const vector<vector<double>> &approximate_distribution, int instance_index);
};

double Round(double number, unsigned int bits);

#endif //BAYESIANNETWORK_INFERENCE_H
