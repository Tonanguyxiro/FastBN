//
// Created by Linjian Li on 2018/11/29.
//

#include "Node.h"


Node::Node() {}


int Node::GetNodeIndex() const {
  return node_index;
}


void Node::SetNodeIndex(int i) {
  if (i<0) {
    fprintf(stderr, "Error in function %s! \nInvalid index!", __FUNCTION__);
    exit(1);
  }
  node_index = i;
}


void Node::RemoveChild(Node *c) {
  if (set_children_ptrs.find(c)==set_children_ptrs.end()) {
    fprintf(stderr, "Node #%d does not have parent node #%d!", this->GetNodeIndex(), c->GetNodeIndex());
    return;
  }
  set_children_ptrs.erase(c);
}


void Node::RemoveParent(Node *p) {
  if (set_children_ptrs.find(p)==set_parents_ptrs.end()) {
    fprintf(stderr, "Node #%d does not have parent node #%d!", this->GetNodeIndex(), p->GetNodeIndex());
    return;
  }
  set_parents_ptrs.erase(p);
}


/**
 * Generate all combinations of values of parents.
 */
void Node::GenDiscParCombs() {
  // Preprocess. Construct set of sets.
  set<Combination> set_of_sets;
  if (set_parents_ptrs.empty()) return;
  for (const auto par_ptr : set_parents_ptrs) {
    Combination cb;
    pair<int, int> ele;
    for (int i=0; i<par_ptr->num_potential_vals; i++) {
      ele.first = par_ptr->node_index;
      ele.second = par_ptr->potential_vals[i];
      cb.insert(ele);
    }
    set_of_sets.insert(cb);
  }

  // Generate
  set_discrete_parents_combinations = GenAllCombFromSets(&set_of_sets);

}


int Node::SampleNodeGivenParents(Combination evidence) {
  // The evidence should contain all parents of this node.
  // The evidence about other nodes (including children) are IGNORED!!!
  set<int> set_par_indexes;
  for (auto &par : set_parents_ptrs) {
    set_par_indexes.insert(par->GetNodeIndex());
  }
  Combination par_evi;
  for (auto &e : evidence) {
    if (set_par_indexes.find(e.first)!=set_par_indexes.end()) {
      par_evi.insert(e);
    }
  }

  vector<int> weights;
  if (par_evi.empty()) {
    for (int i=0; i<num_potential_vals; ++i) {
      int w = (int)(map_marg_prob_table[potential_vals[i]]*10000);
      weights.push_back(w);
    }
  } else {
    for (int i=0; i<num_potential_vals; ++i) {
      int w = (int)(map_cond_prob_table[potential_vals[i]][par_evi]*10000);
      weights.push_back(w);
    }
  }

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine rand_gen(seed);
  discrete_distribution<int> this_distribution(weights.begin(),weights.end());
  return potential_vals[this_distribution(rand_gen)];
}