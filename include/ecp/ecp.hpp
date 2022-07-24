#ifndef ECP_HPP
#define ECP_HPP

#include <string>
#include <vector>

namespace ecp {

void ecp_create_index(std::string dataset_file_path, std::string ecp_dir_path, int L, int desired_cluster_size, int metric);

int ecp_assign_points_to_cluster(std::string dataset_file_path, std::string ecp_dir_path, int num_chunks);

std::vector<std::vector<unsigned int>> ecp_process_query(std::vector<std::vector<float>> queries, std::string ecp_dir_path, int k, int b, int L);

int ecp_get_distance_calculation_count();

}

#endif