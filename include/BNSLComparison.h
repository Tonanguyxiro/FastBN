//
// Created by jjt on 2021/7/10.
//

#ifndef BAYESIANNETWORK_BNSLCOMPARISON_H
#define BAYESIANNETWORK_BNSLCOMPARISON_H

#include "Network.h"

/**
 * this class is used to measure the quality of the learned Bayesian Networks
 * available metrics include
 *      Structural Hamming Distance (SHD)
 */
class BNSLComparison {
public:
    int GetSHD(Network* true_dag, Network* learned_cpdag);
    int GetSHDOneEdge(Edge true_edge, Edge learned_edge);
};


#endif //BAYESIANNETWORK_BNSLCOMPARISON_H
