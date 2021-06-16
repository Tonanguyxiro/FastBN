#pragma clang diagnostic push
#pragma ide diagnostic ignored "openmp-use-default-none"
//
// Created by Linjian Li on 2018/11/29.
//

#include "Network.h"
#include "ScoreFunction.h"
#include "DiscreteNode.h"
#include "ContinuousNode.h"

Network::Network(): Network(true) {}

Network::Network(bool pure_disc) {
  this->pure_discrete = pure_disc;
}

Network::Network(Network &net) {
  //need to test correctness

  network_name = net.network_name;
  num_nodes = net.num_nodes;
  pure_discrete = net.pure_discrete;
  vec_default_elim_ord = net.vec_default_elim_ord;
  topo_ord = net.GetTopoOrd();//this sequence/order is used for variable elimination.

  //copy the map for mapping id to node ptr
  map_idx_node_ptr = net.map_idx_node_ptr;
  for (int i = 0; i < num_nodes; ++i) {
    auto old_node = map_idx_node_ptr.at(i);
    Node *n = nullptr;
    if (old_node->is_discrete) {
      auto disc_old_node = (DiscreteNode*)old_node;
      n = new DiscreteNode(*disc_old_node);
    } else {
      auto cont_old_node = (ContinuousNode*)old_node;
      n = new ContinuousNode(*cont_old_node);
    }
    map_idx_node_ptr[i] = n;
  }

}

//TODO: double-check correctness
/*!
 * @brief: print the parents of all the nodes.
 * each line corresponds to a node and its parents.
 */
void Network::PrintEachNodeParents() {
  for (const auto &id_node_ptr : map_idx_node_ptr) { // for each <int, Node*> pair
    auto node_ptr = id_node_ptr.second; // get the Node*
    cout << node_ptr->node_name << ":\t";
    for (const auto &par_node_ptr : GetParentPtrsOfNode(node_ptr->GetNodeIndex())) {
      cout << par_node_ptr->node_name << '\t';
    }
    cout << endl;
  }
}

//TODO: double-check correctness
void Network::PrintEachNodeChildren() {//print the child nodes of all the nodes
  for (const auto &id_node_ptr : map_idx_node_ptr) {
    auto node_ptr = id_node_ptr.second;
    cout << node_ptr->node_name << ":\t";
    for (const auto &chi_node_ptr : GetChildrenPtrsOfNode(node_ptr->GetNodeIndex())) {
      cout << chi_node_ptr->node_name << '\t';
    }
    cout << endl;
  }
}

/**
 * @brief: find node ptr by id
 */
Node* Network::FindNodePtrByIndex(const int &index) const {
  if (index < 0 || index >= num_nodes) {  // The node indexes are consecutive integers start at 0.
    fprintf(stderr, "Error in function %s! \nInvalid index [%d]!", __FUNCTION__, index);
    exit(1);
  }
  return map_idx_node_ptr.at(index);
}

/**
 * @brief: find node ptr by name
 */
Node* Network::FindNodePtrByName(const string &name) const {
  Node* node_ptr = nullptr;
  for (const auto i_n_ptr : map_idx_node_ptr) {
    auto n_ptr = i_n_ptr.second;
    if (n_ptr->node_name==name) {
      node_ptr = n_ptr;
      break;
    }
  }
  return node_ptr;
}

/**
 * @brief: construct a Naive Bayesian Network with the class variable as the root and all the other variables as leaves.
 * structure of Naive Bayes: label is the root node; all feature variables are the children of the root node.
 * time complexity: O( number of feature variables * number of possible labels of class variable)
 * (not consider the time spent on generating topological ordering)
 */
void Network::ConstructNaiveBayesNetwork(Dataset *dts) {

  // common part with "StructLearnCompData" TODO
  num_nodes = dts->num_vars;

  // Assign an index for each node.
#pragma omp parallel for
  for (int i = 0; i < num_nodes; ++i) {
    DiscreteNode *node_ptr = new DiscreteNode(i);  // For now, only support discrete node.

    //set the potential values for this node
    node_ptr->SetDomainSize(dts->num_of_possible_values_of_disc_vars[i]);
    for (auto v : dts->map_disc_vars_possible_values[i]) {
      node_ptr->vec_potential_vals.push_back(v);//TODO: keep name convention consistent (i.e. possible vs potential)
    }
#pragma omp critical
    {
      map_idx_node_ptr[i] = node_ptr;
    }
  }

  // Set parents and children.
  Node *class_node_ptr = FindNodePtrByIndex(dts->class_var_index);
  for (auto &i_n : map_idx_node_ptr) { // for each id-node_ptr pair i_n
    if (i_n.second == class_node_ptr) { // TODO: directly "if (xxx != xxx)"
      continue;
    }
    // all feature variables are the children of the root node (i.e., the label)
    SetParentChild(class_node_ptr, i_n.second);
  }

  // Generate parent configurations for all nodes in the network.
  // TODO: for Naive Bayesian Network, it may not need to call "GenDiscParCombsForAllNodes"
  // because "GenDiscParCombsForAllNodes" needs "GetParentPtrsOfNode" -> "GenDiscParCombs",
  // where "GenAllCombinationsFromSets" is expensive.
  // while we can directly generate configurations of the root node,
  // it is exactly the parent configurations of all nodes except for the root;
  // and the parent configurations of the root is ONE empty parent configuration (cf. "GenDiscParCombs" in class Node)
  GenDiscParCombsForAllNodes();

  // Generate topological ordering and default elimination ordering.
  vector<int> topo = GetTopoOrd();

  vec_default_elim_ord.reserve(num_nodes - 1);//-1, because the last one is the value (which must be kept) to be inferred.
  for (int i = 0; i < num_nodes-1; ++i) { // TODO: for num_nodes-1 -> 0, at(i)
    vec_default_elim_ord.push_back(topo.at(num_nodes-1-i));
  }

  //check for correctness (i.e. reserved size == number of variables).
  int vec_size = vec_default_elim_ord.size();
  int vec_capacity = vec_default_elim_ord.capacity();
  if (vec_size != vec_capacity) {
    fprintf(stderr, "Function [%s]: vec_size != vec_capacity\n", __FUNCTION__);
  }
}

/**
 * @brief: learn structure from a data set without missing values
 * @param dts:                  data set
 * @param print_struct:         whether want to print structure while learning
 * @param algo:                 specify a network structure learning algorithm
 * @param topo_ord_constraint:  a constraint to limit the learned network structure (i.e. the # of possible structures is often huge)
 * @param max_num_parents:      a node can only has max_num_parents (to limit the # of possible networks)
 */
void Network::StructLearnCompData(Dataset *dts, bool print_struct, string algo, string topo_ord_constraint, int max_num_parents) {
  fprintf(stderr, "Not be completely implemented yet!");//TODO: most of the functions have been implemented; "algo" hasn't been used.

  cout << "==================================================" << '\n'
       << "Begin structural learning with complete data......" << endl;

  struct timeval start, end;
  double diff;

  gettimeofday(&start,NULL);

  // common part with "ConstructNaiveBayesNetwork" TODO
  num_nodes = dts->num_vars;
  // Assign an index for each node.
#pragma omp parallel for
  for (int i = 0; i < num_nodes; ++i) {
    DiscreteNode *node_ptr = new DiscreteNode(i);  // For now, only support discrete node.

    // TODO: extra part. double-check the usage of "node_name"
    //give this node a name
    if (dts->vec_var_names.size() == num_nodes) {
      node_ptr->node_name = dts->vec_var_names.at(i);
    }
    else {
      node_ptr->node_name = to_string(i);//use id as name
    }

    //set the potential values for this node
    node_ptr->SetDomainSize(dts->num_of_possible_values_of_disc_vars[i]);
    for (auto v : dts->map_disc_vars_possible_values[i]) {
      node_ptr->vec_potential_vals.push_back(v); //TODO: keep name convention consistent (i.e. possible vs potential)
    }
#pragma omp critical
    {
      map_idx_node_ptr[i] = node_ptr;
    }
  }

  //assign an order of the nodes; the order will be used in structure learning
  vector<int> ord;
  ord.reserve(num_nodes);
  for (int i = 0; i < num_nodes; ++i) {
    ord.push_back(i);   // the nodes are created the same order as in the dataset.
  }

  cout << "topo_ord_constraint: " << topo_ord_constraint << endl;
  //choose an order to serve as a constraint during learning
  if (topo_ord_constraint == "dataset-ord") {
    // Do nothing.
  }
  else if (topo_ord_constraint == "random") {
    std::srand(unsigned(std::time(0)));
    std::random_shuffle(ord.begin(), ord.end());
  }
  else if (topo_ord_constraint == "best") {
    ord = vector<int> {};//no order is provided (i.e. no constraint)
  }
  else {
    fprintf(stderr, "Error in function [%s]!\nInvalid topological ordering restriction!", __FUNCTION__);
    exit(1);
  }

  //choose an algorithm for the learning
  if (algo == "ott") {
    StructLearnByOtt(dts, ord);
  }
  else if (algo == "k2-weka") {
    StructLearnLikeK2Weka(dts, ord, max_num_parents);
  }


  cout << "==================================================" << '\n'
       << "Finish structural learning." << endl;

  gettimeofday(&end,NULL);
  diff = (end.tv_sec-start.tv_sec) + ((double)(end.tv_usec-start.tv_usec))/1.0E6;
  setlocale(LC_NUMERIC, "");//formatting the output
  cout << "==================================================" << '\n'
       << "The time spent to learn the structure is " << diff << " seconds" << endl;


  if (print_struct) {

    cout << "==================================================" << '\n'
         << "Topological sorted permutation generated using width-first-traversal: " << endl;
    auto topo = GetTopoOrd();
    for (int m = 0; m < num_nodes; ++m) {
      cout << topo.at(m) << ", ";
    }
    cout << endl;

    cout << "==================================================" << '\n'
         << "Each node's parents: " << endl;
    this->PrintEachNodeParents();
  }
}

/**
 * @brief: add a node to the map
 */
void Network::AddNode(Node *node_ptr) {
  map_idx_node_ptr[node_ptr->GetNodeIndex()] = node_ptr;
  ++num_nodes;
}

/**
 * @brief: remove a node based on id; not used yet.
 */
void Network::RemoveNode(int node_index) {
  map_idx_node_ptr.erase(node_index);
  --num_nodes;
}

/**
 * @brief: add an edge/arc to the network
 * @return true if not form a circle; false if form a circle (also delete the added arc)
 */
bool Network::AddArc(int p_index, int c_index) {
  SetParentChild(p_index, c_index); // set parent and child relationship
  bool contain_circle = ContainCircle();

  // the edge/arc shouldn't be added, because it leads to loops in the network.
  if (contain_circle) {
    DeleteArc(p_index, c_index);
  }
  return !contain_circle;
}

void Network::DeleteArc(int p_index, int c_index) {
  RemoveParentChild(p_index, c_index);
}

/**
 * @brief: swap a parent and child relationship
 * @return true if not form a circle; false if form a circle (also delete the added arc)
 */
bool Network::ReverseArc(int p_index, int c_index) {
  DeleteArc(p_index, c_index);
  return AddArc(c_index, p_index);
}

/**
 * @brief: calculate delta score when modified the arcs
 * this function is based on WEKA: calcScoreWithExtraParent(iAttribute, iAttribute2);
 * @param modification: includes "add", "delete", and "reverse".
 */
double Network::CalcuExtraScoreWithModifiedArc(int p_index, int c_index,
                                               Dataset *dts,
                                               string modification,
                                               string score_metric) {
  // todo: test correctness
  Network new_net(*this); // modify on new_net rather than the original "this"

  // Convert the string to lowercase
  transform(modification.begin(), modification.end(), modification.begin(), ::tolower);

  Node *node = new_net.FindNodePtrByIndex(c_index);

  // do the modification
  if (modification == "add") {
    if (node->set_parent_indexes.find(p_index)!=node->set_parent_indexes.end()) {
      return 0; // The parent already exists.
    }
    new_net.AddArc(p_index, c_index);
  }
  else if (modification == "delete") {
    if (node->set_parent_indexes.find(p_index)==node->set_parent_indexes.end()) {
      return 0; // The parent does not exist.
    }
    new_net.DeleteArc(p_index, c_index);
  }
  else if (modification == "reverse") {
    if (node->set_parent_indexes.find(p_index)==node->set_parent_indexes.end()) {
      return 0; // The parent does not exist.
    }
    new_net.ReverseArc(p_index, c_index);
  }
  else {
    fprintf(stderr, "Fucntion [%s]: Invalid modification string \"%s\"!",
            __FUNCTION__, modification.c_str());
    exit(1);
  }

  Node *old_c_node = this->FindNodePtrByIndex(c_index),
       *new_c_node = new_net.FindNodePtrByIndex(c_index);

  new_c_node->GenDiscParCombs(new_net.GetParentPtrsOfNode(c_index));

  ScoreFunction old_sf(this, dts),
                new_sf(&new_net, dts);

  double old_score = old_sf.ScoreForNode(old_c_node, score_metric),
         new_score = new_sf.ScoreForNode(new_c_node, score_metric);

  double delta = new_score - old_score;
  return delta;
}

/**
 * @brief: set up the parent and child relationship
 * @param p_index: parent index
 * @param c_index: child index
 */
void Network::SetParentChild(int p_index, int c_index) {
  // convert the index format into node ptr format
  Node *p = FindNodePtrByIndex(p_index), *c = FindNodePtrByIndex(c_index);
  SetParentChild(p,c);
}

/**
 * @brief: set parent and child relationship.
 * @param p: parent node ptr
 * @param c: child node ptr
 * add c to p as a child, and add p to c as a parent
 */
void Network::SetParentChild(Node *p, Node *c) {
  if (map_idx_node_ptr.find(p->GetNodeIndex()) == map_idx_node_ptr.end()
      ||
      map_idx_node_ptr.find(c->GetNodeIndex())==map_idx_node_ptr.end()) {
    fprintf(stderr, "Error in function [%s].\nThe nodes [%d] and [%d] do not belong to this network!",
            __FUNCTION__, p->GetNodeIndex(), c->GetNodeIndex());
    exit(1);
  }//end checking whether the nodes belong to the network
  p->AddChild(c);
  c->AddParent(p);
}

/**
 * @brief: remove parent child relationship
 * @param p_index: parent index
 * @param c_index: child index
 * remove c to p as a child, and remove p to c as a parent
 */
void Network::RemoveParentChild(int p_index, int c_index) {
  // convert the index format into node ptr format
  Node *p = FindNodePtrByIndex(p_index), *c = FindNodePtrByIndex(c_index);
  RemoveParentChild(p,c);
}

/**
 * @brief: remove parent child relationship using pointers
 * @param p: parent node ptr
 * @param c: child node ptr
 */
void Network::RemoveParentChild(Node *p, Node *c) {
  if (map_idx_node_ptr.find(p->GetNodeIndex()) == map_idx_node_ptr.end()
      ||
      map_idx_node_ptr.find(c->GetNodeIndex())==map_idx_node_ptr.end()) {
    fprintf(stderr, "Error in function [%s].\nThe nodes [%d] and [%d] do not belong to this network!",
            __FUNCTION__, p->GetNodeIndex(), c->GetNodeIndex());
    exit(1);
  }//end checking whether the nodes belong to the network
  p->RemoveChild(c);
  c->RemoveParent(p);
}

/**
 * @brief: find parents given a node id; used to generate discrete configurations
 * @return a set of pointers to the parents of a node
 */
set<Node*> Network::GetParentPtrsOfNode(int node_index) {
  set<Node*> set_par_ptrs;
  Node *node = map_idx_node_ptr.at(node_index); // TODO: function "FindNodePtrByIndex"
  for (const auto &idx : node->set_parent_indexes) { // "set_parent_indexes" contains both discrete and continuous parents
    set_par_ptrs.insert(map_idx_node_ptr.at(idx));
  }
  return set_par_ptrs;
}

/**
 * @brief: find the children given a node id
 */
set<Node*> Network::GetChildrenPtrsOfNode(int node_index) {
  set<Node*> set_chi_ptrs;
  Node *node = map_idx_node_ptr.at(node_index); // TODO: function "FindNodePtrByIndex"
  for (const auto &idx : node->set_children_indexes) {
    set_chi_ptrs.insert(map_idx_node_ptr.at(idx));
  }
  return set_chi_ptrs;
}

/**
 * @brief: generate all the configurations of the parents for each node
 */
void Network::GenDiscParCombsForAllNodes() {
  for (auto id_np : this->map_idx_node_ptr) { // for each node (id-node_ptr pair) in the network
    auto np = id_np.second;
    np->GenDiscParCombs(GetParentPtrsOfNode(np->GetNodeIndex()));
  }
}

/**
 * @brief: obtain topological order
 * @return a vector<int>, the elements is the indexes of the nodes
 */
vector<int> Network::GetTopoOrd() {
  if (topo_ord.empty()) {
    this->GenTopoOrd();
  }
  return topo_ord;
}

/**
 * @brief: obtain reverse topological order
 */
vector<int> Network::GetReverseTopoOrd() {
  auto ord = this->GetTopoOrd();
  reverse(ord.begin(), ord.end());
  return ord;
}

/*!
 * @brief: generate the topological order
 * @return a vector<int>, the elements is the indexes of the nodes
 * generate 1. a directed adjacency matrix and 2. an in-degree array to generate the topological order
 */
// TODO: maybe not need to generate a directed adjacency matrix
// just use in-degree array and "set_children_indexes" to generate the ordering
// TODO: potential bug in "TopoSortOfDAGZeroInDegreeFirst" -> "TopoSortOfDAGZeroInDegreeFirst"
vector<int> Network::GenTopoOrd() {

  if (this->pure_discrete) {

    // convert the network to a directed adjacency matrix (n*n)
    // direct: p->c (i.e., graph[p][c] = 1)
    // TODO: use function "ConvertDAGNetworkToAdjacencyMatrix"
    // TODO: adjacency matrix or adjacency list? more memory for adjacency matrix
    int **graph = new int*[num_nodes];
#pragma omp for
    for (int i=0; i<num_nodes; ++i) {
      graph[i] = new int[num_nodes]();
    }

    // TODO: calculate the in-degrees here instead of in "TopoSortOfDAGZeroInDegreeFirst"
    for (auto &i_n_p : map_idx_node_ptr) { // for each node
      auto n_p = i_n_p.second;
      for (auto &c_p : GetChildrenPtrsOfNode(n_p->GetNodeIndex())) {
        // TODO: each time assigning 1, add 1 to the in-degree of "c_p->GetNodeIndex()"
        graph[n_p->GetNodeIndex()][c_p->GetNodeIndex()] = 1;
      }
    }

    topo_ord = TopoSortOfDAGZeroInDegreeFirst(graph, num_nodes);

    for (int i=0; i<num_nodes; ++i) {
      delete[] graph[i];
    }
    delete[] graph;

  }
  else { // TODO: double-check, not check for the continuous cases

    // If the network is not pure discrete, then it is conditional Gaussian.
    // Discrete nodes should not have continuous parents.
    // Continuous nodes can have both discrete and continuous parents.
    // In topological ordering, all discrete nodes should occur before any continuous node.
    // todo: test correctness of the case of Gaussian network

    set<Node*> set_disc_node_ptr, set_cont_node_ptr;
    for (const auto &i_n_p : map_idx_node_ptr) {
      auto n_p = i_n_p.second;
      if (n_p->is_discrete) {
        set_disc_node_ptr.insert(n_p);
      } else {
        set_cont_node_ptr.insert(n_p);
      }
    }
    int **graph_disc = new int*[set_disc_node_ptr.size()];
    int **graph_cont = new int*[set_cont_node_ptr.size()];
    for (int i=0; i<set_disc_node_ptr.size(); ++i) {
      graph_disc[i] = new int[set_disc_node_ptr.size()]();
    }
    for (int i=0; i<set_cont_node_ptr.size(); ++i) {
      graph_disc[i] = new int[set_disc_node_ptr.size()]();
      graph_cont[i] = new int[set_cont_node_ptr.size()]();
    }

    // Generate the ordering for discrete nodes.
    map<int, int> disc_order_index, disc_index_order;
    int disc_ord = 0;
    for (const auto &n_p : set_disc_node_ptr) {
      disc_order_index[disc_ord] = n_p->GetNodeIndex();
      disc_index_order[n_p->GetNodeIndex()] = disc_ord;
      ++disc_ord;
    }
    for (const auto &n_p : set_disc_node_ptr) {
      for (const auto &c_p : GetChildrenPtrsOfNode(n_p->GetNodeIndex())) {
        if (!c_p->is_discrete) { continue; }
        graph_disc[ disc_index_order[n_p->GetNodeIndex()] ]
                  [ disc_index_order[c_p->GetNodeIndex()] ] = 1;
      }
    }
    vector<int> topo_ord_disc = TopoSortOfDAGZeroInDegreeFirst(graph_disc, set_disc_node_ptr.size());

    // Generate the ordering for continuous nodes.
    map<int, int> cont_order_index, cont_index_order;
    int cont_ord = 0;
    for (const auto &n_p : set_cont_node_ptr) {
      cont_order_index[cont_ord] = n_p->GetNodeIndex();
      cont_index_order[n_p->GetNodeIndex()] = cont_ord;
      ++cont_ord;
    }
    for (const auto &n_p : set_cont_node_ptr) {
      for (const auto &c_p : GetChildrenPtrsOfNode(n_p->GetNodeIndex())) {
        graph_cont[ cont_index_order[n_p->GetNodeIndex()] ]
                  [ cont_index_order[c_p->GetNodeIndex()] ] = 1;
      }
    }
    vector<int> topo_ord_cont = TopoSortOfDAGZeroInDegreeFirst(graph_cont, set_cont_node_ptr.size());
    // Restore the index from the ordering.
    for (auto &o : topo_ord_cont) {
      o = cont_order_index[o];
    }

    // Concatinate topo_ord_disc and topo_ord_cont.
    topo_ord_disc.insert(topo_ord_disc.end(), topo_ord_cont.begin(), topo_ord_cont.end());
    this->topo_ord = topo_ord_disc;

    for (int i=0; i<set_disc_node_ptr.size(); ++i) {
      delete[] graph_disc[i];
    }
    for (int i=0; i<set_cont_node_ptr.size(); ++i) {
      delete[] graph_cont[i];
    }
    delete[] graph_disc;
    delete[] graph_cont;

  }

  return topo_ord;
}

/**
 * @brief: convert network to a dense directed adjacency matrix (n*n)
 */
int** Network::ConvertDAGNetworkToAdjacencyMatrix() {
  int **matrix = new int* [num_nodes];
  for (int i=0; i<num_nodes; ++i) {
    matrix[i] = new int[num_nodes]();
  }

  // TODO: calculate the in-degrees here
  // TODO: instead of in "TopoSortOfDAGZeroInDegreeFirst" and "DirectedGraphContainsCircleByBFS"
  // direct: node_ptr->child_ptr (i.e., graph[node_ptr][child_ptr] = 1)
  for (auto &id_node_ptr : map_idx_node_ptr) { // for each node
    auto node_ptr = id_node_ptr.second;

    for (auto &child_ptr : GetChildrenPtrsOfNode(node_ptr->GetNodeIndex())) {
      // TODO: each time assigning 1, add 1 to the in-degree of "child_ptr->GetNodeIndex()"
      matrix[node_ptr->GetNodeIndex()][child_ptr->GetNodeIndex()] = 1;
    }
  }
  return matrix;
}

/**
 * @brief: check if network has loops.
 */
bool Network::ContainCircle() {
  int **graph = ConvertDAGNetworkToAdjacencyMatrix();
  bool result = DirectedGraphContainsCircleByBFS(graph, num_nodes);

  for (int i = 0; i < num_nodes; ++i) {
    delete[] graph[i];
  }
  delete[] graph;
  return result;
}

/**
 * @brief: learn the weights or probability tables TODO: check algorithms for parameter learning
 * @brief: get each node's conditional probability table
 */
void Network::LearnParamsKnowStructCompData(const Dataset *dts, int alpha, bool print_params){
  cout << "==================================================" << '\n'
       << "Begin learning parameters with known structure and complete data." << '\n'
       << "Laplace smoothing param: alpha = " << alpha << endl;

  struct timeval start, end;
  double diff;// j is a node index and also an array index
  gettimeofday(&start,NULL);

  int num_cores = omp_get_num_procs();
  omp_set_num_threads(num_cores);
  int max_work_per_thread = (dts->num_vars + num_cores - 1) / num_cores;
  #pragma omp parallel
  {
    int thread_id = omp_get_thread_num();

    // a thread for one or more nodes
    for (int i = max_work_per_thread * thread_id;
         i < max_work_per_thread * (thread_id + 1) && i < dts->num_vars;
         ++i) {
//    for (int i=0; i<dts->num_vars; ++i) {
      // for each variable/node, update probability table of (node | parent configurations)
      DiscreteNode *this_node = dynamic_cast<DiscreteNode*>(FindNodePtrByIndex(i));   // todo: support continuous node
      this_node->SetLaplaceSmooth(alpha);

      for (int s = 0; s < dts->num_instance; ++s) { // for each instance
        // create the vector "values" by copying the array "dts->dataset_all_vars[s]"
        vector<int> values = vector<int>(dts->dataset_all_vars[s], dts->dataset_all_vars[s] + dts->num_vars);
        //convert an instance to discrete configuration
        DiscreteConfig instance; //set<pair<int, int> >
        for (int j = 0; j < values.size(); ++j) { // for each variable of this instance
          instance.insert(pair<int, int>(j, values.at(j)));
        }
        this_node->AddInstanceOfVarVal(instance);//an instance affects all the nodes in the network, because the instance here is dense.
      }
    }
  }   // end of: #pragma omp parallel
  cout << "==================================================" << '\n'
       << "Finish training with known structure and complete data." << endl;

  if (print_params) {
    cout << "==================================================" << '\n'
         << "Each node's conditional probability table: " << endl;
    for (const auto &id_node_ptr : map_idx_node_ptr) {  // For each node
      dynamic_cast<DiscreteNode*>(id_node_ptr.second)->PrintProbabilityTable();
    }
  }

  gettimeofday(&end,NULL);
  diff = (end.tv_sec-start.tv_sec) + ((double)(end.tv_usec-start.tv_usec))/1.0E6;
  setlocale(LC_NUMERIC, "");
  cout << "==================================================" << '\n'
       << "The time spent to learn the parameters is " << diff << " seconds" << endl;

}

/**
 * @brief: get the number of parameters of the network, based on probability tables
 *         (and other parameters in continuous variables)
 */
int Network::GetNumParams() const {
  int result = 0;
  for (const auto &i_n : map_idx_node_ptr) { // TODO: function "FindNodePtrByIndex"
    result += i_n.second->GetNumParams(); // TODO: Node::GetNumParams is a virtual function
  }
  return result;
}

/**
 * @brief: clear structure; mainly for reuse some content for fast testing
 *
 * Important: may have bugs! TODO
 */
void Network::ClearStructure() {
  for (auto &i_n_p : this->map_idx_node_ptr) {
    i_n_p.second->ClearParams();
    i_n_p.second->ClearParents();
    i_n_p.second->ClearChildren();
  }
}

/**
 * @brief: clear structure; mainly for reuse some content for fast testing
 *
 * Important: may have bugs! TODO
 */
void Network::ClearParams() {
  for (auto &i_n_p : this->map_idx_node_ptr) { // TODO: function "FindNodePtrByIndex"
    i_n_p.second->ClearParams(); // TODO: Node::ClearParams is a virtual function
  }
}

/**
 * @brief: this is a virtual function;
 * The performance of variable elimination relies heavily on the elimination ordering
 */
vector<int> Network::SimplifyDefaultElimOrd(DiscreteConfig evidence) {//TODO: use C++ pure virtual function
  fprintf(stderr, "Function [%s] not implemented yet!", __FUNCTION__);
  exit(1);
}

/**
 * @brief: this is a virtual function;
 * The performance of variable elimination relies heavily on the elimination ordering
 */
vector<int> Network::SimplifyDefaultElimOrd2(DiscreteConfig evidence, vector<int> left_nodes) {//TODO: use C++ pure virtual function
  fprintf(stderr, "Function [%s] not implemented yet!", __FUNCTION__);
  exit(1);
}

/**
 * @brief: construct a set of factors using a set of nodes and/or a node
 * @param Z: a set of nodes identified by IDs
 * @param Y: a node
 * @example 1: using the elimination order and a target node
 * @example 2: using the left nodes (filter out the barren nodes and m-separated nodes)
 * @return: a set of Factors, where each factor corresponds to a node
 */
vector<Factor> Network::ConstructFactors(vector<int> Z, Node* Y) {
  vector<Factor> factors_list;
  if(Y) {
    factors_list.push_back(Factor(dynamic_cast<DiscreteNode*>(Y), this));
  }
  for (int i = 0; i < Z.size(); ++i) {
    Node* n = FindNodePtrByIndex(Z.at(i));
    factors_list.push_back(Factor(dynamic_cast<DiscreteNode*>(n), this));
  }
  return factors_list;
}

/**
 * @brief: update the probabilities/weights of all the factors related to the nodes between the target node and the evidence/observation
 * @param factors_list: a list factors related to the nodes between the target node and the evidence/observation
 * @param E: a new observation
 * @param all_related_vars: all the related variables between the target node and the node with the evidence/observation
 */
void Network::LoadEvidenceIntoFactors(vector<Factor> *factors_list,
                                      DiscreteConfig E, set<int> all_related_vars) {

  // TODO: check openmp
  // I do not know why this function cannot use omp to parallel.
  // If I set number of threads more than 1, the accuracy will decrease!
//  int num_cores = omp_get_num_procs();
//  omp_set_num_threads(num_cores);
//  int max_work_per_thread = (factors_list->size()+num_cores-1)/num_cores;
//  #pragma omp parallel
  {
//    for (int i = omp_get_thread_num() * max_work_per_thread;
//         i < (omp_get_thread_num()+1) * max_work_per_thread && i < factors_list->size();
//         ++i) {

    for (int i = 0; i < factors_list->size(); ++i) {
      Factor &f = factors_list->at(i);   // For each factor. "std::vector::at" returns reference.
      f.FactorReduction(E); // factor reduction given evidence

      // fix the bug occurring after removing the irrelevant nodes (barren nodes and m-separated nodes)
      // For example:  X--> Y (evidence/obs) --> Z --> A (target node) --> B, where X and B are the irrelevant nodes,
      // there is no need to calculate {X, B}, but X is the parent of Y (which is a relevant node),
      // so X still appears in the constructed factor of the node Y
      // what we need to do is to eliminate it before the main variable elimination (VE) process.
      // since B is a child of the relevant node, so it does not have this problem
      set<int> related_vars_of_f = f.related_variables;
      for (auto &v : related_vars_of_f) { // for each related variables of the factor f
        if (all_related_vars.find(v) == all_related_vars.end()) { // if v is not in the left nodes
          f = f.SumOverVar(v); //X and G will be sum over, i.e. eliminate X and G given the evidence/observations
        }
      }
    }
  }   // end of: #pragma omp parallel
}

/**
 * @brief: the main variable elimination (VE) process
 * gradually eliminate variables until only one (i.e. the target node) left
 */
Factor Network::SumProductVarElim(vector<Factor> factor_list, vector<int> elim_order) {
  for (int i = 0; i < elim_order.size(); ++i) { // consider each node i according to the elimination order
    vector<Factor> temp_factor_list;
    Node* nodePtr = FindNodePtrByIndex(elim_order.at(i));

    // Move every factor that is related to the node elim_order[i] from factors_list to tempFactorsList.
    /*
     * Note: This for loop does not contain "++it" in the parentheses.
     *      When finding "elim_order[i]" during the traverse, the iterator "it" points to this vector.
     *      We use "erase" to delete this element from "factors_list" via iterator "it";
     *      the function "erase" returns an iterator pointing to the next element of the delete element.
     */
    for (auto it = factor_list.begin(); it != factor_list.end(); /* no ++it */) {
      // if the factor "it" is related to the node "elim_order[i]" (i.e., the node to be eliminated now)
      if ((*it).related_variables.find(nodePtr->GetNodeIndex()) != (*it).related_variables.end()) {
        temp_factor_list.push_back(*it);
        factor_list.erase(it);
        continue;
      }
      else {
        ++it;
      }
    }

    // merge all the factors in tempFactorsList into one factor
    while(temp_factor_list.size() > 1) {
      // every time merge two factors into one
      Factor temp1, temp2, product;
      temp1 = temp_factor_list.back(); // get the last element
      temp_factor_list.pop_back();  // remove the last element
      temp2 = temp_factor_list.back();
      temp_factor_list.pop_back();

      product = temp1.MultiplyWithFactor(temp2);
      temp_factor_list.push_back(product);
    }

    // eliminate variable "nodePtr" by summation of the factor "tempFactorsList.back()" over "nodePtr"
    Factor newFactor = temp_factor_list.back().SumOverVar(dynamic_cast<DiscreteNode*>(nodePtr));
    factor_list.push_back(newFactor);
  } // finish eliminating variables and only one variable left

  // if the "factor_list" contains several factors, we need to multiply these several factors
  // for example, the case when we have a full evidence...
  // then "factor_list" contains "num_nodes" factor while "elim_order" is empty
  while (factor_list.size() > 1) {
    Factor temp1, temp2, product;
    temp1 = factor_list.back(); // get the last element
    factor_list.pop_back();  // remove the last element
    temp2 = factor_list.back();
    factor_list.pop_back();

    product = temp1.MultiplyWithFactor(temp2);
    factor_list.push_back(product);
  }

  // After all the processing shown above, the only remaining factor is the factor about Y.
  return factor_list.back();
}

/**
 * @brief: infer the marginal probability of the target node, given the evidence
 * @param evid: the given evidence
 * @param target_node: the node whose probability needs to be updated given the evidence/observation.
 * @param elim_order: elimination order
 * @return the factor which contains the marginal probability table of the target node
 */
Factor Network::VarElimInferReturnPossib(DiscreteConfig evid, Node *target_node, vector<int> elim_order) {

//  cout << endl << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << '\n'
//       << "The evidence node: " << endl;
//  for (auto e: evid) {
//    cout << e.first << ", ";
//  }

  if (elim_order.empty()) {
    // call "ChowLiuTree::SimplifyDefaultElimOrd"; "elim_order" is the reverse topological order removing barren nodes and m-separated nodes
    elim_order = SimplifyDefaultElimOrd(evid);
  }

  // "factorsList" corresponds to all the nodes which are between the target node and the observation/evidence
  // because we have removed barren nodes and m-separated nodes
  vector<Factor> factorsList = ConstructFactors(elim_order, target_node);

  //--------------------------------------------------------------------------------
  // // Fix bug Step 1/2.
  // This block is to fix the bug occurring when the target node
  // is not the root and the variable elimination order do not start at root.
  // For example:  X--> Y (evidence/obs) --> Z --> A (target node) --> B --> C --> E --> F (evidence/observation; not important) --> G
  //
  // For example: A --> B --> C (old example)
  // When given the markov blanket of node "C", which is "{B}",
  // there is no need to calculate the other nodes, which is "{A}".
  // However, when using this function,
  // the parent of parent of this node, which is "A",
  // still appears in the constructed factor of the parent which is "B".
  // my understanding:
  // maybe some variables are not in the elimination order, but they are also existed in some related factors,
  // so what we need to do is to eliminate these variables before the main variable elimination (VE) process.
  set<int> all_related_vars;
  all_related_vars.insert(target_node->GetNodeIndex());
  for (int i = 0; i < elim_order.size(); ++i) {
    all_related_vars.insert(elim_order.at(i));
  }
  //--------------------------------------------------------------------------------

  // load evidence function below returns a factorsList with fewer configurations.
  LoadEvidenceIntoFactors(&factorsList, evid, all_related_vars);

  // compute the probability table of the target node
  Factor target_node_factor = SumProductVarElim(factorsList, elim_order);
  // renormalization
  target_node_factor.Normalize();

  return target_node_factor;
}

///**
// * @brief: for inference given a target variable id and some evidences/observations.
// */
//Factor Network::GetMarginalProbabilitiesUseBruteForce(int target_var_index, DiscreteConfig evidence) {
//  if (!this->pure_discrete) {
//    fprintf(stderr, "Function [%s] only works on pure discrete networks!", __FUNCTION__);
//    exit(1);
//  }
//
//  /// reduce each factor of variable according to the evidence and construct "factors_list"
//  vector<Factor> factors_list;
//  for (int i = 0; i < num_nodes; ++i) { // for each node in the network
//    Node* node = FindNodePtrByIndex(i);
//    Factor factor(dynamic_cast<DiscreteNode*>(node), this); // each node corresponds to a factor
//
//    // factor reduction given the evidence
//    factor.FactorReduction(evidence);
//    factors_list.push_back(factor);
//  }
//
//  /// get the product of factors in "factors_list"
//  while(factors_list.size() > 1) {
//    // every time merge two factors into one
//    Factor temp1, temp2, product;
//    temp1 = factors_list.back(); // get the last element
//    factors_list.pop_back();  // remove the last element
//    temp2 = factors_list.back();
//    factors_list.pop_back();
//
//    product = temp1.MultiplyWithFactor(temp2);
//    factors_list.push_back(product);
//  }
//
//  /// get the variables to be marginalized
//  vector<int> marginalize_vars;
//  for (int i = 0; i < num_nodes; ++i) {
//    bool is_target_or_evidence = false;
//    if (i == target_var_index) {
//      is_target_or_evidence = true; // i is the target node
//    }
//    else {
//      for (auto &e: evidence) {
//        if (i == e.first) {
//          is_target_or_evidence = true; // i is an evidence
//          break;
//        }
//      }
//    }
//    // if the variable is not the target and the evidence,
//    // then the variable needs to be marginalized
//    if (!is_target_or_evidence) {
//      marginalize_vars.push_back(i);
//    }
//  }
//
//  /// sum out the variables in "marginalize_vars"
//  for (int i = 0; i < marginalize_vars.size(); ++i) { // for each variable to be summed out
//    Node* node = FindNodePtrByIndex(marginalize_vars.at(i));
//    Factor new_factor = factors_list.back().SumOverVar(dynamic_cast<DiscreteNode*>(node));
//    factors_list.pop_back();
//    factors_list.push_back(new_factor);
//  }
//
//  /// renormalization
//  Factor target_node_factor = factors_list.back();
//  target_node_factor.Normalize();
//
//  return target_node_factor;
//}
//
///**
// * @brief: predict label given (partial or full observation) evidence
// * @return label of the target variable
// */
//int Network::PredictUseBruteForce(DiscreteConfig evid, int target_node_idx) {
//  // get the factor (marginal probability) of the target node given the evidences
//  Factor F = GetMarginalProbabilitiesUseBruteForce(target_node_idx, evid);
//
//  // find the configuration with the maximum probability
//  double max_prob = 0;
//  DiscreteConfig comb_predict;
//  for (auto &comb : F.set_disc_config) { // for each configuration of the related variables
//    if (F.map_potentials[comb] > max_prob) {
//      max_prob = F.map_potentials[comb];
//      comb_predict = comb;
//    }
//  }
//  int label_predict = comb_predict.begin()->second;
//  return label_predict;
//}
//
///**
// * @brief: predict the label of different evidences
// * it just repeats the function above multiple times, and print the progress at the meantime
// */
//vector<int> Network::PredictUseBruteForce(vector<DiscreteConfig> evidences, int target_node_idx) {
//  int size = evidences.size();
//
//  cout << "Progress indicator: ";
//  int every_1_of_20 = size / 20;
//  int progress = 0;
//
//
//  vector<int> results(size, 0);
//#pragma omp parallel for
//  for (int i = 0; i < size; ++i) {
//#pragma omp critical
//    { ++progress; }
//
//    if (progress % every_1_of_20 == 0) {
//      string progress_percentage = to_string((double)progress/size * 100) + "%...\n";
//      fprintf(stdout, "%s\n", progress_percentage.c_str());
//      fflush(stdout);
//    }
//
//    int pred = PredictUseBruteForce(evidences.at(i), target_node_idx);
//    results.at(i) = pred;
//  }
//  return results;
//}

/**
 * @brief: for approximate inference; this function generate an instance using the network
 * @return an instance
 */
DiscreteConfig Network::GenerateInstanceByProbLogicSampleNetwork() {
  // Probabilistic logic sampling is a method
  // proposed by Max Henrion at 1988: "Propagating uncertainty in Bayesian networks by probabilistic logic sampling" TODO: double-check

  DiscreteConfig instance;
  // Cannot use OpenMP, because must draw samples in the topological ordering.
  // TODO: if we directly use "value", or use index to randomly pick one possible value of "n_p",
  // TODO: then we do not need to use the topological ordering...
  for (const auto &index : this->GetTopoOrd()) { // for each node following the topological ordering
    Node *n_p = FindNodePtrByIndex(index);
    int drawn_value = dynamic_cast<DiscreteNode*>(n_p)->SampleNodeGivenParents(instance); // todo: support continuous nodes
    instance.insert(pair<int,int>(index, drawn_value));
  }
  return instance;
}

/**
 * @brief: get the Markov Blanket of a node
 */
set<int> Network::GetMarkovBlanketIndexesOfNode(Node *node_ptr) {
  // node: set does not contain repeated elements
  set<int> markov_blanket_node_index;

  // Add parents.
  for (auto &par_ptr : GetParentPtrsOfNode(node_ptr->GetNodeIndex())) {
    markov_blanket_node_index.insert(par_ptr->GetNodeIndex());
  }

  // Add children and parents of children.
  for (auto &chil_ptr : GetChildrenPtrsOfNode(node_ptr->GetNodeIndex())) {
    markov_blanket_node_index.insert(chil_ptr->GetNodeIndex());
    for (auto &par_chil_ptr : GetParentPtrsOfNode(chil_ptr->GetNodeIndex())) {
      markov_blanket_node_index.insert(par_chil_ptr->GetNodeIndex());
    }
  }

  markov_blanket_node_index.erase(node_ptr->GetNodeIndex());

  return markov_blanket_node_index;
}

/**
 * @brief: draw multiple instances
 * @param num_samp: the number of instances to draw
 * @param num_burn_in: a terminology in MCMC and Gibbs sampling; the number of instances drawn at the beginning to be ignored
 * @return a set of instances
 */
vector<DiscreteConfig> Network::DrawSamplesByGibbsSamp(int num_samp, int num_burn_in) {

  vector<DiscreteConfig> samples;
  samples.reserve(num_samp);

  // randomly pick one sample
  DiscreteConfig single_sample = this->GenerateInstanceByProbLogicSampleNetwork();

  auto it_idx_node = this->map_idx_node_ptr.begin(); // begin at the first node

  // Need burning in.
//  #pragma omp parallel for
  for (int i = 1; i < num_burn_in + num_samp; ++i) {//draw instances

    Node *node_ptr = (*(it_idx_node++)).second;
    if (it_idx_node == map_idx_node_ptr.end()) {
      it_idx_node = this->map_idx_node_ptr.begin();
    }

    set<int> markov_blanket_node_index = GetMarkovBlanketIndexesOfNode(node_ptr);

    // construct the markov blanket from the picked "single_sample"
    // i.e., filter out the variable-values that are not in the markov blanket
    DiscreteConfig markov_blanket;
    for (auto &p : single_sample) { // for each variable-value of the picked "single_sample"
      // check if the variable is in the Markov Blanket
      if (markov_blanket_node_index.find(p.first) != markov_blanket_node_index.end()) {
        markov_blanket.insert(p);
      }
    }

    // obtain a value of a variable given the Markov Blanket
    int value_index = SampleNodeGivenMarkovBlanketReturnValIndex(node_ptr, markov_blanket);

    // replace the value of the previous instance with the new value
    for (auto p : single_sample) {
      if (p.first == node_ptr->GetNodeIndex()) {
        single_sample.erase(p);
        p.second = dynamic_cast<DiscreteNode*>(node_ptr)->vec_potential_vals.at(value_index);
        single_sample.insert(p);
        break;
      }
    }

    // After burning in, we can store the samples now.
    #pragma omp critical
    { if (i >= num_burn_in) { samples.push_back(single_sample); } }
  }

  return samples;
}

/**
 * @brief: obtain a value index given the Markov Blanket
 * @param node_ptr: target node
 * @param markov_blanket includes (i) direct parents, (ii) direct children and (iii) direct parents of direct children of the target node
 * @return value index of the target node
 */
int Network::SampleNodeGivenMarkovBlanketReturnValIndex(Node *node_ptr, DiscreteConfig markov_blanket) {
  //use the Markov blanket to serve as the elimination order
  //int num_elim_ord = markov_blanket.size();
  vector<int> var_elim_ord;
  var_elim_ord.reserve(markov_blanket.size());
  for (auto &n_v : markov_blanket) {
    var_elim_ord.push_back(n_v.first);
  }

  // TODO: the same problem with "DiscreteNode::SampleNodeGivenParents"
  //obtain the marginal probabilities of the target node
  Factor f = VarElimInferReturnPossib(markov_blanket, node_ptr, var_elim_ord);

  //use the marginal probabilities of the target node for sampling
  vector<int> weights;
  for (int i = 0; i < dynamic_cast<DiscreteNode*>(node_ptr)->GetDomainSize(); ++i) {//for each possible value of the target node
    DiscreteConfig temp;
    temp.insert(pair<int,int>(node_ptr->GetNodeIndex(),
                              dynamic_cast<DiscreteNode*>(node_ptr)->vec_potential_vals.at(i)));
    weights.push_back(f.map_potentials[temp]*10000);//the marginal probability is converted into int
  }

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine rand_gen(seed);
  discrete_distribution<int> this_distribution(weights.begin(),weights.end());
  return this_distribution(rand_gen); // randomly pick an index and return
}

/**
 * @brief: compute F(node, candidate_parents) using dynamic programming
 * used in Ott's algorithm for structure learning
 * the paper chooses minimum score but this program chooses maximum score, this depends on the specific scoring function
 * @param dynamic_program_for_F: map<Node*, map<set<Node*>, double>>, where Node* refers to "node", set<Node*> refers to "candidate_parents"
 * @return pair<double, set<Node*>>: function value, candidate_parents
 */
pair<double, set<Node*>> Network::F(Node *node,
        set<Node*> &candidate_parents,
        Dataset *dts,
        map<Node*, map<set<Node*>, double>> &dynamic_program_for_F) {

  map<set<Node*>, double> &this_node_dynamic_program = dynamic_program_for_F[node];

  // look up the table and return the value
  if (this_node_dynamic_program.find(candidate_parents) != this_node_dynamic_program.end()) {
    return pair<double, set<Node*>>(this_node_dynamic_program[candidate_parents], candidate_parents);
  }

  pair<double, set<Node*>> max_score_parents;
  max_score_parents.first = -DBL_MAX;

  // Look up the dynamic programming table.
  for (auto n : candidate_parents) {
    // recursively compute F of (node, candidate_parents_temp), i.e., F(node, candidate_parents-{n})
    set<Node*> candidate_parents_temp = candidate_parents;
    candidate_parents_temp.erase(n);

    // if F(node, candidate_parents-{n}) has not been computed
    if (this_node_dynamic_program.find(candidate_parents_temp) == this_node_dynamic_program.end()) {
      this_node_dynamic_program[candidate_parents_temp] =
              F(node, candidate_parents_temp, dts, dynamic_program_for_F).first;
    }

    // find the maximum of F(node, candidate_parents-{n})
    if (this_node_dynamic_program[candidate_parents_temp] > max_score_parents.first) {
      max_score_parents.first = this_node_dynamic_program[candidate_parents_temp];
      max_score_parents.second = candidate_parents_temp;
    }
  }

  // if F(node, candidate_parents) has not been computed
  if (this_node_dynamic_program.find(candidate_parents)==this_node_dynamic_program.end()) {

    // construct a network with node and candidate_parents
    DiscreteNode node_copy = *dynamic_cast<DiscreteNode*>(node);
    vector<DiscreteNode> candidate_parents_copy;
    for (auto n : candidate_parents) {
      candidate_parents_copy.push_back(*(DiscreteNode*)(n));
    }

    Network temp_net;
    temp_net.AddNode(&node_copy);
    for (auto n : candidate_parents_copy) {
      temp_net.AddNode(&n);
      temp_net.SetParentChild(&n, &node_copy);
    }
    temp_net.GenDiscParCombsForAllNodes();

    // compute the scoring function s(node, candidate_parents)
    ScoreFunction sf(&temp_net, dts);
    this_node_dynamic_program[candidate_parents] = sf.ScoreForNode(&node_copy,"log BDeu");

  }

  // find the maximum and get F(node, candidate_parents)
  if (this_node_dynamic_program[candidate_parents] > max_score_parents.first) {
    max_score_parents.first = this_node_dynamic_program[candidate_parents];
    max_score_parents.second = candidate_parents;
  }

  return max_score_parents;
}

/*!
 * @brief: compute Q^{set_nodes}(M(set_nodes)) = Q^{set_nodes}(topo_ord)
 * @param topo_ord = M(set_nodes)
 * @param dynamic_program_for_Q: map<pair<set<Node*>, vector<int>>,   pair<double, vector<pair<Node*, set<Node*>>>>>
 *              1. set<Node*>:      set_nodes
 *              2. vector<int>>:    topo_ord
 *              3. double:          function value
 *              4. Node*:           node for F
 *              5. set<Node*>:      candidate_parents for F
 * @return pair<double, vector<pair<Node*, set<Node*>>>>: function value, (one node in order, set_nodes)?
 */
pair<double, vector<pair<Node*, set<Node*>>>>
 Network::Q(set<Node*> &set_nodes,
         vector<int> topo_ord,
         Dataset *dts,
         map<Node*,   map<set<Node*>, double>> &dynamic_program_for_F,
         map<pair<set<Node*>, vector<int>>,   pair<double, vector<pair<Node*, set<Node*>>>>> dynamic_program_for_Q) {

  // look up the table and return the value
  pair<set<Node*>, vector<int>> to_find(set_nodes, topo_ord);
  if (dynamic_program_for_Q.find(to_find)!=dynamic_program_for_Q.end()) {
    return dynamic_program_for_Q[to_find];
  }

  double score = 0;
  vector<pair<Node*, set<Node*>>> vec_node_parents;

  // accumulate Q^{set_nodes}(topo_ord)
  // Q^{set_nodes}(topo_ord) = sum of F(n, pre nodes of n according to topo_ord) over n
  for (auto &n : set_nodes) { // for each node n
    set<Node*> candidate_parents;
    // insert all the pre nodes of n according to the topological ordering "topo_ord"
    for (const auto &i : topo_ord) {
      if (n->GetNodeIndex() == i) {
        break;
       }
      candidate_parents.insert(this->FindNodePtrByIndex(i));
    }
    pair<double, set<Node*>> best_score_parents = F(n, candidate_parents, dts, dynamic_program_for_F);

    score += best_score_parents.first;
    vec_node_parents.push_back(pair<Node*, set<Node*>>(n, best_score_parents.second));
  }

  return pair<double, vector<pair<Node*, set<Node*>>>>(score, vec_node_parents);
}

/*!
 * @brief: compute M(set_nodes) using dynamic programming
 * used in Ott's algorithm for structure learning
 * @param set_nodes: all node ptrs of the network
 * @return M(set_nodes), which is a subset of all nodes, # also equals to |set_nodes|
 */
vector<int> Network::M(set<Node*> &set_nodes,
        Dataset *dts,
        map<Node*, map<set<Node*>, double>> &dynamic_program_for_F,
        map<pair<set<Node*>, vector<int>>,   pair<double, vector<pair<Node*, set<Node*>>>>> dynamic_program_for_Q,
        map<set<Node*>, vector<int>> dynamic_program_for_M) {

  if (set_nodes.empty()) { // M(∅) = ∅
    return vector<int> {};
  }

  // look up the table and return the value
  if (dynamic_program_for_M.find(set_nodes) != dynamic_program_for_M.end()) {
    return dynamic_program_for_M[set_nodes];
  }

  Node *g_star = nullptr;
  double score = -DBL_MAX;
  for (auto n : set_nodes) {
    // recursively compute M of set_nodes_temp, i.e., M(set_nodes-{n})
    set<Node*> set_nodes_temp = set_nodes;
    set_nodes_temp.erase(n);
    vector<int> m_of_set_nodes_temp = M(set_nodes_temp, dts,
                                        dynamic_program_for_F, dynamic_program_for_Q,
                                        dynamic_program_for_M);

    // compute F + Q TODO: double-check
    double score_temp = F(n, set_nodes_temp, dts, dynamic_program_for_F).first
                        + Q(set_nodes_temp, m_of_set_nodes_temp, dts, dynamic_program_for_F, dynamic_program_for_Q).first;

    // find the maximum.
    if (score_temp > score) {
      score = score_temp;
      g_star = n; // get the g*
    }
  }

  // compute M(set_nodes) and return
  set<Node*> set_nodes_remove_g_star = set_nodes;
  set_nodes_remove_g_star.erase(g_star);
  // for 1 <= i < |set_nodes|, M(set_nodes)[i] = M(set_nodes - {g*})[i]
  vector<int> result = M(set_nodes_remove_g_star, dts, dynamic_program_for_F, dynamic_program_for_Q, dynamic_program_for_M);
  // M(set_nodes)[|set_nodes|] = g*
  result.push_back(g_star->GetNodeIndex());

  return result;

}

/**
 * @brief: learn the structure with Ott's algorithm.
     * Ott, Sascha, Seiya Imoto, and Satoru Miyano. "Finding optimal models for small gene networks."
     * In Biocomputing 2004, pp. 557-567. 2003.
 */
void Network::StructLearnByOtt(Dataset *dts, vector<int> topo_ord_constraint) {//TODO: double check correctness

  map<Node*, map<set<Node*>, double>> dynamic_program_for_F;
  map< pair<set<Node*>, vector<int>>,  pair<double, vector<pair<Node*, set<Node*>>> > > dynamic_program_for_Q;

  set<Node*> set_node_ptr_container;
  for (auto id_np : map_idx_node_ptr) {
    set_node_ptr_container.insert(id_np.second); // insert all node ptrs of the network
  }

  // if "topo_ord_constraint" = "best"; no order is provided (i.e. no constraint)
  if (topo_ord_constraint.empty() || topo_ord_constraint.size() != num_nodes) {
    map<set<Node*>, vector<int>> dynamic_program_for_M;
    vector<int> m_of_all_nodes = M(set_node_ptr_container, dts, dynamic_program_for_F, dynamic_program_for_Q,
                                   dynamic_program_for_M);
    topo_ord_constraint = m_of_all_nodes; // provide a order constraint
  }

  pair<double, vector<pair<Node*, set<Node*>>>> score_vec_node_parents = Q(set_node_ptr_container, topo_ord_constraint, dts, dynamic_program_for_F, dynamic_program_for_Q);
  vector<pair<Node*, set<Node*>>> vec_node_parents = score_vec_node_parents.second;

  cout << "==================================================" << '\n'
       << "Setting children and parents......" << endl;
  for (auto p : vec_node_parents) {
    Node *chi = p.first;
    for (auto par : p.second) {
      SetParentChild(par, chi);
    }
  }

  cout << "==================================================" << '\n'
       << "Generating parents combinations for each node......" << endl;
  GenDiscParCombsForAllNodes();
}

/**
 * @brief: learning network structure using Weka's algorithm
 * https://github.com/Waikato/weka-3.8
 *      under: weka/src/main/java/weka/classifiers/bayes/net/search/global/K2.java
 *      or     weka/src/main/java/weka/classifiers/bayes/net/search/local/K2.java
 *
 * @key idea: the key idea is to add a parent to a node based on topo_ord_constraint,
 *            if adding the node as a parent results in increment of the score
 *            -- there are different scoring functions.
 */
void Network::StructLearnLikeK2Weka(Dataset *dts, vector<int> topo_ord_constraint, int max_num_parents) { //checked
  // todo: test the correctness
  // if "topo_ord_constraint" = "best"; no order is provided (i.e. no constraint)
  if (topo_ord_constraint.empty() || topo_ord_constraint.size() != num_nodes) {
    topo_ord_constraint.reserve(num_nodes);
    for (int i = 0; i < num_nodes; ++i) {
      topo_ord_constraint.push_back(i); // provide a order constraint: 1, 2, ..., num_nodes-1
    }
  }
  GenDiscParCombsForAllNodes(); // generate all possible parent configurations ("set_discrete_parents_combinations") for all nodes

//#pragma omp parallel for
  for (int i = 0; i < num_nodes; ++i) {

    int var_index = topo_ord_constraint.at(i);
    DiscreteNode *node = (DiscreteNode*) this->map_idx_node_ptr.at(var_index); // TODO: function "FindNodePtrByIndex"？

    bool ok_to_proceed = (node->GetNumParents() < max_num_parents);
    while (ok_to_proceed) {
      int best_par_index = -1;
      double best_extra_score = 0;

      for (int j = 0; j < i; ++j) { // j < i: traverse all pre nodes, satisfy the ordering constraint
        int par_index = topo_ord_constraint.at(j);

        // TODO: this function calculates old score and new score and returns the delta score as "extra_score"
        // TODO: why not
        // TODO:      1. directly compute the new score and compare the i scores to find the maximum, rather than compare the delta scores
        // TODO:      2. compute the old score before calling this function, because the i old scores seem to be the same
        double extra_score = CalcuExtraScoreWithModifiedArc(par_index, var_index, dts, "add", "log K2");//use K2 as scoring function
        if (extra_score > best_extra_score) { // find the max g() as the "best_extra_score" iteratively
          // TODO: may not need to addarc and deletearc
          // TODO: because if "AddArc" function returns false, then current "extra_score" equals to 0 and cannot be the "best_extra_score"
          if (this->AddArc(par_index, var_index)) {
            best_par_index = j;
            best_extra_score = extra_score;
            this->DeleteArc(par_index, var_index);
          }
        }
      }
      // if the maximum new score is less than or equal to the old score, stop
      if (best_par_index == -1) {
        ok_to_proceed = false;
      }
      // if the maximum new score is better than the old score, add this arc and continue
      else {
        this->AddArc(best_par_index, var_index);
        ok_to_proceed = (node->GetNumParents() < max_num_parents);
      }
    }

  }
}

#pragma clang diagnostic pop