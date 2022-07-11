#ifndef INDEX_HPP
#define INDEX_HPP

#include "datastructure.hpp"
#include <string>
#include <vector>

int create_index(std::string dataset_file_path, std::string ecp_dir_path, int L, int desired_cluster_size);

std::vector<Node> load_index(std::string index_file_path);

#endif