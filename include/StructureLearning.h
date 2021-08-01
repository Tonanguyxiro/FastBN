//
// Created by jjt on 2021/6/17.
//

#ifndef BAYESIANNETWORK_STRUCTURELEARNING_H
#define BAYESIANNETWORK_STRUCTURELEARNING_H

#include "Network.h"
#include "Dataset.h"


class StructureLearning {
public:
    Network *network; // the network to be learned

    virtual void StructLearnCompData(Dataset *dts, bool print_struct, bool verbose) = 0;

    virtual ~StructureLearning() {};

    void AssignNodeInformation(Dataset *dts);
    vector<int> AssignNodeOrder(string topo_ord_constraint);
};

#endif //BAYESIANNETWORK_STRUCTURELEARNING_H
