#include "datastructure.hpp"
#include "distance.hpp"

#include <fstream>
#include <iostream>
#include <math.h>
#include <random>
#include <string>
#include <sstream>

using namespace std;

uint32_t read_nodes = 0;

struct binary_point
{
    int8_t descriptors[100]; // FIXME: Change 100 to num_dimensions later
};

struct point_meta
{
    uint32_t buffer_position;
    uint32_t point_id;
    uint32_t cluster_id;
};

// fstream ecp_cluster_metadata;
// fstream ecp_clusters;
// map<uint32_t, uint32_t> meta_data;

/**
 * Load a node from binary file and place it in in-memory index tree structure
 */
Node load_node()
{
    read_nodes++;
    Node node;
    uint32_t node_id;
    Point point;
    uint32_t point_id;
    vector<int8_t> descriptors(num_dimensions);
    uint32_t num_children;
    ecp_index.read(reinterpret_cast<char *>(&node_id), sizeof(uint32_t));
    ecp_index.read(reinterpret_cast<char *>(&point_id), sizeof(uint32_t));
    ecp_index.read(reinterpret_cast<char *>(descriptors.data()), sizeof(int8_t) * num_dimensions);
    ecp_index.read(reinterpret_cast<char *>(&num_children), sizeof(uint32_t));
    node.id = node_id;
    point.id = point_id;
    point.descriptors = descriptors;
    node.points.push_back(point);
    for (int i = 0; i < num_children; i++)
    {
        node.children.push_back(load_node());
    }
    return node;
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

// bool is_leaf(Node &node)
// {
//     return node.children.empty();
// }

// void print_index_levels(vector<Node> &root)
// {
//     queue<Node> q;
//     for (auto &cluster : root)
//     {
//         q.push(cluster);
//     }
//     while (!q.empty())
//     {
//         int n = q.size();
//         while (n > 0)
//         {
//             Node node = q.front();
//             q.pop();
//             if (is_leaf(node))
//             {
//                 cout << " [L: " << node.points.size() << "] ";
//             }
//             else
//                 cout << " [N: " << node.children.size() << "] ";
//             for (unsigned int i = 0; i < node.children.size(); i++)
//                 q.push(node.children[i]);
//             n--;
//         }
//         cout << "\n--------------\n";
//     }
// }

// vector<Node> find_all_leafs(vector<Node> &root)
// {
//     vector<Node> leafs;
//     queue<Node> q;
//     for (auto &cluster : root)
//     {
//         q.push(cluster);
//     }
//     while (!q.empty())
//     {
//         int n = q.size();
//         while (n > 0)
//         {
//             Node node = q.front();
//             q.pop();
//             if (is_leaf(node))
//             {
//                 leafs.push_back(node);
//             }
//             for (unsigned int i = 0; i < node.children.size(); i++)
//             {
//                 q.push(node.children[i]);
//             }
//             n--;
//         }
//     }
//     return leafs;
// }

/**
 * Load index tree structure from binary file and assign every point in given dataset to closest cluster
 * @param dataset_file_path Path to dataset binary file
 * @param index_file_path Path to index binary file
 */
int assign_points_to_cluster(string dataset_file_path, string index_file_path)
{
    // Read index from binary file
    ecp_index.open(index_file_path, ios::in | ios::binary);
    uint32_t num_nodes_to_read;
    ecp_index.read(reinterpret_cast<char *>(&num_nodes_to_read), sizeof(uint32_t));
    vector<Node> index;
    for (read_nodes; read_nodes < num_nodes_to_read;)
    {
        index.push_back(load_node());
    }

    // Allocate memory buffer
    binary_point *chunk{new binary_point[chunk_size]{}};

    // Open given input dataset binary file
    dataset.open(dataset_file_path, ios::in | ios::binary);
    // Read chunk into buffer
    dataset.seekg((sizeof(uint32_t) + sizeof(uint32_t)), dataset.beg);
    dataset.read(reinterpret_cast<char *>(chunk), chunk_size * sizeof(binary_point));

    point_meta *point_meta_data{new point_meta[chunk_size * num_dimensions]{}};

    for (int i = 0; i < chunk_size; i++)
    {
        Node *cluster = find_nearest_leaf(chunk[i].descriptors, index);
        point_meta_data[i].buffer_position = i;
        point_meta_data[i].cluster_id = cluster->id;
        point_meta_data[i].point_id = i;
    }

    // for (i = 0; i < 100; i++)
    // {
    //     buff_meta[i].AssignedClusterID = tree.assign(buff[i])
    // }

        // Asign all points from input dataset to a cluster leaf

    // uint32_t num_chunks = num_points / chunk_size;
    // for (int cur_chunk = 0; cur_chunk < num_chunks; cur_chunk++)
    // {
    //     vector<Node> temp_index = index;

    //     int8_t *chunk = new int8_t[chunk_size * num_dimensions];
    //     dataset.seekg(sizeof(num_points) + sizeof(num_dimensions), dataset.beg);
    //     dataset.read(reinterpret_cast<char *>(chunk), chunk_size * num_dimensions * sizeof(int8_t));

    //     for (uint32_t cur_pos = 0; cur_pos < chunk_size; cur_pos++)
    //     {
    //         // Point cur_point = read_point_from_binary(cur_pos);
    //         Point cur_point;
    //         cur_point.id = (cur_chunk * chunk_size) + cur_pos;
    //         vector<int8_t> cur_descriptors(num_dimensions);
    //         memcpy(&cur_descriptors[0], &chunk[cur_pos * num_dimensions], num_dimensions * sizeof(int8_t));
    //         cur_point.descriptors = cur_descriptors;

    //         auto *leaf = find_nearest_leaf(cur_point.descriptors.data(), temp_index);
    //         if (cur_pos != leaf->points[0].id) // Prevent adding leader twice
    //         {
    //             leaf->points.emplace_back(cur_point);
    //         }
    //     }

    //     // | num_leafs | node_id | num_points |

    //     // Write leafs to binary file
    //     ecp_cluster_metadata.open("cluster_metadata_" + to_string(cur_chunk) + ".i8bin", ios::out | ios::binary);
    //     ecp_clusters.open("clusters_" + to_string(cur_chunk) + ".i8bin", ios::out | ios::binary);
    //     vector<Node> leafs = find_all_leafs(temp_index);
    //     uint32_t num_leafs = leafs.size();
    //     ecp_cluster_metadata.write(reinterpret_cast<char *>(&num_leafs), sizeof(uint32_t));
    //     ecp_clusters.write(reinterpret_cast<char *>(&num_leafs), sizeof(uint32_t));
    //     for (auto &node : leafs)
    //     {
    //         if (meta_data.find(node.id) != meta_data.end())
    //         {
    //             meta_data[node.id] = meta_data[node.id] + node.points.size();
    //         }
    //         else
    //         {
    //             meta_data[node.id] = node.points.size();
    //         }
    //         uint32_t num_cluster_points = node.points.size();
    //         ecp_cluster_metadata.write(reinterpret_cast<char *>(&node.id), sizeof(uint32_t));
    //         ecp_cluster_metadata.write(reinterpret_cast<char *>(&num_cluster_points), sizeof(uint32_t));
    //         ecp_clusters.write(reinterpret_cast<char *>(&node.id), sizeof(uint32_t));
    //         ecp_clusters.write(reinterpret_cast<char *>(&num_cluster_points), sizeof(uint32_t));
    //         for (int i = 0; i < num_cluster_points; i++)
    //         {
    //             ecp_clusters.write(reinterpret_cast<char *>(&node.points.at(i).id), sizeof(uint32_t));
    //             ecp_clusters.write(reinterpret_cast<char *>(node.points.at(i).descriptors.data()), sizeof(int8_t) * num_dimensions);
    //         }
    //     }
    //     ecp_cluster_metadata.close();
    //     ecp_clusters.close();
    // }

    // // Merge all chunks//////////////////////////////////////////////////////////////////////////////////////////////
    // for (auto node_id : meta_data)
    // {
    //     for (int i = 0; i < num_chunks; i++)
    //     {
    //         vector<tuple<uint32_t, uint32_t>> meta;
    //         // Read cluster meta data from binary file
    //         ifstream cur_chunk_file_meta;
    //         cur_chunk_file_meta.open("cluster_metadata_" + to_string(i) + ".i8bin", ios::in | ios::binary);
    //         uint32_t num_clu_node_to_read;
    //         cur_chunk_file_meta.read(reinterpret_cast<char *>(&num_clu_node_to_read), sizeof(uint32_t));
    //         for (int i = 0; i < num_clu_node_to_read; i++)
    //         {
    //             uint32_t id;
    //             uint32_t n_points;
    //             cur_chunk_file_meta.read(reinterpret_cast<char *>(&id), sizeof(uint32_t));
    //             cur_chunk_file_meta.read(reinterpret_cast<char *>(&n_points), sizeof(uint32_t));
    //             meta.push_back(make_tuple(id, n_points));
    //         }
    //         ifstream cur_chunk_file;
    //         cur_chunk_file.open("clusters_" + to_string(i) + ".i8bin", ios::in | ios::binary);
    //         uint32_t off_set = 0;
    //         for (auto i : meta)
    //         {
    //             if (!(get<0>(i) == node_id.first))
    //             {
    //                 off_set = off_set + get<1>(i);
    //             }
    //             else
    //             {
    //                 break;
    //             }
    //         }
    //         cur_chunk_file.seekg((sizeof(uint32_t)) + off_set * num_dimensions, cur_chunk_file.beg);
    //         vector<int8_t> read_test_vec(num_dimensions);
    //         cur_chunk_file.read(reinterpret_cast<char *>(read_test_vec.data()), sizeof(int8_t) * num_dimensions);
    //     }
    // }

    return 0;
}