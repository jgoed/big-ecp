#include "index.hpp"
#include "assignment.hpp"
#include "query.hpp"

namespace ecp
{

    std::string ecp_create_index(std::string dataset_file_path, int L, int desired_cluster_size)
    {
        return create_index(dataset_file_path, L, desired_cluster_size);
    }

    std::string ecp_assign_points_to_cluster(std::string dataset_file_path, std::string index_file_path, uint32_t chunk_size)
    {
        return assign_points_to_cluster(dataset_file_path, index_file_path, chunk_size);
    }

    std::pair<std::vector<unsigned int>, std::vector<float>> ecp_process_query(std::string query_file_path, std::string index_file_path, std::string meta_data_file_path, int k, int b, int L)
    {
        return process_query(query_file_path, index_file_path, meta_data_file_path, k, b, L);
    }

}