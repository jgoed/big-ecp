#include "datastructure.hpp"
#include "distance.hpp"
#include "index.hpp"

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
 * @param chunk_size Number of points per chunk
 */
int assign_points_to_cluster(string dataset_file_path, string index_file_path, uint32_t chunk_size)
{
    // Read index from binary file
    vector<Node> index = load_index(index_file_path);

    // Allocate memory buffer
    binary_point *chunk{new binary_point[chunk_size]{}};       // Buffer for data points
    point_meta *point_meta_data{new point_meta[chunk_size]{}}; // Meta data for each data point in buffer

    // Open given input dataset binary file
    fstream dataset_file;
    dataset_file.open(dataset_file_path, ios::in | ios::binary);

    // Read total number of points from bianry file
    uint32_t num_points;
    dataset_file.read((char *)&num_points, sizeof(uint32_t));

    // Calculate total number of chunks needed
    uint32_t num_chunks = num_points / chunk_size;

    for (int cur_chunk = 0; cur_chunk < num_chunks; cur_chunk++)
    {
        memset(chunk, 0, chunk_size * sizeof(binary_point));         // Reset buffer
        memset(point_meta_data, 0, chunk_size * sizeof(point_meta)); // Reset meta data for buffer

        // Read chunk into buffer
        dataset_file.seekg((sizeof(uint32_t) + sizeof(uint32_t) + (cur_chunk * chunk_size * sizeof(binary_point))), dataset_file.beg);
        dataset_file.read(reinterpret_cast<char *>(chunk), chunk_size * sizeof(binary_point));

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
        fstream chunk_file;
        chunk_file.open("ecp_chunk_" + to_string(cur_chunk) + ".bin", ios::out | ios::binary);
        for (int i = 0; i < chunk_size; i++)
        {
            chunk_file.write(reinterpret_cast<char *>(&point_meta_data[i].cluster_id), sizeof(uint32_t));
            chunk_file.write(reinterpret_cast<char *>(&point_meta_data[i].point_id), sizeof(uint32_t));
            chunk_file.write(reinterpret_cast<char *>(&chunk[point_meta_data[i].buffer_position].descriptors), sizeof(binary_point));
        }
        chunk_file.close();
    }

    delete [] chunk;
    delete [] point_meta_data;

    dataset_file.close();

    // Merge all chunks
    vector<uint32_t> leafs = find_all_leafs(index); // All cluster leaf ids in index
    vector<cluster_meta> ecp_cluster_meta_data;     // Meta data describing final database file of cluster assignments

    fstream cluster_file;
    cluster_file.open("ecp_clusters.bin", ios::out | ios::binary);
    fstream chunk_file;

    // For every leaf in the index tree
    for (auto leaf : leafs)
    {
        cluster_meta cur_cluster_meta;
        cur_cluster_meta.cluster_id = leaf;                // Remeber cluster id
        cur_cluster_meta.offset = cluster_file.tellp();    // Remember starting position in binary file for current cluster id
        ecp_cluster_meta_data.push_back(cur_cluster_meta); // Save meta data for current cluster

        // Search every chunk file for all points assigned to current leaf
        for (int cur_chunk = 0; cur_chunk < num_chunks; cur_chunk++)
        {
            chunk_file.open("ecp_chunk_" + to_string(cur_chunk) + ".bin", ios::in | ios::binary);
            cluster_point cur_cluster_point;

            // Search every point in chunk file
            for (int i = 0; i < chunk_size; i++)
            {
                chunk_file.read((char *)&cur_cluster_point, sizeof(cluster_point));
                if (cur_cluster_point.cluster_id == leaf) // If point is assigned to current leaf
                {
                    cluster_file.write(reinterpret_cast<char *>(&cur_cluster_point), sizeof(cluster_point));
                }
            }
            chunk_file.close();
        }
    }

    cluster_file.close();

    // Delete all chunk files
    for (int cur_chunk = 0; cur_chunk < num_chunks; cur_chunk++)
    {
        filesystem::remove("ecp_chunk_" + to_string(cur_chunk) + ".bin");
    }

    return 0;
}