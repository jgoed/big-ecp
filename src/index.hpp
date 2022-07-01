#ifndef INDEX_HPP
#define INDEX_HPP

#include <string>
#include <vector>

std::string create_index(std::string dataset_file_path, int L, int desired_cluster_size);

std::vector<Node> load_index(std::string index_file_path);

#endif