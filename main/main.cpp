#include <ecp/ecp.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

void load_ground_truth()
{
    fstream ground_truth_file;
    ground_truth_file.open("../../data/msspacev-10M", ios::in | ios::binary);

    uint32_t num_queries = 0;
    uint32_t num_knn = 0;
    ground_truth_file.read((char *)&num_queries, sizeof(uint32_t));
    ground_truth_file.read((char *)&num_knn, sizeof(uint32_t));

    vector<vector<uint32_t>> knns;

    for (int i = 0; i < num_queries; i++)
    {
        vector<uint32_t> ids(num_knn);
        ground_truth_file.read(reinterpret_cast<char *>(ids.data()), sizeof(uint32_t) * num_knn);
        knns.push_back(ids);
    }

    vector<vector<float>> dists;

    for (int i = 0; i < num_queries; i++)
    {
        vector<float> dis(num_knn);
        ground_truth_file.read(reinterpret_cast<char *>(dis.data()), sizeof(float) * num_knn);
        dists.push_back(dis);
    }
    cout << "done" << endl;
}

int main()
{
    string data_folder_path = "../../data/";
    string dataset_file_path = data_folder_path + "spacev1b_base_1M.i8bin";
    int L = 3;                         // Number of index levels
    int desired_cluster_size = 512000; // 512000 byte is default block size for SSDs
    uint32_t chunk_size = 200000;

    string query_file_path = data_folder_path + "query.i8bin";
    int k = 10;
    int b = 1;

    string index_file_path = ecp::ecp_create_index(dataset_file_path, L, desired_cluster_size);
    string meta_data_file_path = ecp::ecp_assign_points_to_cluster(dataset_file_path, index_file_path, chunk_size);
    auto results = ecp::ecp_process_query(query_file_path, index_file_path, meta_data_file_path, k, b, L);

    load_ground_truth();

    return 0;
}