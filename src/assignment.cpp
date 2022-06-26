#include "datastructure.hpp"
#include "distance.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <math.h>
#include <random>
#include <stdio.h>
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

struct cluster_meta
{
    uint32_t cluster_id;
    streampos offset;
};

struct cluster_point
{
    uint32_t cluster_id;
    uint32_t point_id;
    int8_t descriptor[100];
};

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

/**
 * Retrun closest node to a query point from a given vector of nodes
 * @param nodes Vector of nodes to search for closest node
 * @param query Query point
 */
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

/**
 * Find the nearest leaf for a given query point
 * @param query Query point
 * @param nodes Top level for index to find nearest leaf from
 */
Node *find_nearest_leaf(int8_t *query, std::vector<Node> &nodes)
{
    Node *closest_cluster = get_closest_node(nodes, query);
    if (!closest_cluster->children.empty())
    {
        return find_nearest_leaf(query, closest_cluster->children);
    }
    return closest_cluster;
}

/**
 * Check if one point id is greater then another
 * @param p1 First point
 * @param p2 Second point
 */
bool compare_cluster_id(point_meta p1, point_meta p2)
{
    if (p1.cluster_id < p2.cluster_id)
    {
        return true;
    }
    return false;
}

/**
 * Find all leafs in a given index tree structure
 * @param root Top level of index to search for all leafs
 */
vector<uint32_t> find_all_leafs(vector<Node> &root)
{
    vector<uint32_t> leaf_ids;
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
            if (node.children.empty())
            {
                leaf_ids.push_back(node.id);
            }
            for (unsigned int i = 0; i < node.children.size(); i++)
            {
                q.push(node.children[i]);
            }
            n--;
        }
    }
    return leaf_ids;
}

/**
 * Load index tree structure from binary file and assign every point in given dataset to closest cluster
 * @param dataset_file_path Path to dataset binary file
 * @param index_file_path Path to index binary file
 */
int assign_points_to_cluster(string dataset_file_path, string index_file_path)
{
    // Read index from binary file
    ecp_index.open(index_file_path, ios::in | ios::binary);
    uint32_t num_nodes_to_read; // Number of nodes of the index
    ecp_index.read(reinterpret_cast<char *>(&num_nodes_to_read), sizeof(uint32_t));
    vector<Node> index;
    for (read_nodes; read_nodes < num_nodes_to_read;)
    {
        index.push_back(load_node());
    }

    // Allocate memory buffer
    binary_point *chunk{new binary_point[chunk_size]{}};       // Buffer for data points
    point_meta *point_meta_data{new point_meta[chunk_size]{}}; // Meta data for each data point in buffer

    // Open given input dataset binary file
    dataset.open(dataset_file_path, ios::in | ios::binary);

    // Read total number of points from bianry file
    uint32_t num_points;
    dataset.read((char *)&num_points, sizeof(uint32_t));

    // Calculate total number of chunks needed
    uint32_t num_chunks = num_points / chunk_size;

    for (int cur_chunk = 0; cur_chunk < num_chunks; cur_chunk++)
    {
        memset(chunk, 0, chunk_size * sizeof(binary_point));         // Reset buffer
        memset(point_meta_data, 0, chunk_size * sizeof(point_meta)); // Reset meta data for buffer

        // Read chunk into buffer
        dataset.seekg((sizeof(uint32_t) + sizeof(uint32_t) + (cur_chunk * chunk_size * sizeof(binary_point))), dataset.beg);
        dataset.read(reinterpret_cast<char *>(chunk), chunk_size * sizeof(binary_point));

        // Asign all points from chunk to a cluster leaf
        for (int i = 0; i < chunk_size; i++)
        {
            Node *cluster = find_nearest_leaf(chunk[i].descriptors, index);
            point_meta_data[i].buffer_position = i;
            point_meta_data[i].cluster_id = cluster->id;
            point_meta_data[i].point_id = (chunk_size * cur_chunk + i);
        }

        // Sort chunk meta data ascending after cluster_id
        sort(point_meta_data, point_meta_data + chunk_size, compare_cluster_id);

        // Write assignments to binary chunk
        fstream ecp_cluster;
        ecp_cluster.open("ecp_cluster_chunk_" + to_string(cur_chunk), ios::out | ios::binary);
        for (int i = 0; i < chunk_size; i++)
        {
            ecp_cluster.write(reinterpret_cast<char *>(&point_meta_data[i].cluster_id), sizeof(uint32_t));
            ecp_cluster.write(reinterpret_cast<char *>(&point_meta_data[i].point_id), sizeof(uint32_t));
            ecp_cluster.write(reinterpret_cast<char *>(&chunk[point_meta_data[i].buffer_position].descriptors), sizeof(binary_point));
        }
        ecp_cluster.close();
    }

    // Merge all chunks
    vector<uint32_t> leafs = find_all_leafs(index); // All cluster leaf ids in index
    vector<cluster_meta> ecp_cluster_meta_data;     // Meta data describing final database file of cluster assignments

    fstream ecp_clusters;
    ecp_clusters.open("ecp_clusters", ios::out | ios::binary);
    fstream ecp_chunk_file;

    // For every leaf in the index tree
    for (auto leaf : leafs)
    {
        cluster_meta cur_cluster_meta;
        cur_cluster_meta.cluster_id = leaf;                // Remeber cluster id
        cur_cluster_meta.offset = ecp_clusters.tellp();    // Remember starting position in binary file for current cluster id
        ecp_cluster_meta_data.push_back(cur_cluster_meta); // Save meta data for current cluster

        // Search every chunk file for all points assigned to current leaf
        for (int cur_chunk = 0; cur_chunk < num_chunks; cur_chunk++)
        {
            ecp_chunk_file.open("ecp_cluster_chunk_" + to_string(cur_chunk), ios::in | ios::binary);
            cluster_point cur_cluster_point;

            // Search every point in chunk file
            for (int i = 0; i < chunk_size; i++)
            {
                ecp_chunk_file.read((char *)&cur_cluster_point, sizeof(cluster_point));
                if (cur_cluster_point.cluster_id == leaf) // If point is assigned to current leaf
                {
                    ecp_clusters.write(reinterpret_cast<char *>(&cur_cluster_point), sizeof(cluster_point));
                }
            }
            ecp_chunk_file.close();
        }
    }

    ecp_clusters.close();

    // Delete all chunk files
    for (int cur_chunk = 0; cur_chunk < num_chunks; cur_chunk++)
    {
        filesystem::remove("ecp_cluster_chunk_" + to_string(cur_chunk));
    }

    return 0;
}