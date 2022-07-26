#ifndef QUERY_HPP
#define QUERY_HPP

#include "datastructures.hpp"

#include <string>
#include <vector>

/**
 * Search k nearest neighbors in b clusters for every given query
 */
std::vector<std::vector<unsigned int>> process_query(std::vector<std::vector<float>> queries, std::string ecp_dir_path, int k, int b, int L, int metric);

/**
 * Function prototypes
 */
std::vector<ClusterMeta> load_meta_data(std::string, int);
std::vector<unsigned int> query(QueryIndex, std::vector<DATATYPE>, unsigned int, int, int);
std::vector<std::pair<unsigned int, float>> k_nearest_neighbors(QueryIndex &, DATATYPE *, const unsigned int, const unsigned int, unsigned int);
std::vector<Node *> find_b_nearest_clusters(std::vector<Node> &, DATATYPE *, unsigned int, unsigned int);
void scan_node(DATATYPE *, std::vector<Node> &, unsigned int &, std::vector<Node *> &);
std::pair<int, float> find_furthest_node(DATATYPE *&, std::vector<Node *> &);
void scan_leaf_node(QueryIndex &, DATATYPE *, uint32_t &, const unsigned int , std::vector<std::pair<unsigned int, float>> &);
unsigned index_to_max_element(std::vector<std::pair<unsigned int, float>> &);
bool smallest_distance(std::pair<unsigned int, float> &, std::pair<unsigned int, float> &);

#endif