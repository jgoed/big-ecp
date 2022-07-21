#include "datastructure.hpp"
#include "distance.hpp"
#include "index.hpp"

#include <future>
#include <iostream>
#include <limits.h>
#include <queue>
#include <thread>
#include <vector>

using namespace std;

/**
 * Compare to distances
 */
bool smallest_distance(pair<unsigned int, float> &a, pair<unsigned int, float> &b)
{
    return a.second < b.second;
}

/**
 * Helper function
 */
unsigned index_to_max_element(vector<pair<unsigned int, float>> &point_pairs)
{
    unsigned int index = 0;
    float current_max = point_pairs[0].second;
    for (unsigned int i = 1; i < point_pairs.size(); ++i)
    {
        if (point_pairs[i].second > current_max)
        {
            current_max = point_pairs[i].second;
            index = i;
        }
    }
    return index;
}

/**
 * Compares query point to each point in cluster and accumulates the k nearest points in 'nearest_points'.
 */
void scan_leaf_node(QueryIndex &index, DATATYPE *query, uint32_t &cluster_id, const unsigned int k, vector<pair<unsigned int, float>> &nearest_points)
{
    float max_distance = std::numeric_limits<float>::max();

    if (nearest_points.size() >= k)
    {
        max_distance = nearest_points[index_to_max_element(nearest_points)].second;
    }

    ClusterMeta leaf_meta_data;
    for (auto cur_cluster_meta : index.meta_data)
    {
        if (cur_cluster_meta.cluster_id == cluster_id)
        {
            leaf_meta_data = cur_cluster_meta;
            break;
        }
    }
    ClusterPoint *search_buffer{new ClusterPoint[leaf_meta_data.num_points_in_leaf]{}};
    fstream cluster_file;
    cluster_file.open(index.clusters_file_path, ios::in | ios::binary);
    cluster_file.seekg(leaf_meta_data.offset, cluster_file.beg);
    cluster_file.read(reinterpret_cast<char *>(search_buffer), leaf_meta_data.num_points_in_leaf * sizeof(ClusterPoint));

    for (unsigned int i = 0; i < leaf_meta_data.num_points_in_leaf; i++)
    {
        if (nearest_points.size() < k)
        {
            float dist = distance::g_distance_function(query, search_buffer[i].descriptor, globals::FLOAT_MAX);
            nearest_points.emplace_back(search_buffer[i].point_id, dist);
            if (nearest_points.size() == k)
            {
                max_distance = nearest_points[index_to_max_element(nearest_points)].second;
            }
        }
        else
        {
            float dist = distance::g_distance_function(query, search_buffer[i].descriptor, max_distance);
            if (dist < max_distance)
            {
                const unsigned int max_index = index_to_max_element(nearest_points);
                nearest_points[max_index] = make_pair(search_buffer[i].point_id, dist);
                max_distance = nearest_points[index_to_max_element(nearest_points)].second;
            }
        }
    }

    delete[] search_buffer;
}

/**
 * Goes through vector of nodes and returns the one furthest away from given query vector.
 * O(b) where b is requested number of clusters to do k-nn on.
 * Returns tuple containing (index, worst_distance)
 */
pair<int, float> find_furthest_node(DATATYPE *&query, vector<Node *> &nodes)
{
    pair<int, float> worst = make_pair(-1, -1.0);
    for (unsigned int i = 0; i < nodes.size(); i++)
    {
        const float dst =
            distance::g_distance_function(query, nodes[i]->leader.descriptors, globals::FLOAT_MAX);
        if (dst > worst.second)
        {
            worst.first = i;
            worst.second = dst;
        }
    }
    return worst;
}

/**
 * Compares vector of nodes to query and returns b closest nodes in given accumulator vector.
 */
void scan_node(DATATYPE *query, vector<Node> &nodes, unsigned int &b, vector<Node *> &nodes_accumulated)
{
    pair<int, float> furthest_node = make_pair(-1, -1.0);
    if (nodes_accumulated.size() >= b)
    {
        furthest_node = find_furthest_node(query, nodes_accumulated);
    }
    for (Node &node : nodes)
    {
        if (nodes_accumulated.size() < b)
        {
            nodes_accumulated.emplace_back(&node);
            if (nodes_accumulated.size() == b)
            {
                furthest_node = find_furthest_node(query, nodes_accumulated);
            }
        }
        else
        {
            if (distance::g_distance_function(query, node.leader.descriptors, furthest_node.second) < furthest_node.second)
            {
                nodes_accumulated[furthest_node.first] = &node;
                furthest_node = find_furthest_node(query, nodes_accumulated);
            }
        }
    }
}

/**
 * Traverses node children one level at a time to find b nearest
 */
vector<Node *> find_b_nearest_clusters(vector<Node> &root, DATATYPE *query, unsigned int b, unsigned int L)
{
    vector<Node *> b_best;
    b_best.reserve(b);
    scan_node(query, root, b, b_best);
    while (L > 1)
    {
        vector<Node *> new_best_nodes;
        new_best_nodes.reserve(b);
        for (auto *node : b_best)
        {
            scan_node(query, node->children, b, new_best_nodes);
        }
        L = L - 1;
        b_best = new_best_nodes;
    }
    return b_best;
}

/**
 * Find nearest neighbors
 */
vector<pair<unsigned int, float>> k_nearest_neighbors(QueryIndex &index, DATATYPE *query, const unsigned int k, const unsigned int b, unsigned int L)
{
    vector<Node *> b_nearest_clusters{find_b_nearest_clusters(index.top_level, query, b, L)};
    vector<pair<unsigned int, float>> k_nearest_points;
    k_nearest_points.reserve(k);
    for (Node *cluster : b_nearest_clusters)
    {
        scan_leaf_node(index, query, cluster->id, k, k_nearest_points);
    }
    sort(k_nearest_points.begin(), k_nearest_points.end(), smallest_distance);
    return k_nearest_points;
}

/**
 * Search k nearest indexes in b nearest cluster from given index
 */
vector<unsigned int> query(QueryIndex index, vector<DATATYPE> query, unsigned int k, int b, int L)
{
    auto nearest_points = k_nearest_neighbors(index, query.data(), k, b, L);
    vector<unsigned int> nearest_indexes;
    for (auto it = make_move_iterator(nearest_points.begin()), end = make_move_iterator(nearest_points.end()); it != end; ++it)
    {
        nearest_indexes.push_back(it->first);
    }
    return nearest_indexes;
}

/**
 * Load meta data for clusters from binary file
 */
vector<ClusterMeta> load_meta_data(string meta_data_file_path)
{
    fstream meta_data_file;
    meta_data_file.open(meta_data_file_path, ios::in | ios::binary);
    uint32_t num_leafs = 0;
    meta_data_file.read((char *)&num_leafs, sizeof(uint32_t));
    vector<ClusterMeta> cluster_meta_data;
    for (uint32_t i = 0; i < num_leafs; i++)
    {
        ClusterMeta cur_meta_data;
        meta_data_file.read(reinterpret_cast<char *>(&cur_meta_data), sizeof(ClusterMeta));
        cluster_meta_data.push_back(cur_meta_data);
    }
    return cluster_meta_data;
}

/**
 * Search k nearest neighbors in b clusters for every given query
 */
vector<vector<unsigned int>> process_query(vector<vector<float>> queries, string ecp_dir_path, int k, int b, int L)
{
    QueryIndex index; // Struct to pass around during search
    index.top_level = load_index(ecp_dir_path + "ecp_index.bin");           // Load index from binary file
    index.meta_data = load_meta_data(ecp_dir_path + "ecp_cluster_meta.bin"); // Load index meta data from binary file
    index.clusters_file_path = ecp_dir_path + "ecp_clusters.bin";        // Set file path for clusters meta data

    int num_queries = queries.size();

    assert(queries[0].size() == DIMENSIONS);

    vector<vector<DATATYPE>> converted_queries;

    for (int x = 0; x < num_queries; x++) // Convert all queries to DATATYPE
    {
        vector<DATATYPE> cur_conv_query;
        for (int y = 0; y < DIMENSIONS; y++)
        {
            cur_conv_query.push_back(static_cast<DATATYPE>(queries[x][y]));
        }
        converted_queries.push_back(cur_conv_query);
    }

    const auto num_threads = thread::hardware_concurrency(); // Total number of availbale threads
    queue<future<vector<unsigned int>>> queued_future_results; // Collect results from threads
    vector<vector<unsigned int>> results; // Sorted final results

    for (int i = 0; i < num_queries; i++) // Go through every query
    {
        if (queued_future_results.size() >= num_threads) // Check if another thread can be started
        {
            results.push_back(queued_future_results.front().get()); // Blocks until thread is done
            queued_future_results.pop();
        }

        queued_future_results.emplace(async(launch::async, query, index, converted_queries[i], k, b, L)); // Start async function call
    }

    while (!queued_future_results.empty())
    {
        results.push_back(queued_future_results.front().get()); // Blocks until thread is done
        queued_future_results.pop();
    }

    return results;
}