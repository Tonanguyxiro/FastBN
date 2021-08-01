//
// Created by LinjianLi on 2019/1/23.
//

#ifndef BAYESIANNETWORK_CHOWLIUTREE_H
#define BAYESIANNETWORK_CHOWLIUTREE_H

#include "Dataset.h"
#include "Network.h"
#include "StructureLearning.h"
#include "Node.h"
#include "DiscreteNode.h"
#include "Factor.h"
#include "gadget.h"
#include <set>
#include <queue>
#include <vector>
#include <cmath>
#include <iostream>
#include <sys/time.h>
#include <locale.h>
#include "omp.h"

using namespace std;

typedef set< pair<int, int> > DiscreteConfig;


class ChowLiuTree : public StructureLearning {
 public:

  /* About (tree)"default_elim_ord":
   *   This attribute is not supposed to exist.
   *   But for now, I have just implemented the part of ChowLiu tree.
   *   And I have not implemented the part of generating an elimination order automatically.
   *   So, I just add this attribute to store a relatively "fixed" order.
   *   The order is fixed for one tree, but different for different trees.
   *   It is just the reverse order of topological sorting using width-first-traversal start at the root node.
   */
//  int *default_elim_ord;

  int root_node_index = -1;

  ChowLiuTree(Network *net) {network = net;};

  double ComputeMutualInformation(Node *Xi, Node *Xj, const Dataset *dts);
  virtual void StructLearnCompData(Dataset *dts, bool print_struct, bool verbose);
  void StructLearnChowLiuTreeCompData(Dataset *dts, bool print_struct=true);


//  vector<int> SimplifyDefaultElimOrd(DiscreteConfig evidence) ;
//  vector<int> SimplifyDefaultElimOrd2(DiscreteConfig evidence, vector<int> left_nodes) ;
//  vector<int> SimplifyTreeDefaultElimOrd(DiscreteConfig evidence);
//  vector<int> SimplifyTreeDefaultElimOrd2(DiscreteConfig evidence, vector<int> left_nodes);

 protected:
  void DepthFirstTraversalUntillMeetObserved(DiscreteConfig evidence, int start, set<int>& visited, set<int>& to_be_removed);
  void DepthFirstTraversalToRemoveMSeparatedNodes(int start, set<int>& visited, set<int>& to_be_removed);

};


#endif //BAYESIANNETWORK_CHOWLIUTREE_H
