#include "datastructure.hpp"
#include "distance.hpp"
#include "index.hpp"

#include <limits.h>

using namespace std;

vector<Cluster_meta> cluster_meta_data;

void load_ground_truth()
{
    fstream ground_truth_file;
    ground_truth_file.open("msspacev-10M", ios::in | ios::binary);

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

bool smallest_distance(pair<unsigned int, float> &a, pair<unsigned int, float> &b)
{
    return a.second < b.second;
}

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

vector<Cluster_point> load_leaf(string cluster_file_path, vector<Cluster_meta> cluster_meta_data, uint32_t leaf_id)
{
    Cluster_meta leaf_meta_data;
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
    vector<Cluster_point> points_in_leaf;
    for (int i = 0; i < leaf_meta_data.num_points_in_leaf; i++)
    {
        Cluster_point cur_cluster_point;
        cluster_file.read((char *)&cur_cluster_point, sizeof(Cluster_point));
        points_in_leaf.push_back(cur_cluster_point);
    }
    cluster_file.close();
    return points_in_leaf;
}

void scan_leaf_node(int8_t *&query, uint32_t &cluster_id, const unsigned int k, vector<pair<unsigned int, float>> &nearest_points)
{
    float max_distance = std::numeric_limits<float>::max();

    if (nearest_points.size() >= k)
    {
        max_distance = nearest_points[index_to_max_element(nearest_points)].second;
    }
    vector<Cluster_point> points = load_leaf("ecp_clusters.bin", cluster_meta_data, cluster_id);
    for (Cluster_point &point : points)
    {
        if (nearest_points.size() < k)
        {
            float dist = euclidean_distance(query, point.descriptor);
            nearest_points.emplace_back(point.point_id, dist);
            if (nearest_points.size() == k)
            {
                max_distance = nearest_points[index_to_max_element(nearest_points)].second;
            }
        }
        else
        {
            float dist = euclidean_distance(query, point.descriptor);
            if (dist < max_distance)
            {
                const unsigned int max_index = index_to_max_element(nearest_points);
                nearest_points[max_index] = make_pair(point.point_id, dist);
                max_distance = nearest_points[index_to_max_element(nearest_points)].second;
            }
        }
    }
}

pair<int, float> find_furthest_node(int8_t *&query, vector<Node *> &nodes)
{
    pair<int, float> worst = make_pair(-1, -1.0);
    for (unsigned int i = 0; i < nodes.size(); i++)
    {
        const float dst =
            euclidean_distance(query, nodes[i]->leader.descriptors);
        if (dst > worst.second)
        {
            worst.first = i;
            worst.second = dst;
        }
    }
    return worst;
}

void scan_node(int8_t *&query, vector<Node> &nodes, unsigned int &b, vector<Node *> &nodes_accumulated)
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
            if (euclidean_distance(query, node.leader.descriptors) < furthest_node.second)
            {
                nodes_accumulated[furthest_node.first] = &node;
                furthest_node = find_furthest_node(query, nodes_accumulated);
            }
        }
    }
}

vector<Node *> find_b_nearest_clusters(vector<Node> &root, int8_t *&query, unsigned int b, unsigned int L)
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

vector<pair<unsigned int, float>> k_nearest_neighbors(vector<Node> &root, int8_t *&query, const unsigned int k, const unsigned int b = 1, unsigned int L = 1)
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

pair<vector<unsigned int>, vector<float>> query(vector<Node> &index, int8_t *query, unsigned int k)
{
    auto nearest_points = k_nearest_neighbors(index, query, k, 5, 3);
    vector<unsigned int> nearest_indexes = {};
    vector<float> nearest_dist = {};
    for (auto it = make_move_iterator(nearest_points.begin()), end = make_move_iterator(nearest_points.end()); it != end; ++it)
    {
        nearest_indexes.push_back(it->first);
        nearest_dist.push_back(it->second);
    }
    return make_pair(nearest_indexes, nearest_dist);
}

vector<Point> load_queries(string query_file_path)
{
    fstream query_file;
    query_file.open(query_file_path, ios::in | ios::binary);
    uint32_t num_queries = 0;
    query_file.read((char *)&num_queries, sizeof(uint32_t));
    query_file.read((char *)&global_num_dimensions, sizeof(uint32_t));
    vector<Point> queries;
    for (int i = 0; i < num_queries; i++)
    {
        Point point;
        point.id = i;
        query_file.read(reinterpret_cast<char *>(point.descriptors), global_point_size);
        queries.push_back(point);
    }
    return queries;
}

vector<Cluster_meta> load_meta_data(string meta_data_file_path)
{
    fstream meta_data_file;
    meta_data_file.open(meta_data_file_path, ios::in | ios::binary);
    uint32_t num_leafs = 0;
    meta_data_file.read((char *)&num_leafs, sizeof(uint32_t));
    vector<Cluster_meta> cluster_meta_data;
    for (int i = 0; i < num_leafs; i++)
    {
        Cluster_meta cur_meta_data;
        meta_data_file.read(reinterpret_cast<char *>(&cur_meta_data), sizeof(Cluster_meta));
        cluster_meta_data.push_back(cur_meta_data);
    }
    return cluster_meta_data;
}

int process_query(string query_file_path, string index_file_path, string meta_data_file_path)
{
    // Load index from binary file
    vector<Node> index = load_index(index_file_path);

    // Load index meta data from binary file
    cluster_meta_data = load_meta_data(meta_data_file_path);

    // Load queries from binary file
    vector<Point> queries = load_queries(query_file_path);

    auto result = query(index, queries[0].descriptors, 50);

    load_ground_truth();

    return 0;
}