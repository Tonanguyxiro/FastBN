//
// Created by LinjianLi on 2019/2/16.
//

#ifndef BAYESIANNETWORK_JUNCTIONTREE_H
#define BAYESIANNETWORK_JUNCTIONTREE_H

#include <set>
#include <vector>
#include "Clique.h"
#include "Separator.h"
#include "Network.h"


class JunctionTree {
 public:
  Network *network;
  set<Clique*> set_clique_ptr_container;
  set<Separator*> set_separator_ptr_container;

  vector<int> elimination_ordering;
  map<int, Clique*> map_elim_var_to_clique;


  JunctionTree() = default;
  explicit JunctionTree(Network *net);
  JunctionTree(Network *net, bool elim_redundant_cliques);
  JunctionTree(Network *net, string elim_ord_strategy, bool elim_redundant_cliques);
  JunctionTree(Network *net, string elim_ord_strategy, bool elim_redundant_cliques, vector<int> custom_elim_ord);
  explicit JunctionTree(JunctionTree*);
  virtual ~JunctionTree() = default;

  void ResetJunctionTree();
  void LoadEvidenceAndMessagePassingUpdateJT(const Combination &E);

  void PrintAllCliquesPotentials() const;
  void PrintAllSeparatorsPotentials() const;

  Factor BeliefPropagationReturnPossib(set<int> &indexes);
  int InferenceUsingBeliefPropagation(set<int> &indexes);

  double TestNetReturnAccuracy(int class_var, Trainer *tst);

 protected:
  map<Clique*,Clique> map_cliques_backup;
  map<Separator*,Separator> map_separators_backup;

  void Triangulate(Network *net,
                   int **adjac_matrix,
                   int &num_nodes,
                   vector<int> elim_ord,
                   set<Clique*> &cliques);
  void ElimRedundantCliques();
  void FormListShapeJunctionTree(set<Clique*> &cliques);
  void FormJunctionTree(set<Clique*> &cliques);
  void NumberTheCliquesAndSeparators();
  void AssignPotentials();
  void BackUpJunctionTree();
  virtual void LoadEvidence(const Combination &E);
  void MessagePassingUpdateJT();
  static vector<int> MinNeighbourElimOrd(int **adjac_matrix, int &num_nodes);
  static void Moralize(int **direc_adjac_matrix, int &num_nodes);
  void GenMapElimVarToClique();

};


#endif //BAYESIANNETWORK_JUNCTIONTREE_H
