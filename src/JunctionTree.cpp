#include "JunctionTree.h"

JunctionTree::JunctionTree(Network *net, Dataset *dts, bool is_dense) : Inference(net, dts, is_dense) {
    cout << "begin construction function of JunctionTree..." << endl;

    Timer *timer = new Timer();
    // record time
    timer->Start("construct jt");

    tree = new JunctionTreeStructure(net);

    // Arbitrarily select a clique as the root.
    // TODO: find a better root that generate a more balanced tree structure
    auto iter = tree->vector_clique_ptr_container.begin();
    arb_root = *iter;

    MarkLevel();
    cout << "finish MarkLevel" << endl;

    ReorganizeTableStorage(num_instances);
    cout << "finish reorganizing table storage" << endl;

    BackUpJunctionTree();
    cout << "finish BackUpJunctionTree" << endl;

    timer->Stop("construct jt");
    cout << "==================================================";
    cout << endl; timer->Print("construct jt"); cout << endl;
    SAFE_DELETE(timer);
}

JunctionTree::~JunctionTree() {
    SAFE_DELETE(tree);
    SAFE_DELETE_ARRAY(clique_backup);
    SAFE_DELETE_ARRAY(separator_backup);
}

/**
 * @brief: test the Junction Tree given a data set
 */
double JunctionTree::EvaluateAccuracy(string path, int num_threads) {

    cout << "==================================================" << '\n'
         << "Begin testing the trained network." << endl;

    Timer *timer = new Timer();
    // record time
    timer->Start("jt");

    // predict the labels of the test instances
    vector<int> predictions = PredictUseJTInfer(num_threads, timer);

    double accuracy = Accuracy(predictions);

    timer->Stop("jt");
    setlocale(LC_NUMERIC, "");

    cout << '\n' << "Accuracy: " << accuracy << endl;
    cout << "==================================================" << endl;
    timer->Print("jt"); cout << "removing reset: " << timer->time["jt"] - timer->time["reset"] << " s";
    double total = timer->time["jt"] - timer->time["reset"] - timer->time["norm"]; cout << " removing norm: " << total << " s"<< endl;

    timer->Print("load evidence"); cout << "(" << timer->time["load evidence"] / timer->time["jt"] * 100 << "%) ";
    timer->Print("msg passing"); cout << "(" << timer->time["msg passing"] / timer->time["jt"] * 100 << "%) ";
    timer->Print("reset"); cout << "(" << timer->time["reset"] / timer->time["jt"] * 100 << "%) ";
    timer->Print("predict"); cout << "(" << timer->time["predict"] / timer->time["jt"] * 100 << "%)" << endl;

    timer->Print("upstream"); timer->Print("downstream"); cout << endl << endl;

    timer->Print("pre-evi"); timer->Print("main-evi"); timer->Print("post-evi"); cout << endl;
    timer->Print("pre-sep"); timer->Print("main-sep"); timer->Print("post-sep"); cout << endl;
    timer->Print("pre-clq"); timer->Print("main-clq"); timer->Print("post-clq"); cout << endl;

    timer->Print("norm"); cout << endl << endl;

    timer->Print("parallel");
    cout << "(" << timer->time["parallel"] / (timer->time["jt"] - timer->time["norm"])* 100 << "%)";
    cout << "(" << timer->time["parallel"] / total * 100 << "%)" << endl;

    SAFE_DELETE(timer);

    return accuracy;
}

/**
 * @brief: do level traverse of the tree, at the same time:
 *      1 add all the cliques & separators in "nodes_by_level" by level
 *      2 add all the separators in "seps_by_level" by level
 *      3 mark both "ptr_upstream_clique" and "ptr_downstream_cliques"
 */
void JunctionTree::MarkLevel() {
    vector<Clique*> vec; // a set of cliques/seps in one level
    vec.push_back(arb_root); // push the root into vec
    nodes_by_level.push_back(vec); // the first level only has the root clique

    while (!vec.empty()) {
        vector<Clique*> vec2;
        vector<Separator*> vsep2;
        /*
         * think of nodes...:       clq, sep, clq, sep, ... clq
         * size of node by levels:  1,   2,   3,   4,   ...
         * if nodes.size % 2 == 0, "vec" is sep, then vec's downstream neighbors are clqs
         */
        bool is_sep = nodes_by_level.size() % 2 == 0 ? false : true;

        for (int i = 0; i < vec.size(); ++i) { // for each clique/sep in the current level
            Clique *clique = vec[i];
            for (auto &ptr_neighbor : clique->set_neighbours_ptr) {
                // all neighbor cliques of "clique" contain the upstream clique and downstream clique(s)
                // if the current neighbor "ptr_separator" is the upstream clique, do nothing
                if (ptr_neighbor == clique->ptr_upstream_clique) {
                    continue;
                }
                // the current neighbor "ptr_separator" is a downstream clique of "clique"
                clique->ptr_downstream_cliques.push_back(ptr_neighbor);
                ptr_neighbor->ptr_upstream_clique = clique;  // Let the callee know the caller.
                vec2.push_back(ptr_neighbor);
                if (is_sep) { // cast and push to the separator vector
                    Separator* ptr_sep = dynamic_cast<Separator*>(ptr_neighbor);
                    vsep2.push_back(ptr_sep);
                }
            }
        }

        nodes_by_level.push_back(vec2);
        if (is_sep) {
            separators_by_level.push_back(vsep2);
        }
        vec = vec2;
    }

    nodes_by_level.pop_back();
    separators_by_level.pop_back();
    max_level = nodes_by_level.size();
}

/**
 * @brief: re-organize table storage before message passing, by constraining the order of variables
 *      for each non-root clique C, it has an upstreaming separator S,
 *      the variables in the higher order are the variables not in S,
 *      the variables in the lower order are the variables in S
 * the computations of marginalization in collection and extension in distribution can be simplified
 * by reducing the computation of finding the index mappings between the involved tables
 */
void JunctionTree::ReorganizeTableStorage(int num_threads) {
    for (int i = 0; i < tree->vector_clique_ptr_container.size(); ++i) { // for each clique in the junction tree
        Clique *clique = tree->vector_clique_ptr_container[i];

        if (!clique->ptr_upstream_clique) { // skip the root clique
            continue;
        }

        bool need_reorganize = false;
        Clique *separator = clique->ptr_upstream_clique;

        for (int j = 0; j < separator->p_table.num_variables; ++j) {
            if (clique->p_table.vec_related_variables[clique->p_table.num_variables - j - 1] !=
                separator->p_table.vec_related_variables[separator->p_table.num_variables - j - 1]) {
                need_reorganize = true;
                break;
            }
        }
        if (!need_reorganize) { // skip the clique that is in the right order
            continue;
        }

        /**
         * do table re-organization
         */
        int *config1 = new int[clique->p_table.num_variables];
        int *config2 = new int[clique->p_table.num_variables];
        int *table_index = new int[clique->p_table.table_size];

        PotentialTable new_table;
        vector<int> locations;
        clique->p_table.TableReorganizationPre(separator->p_table.vec_related_variables, new_table, locations);

        // the main loop
        for (int k = 0; k < new_table.table_size; ++k) {
            table_index[k] = new_table.TableReorganizationMain(k, config1, config2, clique->p_table, locations);
        }
        new_table.TableReorganizationPost(clique->p_table, table_index);

        clique->p_table = new_table;

        SAFE_DELETE_ARRAY(config1);
        SAFE_DELETE_ARRAY(config2);
        SAFE_DELETE_ARRAY(table_index);
    }
}

/**
 * backup & reset:
 * The inference process will modify the junction tree itself.
 * So, we need to backup the tree and restore it after an inference.
 * Otherwise, we need to re-construct the tree each time we want to make inference.
 */
void JunctionTree::BackUpJunctionTree() {
    clique_backup = new Clique[tree->vector_clique_ptr_container.size()];
    separator_backup = new Separator[tree->vector_separator_ptr_container.size()];
    int i = 0;
    for (const auto &c : tree->vector_clique_ptr_container) {
        clique_backup[i++] = *c;
    }
    i = 0;
    for (const auto &s : tree->vector_separator_ptr_container) {
        separator_backup[i++] = *s;
    }
}

void JunctionTree::ResetJunctionTree() {
    int i = 0;
    for (auto &c : tree->vector_clique_ptr_container) {
        *c = clique_backup[i++];
    }
    i = 0;
    for (auto &s : tree->vector_separator_ptr_container) {
        *s = separator_backup[i++];
    }
}

/**
 * @brief: when inferring, an evidence is given. The evidence needs to be loaded and propagate in the network.
 */
void JunctionTree::LoadDiscreteEvidence(const DiscreteConfig &E, int num_threads, Timer *timer) {
    /**
     * version 1: use TableReduction method
     * including both nested and flattened version
     */
    for (auto &e: E) { // for each observation of variable
        timer->Start("pre-evi");
        // we need the index of the the evidence and the value index
        int index = e.first;
        int value = e.second;
        if (index >= network->num_nodes) {
            cout << "!!!!" << endl;
            // todo: this is because the testing set has more features than the training set
            //  for this case, we just ignore such evidence in the current implementation
            continue;
        }

//        /**
//         * 1. nested version, without flattening
//         */
//        for (auto &c: tree->vector_clique_ptr_container) {
//            if (c->p_table.vec_related_variables.find(index) != c->p_table.vec_related_variables.end()) {
//                c->p_table.TableReduction(index, value, num_threads);
//            }
//        }
//        for (auto &c: tree->vector_separator_ptr_container) {
//            if (c->p_table.vec_related_variables.find(index) != c->p_table.vec_related_variables.end()) {
//                c->p_table.TableReduction(index, value, num_threads);
//            }
//        }
//        timer->Stop("pre-evi");

        /**
         * 2. flatten version
         */
        vector<Clique*> vector_reduced_clique_and_separator_ptr;
        for (auto &c: tree->vector_clique_ptr_container) {
            for (int i = 0; i < c->p_table.num_variables; ++i) { // for each related variable
                if (c->p_table.vec_related_variables[i] == index) {
                    vector_reduced_clique_and_separator_ptr.push_back(c);
                    break;
                }
            }
        }
        for (auto &c: tree->vector_separator_ptr_container) {
            for (int i = 0; i < c->p_table.num_variables; ++i) { // for each related variable
                if (c->p_table.vec_related_variables[i] == index) {
                    vector_reduced_clique_and_separator_ptr.push_back(c);
                    break;
                }
            }
        }
        timer->Stop("pre-evi");

        LoadEvidenceToNodes(vector_reduced_clique_and_separator_ptr, index, value, num_threads, timer);
    }

//    /**
//     * version 2: use TableReduction v2 method
//     * TODO: create the flattened version
//     */
//    for (auto &c: tree->vector_clique_ptr_container) {
//        c->p_table.TableReduction(E, num_threads);
//    }
//    for (auto &c: tree->vector_separator_ptr_container) {
//        c->p_table.TableReduction(E, num_threads);
//    }
}


void JunctionTree::LoadEvidenceToNodes(vector<Clique*> &vector_reduced_node_ptr,
                                       int index, int value_index, int num_threads, Timer *timer) {

    timer->Start("pre-evi");
    int red_size = vector_reduced_node_ptr.size();

    int *e_loc = new int[red_size];
    int **full_config = new int*[red_size];
    int **v_index = new int*[red_size];

    int *cum_sum = new int[red_size];
    int final_sum = 0;

    /**
     * pre-computing
     */
    for (int k = 0; k < red_size; ++k) {
        auto clique_ptr = vector_reduced_node_ptr[k];

        // update sum
        cum_sum[k] = final_sum;
        final_sum += clique_ptr->p_table.table_size;
    }

    timer->Start("parallel");
    omp_set_num_threads(num_threads);
#pragma omp parallel for
    for (int k = 0; k < red_size; ++k) {
        auto clique_ptr = vector_reduced_node_ptr[k];
        full_config[k] = new int[clique_ptr->p_table.table_size * clique_ptr->p_table.num_variables];
        v_index[k] = new int[clique_ptr->p_table.table_size];

        e_loc[k] = clique_ptr->p_table.TableReductionPre(index);
    }

    timer->Stop("pre-evi");

    timer->Start("main-evi");
    // the main loop
    omp_set_num_threads(num_threads);
#pragma omp parallel for
    for (int s = 0; s < final_sum; ++s) {
        // compute k and i
        int k, i;
        Compute2DIndex(k, i, s, red_size, cum_sum);

        v_index[k][i] = vector_reduced_node_ptr[k]->p_table.TableReductionMain(i, full_config[k], e_loc[k]);
    }
    timer->Stop("main-evi");

    timer->Start("post-evi");
    /**
     * post-computing
     */
    omp_set_num_threads(num_threads);
#pragma omp parallel for
    for (int k = 0; k < red_size; ++k) {
        auto clique_ptr = vector_reduced_node_ptr[k];
        clique_ptr->p_table.TableReductionPost(index, value_index, v_index[k], e_loc[k]);

        SAFE_DELETE_ARRAY(full_config[k]);
        SAFE_DELETE_ARRAY(v_index[k]);
    }
    timer->Stop("parallel");

    SAFE_DELETE_ARRAY(e_loc);
    SAFE_DELETE_ARRAY(full_config);
    SAFE_DELETE_ARRAY(v_index);
    SAFE_DELETE_ARRAY(cum_sum);
    timer->Stop("post-evi");
}

/**
 *
 * Message passing is just COLLECT and DISTRIBUTE (these two words is used by paper and text book).
 * The order between COLLECT and DISTRIBUTE does not matter, but they must not interleave.
 * After message passing, any clique (junction tree node) contains the right distribution of the related variables.
 */
void JunctionTree::MessagePassingUpdateJT(int num_threads, Timer *timer) {

    /**
     * 1. omp task
     */
//    timer->Start("upstream");
//#pragma omp parallel num_threads(num_threads)
//    {
//#pragma omp single
//        {
//            arb_root->Collect2();
//        }
//    }
//    timer->Stop("upstream");
//
//    timer->Start("downstream");
//#pragma omp parallel num_threads(num_threads)
//    {
//#pragma omp single
//        {
//            arb_root->Distribute2();
//        }
//    }
//    timer->Stop("downstream");

    /**
     * 2. omp parallel for
     */
    timer->Start("upstream");
//    arb_root->Collect3(nodes_by_level, max_level, num_threads, timer);
    Collect(num_threads, timer);
    timer->Stop("upstream");

    timer->Start("downstream");
//    arb_root->Distribute3(nodes_by_level, max_level, num_threads);
    Distribute(num_threads, timer);
    timer->Stop("downstream");
}

/**
 * operations for separators, including marginalization and division
 * @param i  corresponds to level
 *
 * the only difference between collection and distribution is:
 *      collection finds each separator and its child, marginalizes from child to it
 *      distribution finds each separator and its parent, marginalizes from parent to it
 */
void JunctionTree::SeparatorLevelCollection(int i, int num_threads, Timer *timer) {
    timer->Start("pre-sep");
    int size = separators_by_level[i/2].size();
    // used to store the (clique) potential tables that are needed to be marginalized
    vector<PotentialTable> tmp_pt;
    tmp_pt.reserve(size);

    // used to store the (clique) potential tables that are used to be divided
    // it is different from "tmp_pt" because not all the tables are needed to be marginalized
    vector<PotentialTable> div_pt;
    div_pt.resize(size);

    // store number_variables and cum_levels of the original table
    // rather than storing the whole potential table
    int *nv_old = new int[size];
    vector<vector<int>> cl_old;
    cl_old.reserve(size);

    int *cum_sum = new int[size];
    int final_sum = 0;
    int sum_index = 0;

    // not all tables need to do the marginalization
    // there are "size" tables in total
    // use a vector to show which tables need to do the marginalization
    vector<int> vector_marginalization;
    vector_marginalization.reserve(size);

    // set of arrays, showing the locations of the variables of the new table in the old table
    int **loc_in_old = new int*[size];
    int **full_config = new int*[size];
    int **partial_config = new int*[size];
    int **table_index = new int*[size];

    /**
     * pre computing
     */
    for (int j = 0; j < size; ++j) { // for each separator in this level
        auto separator = separators_by_level[i/2][j];
        Clique *clique = separator->ptr_downstream_cliques[0]; // there is only one child for each separator

        div_pt[j] = clique->p_table;

        // store the old table before marginalization, used for division
        separator->old_ptable = separator->p_table;

        if (clique->p_table.num_variables - separator->p_table.num_variables != 0) {
            // record the index (that requires to do the marginalization)
            vector_marginalization.push_back(j);
            nv_old[sum_index] = clique->p_table.num_variables;
            cl_old.push_back(clique->p_table.cum_levels);

            PotentialTable pt;
            pt.TableMarginalizationPre(separator->p_table.vec_related_variables, separator->old_ptable.var_dims);

            // get this table's location -- it is currently the last one
            int last = vector_marginalization.size() - 1;
            // generate an array showing the locations of the variables of the new table in the old table
            loc_in_old[last] = new int[separator->p_table.num_variables];
            for (int k = 0; k < separator->p_table.num_variables; ++k) {
                loc_in_old[last][k] = clique->p_table.TableReductionPre(pt.vec_related_variables[k]);
            }
            table_index[last] = new int[clique->p_table.table_size];

            tmp_pt.push_back(pt);

            // malloc in pre-, not to parallelize
            full_config[last] = new int[clique->p_table.table_size * clique->p_table.num_variables];
            partial_config[last] = new int[clique->p_table.table_size * separator->p_table.num_variables];

            // update sum
            cum_sum[sum_index++] = final_sum;
            final_sum += clique->p_table.table_size;
        }
    }
    timer->Stop("pre-sep");

    timer->Start("main-sep");
    int size_m = vector_marginalization.size(); // the number of variables to be marginalized

    timer->Start("parallel");
    // the main loop
    omp_set_num_threads(num_threads);
#pragma omp parallel for
    for (int s = 0; s < final_sum; ++s) {
        int j, k;
        Compute2DIndex(j, k, s, size_m, cum_sum); // compute j and k
        table_index[j][k] = tmp_pt[j].TableMarginalizationMain(k, full_config[j], partial_config[j],
                                                               nv_old[j], cl_old[j], loc_in_old[j]);
    }
    timer->Stop("parallel");
    timer->Stop("main-sep");

    timer->Start("post-sep");
    // post-computing
    int *cum_sum2 = new int[size];
    int final_sum2 = 0;

    int l = 0;
    for (int j = 0; j < size; ++j) {
        auto separator = separators_by_level[i/2][j];
        Clique *clique = separator->ptr_downstream_cliques[0]; // there is only one child for each separator

        if (l < size_m && j == vector_marginalization[l]) { // index j have done the marginalization
            tmp_pt[l].TableMarginalizationPost(clique->p_table, table_index[l]);
            div_pt[j] = tmp_pt[l];
            l++;
        }

        cum_sum2[j] = final_sum2;
        final_sum2 += div_pt[j].table_size;
    }

    for (int l = 0; l < size_m; ++l) {
        SAFE_DELETE_ARRAY(loc_in_old[l]);
        SAFE_DELETE_ARRAY(full_config[l]);
        SAFE_DELETE_ARRAY(partial_config[l]);
        SAFE_DELETE_ARRAY(table_index[l]);
    }
    SAFE_DELETE_ARRAY(loc_in_old);
    SAFE_DELETE_ARRAY(full_config);
    SAFE_DELETE_ARRAY(partial_config);
    SAFE_DELETE_ARRAY(table_index);
    SAFE_DELETE_ARRAY(cum_sum);
    SAFE_DELETE_ARRAY(nv_old);

    timer->Start("parallel");
    omp_set_num_threads(num_threads);
#pragma omp parallel for
    for (int s = 0; s < final_sum2; ++s) {
        int j, k;
        Compute2DIndex(j, k, s, size, cum_sum2); // compute j and k

        if (separators_by_level[i/2][j]->old_ptable.potentials[k] == 0) {
            separators_by_level[i/2][j]->p_table.potentials[k] = 0;
        } else {
            separators_by_level[i/2][j]->p_table.potentials[k] = div_pt[j].potentials[k] / separators_by_level[i/2][j]->old_ptable.potentials[k];
        }
    }

    timer->Stop("parallel");
    SAFE_DELETE_ARRAY(cum_sum2);
    timer->Stop("post-sep");
}

void JunctionTree::SeparatorLevelDistribution(int i, int num_threads, Timer *timer) {
    timer->Start("pre-sep");
    int size = separators_by_level[i/2].size();
    // used to store the (clique) potential tables that are needed to be marginalized
    vector<PotentialTable> tmp_pt;
    tmp_pt.reserve(size);

    // used to store the (clique) potential tables that are used to be divided
    // it is different from "tmp_pt" because not all the tables are needed to be marginalized
    vector<PotentialTable> div_pt;
    div_pt.resize(size);

    // store number_variables and cum_levels of the original table
    // rather than storing the whole potential table
    int *nv_old = new int[size];
    vector<vector<int>> cl_old;
    cl_old.reserve(size);

    int *cum_sum = new int[size];
    int final_sum = 0;
    int sum_index = 0;

    // not all tables need to do the marginalization
    // there are "size" tables in total
    // use a vector to show which tables need to do the marginalization
    vector<int> vector_marginalization;
    vector_marginalization.reserve(size);

    // set of arrays, showing the locations of the variables of the new table in the old table
    int **loc_in_old = new int*[size];
    int **full_config = new int*[size];
    int **partial_config = new int*[size];
    int **table_index = new int*[size];

    /**
     * pre computing
     */
    for (int j = 0; j < size; ++j) { // for each separator in this level
        auto separator = separators_by_level[i/2][j];
        Clique *clique = separator->ptr_upstream_clique;

        div_pt[j] = clique->p_table;

        // store the old table before marginalization, used for division
        separator->old_ptable = separator->p_table;

        if (clique->p_table.num_variables - separator->p_table.num_variables != 0) {
            // record the index (that requires to do the marginalization)
            vector_marginalization.push_back(j);
            nv_old[sum_index] = clique->p_table.num_variables;
            cl_old.push_back(clique->p_table.cum_levels);

            PotentialTable pt;
            pt.TableMarginalizationPre(separator->p_table.vec_related_variables, separator->old_ptable.var_dims);

            // get this table's location -- it is currently the last one
            int last = vector_marginalization.size() - 1;
            // generate an array showing the locations of the variables of the new table in the old table
            loc_in_old[last] = new int[separator->p_table.num_variables];
            for (int k = 0; k < separator->p_table.num_variables; ++k) {
                loc_in_old[last][k] = clique->p_table.TableReductionPre(pt.vec_related_variables[k]);
            }
            table_index[last] = new int[clique->p_table.table_size];

            tmp_pt.push_back(pt);

            // malloc in pre-, not to parallelize
            full_config[last] = new int[clique->p_table.table_size * clique->p_table.num_variables];
            partial_config[last] = new int[clique->p_table.table_size * separator->p_table.num_variables];

            // update sum
            cum_sum[sum_index++] = final_sum;
            final_sum += clique->p_table.table_size;
        }
    }
    timer->Stop("pre-sep");

    timer->Start("main-sep");
    int size_m = vector_marginalization.size(); // the number of variables to be marginalized

    timer->Start("parallel");
    // the main loop
    omp_set_num_threads(num_threads);
#pragma omp parallel for
    for (int s = 0; s < final_sum; ++s) {
        int j, k;
        Compute2DIndex(j, k, s, size_m, cum_sum); // compute j and k
        table_index[j][k] = tmp_pt[j].TableMarginalizationMain(k, full_config[j], partial_config[j],
                                                               nv_old[j], cl_old[j], loc_in_old[j]);
    }
    timer->Stop("parallel");
    timer->Stop("main-sep");

    timer->Start("post-sep");
    // post-computing
    int *cum_sum2 = new int[size];
    int final_sum2 = 0;

    int l = 0;
    for (int j = 0; j < size; ++j) {
        auto separator = separators_by_level[i/2][j];
        Clique *clique = separator->ptr_upstream_clique;

        if (l < size_m && j == vector_marginalization[l]) { // index j have done the marginalization
            tmp_pt[l].TableMarginalizationPost(clique->p_table, table_index[l]);
            div_pt[j] = tmp_pt[l];
            l++;
        }

        cum_sum2[j] = final_sum2;
        final_sum2 += div_pt[j].table_size;
    }

    for (int l = 0; l < size_m; ++l) {
        SAFE_DELETE_ARRAY(loc_in_old[l]);
        SAFE_DELETE_ARRAY(full_config[l]);
        SAFE_DELETE_ARRAY(partial_config[l]);
        SAFE_DELETE_ARRAY(table_index[l]);
    }
    SAFE_DELETE_ARRAY(loc_in_old);
    SAFE_DELETE_ARRAY(full_config);
    SAFE_DELETE_ARRAY(partial_config);
    SAFE_DELETE_ARRAY(table_index);
    SAFE_DELETE_ARRAY(cum_sum);
    SAFE_DELETE_ARRAY(nv_old);

    timer->Start("parallel");
    omp_set_num_threads(num_threads);
#pragma omp parallel for
    for (int s = 0; s < final_sum2; ++s) {
        int j, k;
        Compute2DIndex(j, k, s, size, cum_sum2); // compute j and k

        if (separators_by_level[i/2][j]->old_ptable.potentials[k] == 0) {
            separators_by_level[i/2][j]->p_table.potentials[k] = 0;
        } else {
            separators_by_level[i/2][j]->p_table.potentials[k] = div_pt[j].potentials[k] / separators_by_level[i/2][j]->old_ptable.potentials[k];
        }
    }

    timer->Stop("parallel");
    SAFE_DELETE_ARRAY(cum_sum2);
    timer->Stop("post-sep");
}

/**
 * operations for cliques, including extension and multiplication
 * the multiplication is conducted on a clique and a separator,
 * so the separator may be extended, but the clique is never extended. (!!!)
 * the difference between collection and distribution:
 *      distribution finds each clique and its parent, extends the parent, then multiplies them
 *      collection finds each clique and its k-th child, extends the child, then multiplies them
 * @param i corresponds to level
 * @param size number of clique-separator pairs to be processed
 * @param has_kth_child an array containing all cliques in the current level that has the k-th child; only used for collection
 * @param k we need "k"-th child; only used for collection
 */
void JunctionTree::CliqueLevelCollection(int i, int size, const vector<int> &has_kth_child, int k,
                                         int num_threads, Timer *timer) {
    timer->Start("pre-clq");

    // used to store the (separator) potential tables that are needed to be extended
    vector<PotentialTable> tmp_pt;
    tmp_pt.reserve(size);

    // used to store the (separator) potential tables that are used to be multiplied
    // it is different from "tmp_pt" because not all the tables are needed to be extended
    vector<PotentialTable> multi_pt;
    multi_pt.resize(size);

    // store number_variables and cum_levels of the original table
    // rather than storing the whole potential table
    int *nv_old = new int[size];
    vector<vector<int>> cl_old;
    cl_old.reserve(size);

    int *cum_sum = new int[size];
    int final_sum = 0;
    int sum_index = 0;

    // not all tables need to do the extension
    // there are "size" tables in total
    // use a vector to show which tables need to do the extension
    vector<int> vector_extension;
    vector_extension.reserve(size);

    // set of arrays, showing the locations of the variables of the new table in the old table
    int **loc_in_new = new int*[size];
    int **full_config = new int*[size];
    int **partial_config = new int*[size];
    int **table_index = new int*[size];

    /**
     * pre computing
     */
    for (int j = 0; j < size; ++j) { // for each clique in this level
        Clique *clique = nodes_by_level[i][has_kth_child[j]];
        Clique *separator = clique->ptr_downstream_cliques[k];

        multi_pt[j] = separator->p_table;

        /**
         * we only need to decide whether the separator needs to be extended
         */
        if (clique->p_table.num_variables - separator->p_table.num_variables > 0) { // if the separator table should be extended
            // record the index (that requires to do the extension)
            vector_extension.push_back(j);
            nv_old[sum_index] = separator->p_table.num_variables;
            cl_old.push_back(separator->p_table.cum_levels);

            PotentialTable pt;
            pt.TableExtensionPre(clique->p_table.vec_related_variables, clique->p_table.var_dims);

            // get this table's location -- it is currently the last one
            int last = vector_extension.size() - 1;
            // generate an array showing the locations of the variables of the new table in the old table
            loc_in_new[last] = new int[separator->p_table.num_variables];
            for (int k = 0; k < separator->p_table.num_variables; ++k) {
                loc_in_new[last][k] = pt.TableReductionPre(separator->p_table.vec_related_variables[k]);
            }
            table_index[last] = new int[pt.table_size];

            tmp_pt.push_back(pt);

            // malloc in pre-, not to parallelize
            full_config[last] = new int[pt.table_size * pt.num_variables];
            partial_config[last] = new int[pt.table_size * separator->p_table.num_variables];

            // update sum
            cum_sum[sum_index++] = final_sum;
            final_sum += pt.table_size;
        }
    }
    timer->Stop("pre-clq");

    timer->Start("main-clq");
    int size_e = vector_extension.size(); // the number of variables to be extended

    timer->Start("parallel");
    // the main loop
    omp_set_num_threads(num_threads);
#pragma omp parallel for
    for (int s = 0; s < final_sum; ++s) {
        int j, k;
        Compute2DIndex(j, k, s, size_e, cum_sum); // compute j and k
        table_index[j][k] = tmp_pt[j].TableExtensionMain(k, full_config[j], partial_config[j],
                                                         nv_old[j], cl_old[j], loc_in_new[j]);
    }
    timer->Stop("parallel");
    timer->Stop("main-clq");

    timer->Start("post-clq");
    // post-computing
    int *cum_sum2 = new int[size];
    int final_sum2 = 0;

    int l = 0;
    for (int j = 0; j < size; ++j) {
        Clique *clique, *separator;
        clique = nodes_by_level[i][has_kth_child[j]];
        separator = clique->ptr_downstream_cliques[k];

        if (l < size_e && j == vector_extension[l]) { // index j have done the extension
            tmp_pt[l].TableExtensionPost(separator->p_table, table_index[l]);
            multi_pt[j] = tmp_pt[l];
            l++;
        }

        cum_sum2[j] = final_sum2;
        final_sum2 += multi_pt[j].table_size;
    }

    for (int l = 0; l < size_e; ++l) {
        SAFE_DELETE_ARRAY(loc_in_new[l]);
        SAFE_DELETE_ARRAY(full_config[l]);
        SAFE_DELETE_ARRAY(partial_config[l]);
        SAFE_DELETE_ARRAY(table_index[l]);
    }
    SAFE_DELETE_ARRAY(loc_in_new);
    SAFE_DELETE_ARRAY(full_config);
    SAFE_DELETE_ARRAY(partial_config);
    SAFE_DELETE_ARRAY(table_index);
    SAFE_DELETE_ARRAY(cum_sum);
    SAFE_DELETE_ARRAY(nv_old);

    timer->Start("parallel");
    omp_set_num_threads(num_threads);
#pragma omp parallel for
    for (int s = 0; s < final_sum2; ++s) {
        int j, k;
        Compute2DIndex(j, k, s, size, cum_sum2); // compute j and k
        nodes_by_level[i][has_kth_child[j]]->p_table.potentials[k] *= multi_pt[j].potentials[k];
//        timer->Start("norm");
//        nodes_by_level[i][has_kth_child[j]]->p_table.Normalize();
//        timer->Stop("norm");
    }
    timer->Stop("parallel");
    SAFE_DELETE_ARRAY(cum_sum2);
    timer->Stop("post-clq");
}

void JunctionTree::CliqueLevelDistribution(int i, int num_threads, Timer *timer) {
    timer->Start("pre-clq");
    int size = nodes_by_level[i].size();
    // used to store the (separator) potential tables that are needed to be extended
    vector<PotentialTable> tmp_pt;
    tmp_pt.reserve(size);

    // used to store the (separator) potential tables that are used to be multiplied
    // it is different from "tmp_pt" because not all the tables are needed to be extended
    vector<PotentialTable> multi_pt;
    multi_pt.resize(size);

    // store number_variables and cum_levels of the original table
    // rather than storing the whole potential table
    int *nv_old = new int[size];
    vector<vector<int>> cl_old;
    cl_old.reserve(size);

    int *cum_sum = new int[size];
    int final_sum = 0;
    int sum_index = 0;

    // not all tables need to do the extension
    // there are "size" tables in total
    // use a vector to show which tables need to do the extension
    vector<int> vector_extension;
    vector_extension.reserve(size);

    // set of arrays, showing the locations of the variables of the new table in the old table
    int **loc_in_new = new int*[size];
    int **full_config = new int*[size];
    int **partial_config = new int*[size];
    int **table_index = new int*[size];

    /**
     * pre computing
     */
    for (int j = 0; j < size; ++j) { // for each clique in this level
        Clique *clique = nodes_by_level[i][j];
        Clique *separator = clique->ptr_upstream_clique;

        multi_pt[j] = separator->p_table;

        /**
         * we only need to decide whether the separator needs to be extended
         */
        if (clique->p_table.num_variables - separator->p_table.num_variables > 0) { // if the separator table should be extended
            // record the index (that requires to do the extension)
            vector_extension.push_back(j);
            nv_old[sum_index] = separator->p_table.num_variables;
            cl_old.push_back(separator->p_table.cum_levels);

            PotentialTable pt;
            pt.TableExtensionPre(clique->p_table.vec_related_variables, clique->p_table.var_dims);

            // get this table's location -- it is currently the last one
            int last = vector_extension.size() - 1;
            // generate an array showing the locations of the variables of the new table in the old table
            loc_in_new[last] = new int[separator->p_table.num_variables];
            for (int k = 0; k < separator->p_table.num_variables; ++k) {
                loc_in_new[last][k] = pt.TableReductionPre(separator->p_table.vec_related_variables[k]);
            }
            table_index[last] = new int[pt.table_size];

            tmp_pt.push_back(pt);

            // malloc in pre-, not to parallelize
            full_config[last] = new int[pt.table_size * pt.num_variables];
            partial_config[last] = new int[pt.table_size * separator->p_table.num_variables];

            // update sum
            cum_sum[sum_index++] = final_sum;
            final_sum += pt.table_size;
        }
    }
    timer->Stop("pre-clq");

    timer->Start("main-clq");
    int size_e = vector_extension.size(); // the number of variables to be extended

    timer->Start("parallel");
    // the main loop
    omp_set_num_threads(num_threads);
#pragma omp parallel for
    for (int s = 0; s < final_sum; ++s) {
        int j, k;
        Compute2DIndex(j, k, s, size_e, cum_sum); // compute j and k
        table_index[j][k] = tmp_pt[j].TableExtensionMain(k, full_config[j], partial_config[j],
                                                         nv_old[j], cl_old[j], loc_in_new[j]);
    }
    timer->Stop("parallel");
    timer->Stop("main-clq");

    timer->Start("post-clq");
    // post-computing
    int *cum_sum2 = new int[size];
    int final_sum2 = 0;

    int l = 0;
    for (int j = 0; j < size; ++j) {
        Clique *clique = nodes_by_level[i][j];
        Clique *separator = clique->ptr_upstream_clique;

        if (l < size_e && j == vector_extension[l]) { // index j have done the extension
            tmp_pt[l].TableExtensionPost(separator->p_table, table_index[l]);
            multi_pt[j] = tmp_pt[l];
            l++;
        }

        cum_sum2[j] = final_sum2;
        final_sum2 += multi_pt[j].table_size;
    }

    for (int l = 0; l < size_e; ++l) {
        SAFE_DELETE_ARRAY(loc_in_new[l]);
        SAFE_DELETE_ARRAY(full_config[l]);
        SAFE_DELETE_ARRAY(partial_config[l]);
        SAFE_DELETE_ARRAY(table_index[l]);
    }
    SAFE_DELETE_ARRAY(loc_in_new);
    SAFE_DELETE_ARRAY(full_config);
    SAFE_DELETE_ARRAY(partial_config);
    SAFE_DELETE_ARRAY(table_index);
    SAFE_DELETE_ARRAY(cum_sum);
    SAFE_DELETE_ARRAY(nv_old);

    timer->Start("parallel");
    omp_set_num_threads(num_threads);
#pragma omp parallel for
    for (int s = 0; s < final_sum2; ++s) {
        int j, k;
        Compute2DIndex(j, k, s, size, cum_sum2); // compute j and k
        nodes_by_level[i][j]->p_table.potentials[k] *= multi_pt[j].potentials[k];
//        timer->Start("norm");
//        nodes_by_level[i][j]->p_table.Normalize(); // todo: wrong..
//        timer->Stop("norm");
    }
    timer->Stop("parallel");
    SAFE_DELETE_ARRAY(cum_sum2);
    timer->Stop("post-clq");
}

void JunctionTree::Collect(int num_threads, Timer *timer) {
    for (int i = max_level - 2; i >= 0 ; --i) { // for each level

        if (i % 2) {
            /**
             * case 1: levels 1, 3, 5, ... are separator levels
             * collect msg from its child (a clique) to it (a separator)
             * do marginalization + division for separator levels
             */
            SeparatorLevelCollection(i, num_threads, timer);
        }
        else {
            /**
             * case 2: levels 0, 2, 4, ... are clique levels
             * collect msg from its children (separators) to it (a clique)
             * do extension + multiplication for clique levels
             */
            timer->Start("pre-up-clq");
            int size = nodes_by_level[i].size();

            // get the maximum number of children for the cliques in the current level
            int max_num_children = 0;
            for (int j = 0; j < size; ++j) { // for each clique of this level
                auto clique = nodes_by_level[i][j];
                // first, find the max number of children for this level
                if (clique->ptr_downstream_cliques.size() > max_num_children) {
                    max_num_children = clique->ptr_downstream_cliques.size();
                }
            }
            timer->Stop("pre-up-clq");

            /**
             * there may be multiple children for a clique
             * first process the first child of each clique in this level,
             * then the second child of each clique (if has), ...
             * until all the children of all the cliques in this level have been processed
             *
             */
            for (int k = 0; k < max_num_children; ++k) { // process the k-th child
                timer->Start("pre-up-clq");
                // use a vector to mark which clique(s) has the k-th children
                vector<int> has_kth_child;
                has_kth_child.reserve(size);
                for (int j = 0; j < size; ++j) { // of each clique
                    auto clique = nodes_by_level[i][j];
                    if (clique->ptr_downstream_cliques.size() > k) {
                        // this clique has the k-th child
                        has_kth_child.push_back(j);
                    }
                }

                /**
                 * then, inside this loop (k), the following operations are similar to before:
                 * before: process "size" cliques in parallel, each update once
                 * now: process "process_size" cliques in parallel, each update once
                 */
                int process_size = has_kth_child.size();
                CliqueLevelCollection(i, process_size, has_kth_child, k, num_threads, timer);
            }
        }

        /**
         * there are some issues with datasets munin2, munin3, munin4
         * after debugging -- caused by table multiplication
         * don't have enough precision so it may cause 0 prob after multiplication
         * therefore, I add a normalization after collection of each level
         * we can remove this part for other datasets
         */
        timer->Start("norm");
        omp_set_num_threads(num_threads);
#pragma omp parallel for
        for (int i = 0; i < tree->vector_clique_ptr_container.size(); ++i) {
            tree->vector_clique_ptr_container[i]->p_table.Normalize();
        }
#pragma omp parallel for
        for (int i = 0; i < tree->vector_separator_ptr_container.size(); ++i) {
            tree->vector_separator_ptr_container[i]->p_table.Normalize();
        }
        timer->Stop("norm");
    }
}

void JunctionTree::Distribute(int num_threads, Timer *timer) {
    for (int i = 1; i < max_level; ++i) { // for each level

        if (i % 2) {
            /**
             * case 1: levels 1, 3, 5, ... are separator levels
             * distribute msg from its parent (a clique) to it (a separator)
             * do marginalization + division for separator levels
             */
            SeparatorLevelDistribution(i, num_threads, timer);
        }
        else {
            /**
             * case 2: levels 0, 2, 4,... are clique levels
             * distribute msg from its parent (a separator) to it (a clique)
             * do extension + multiplication for clique levels
             */
            CliqueLevelDistribution(i, num_threads, timer);
        }
    }
}

/**
 * @brief: compute the marginal distribution for a query variable
 * @param query_index the index of query variable TODO: here only support one query variable
 * @return a potential table (factor) representing the marginal of the query variable
 **/
PotentialTable JunctionTree::CalculateMarginalProbability(int query_index) {

    int min_size = INT32_MAX;
    Clique *selected_clique = nullptr;

    // Find the clique that contains this variable,
    // whose size of potentials table is the smallest,
    // which can reduce the number of sum operation.
    // TODO: find from separator
    for (auto &c : tree->vector_clique_ptr_container) {

        set<int> rv;
        for (int i = 0; i < c->p_table.num_variables; ++i) { // for each related variable
            rv.insert(c->p_table.vec_related_variables[i]);
        }
        if (rv.size() >= min_size) {
            continue;
        }
        if (rv.find(query_index) == rv.end()) { // cannot find the query variable
            continue;
        }

        min_size = c->p_table.vec_related_variables.size();
        selected_clique = c;
    }

    if (selected_clique == nullptr) {
        fprintf(stderr, "Error in function [%s]\n"
                        "Variable [%d] does not appear in any clique!", __FUNCTION__, query_index);
        exit(1);
    }

    if (selected_clique->p_table.num_variables == 1) {
        return selected_clique->p_table;
    }

//    set<int> other_vars;
//    for (int i = 0; i < selected_clique->p_table.num_variables; ++i) {
//        other_vars.insert(selected_clique->p_table.vec_related_variables[i]);
//    }
//    other_vars.erase(query_index);

    int dim = dynamic_cast<DiscreteNode*>(network->FindNodePtrByIndex(query_index))->GetDomainSize();

    PotentialTable pt = selected_clique->p_table;
    pt.TableMarginalization(vector<int>(1, query_index), vector<int>(1, dim));
//    pt.TableMarginalization(other_vars);
//    pt.Normalize(); // todo: no need to do normalization, because we do normalization for each clique in message passing

    return pt;
}

/**
 * @brief: get probabilities for all possible values of all non-evidence nodes
 */
void JunctionTree::GetProbabilitiesAllNodes(const DiscreteConfig &E) {
    for (int i = 0; i < network->num_nodes; ++i) {
        GetProbabilitiesOneNode(E, i);
        cout << endl;
    }
}

void JunctionTree::GetProbabilitiesOneNode(const DiscreteConfig &E, int index) {
    for (auto &e: E) {
        if (index == e.first) {
            /**
             * case 1: the node is evidence node
             * do nothing
             */
            return;
        }
    }

    /**
     * case 2: the node is non-evidence node, do the sampling like pls; has two steps:
     * output the probabilities by finding a clique containing this node
     */
    int min_size = INT32_MAX;
    Clique *selected_clique = nullptr;

    // Find the clique that contains this variable,
    // whose size of potentials table is the smallest,
    // which can reduce the number of sum operation.
    // TODO: find from separator
    for (auto &c : tree->vector_clique_ptr_container) {

        set<int> rv;
        for (int i = 0; i < c->p_table.num_variables; ++i) { // for each related variable
            rv.insert(c->p_table.vec_related_variables[i]);
        }
        if (rv.size() >= min_size) {
            continue;
        }
        if (rv.find(index) == rv.end()) { // cannot find the query variable
            continue;
        }

        min_size = c->p_table.vec_related_variables.size();
        selected_clique = c;
    }

    if (selected_clique == nullptr) {
        fprintf(stderr, "Error in function [%s]\n"
                        "Variable [%d] does not appear in any clique!", __FUNCTION__, index);
        exit(1);
    }

//    set<int> other_vars;
//    for (int i = 0; i < selected_clique->p_table.num_variables; ++i) {
//        other_vars.insert(selected_clique->p_table.vec_related_variables[i]);
//    }
//    other_vars.erase(query_index);

    int dim = dynamic_cast<DiscreteNode*>(network->FindNodePtrByIndex(query_index))->GetDomainSize();

    PotentialTable pt = selected_clique->p_table;
    pt.TableMarginalization(vector<int>(1, query_index), vector<int>(1, dim));
//    pt.TableMarginalization(other_vars);
    pt.Normalize();

    for (int i = 0; i < pt.table_size; ++i) {
        cout << pt.potentials[i] << " ";
    }
}

/**
 * @brief: predict the label for a given variable.
 */
int JunctionTree::InferenceUsingJT(int &query_index) {

    PotentialTable pt = CalculateMarginalProbability(query_index);

    // find the maximum probability
    // "pt" has only one related variable, which is exactly the query variable,
    // so the "max_index" exactly means which value of the query variable gets the max probability
    return ArgMax(pt.potentials);
}

/**
 * @brief: predict label given evidence E and target variable id Y_index
 * @return label of the target variable
 */
int JunctionTree::PredictUseJTInfer(const DiscreteConfig &E, int num_threads, Timer *timer) {
    timer->Start("load evidence");
    //update a clique using the evidence
    LoadDiscreteEvidence(E, num_threads, timer);
    timer->Stop("load evidence");

    timer->Start("norm");
    for (int i = 0; i < tree->vector_clique_ptr_container.size(); ++i) {
        tree->vector_clique_ptr_container[i]->p_table.Normalize();
    }
    timer->Stop("norm");

    timer->Start("msg passing");
    //update the whole Junction Tree
    MessagePassingUpdateJT(num_threads, timer);
    timer->Stop("msg passing");

    timer->Start("predict");
//    GetProbabilitiesAllNodes(E); // todo: used for print probabilities of all values of all non-evidence nodes
    int label_predict = InferenceUsingJT(query_index);
    timer->Stop("predict");

    timer->Start("reset");
    ResetJunctionTree();
    timer->Stop("reset");

//    cout << label_predict << endl;
    return label_predict;
}

/**
 * @brief: predict the labels given different evidences
 * it just repeats the function above multiple times, and print the progress at the meantime
 */
vector<int> JunctionTree::PredictUseJTInfer(int num_threads, Timer *timer) {
    cout << "Progress indicator: ";
    int every_1_of_20 = num_instances / 20; // used to print, print 20 times in total
    int progress = 0;

    vector<int> results(num_instances, 0);

    for (int i = 0; i < num_instances; ++i) {
        ++progress;

        if (progress % every_1_of_20 == 0) {
            string progress_percentage = to_string((double)progress/num_instances * 100) + "%...";
            fprintf(stdout, "%s\n", progress_percentage.c_str());
            fflush(stdout);
        }

        int label_predict = PredictUseJTInfer(evidences.at(i), num_threads, timer);
        results.at(i) = label_predict;
    }
    return results;
}