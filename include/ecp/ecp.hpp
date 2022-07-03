#ifndef ECP_HPP
#define ECP_HPP

#include <ecp/index.hpp>
#include <ecp/assignment.hpp>
#include <ecp/query.hpp>

#include <string>

namespace ecp {

std::string create_index(std::string dataset_file_path, int L, int desired_cluster_size);

std::string assign_points_to_cluster(std::string dataset_file_path, std::string index_file_path, uint32_t chunk_size);

int process_query(std::string query_file_path, std::string index_file_path, std::string meta_data_file_path, int k, int b, int L);

}

#endif