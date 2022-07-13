#include "datastructure.hpp"
#include "distance.hpp"
#include "index.hpp"

#include <limits.h>

using namespace std;

vector<ClusterMeta> cluster_meta_data; // Global variable to prevent passing parameter though many functions
string cluster_file_path;              // Global variable to prevent passing parameter though many functions

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
 * Load all points for a leaf node from binary file into memory
 */
vector<ClusterPoint> load_leaf(string cluster_file_path, vector<ClusterMeta> cluster_meta_data, uint32_t leaf_id)
{
    ClusterMeta leaf_meta_data;
    for (auto cur_cluster_meta : cluster_meta_data)
    {
        if (cur_cluster_meta.cluster_id == leaf_id)
        {
            leaf_meta_data = cur_cluster_meta;
            break;
        }
    }
    fstream cluster_file;
    cluster_file.open(cluster_file_path, ios::in | ios::binary);
    cluster_file.seekg(leaf_meta_data.offset, cluster_file.beg);
    vector<ClusterPoint> points_in_leaf;
    for (uint32_t i = 0; i < leaf_meta_data.num_points_in_leaf; i++)
    {
        ClusterPoint cur_cluster_point;
        cluster_file.read((char *)&cur_cluster_point, sizeof(ClusterPoint));
        points_in_leaf.push_back(cur_cluster_point);
    }
    cluster_file.close();
    return points_in_leaf;
}

/**
 * Compares query point to each point in cluster and accumulates the k nearest points in 'nearest_points'.
 */
void scan_leaf_node(DATATYPE *query, uint32_t &cluster_id, const unsigned int k, vector<pair<unsigned int, float>> &nearest_points)
{
    float max_distance = std::numeric_limits<float>::max();

    if (nearest_points.size() >= k)
    {
        max_distance = nearest_points[index_to_max_element(nearest_points)].second;
    }
    vector<ClusterPoint> points = load_leaf(cluster_file_path, cluster_meta_data, cluster_id);
    for (ClusterPoint &point : points)
    {
        if (nearest_points.size() < k)
        {
            float dist = distance::g_distance_function(query, point.descriptor, globals::FLOAT_MAX);
            nearest_points.emplace_back(point.point_id, dist);
            if (nearest_points.size() == k)
            {
                max_distance = nearest_points[index_to_max_element(nearest_points)].second;
            }
        }
        else
        {
            float dist = distance::g_distance_function(query, point.descriptor, max_distance);
            if (dist < max_distance)
            {
                const unsigned int max_index = index_to_max_element(nearest_points);
                nearest_points[max_index] = make_pair(point.point_id, dist);
                max_distance = nearest_points[index_to_max_element(nearest_points)].second;
            }
        }
    }
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
vector<pair<unsigned int, float>> k_nearest_neighbors(vector<Node> &root, DATATYPE *query, const unsigned int k, const unsigned int b, unsigned int L)
{
    vector<Node *> b_nearest_clusters{find_b_nearest_clusters(root, query, b, L)};
    vector<pair<unsigned int, float>> k_nearest_points;
    k_nearest_points.reserve(k);
    for (Node *cluster : b_nearest_clusters)
    {
        scan_leaf_node(query, cluster->id, k, k_nearest_points);
    }
    sort(k_nearest_points.begin(), k_nearest_points.end(), smallest_distance);
    return k_nearest_points;
}

/**
 * Search k nearest indexes in b nearest cluster from given index
 */
vector<unsigned int> query(vector<Node> &index, DATATYPE *query, unsigned int k, int b, int L)
{
    auto nearest_points = k_nearest_neighbors(index, query, k, b, L);
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
vector<vector<unsigned int>> process_query(vector<vector<float>> queries, string ecp_dir_path, int metric, int k, int b, int L)
{
    vector<Node> index = load_index(ecp_dir_path + "ecp_index.bin");           // Load index from binary file
    cluster_meta_data = load_meta_data(ecp_dir_path + "ecp_cluster_meta.bin"); // Load index meta data from binary file
    cluster_file_path = ecp_dir_path + "ecp_clusters.bin";                     // Set file path for clusters meta data

    cout << "GLOBAL DIMENSIONS IN QUERY:" << globals::NUM_DIMENSIONS << endl;
    // auto cur_metric = static_cast<distance::Metric>(metric);
    // distance::set_distance_function(cur_metric); // Set distance function globally

    vector<vector<unsigned int>> results;

    for (auto q : queries)
    {
        vector<DATATYPE> cur_query_point;
        cur_query_point.reserve(q.size());
        for (const auto &f : q) // Convert every query value from float to DATATYPE
        {
            cur_query_point.push_back(static_cast<DATATYPE>(f));
        }

        vector<unsigned int> result = query(index, cur_query_point.data(), k, b, L); // Start query process
        results.push_back(result);
    }

    return results;
}