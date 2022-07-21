#include <ecp/ecp.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

pair<vector<vector<uint32_t>>, vector<vector<float>>> load_ground_truth(string ground_truth_file_path)
{
    fstream ground_truth_file;
    ground_truth_file.open(ground_truth_file_path, ios::in | ios::binary);

    uint32_t num_queries = 0;
    uint32_t num_knn = 0;
    ground_truth_file.read((char *)&num_queries, sizeof(uint32_t));
    ground_truth_file.read((char *)&num_knn, sizeof(uint32_t));

    vector<vector<uint32_t>> knns;

    for (int i = 0; i < (int)num_queries; i++)
    {
        vector<uint32_t> ids(num_knn);
        ground_truth_file.read(reinterpret_cast<char *>(ids.data()), sizeof(uint32_t) * num_knn);
        knns.push_back(ids);
    }

    vector<vector<float>> dists;

    for (int i = 0; i < (int)num_queries; i++)
    {
        vector<float> dis(num_knn);
        // TODO: Gylfi says: Are you sure the .read() will always read as much as you ask it to? 
        // It may be wise to check is ground_truth_file.gcount the size we expected (or just if there was an error).
        // same goes for the previous call to read in line #25
        ground_truth_file.read(reinterpret_cast<char *>(dis.data()), sizeof(float) * num_knn);
        dists.push_back(dis);
    }
<<<<<<< HEAD
    ground_truth_file.close();
    return make_pair(knns, dists);
=======
    // Gylfi says: You could add a sanity check that checks if the length of the distance vector is equal to the length of the vector of IDs.
    // If they are not... 
>>>>>>> 11f783cbf8523f0b9f16c52eebf71a10a33f02e4
}

/**
 * Load queries from binary file in vector<vector<float>> because benchmark provides queries in that way
 */
vector<vector<float>> load_queries(string query_file_path)
{
    fstream query_file;
    query_file.open(query_file_path, ios::in | ios::binary);
    uint32_t num_queries = 0;
    uint32_t num_dimensions = 0;
    query_file.read((char *)&num_queries, sizeof(uint32_t));
    query_file.read((char *)&num_dimensions, sizeof(uint32_t));
    vector<vector<float>> queries;
    for (uint32_t i = 0; i < num_queries; i++)
    {
        vector<float> query;
        for (uint32_t j = 0; j < num_dimensions; j++)
        {
            int8_t buffer = 0;
            query_file.read(reinterpret_cast<char *>(&buffer), sizeof(int8_t));
            query.push_back((float)buffer);
        }
        queries.push_back(query);
    }
    query_file.close();
    return queries;
}

/**
 * Main function to test ecp algorithm without benchmark
 */
int main()
{
    string ecp_dir_path = "../../data/";
    string dataset_file_path = ecp_dir_path + "spacev1b_base_1M.i8bin";
    string query_file_path = ecp_dir_path + "query.i8bin";
    string ground_truth_file_path = "../../data/msspacev-10M";

    int L = 3;
    int desired_cluster_size = 512000;
    int metric = 0;
    int num_chunks = 2;
    int k = 10;
    int b = 1;

    vector<vector<float>> queries = load_queries(query_file_path);

    ecp::ecp_create_index(dataset_file_path, ecp_dir_path, L, desired_cluster_size, metric);
    ecp::ecp_assign_points_to_cluster(dataset_file_path, ecp_dir_path, num_chunks);
    auto results = ecp::ecp_process_query(queries, ecp_dir_path, k, b, L);

    return 0;
}
