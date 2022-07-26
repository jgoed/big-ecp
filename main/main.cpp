#include <ecp/ecp.hpp>

#include <assert.h>
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
        if ((unsigned int)ground_truth_file.gcount() != sizeof(uint32_t) * num_knn) // If there was an error reading, repeat
        {
            cout << "Error: Read amount not enough, repeat." << endl;
            ground_truth_file.seekg(ground_truth_file.tellg() - ground_truth_file.gcount());
            ground_truth_file.read(reinterpret_cast<char *>(ids.data()), sizeof(uint32_t) * num_knn);
        }
        knns.push_back(ids);
    }

    vector<vector<float>> dists;

    for (int i = 0; i < (int)num_queries; i++)
    {
        vector<float> dis(num_knn);
        ground_truth_file.read(reinterpret_cast<char *>(dis.data()), sizeof(float) * num_knn);
        assert(ground_truth_file.fail() == false); // Abort if there is a error while reading
        dists.push_back(dis);
    }
    ground_truth_file.close();

    assert(knns.size() == dists.size()); // Abort if both vectors are not the same length

    return make_pair(knns, dists);
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
    assert(queries.size() == num_queries); // Abort if vector is to small
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
    int b = 10;

    vector<vector<float>> queries = load_queries(query_file_path);

    ecp::ecp_create_index(dataset_file_path, ecp_dir_path, L, desired_cluster_size, metric);
    ecp::ecp_assign_points_to_cluster(dataset_file_path, ecp_dir_path, num_chunks, metric);
    auto results = ecp::ecp_process_query(queries, ecp_dir_path, k, b, L, metric);

    auto ground_truth = load_ground_truth(ground_truth_file_path);

    return 0;
}
