#ifndef BAYESIANNETWORK_CLIQUE_H
#define BAYESIANNETWORK_CLIQUE_H

#include <set>
#include <map>
#include <utility>
#include <string>
#include <queue>
#include "gadget.h"
#include "Node.h"
#include "Factor.h"
#include "Network.h"
#include "PotentialTable.h"
#include "Timer.h"
#include "omp.h"

typedef set< pair<int, int> > DiscreteConfig;

class Clique {

 public:

  bool is_separator;//a clique can separate two other cliques.
  int clique_id;
  int clique_size;//the number of nodes in this clique
  bool pure_discrete;

    set <int> clique_variables;
  // the following three members are the same with the class "Factor"
//  set<int> related_variables; //the variables involved in this clique
//  set<DiscreteConfig> set_disc_configs; //all the configurations of the related variables
//  map<DiscreteConfig, double> map_potentials; //the potential of each discrete config
    Factor table;
    PotentialTable p_table;

  set<Clique*> set_neighbours_ptr; // neighbor cliques

  /**
   * In junction tree algorithm,
   * the "Collect" force messages to flow from downstream to upstream,
   * and the "Distribute" force messages flow from upstream to downstream.
   * So, we need a member to record the upstream of this clique (node).
   */
  Clique *ptr_upstream_clique; // a clique only has one upstream, and all the other neighbours are downstreams.
    vector<Clique *> ptr_downstream_cliques;

  /**
   * Data structures needed for elimination tree
   * (like junction tree) of Gaussian Bayesian network.
   * Proposed in [Local Propagation in Conditional Gaussian Bayesian Networks (Cowell, 2005)]
   * Note that, separators between continuous cliques only retain post_bag but not lp_potential.
   */

  Clique();
//  Clique(set<Node*> set_node_ptrs, int elim_var_index);
    Clique(set<Node*> set_node_ptr);
    Clique(set<int> set_node_index, Network *net);
    virtual ~Clique() {};

//  void Collect(Timer *timer);
//  void Distribute(Timer *timer);

    void Collect2();
    void Collect3(vector<vector<Clique*>> &cliques, int max_level);
    void Collect3(vector<vector<Clique*>> &cliques, int max_level, int num_threads, Timer *timer);

    void Distribute2();
    void Distribute2(PotentialTable &pt);
    void Distribute3(vector<vector<Clique*>> &cliques, int max_level);
    void Distribute3(vector<vector<Clique*>> &cliques, int max_level, int num_threads);

//    virtual void UpdateUseMessage2(const PotentialTable &pt);
//    virtual void ConstructMessage2();
    virtual void UpdateMessage(const PotentialTable &pt);

//  void PrintPotentials() const;
//  void PrintRelatedVars() const;

 protected:
//  Clique(const Clique&) = default;

    void PreInitializePotentials();

//    void Distribute2(PotentialTable &pt, Timer *timer);
};

#endif //BAYESIANNETWORK_CLIQUE_H
