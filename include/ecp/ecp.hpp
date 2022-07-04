#ifndef ECP_HPP
#define ECP_HPP

#include <string>
#include <vector>

namespace ecp {

std::string ecp_create_index(std::string dataset_file_path, int L, int desired_cluster_size);

std::string ecp_assign_points_to_cluster(std::string dataset_file_path, std::string index_file_path, uint32_t chunk_size);

std::pair<std::vector<unsigned int>, std::vector<float>> ecp_process_query(std::string query_file_path, std::string index_file_path, std::string meta_data_file_path, int k, int b, int L);

}

#endif