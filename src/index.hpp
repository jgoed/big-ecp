#ifndef INDEX_HPP
#define INDEX_HPP

#include "datastructures.hpp"
#include <string>
#include <vector>

/**
 * Create index for given dataset and save it to binary file
 */
void create_index(std::string dataset_file_path, std::string ecp_dir_path, int L, int desired_cluster_size, int metric);

/**
 * Load index from given index binary file into memory
 */
std::vector<Node> load_index(std::string index_file_path, int metric);

/**
 * Function prototypes
 */
std::vector<uint32_t> create_random_unique_numbers(uint32_t, uint32_t);
Node create_node(std::fstream &, uint32_t, uint32_t &);
Node *get_closest_node_from_uncomplete_index(std::vector<Node> &, int8_t *, int);
void save_node(std::fstream &, Node, uint32_t &);
Node load_node(std::fstream &, uint32_t &);

#endif