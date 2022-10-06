////
//// Created by jjt on 6/10/22.
////
//#include "VariableElimination.h"
//
//double VariableElimination::EvaluateAccuracy(int num_threads) {
//    cout << "==================================================" << '\n'
//         << "Begin testing the trained network." << endl;
//
//    Timer *timer = new Timer();
//    // record time
//    timer->Start("ve");
//
//    // predict the labels of the test instances
//    vector<int> predictions = PredictUseVEInfer(timer);
//    double accuracy = Accuracy(predictions);
//
//    timer->Stop("ve");
//    setlocale(LC_NUMERIC, "");
//
//    cout << "==================================================";
//    timer->Print("ve");
//    timer->Print("filter out"); cout << " (" << timer->time["filter out"] / timer->time["ve"] * 100 << "%)";
//    timer->Print("load evidence"); cout << " (" << timer->time["load evidence"] / timer->time["ve"] * 100 << "%)";
//    timer->Print("ve process"); cout << " (" << timer->time["ve process"] / timer->time["ve"] * 100 << "%)" << endl;
//
//    SAFE_DELETE(timer);
//
//    return accuracy;
//}
//
///**
// * @brief: predict label given (partial or full observation) evidence
// * check "map_potentials", and the predict label is the one with maximum probability
// * @return label of the target variable
// */
//int VariableElimination::PredictUseVEInfer(DiscreteConfig evid, Timer *timer, vector<int> elim_order) {
//
//    // get the factor (marginal probability) of the target node given the evidences
//    Factor F = GetMarginalProbabilitiesUseVE(target_node_idx, evid, timer, elim_order);
//
//    // find the configuration with the maximum probability TODO: function ArgMax for Factor
//    double max_prob = 0;
//    DiscreteConfig comb_predict;
//    for (auto &comb : F.set_disc_configs) { // for each configuration of the related variables
//        if (F.map_potentials[comb] > max_prob) {
//            max_prob = F.map_potentials[comb];
//            comb_predict = comb;
//        }
//    }
//    int label_predict = comb_predict.begin()->second;
//    return label_predict;
//}
//
///**
// * @brief: predict the labels given different evidences
// * it just repeats the function above multiple times, and print the progress at the meantime
// * @param elim_orders: elimination order which may be different given different evidences due to the simplification of elimination order
// */
//vector<int> VariableElimination::PredictUseVEInfer(Timer *timer, vector<vector<int>> elim_orders) {
//    int size = evidences.size();
//
//    cout << "Progress indicator: ";
//    int every_1_of_20 = size / 20; // used to print, print 20 times in total
//    int progress = 0;
//
//    if (elim_orders.empty()) {
//        // Vector of size "size". Each element is an empty vector.
//        elim_orders = vector<vector<int>> (size, vector<int>{});
//    }
//
//    vector<int> results(size, 0);
//
//    for (int i = 0; i < size; ++i) {
//        ++progress;
//
//        if (progress % every_1_of_20 == 0) {
//            string progress_percentage = to_string((double)progress/size * 100) + "%...\n";
//            fprintf(stdout, "%s\n", progress_percentage.c_str());
//            fflush(stdout);
//        }
//
//        vector<int> elim_ord = elim_orders.at(i);
//        int pred = PredictUseVEInfer(evidences.at(i), timer, elim_ord);
//        results.at(i) = pred;
//    }
//    return results;
//}
//
///**
// * @brief: for inference given a target variable id and some evidences/observations.
// */
//Factor VariableElimination::GetMarginalProbabilitiesUseVE(int target_var_index, DiscreteConfig evidence,
//                                                          Timer *timer, vector<int> elim_order) {
//
//    timer->Start("filter out");
//    // find the nodes to be removed, include barren nodes and m-separated nodes
//    // filter out these nodes and obtain the left nodes
//    vector<int> left_nodes = FilterOutIrrelevantNodes();
//    timer->Stop("filter out");
//
//    Node *n = nullptr;
//    // "factorsList" corresponds to all the nodes which are between the target node and the observation/evidence
//    // because we have removed barren nodes and m-separated nodes
//    vector<Factor> factor_list = network->ConstructFactors(left_nodes, n);
//
//    set<int> all_related_vars;
//    all_related_vars.insert(target_var_index);
//    for (int i = 0; i < left_nodes.size(); ++i) {
//        all_related_vars.insert(left_nodes.at(i));
//    }
//    timer->Start("load evidence");
//    // load evidence function below returns a factorsList with fewer configurations.
//    network->LoadEvidenceIntoFactors(&factor_list, evidence, all_related_vars);
//    timer->Stop("load evidence");
//
//    if (elim_order.empty()) {
//        // call "ChowLiuTree::SimplifyDefaultElimOrd"; "elim_order" is the reverse topological order removing barren nodes and m-separated nodes
////        elim_order = network->SimplifyDefaultElimOrd2(evidence, left_nodes);
//        elim_order = DefaultEliminationOrder(evidence, left_nodes);
//    }
//
//    timer->Start("ve process");
//    // compute the probability table of the target node
//    Factor target_node_factor = SumProductVarElim(factor_list, elim_order);
//    timer->Stop("ve process");
//
//    // renormalization
//    target_node_factor.Normalize();
//
//    return target_node_factor;
//}
//
///**
// * @brief: find the nodes to be removed, including the barren nodes and m-separated nodes
// * filter out these nodes and obtain the left nodes
// * suppose Y is the set of variables observed; X is the set of variables of interest
// * barren node: a leaf which is not in X and not in Y
// * moral graph: obtained by adding an edge between each pair of parents and dropping all directions
// * m-separated node: this node and X are separated by the set Y in the moral graph
// * The implementation is based on "A simple approach to Bayesian network computations" by Zhang and Poole, 1994.
// */
//vector<int> VariableElimination::FilterOutIrrelevantNodes() { // TODO
//    /// find the nodes to be removed TODO
//    set<int> to_be_removed;
//
//    /// filter out these nodes and obtain the left nodes
//    vector<int> left_nodes;
//    int num_of_left = network->num_nodes - to_be_removed.size();
//    left_nodes.reserve(num_of_left);
//    for (int i = 0; i < network->num_nodes; ++i) { // for each nodes in the network
//        if (to_be_removed.find(i) == to_be_removed.end()) { // if this node does not need to be removed
//            left_nodes.push_back(i);
//        }
//    }
//
//    return left_nodes;
//}
//
///**
// * @brief: default elimination order is based on the reverse topological order
// * simplified by removing the evidence and the target nodes from the left nodes
// * @param left_nodes is the left node set by removing barren nodes and m-separated nodes
// * The implementation is based on "A simple approach to Bayesian network computations" by Zhang and Poole, 1994.
// */
//vector<int> VariableElimination::DefaultEliminationOrder(DiscreteConfig evidence, vector<int> left_nodes) {
//
//    // based on the reverse topological order
//    vector<int> vec_default_elim_ord = network->GetReverseTopoOrd();
//
//    // to_be_removed contains: irrelevant nodes & evidence
//    set<int> to_be_removed;
//    // add the evidence
//    for (auto p: evidence) { // for each index-value pair in the evidence
//        to_be_removed.insert(p.first);
//    }
//    // add the irrelevant nodes
//    for (int i = 0; i < network->num_nodes; ++i) { // for each node in the network
//        if (find(left_nodes.begin(), left_nodes.end(), i) == left_nodes.end()) { // if i is not in "left_nodes"
//            to_be_removed.insert(i); // add into "to_be_removed"
//        }
//    }
//
//    vector<int> vec_simplified_order;
//    for (int i = 0; i < network->num_nodes - 1; ++i) { // the last one in the vector is the target node
//        int ord = vec_default_elim_ord.at(i);
//        if (to_be_removed.find(ord) == to_be_removed.end()) { // if it is not removed
//            vec_simplified_order.push_back(ord);
//        }
//    }
////  // this case may happen if the test set contains more features than the training set
////  if (vec_simplified_order.size() != num_of_remain) {
////    fprintf(stderr, "Error in function [%s], simplified order size not equal to number of remaining nodes!\n", __FUNCTION__);
////    exit(1);
////  }
//
//    return vec_simplified_order;
//}
//
///**
// * @brief: the main variable elimination (VE) process
// * gradually eliminate variables until only one (i.e. the target node) left
// */
//Factor VariableElimination::SumProductVarElim(vector<Factor> factor_list, vector<int> elim_order) {
//    for (int i = 0; i < elim_order.size(); ++i) { // consider each node i according to the elimination order
//        vector<Factor> temp_factor_list;
//        Node* nodePtr = network->FindNodePtrByIndex(elim_order.at(i));
//
//        // Move every factor that is related to the node elim_order[i] from factors_list to tempFactorsList.
//        /*
//         * Note: This for loop does not contain "++it" in the parentheses.
//         *      When finding "elim_order[i]" during the traverse, the iterator "it" points to this vector.
//         *      We use "erase" to delete this element from "factors_list" via iterator "it";
//         *      the function "erase" returns an iterator pointing to the next element of the delete element.
//         */
//        for (auto it = factor_list.begin(); it != factor_list.end(); /* no ++it */) {
//            // if the factor "it" is related to the node "elim_order[i]" (i.e., the node to be eliminated now)
//            if ((*it).related_variables.find(nodePtr->GetNodeIndex()) != (*it).related_variables.end()) {
//                temp_factor_list.push_back(*it);
//                factor_list.erase(it);
//                continue;
//            }
//            else {
//                ++it;
//            }
//        }
//
//        // merge all the factors in tempFactorsList into one factor
//        while(temp_factor_list.size() > 1) {
//            // every time merge two factors into one
//            Factor temp1, temp2, product;
//            temp1 = temp_factor_list.back(); // get the last element
//            temp_factor_list.pop_back();  // remove the last element
//            temp2 = temp_factor_list.back();
//            temp_factor_list.pop_back();
//
//            product = temp1.MultiplyWithFactor(temp2);
//            temp_factor_list.push_back(product);
//        }
//
//        // eliminate variable "nodePtr" by summation of the factor "tempFactorsList.back()" over "nodePtr"
//        Factor newFactor = temp_factor_list.back().SumOverVar(dynamic_cast<DiscreteNode*>(nodePtr));
//        factor_list.push_back(newFactor);
//    } // finish eliminating variables and only one variable left
//
//    // if the "factor_list" contains several factors, we need to multiply these several factors
//    // for example, the case when we have a full evidence...
//    // then "factor_list" contains "num_nodes" factor while "elim_order" is empty
//    while (factor_list.size() > 1) {
//        Factor temp1, temp2, product;
//        temp1 = factor_list.back(); // get the last element
//        factor_list.pop_back();  // remove the last element
//        temp2 = factor_list.back();
//        factor_list.pop_back();
//
//        product = temp1.MultiplyWithFactor(temp2);
//        factor_list.push_back(product);
//    }
//
//    // After all the processing shown above, the only remaining factor is the factor about Y.
//    return factor_list.back();
//}