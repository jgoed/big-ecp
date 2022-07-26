#ifndef ASSIGNMENT_HPP
#define ASSIGNMENT_HPP

#include "datastructures.hpp"

#include <string>
#include <vector>

/**
 * Assign all points of given input dataset to leafs of given index and write it down in binary file
 */
void assign_points_to_cluster(std::string dataset_file_path, std::string ecp_dir_path, int num_chunks, int metric);

/**
 * Function prototypes
 */
Node *find_nearest_leaf(DATATYPE *, std::vector<Node> &);
Node *get_closest_node(std::vector<Node> &, DATATYPE *);
bool compare_cluster_id(PointMeta , PointMeta );
std::vector<uint32_t> find_all_leafs(std::vector<Node> &);
bool search_leaf_meta_data(std::vector<ClusterMeta> , uint32_t , ClusterMeta &);

#endif