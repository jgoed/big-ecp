#include "datastructure.hpp"

#include <bits/stdc++.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <random>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

fstream dataset;
uint32_t num_points = 0;
uint32_t num_leaders = 0;
uint32_t unique_node_id = 0;
uint32_t num_nodes_in_index = 0;
ofstream ecp_index;
ofstream ecp_cluster_metadata;
ofstream ecp_clusters;

map<uint32_t, uint32_t> meta_data;

vector<uint32_t> get_random_unique_indexes(uint32_t amount, uint32_t max_index)
{
    vector<uint32_t> collected_samples;
    collected_samples.reserve(amount);
    unordered_set<uint32_t> visited_samples;
    random_device random_seed;
    mt19937 generator(random_seed());
    for (int i = 0; i < amount; i++)
    {
        uniform_int_distribution<uint32_t> distribution(0, max_index); // To-from inclusive
        uint32_t cur_rnd_num = distribution(generator);

        unordered_set<uint32_t>::const_iterator iter = visited_samples.find(cur_rnd_num);
        if (iter == visited_samples.end()) // Not found
        {
            visited_samples.insert(cur_rnd_num);
            collected_samples.emplace_back(cur_rnd_num);
        }
        else // Found
        {
            visited_samples.insert(i);
            collected_samples.emplace_back(i);
        }
    }
    return collected_samples;
}

Point read_point_from_binary(uint32_t position)
{
    vector<int8_t> descriptors(num_dimensions);
    dataset.seekg((sizeof(num_points) + sizeof(num_dimensions) + position * num_dimensions * sizeof(int8_t)), dataset.beg);
    dataset.read(reinterpret_cast<char *>(descriptors.data()), sizeof(int8_t) * num_dimensions);
    Point point;
    point.id = position;
    point.descriptors = descriptors;
    return point;
}

Node read_node_from_binary(uint32_t position)
{
    Node node;
    node.id = unique_node_id;
    unique_node_id++;
    node.points.push_back(read_point_from_binary(position));
    return node;
}

float euclidean_distance(const int8_t *a, const int8_t *b)
{
    float sums[] = {0.0, 0.0, 0.0, 0.0};
    for (unsigned int i = 0; i < num_dimensions; ++i)
    {
        float delta = a[i] - b[i];
        sums[i % 4] += delta * delta;
    }
    return sums[0] + sums[1] + sums[2] + sums[3];
}

Node *get_closest_node_from_uncomplete_index(vector<Node> &nodes, int8_t *query, int deepth)
{
    float max = numeric_limits<float>::max();
    Node *closest = nullptr;
    for (Node &node : nodes)
    {
        const float distance = euclidean_distance(query, &node.points[0].descriptors[0]);
        if (distance < max)
        {
            max = distance;
            closest = &node;
        }
    }
    if (deepth > 1)
    {
        closest = get_closest_node_from_uncomplete_index(closest->children, query, deepth - 1);
    }
    return closest;
}

Node *get_closest_node(std::vector<Node> &nodes, int8_t *query)
{
    float max = numeric_limits<float>::max();
    Node *closest = nullptr;
    for (Node &node : nodes)
    {
        const float distance = euclidean_distance(query, &node.points[0].descriptors[0]);

        if (distance < max)
        {
            max = distance;
            closest = &node;
        }
    }
    return closest;
}

Node *find_nearest_leaf(int8_t *query, std::vector<Node> &nodes)
{
    Node *closest_cluster = get_closest_node(nodes, query);
    if (!closest_cluster->children.empty())
    {
        return find_nearest_leaf(query, closest_cluster->children);
    }
    return closest_cluster;
}

bool is_leaf(Node &node)
{
    return node.children.empty();
}

void print_index_levels(vector<Node> &root)
{
    queue<Node> q;
    for (auto &cluster : root)
    {
        q.push(cluster);
    }
    while (!q.empty())
    {
        int n = q.size();
        while (n > 0)
        {
            Node node = q.front();
            q.pop();
            if (is_leaf(node))
            {
                cout << " [L: " << node.points.size() << "] ";
            }
            else
                cout << " [N: " << node.children.size() << "] ";
            for (unsigned int i = 0; i < node.children.size(); i++)
                q.push(node.children[i]);
            n--;
        }
        cout << "\n--------------\n";
    }
}

void save_node(Node node)
{
    num_nodes_in_index++;
    ecp_index.write(reinterpret_cast<char *>(&node.id), sizeof(uint32_t));
    ecp_index.write(reinterpret_cast<char *>(&node.points.at(0).id), sizeof(uint32_t));
    ecp_index.write(reinterpret_cast<char *>(node.points.at(0).descriptors.data()), sizeof(int8_t) * num_dimensions);
    uint32_t cur_num_children = node.children.size();
    ecp_index.write(reinterpret_cast<char *>(&cur_num_children), sizeof(uint32_t));
    for (int i = 0; i < node.children.size(); i++)
    {
        save_node(node.children[i]);
    }
}

vector<Node> find_all_leafs(vector<Node> &root)
{
    vector<Node> leafs;
    queue<Node> q;
    for (auto &cluster : root)
    {
        q.push(cluster);
    }
    while (!q.empty())
    {
        int n = q.size();
        while (n > 0)
        {
            Node node = q.front();
            q.pop();
            if (is_leaf(node))
            {
                leafs.push_back(node);
            }
            for (unsigned int i = 0; i < node.children.size(); i++)
            {
                q.push(node.children[i]);
            }
            n--;
        }
    }
    return leafs;
}

int preprocessing()
{
    int L = 3;

    // Open binary file
    dataset.open("spacev1b_base.i8bin", ios::in | ios::binary);

    // Read metadata from binary file
    dataset.read((char *)&num_points, sizeof(uint32_t));
    dataset.read((char *)&num_dimensions, sizeof(uint32_t));

    // Calculate the overall number of leaders
    int desired_cluster_size = 512000;                                                                              // 512000 byte is default block size for SSDs
    num_leaders = ceil(num_points / (desired_cluster_size / (sizeof(int8_t) * num_dimensions + sizeof(uint32_t)))); // N/ts

    // Generate random leaders
    // random_leader_indexes.at(i) vector<uint32_t> random_leader_indexes = get_random_unique_indexes(num_leaders, num_points - 1);

    // Create index structure
    vector<Node> index;

    for (int cur_lvl = 1; cur_lvl <= L; cur_lvl++)
    {
        // Calculate number of leaders for current level
        int cur_lvl_leaders = ceil(pow(num_leaders, ((1.0 / L)) * (cur_lvl)));
        for (int i = 0; i < cur_lvl_leaders; i++)
        {
            if (cur_lvl == 1)
            {
                index.push_back(read_node_from_binary(i));
            }
            else
            {
                Node current_node = read_node_from_binary(i);
                Node *clostest_node = get_closest_node_from_uncomplete_index(index, &current_node.points[0].descriptors[0], cur_lvl - 1);
                clostest_node->children.push_back(current_node);
            }
        }
    }

    // Add all points from input dataset to the index incl those duplicated in the index construction.
    uint32_t num_chunks = num_points / chunk_size;
    for (int cur_chunk = 0; cur_chunk < num_chunks; cur_chunk++)
    {
        vector<Node> temp_index = index;

        int8_t *chunk = new int8_t[chunk_size * num_dimensions];
        dataset.seekg(sizeof(num_points) + sizeof(num_dimensions), dataset.beg);
        dataset.read(reinterpret_cast<char *>(chunk), chunk_size * num_dimensions * sizeof(int8_t));

        for (uint32_t cur_pos = 0; cur_pos < chunk_size; cur_pos++)
        {
            // Point cur_point = read_point_from_binary(cur_pos);
            Point cur_point;
            cur_point.id = (cur_chunk * chunk_size) + cur_pos;
            vector<int8_t> cur_descriptors(num_dimensions);
            memcpy(&cur_descriptors[0], &chunk[cur_pos * num_dimensions], num_dimensions * sizeof(int8_t));
            cur_point.descriptors = cur_descriptors;

            auto *leaf = find_nearest_leaf(cur_point.descriptors.data(), temp_index);
            if (cur_pos != leaf->points[0].id) // Prevent adding leader twice
            {
                leaf->points.emplace_back(cur_point);
            }
        }

        // | num_leafs | node_id | num_points |

        // Write leafs to binary file
        ecp_cluster_metadata.open("cluster_metadata_" + to_string(cur_chunk) + ".i8bin", ios::out | ios::binary);
        ecp_clusters.open("clusters_" + to_string(cur_chunk) + ".i8bin", ios::out | ios::binary);
        vector<Node> leafs = find_all_leafs(temp_index);
        uint32_t num_leafs = leafs.size();
        ecp_cluster_metadata.write(reinterpret_cast<char *>(&num_leafs), sizeof(uint32_t));
        ecp_clusters.write(reinterpret_cast<char *>(&num_leafs), sizeof(uint32_t));
        for (auto &node : leafs)
        {
            if (meta_data.find(node.id) != meta_data.end())
            {
                meta_data[node.id] = meta_data[node.id] + node.points.size();
            }
            else
            {
                meta_data[node.id] = node.points.size();
            }
            uint32_t num_cluster_points = node.points.size();
            ecp_cluster_metadata.write(reinterpret_cast<char *>(&node.id), sizeof(uint32_t));
            ecp_cluster_metadata.write(reinterpret_cast<char *>(&num_cluster_points), sizeof(uint32_t));
            ecp_clusters.write(reinterpret_cast<char *>(&node.id), sizeof(uint32_t));
            ecp_clusters.write(reinterpret_cast<char *>(&num_cluster_points), sizeof(uint32_t));
            for (int i = 0; i < num_cluster_points; i++)
            {
                ecp_clusters.write(reinterpret_cast<char *>(&node.points.at(i).id), sizeof(uint32_t));
                ecp_clusters.write(reinterpret_cast<char *>(node.points.at(i).descriptors.data()), sizeof(int8_t) * num_dimensions);
            }
        }
        ecp_cluster_metadata.close();
        ecp_clusters.close();
    }

    // Merge all chunks//////////////////////////////////////////////////////////////////////////////////////////////
    for (auto node_id : meta_data)
    {
        for (int i = 0; i < num_chunks; i++)
        {
            vector<tuple<uint32_t, uint32_t>> meta;
            // Read cluster meta data from binary file
            ifstream cur_chunk_file_meta;
            cur_chunk_file_meta.open("cluster_metadata_" + to_string(i) + ".i8bin", ios::in | ios::binary);
            uint32_t num_clu_node_to_read;
            cur_chunk_file_meta.read(reinterpret_cast<char *>(&num_clu_node_to_read), sizeof(uint32_t));
            for (int i = 0; i < num_clu_node_to_read; i++)
            {
                uint32_t id;
                uint32_t n_points;
                cur_chunk_file_meta.read(reinterpret_cast<char *>(&id), sizeof(uint32_t));
                cur_chunk_file_meta.read(reinterpret_cast<char *>(&n_points), sizeof(uint32_t));
                meta.push_back(make_tuple(id, n_points));
            }
            ifstream cur_chunk_file;
            cur_chunk_file.open("clusters_" + to_string(i) + ".i8bin", ios::in | ios::binary);
            uint32_t off_set = 0;
            for (auto i : meta)
            {
                if (!(get<0>(i) == node_id.first))
                {
                    off_set = off_set + get<1>(i);
                }
                else
                {
                    break;
                }
            }
            cur_chunk_file.seekg((sizeof(uint32_t)) + off_set * num_dimensions, cur_chunk_file.beg);
            vector<int8_t> read_test_vec(num_dimensions);
            cur_chunk_file.read(reinterpret_cast<char *>(read_test_vec.data()), sizeof(int8_t) * num_dimensions);
        }
    }

    // Write index to binary file
    ecp_index.open("index.i8bin", ios::out | ios::binary);
    ecp_index.write(reinterpret_cast<char *>(&num_nodes_in_index), sizeof(uint32_t));
    for (auto &node : index)
    {
        save_node(node);
    }
    ecp_index.seekp(0, ecp_index.beg);
    ecp_index.write(reinterpret_cast<char *>(&num_nodes_in_index), sizeof(uint32_t));
    ecp_index.close();

    return 0;
}