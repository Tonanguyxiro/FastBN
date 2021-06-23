//
// Created by Linjian Li on 2018/12/5.
//

#ifndef BAYESIANNETWORK_GADGET_H
#define BAYESIANNETWORK_GADGET_H

#include <set>
#include <utility>
#include <string>
#include <queue>
#include <vector>
#include <map>
#include <iostream>
#include <cmath>
#include <cfloat>

using namespace std;
typedef set< pair<int, int> > DiscreteConfig;//[variable id, variable value]

/**
 * @brief: to support both discrete and continuous variables
 * a value whose type is int or float
 */
struct Value {
 private:
  int int_ = INT32_MIN;     //for discrete variable
  float float_ = -FLT_MAX;  //for continuous variable
  bool use_int = true;
 public:
  bool UseInt() const { return use_int; }
  void SetInt(int i) { int_ = i; float_ = -FLT_MAX; use_int = true; }
  void SetFloat(float f) { float_ = f; int_ = INT32_MIN; use_int = false; }
  int GetInt() const { if (!use_int) exit(1); return int_; }
  float GetFloat() const { if (use_int) exit(1); return float_; }

  //define the "<" operator; this operator is needed for set/map.
  bool operator <(const Value v) const {
    if (this->UseInt() && v.UseInt()) {
      return this->GetInt() < v.GetInt();
    } else if (this->UseInt() && !v.UseInt()) {
      return this->GetInt() < v.GetFloat();
    } else if (!this->UseInt() && v.UseInt()) {
      return this->GetFloat() < v.GetInt();
    } else {
      return this->GetFloat() < v.GetFloat();
    }
  }

};

// TODO: merge?
typedef pair<int, Value> VarVal; //variable/node id, value of the variable/node
typedef pair<int, int> DiscVarVal;//similar to VarVal, but for discrete varaible

/** this configuration is for both discrete and continuous.
 *  the set contains the observed values of each variable.
 * **/
typedef set<VarVal> Configuration;

/**
 * @brief: generate all the combinations given the set of parents
 */
template <typename T>
set<set<T>> GenAllCombinationsFromSets(set<set<T>> *set_of_sets) {//checked 11/Apr/2020
  if (set_of_sets->empty()) {
    return set<set<T>>(); //this func uses recursion, because the number of sets is unknown sometimes.
  }

  auto its = set_of_sets->begin(); // get the set of all possible values of the first parent
  set<T> to_be_inserted = *its;
  set<set<T>> result, temp_result;

  // Base Case
  if (set_of_sets->size()==1) {
    for (T p : to_be_inserted){ // for each possible value of the parent
      set<T> c;
      c.insert(p);
      result.insert(c);
    }
    return result;
  }

  // Recursive Case (the size of set_of_sets is greater than 1)
  set_of_sets->erase(its);
  temp_result = GenAllCombinationsFromSets(set_of_sets); // get the result except for the first parent
  for (T p : to_be_inserted){ // for each possible value of the first parent
    for (set<T> c : temp_result) { // for each of the current parent combinations
      c.insert(p);
      result.insert(c);
    }
  }
  return result;
};

/**
 * @brief: generate a power set (i.e., all the subsets of a given set) for a given set;
 */
template <typename T> set<set<T>> GenPowerSet(set<T> &src_set) {//TODO: need to double check correctness
  // Avoid recursion.
  set<set<T>> power_set;
  set<T> empty;
  power_set.insert(empty);
  for (T elem : src_set) {
    set<set<T>> power_set_temp = power_set;
    for (set<T> subset : power_set_temp) {
      subset.insert(elem);
      power_set.insert(subset);
    }
  }
  return power_set;
};

int ArgMax(map<int, double> &x);
map<int, double> Normalize(map<int, double> &x);

DiscreteConfig ArrayToDiscreteConfig(int *nodes_indexes, int *observations, int num_of_observations);

map<int, int> DiscreteConfigToMap(DiscreteConfig &disc_cfg);

set<DiscreteConfig> ExpandConfgFromTwoConfgs(const set<DiscreteConfig> *one, const set<DiscreteConfig> *two);
bool FirstIsSubsetOfSecond(const DiscreteConfig *first, const DiscreteConfig *second);
bool FirstCompatibleSecond(const DiscreteConfig *, const DiscreteConfig *);
bool Conflict(const DiscreteConfig *cfg1, const DiscreteConfig *cfg2);
bool OccurInCorrectOrder(int a, int b, vector<int> vec);
bool DAGObeyOrdering(int **graph, int num_nodes, vector<int> ord);
bool DirectedGraphContainsCircleByBFS(int **graph, int num_nodes);
int* BreadthFirstTraversalWithAdjacencyMatrix(int **graph, int num_nodes, int start);
vector<int> TopoSortOfDAGZeroInDegreeFirst(int **graph, int num_nodes);

string TrimRight(string);
string TrimLeft(string);
string Trim(string &);
vector<string> Split(string &s, string delimiter);

int FactorialForSmallInteger(int);
double LogOfFactorial(int);

vector<vector<int>> NaryCount(vector<int> vec_range_each_digit);
vector<int> TheNthNaryCount(vector<int> vec_range_each_digit, int n);

#endif //BAYESIANNETWORK_GADGET_H
