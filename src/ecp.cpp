#include "index.hpp"
#include "assignment.hpp"
#include "query.hpp"

namespace ecp
{

    int ecp_create_index(std::string dataset_file_path, std::string ecp_dir_path, int L, int desired_cluster_size)
    {
        return create_index(dataset_file_path, ecp_dir_path, L, desired_cluster_size);
    }

    int ecp_assign_points_to_cluster(std::string dataset_file_path, std::string ecp_dir_path, unsigned int chunk_size)
    {
        return assign_points_to_cluster(dataset_file_path, ecp_dir_path, chunk_size);
    }

    std::vector<std::vector<unsigned int>> ecp_process_query(std::vector<std::vector<float>> queries, std::string ecp_dir_path, int k, int b, int L)
    {
        return process_query(queries, ecp_dir_path, k, b, L);
    }

}