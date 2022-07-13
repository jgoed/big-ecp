#include "index.hpp"
#include "assignment.hpp"
#include "query.hpp"
#include "distance.hpp"

namespace ecp
{

    int ecp_create_index(std::string dataset_file_path, std::string ecp_dir_path, int L, int desired_cluster_size, int metric)
    {
        return create_index(dataset_file_path, ecp_dir_path, L, desired_cluster_size, metric);
    }

    int ecp_assign_points_to_cluster(std::string dataset_file_path, std::string ecp_dir_path, int num_chunks, int metric)
    {
        return assign_points_to_cluster(dataset_file_path, ecp_dir_path, num_chunks, metric);
    }

    std::vector<std::vector<unsigned int>> ecp_process_query(std::vector<std::vector<float>> queries, std::string ecp_dir_path, int metric, int k, int b, int L)
    {
        return process_query(queries, ecp_dir_path, metric, k, b, L);
    }

}