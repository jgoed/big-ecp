#include "assignment.hpp"
#include "datastructures.hpp"
#include "distance.hpp"
#include "index.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <math.h>
#include <random>
#include <stdio.h>
#include <string>
#include <sstream>

using namespace std;

/**
 * Assign all points of given input dataset to leafs of given index and write it down in binary file
 */
void assign_points_to_cluster(string dataset_file_path, string ecp_dir_path, int num_chunks, int metric)
{
    fstream existing_clusters_file(ecp_dir_path + ECP_CLUSTERS_FILE_NAME);
    if (existing_clusters_file.is_open())
    {
        cout << "ECP: NOT CREATING NEW CLUSTERS, USING EXISTING CLUSTERS FILE FROM " << ecp_dir_path << endl;
        existing_clusters_file.close();
        return;
    }

    vector<Node> index = load_index(ecp_dir_path + ECP_INDEX_FILE_NAME, metric); // Read index from binary file

    fstream dataset_file;
    dataset_file.open(dataset_file_path, ios::in | ios::binary); // Open given input dataset binary file
    assert(dataset_file.fail() == false);                        // Abort if file can not be opened

    uint32_t num_points = 0;
    uint32_t num_dimensions = 0;
    dataset_file.read((char *)&num_points, sizeof(uint32_t));     // Read total number of points from binary file
    dataset_file.read((char *)&num_dimensions, sizeof(uint32_t)); // Total number of dimensions for one point

    assert(num_dimensions == DIMENSIONS);

    uint32_t chunk_size = num_points / num_chunks; // Calculate size of one chunk

    BinaryPoint *chunk{new BinaryPoint[chunk_size]{}};       // Buffer for data points
    PointMeta *point_meta_data{new PointMeta[chunk_size]{}}; // Meta data for each data point in buffer

    vector<vector<ClusterMeta>> all_chunk_meta; // Vector containing meta data of each chunk file

    ///////////////////////////////
    // CREATE BINARY CHUNK FILES //
    ///////////////////////////////

    for (int cur_chunk = 0; cur_chunk < num_chunks; cur_chunk++) // Go through each needed chunk
    {
        memset(chunk, 0, chunk_size * sizeof(BinaryPoint));         // Reset buffer
        memset(point_meta_data, 0, chunk_size * sizeof(PointMeta)); // Reset meta data for buffer

        dataset_file.seekg((sizeof(uint32_t) + sizeof(uint32_t) + (cur_chunk * chunk_size * sizeof(BinaryPoint))), dataset_file.beg); // Jump to begin of current chunk
        dataset_file.read(reinterpret_cast<char *>(chunk), chunk_size * sizeof(BinaryPoint));                                         // Read chunk into buffer

        for (unsigned int i = 0; i < chunk_size; i++) // Assign all points from chunk to a cluster leaf
        {
            Node *cluster = find_nearest_leaf(chunk[i].descriptors, index);
            point_meta_data[i].buffer_position = i;
            point_meta_data[i].cluster_id = cluster->node_id;
            point_meta_data[i].point_id = (chunk_size * cur_chunk + i);
        }

        sort(point_meta_data, point_meta_data + chunk_size, compare_cluster_id); // Sort chunk meta data ascending after cluster_id

        fstream chunk_file;
        chunk_file.open(ecp_dir_path + "ecp_chunk_" + to_string(cur_chunk) + ".bin", ios::out | ios::binary);
        assert(chunk_file.fail() == false); // Abort if file can not be opened

        for (unsigned int i = 0; i < chunk_size; i++) // Write assignments to binary chunk file
        {
            chunk_file.write(reinterpret_cast<char *>(&point_meta_data[i].cluster_id), sizeof(uint32_t));
            chunk_file.write(reinterpret_cast<char *>(&point_meta_data[i].point_id), sizeof(uint32_t));
            chunk_file.write(reinterpret_cast<char *>(&chunk[point_meta_data[i].buffer_position].descriptors), sizeof(BinaryPoint));
        }

        chunk_file.close();

        vector<ClusterMeta> chunk_meta_data;
        uint32_t num_points = 0;
        uint32_t cur_offset = 0;

        for (uint32_t i = 0; i < chunk_size; i++) // Write chunk meta data to binary
        {
            if (point_meta_data[i].cluster_id == point_meta_data[i + 1].cluster_id) // Continue as long as cluster id is the same
            {
                num_points++;
            }
            else // If next cluster id begins
            {
                num_points++;
                ClusterMeta cur_chunk_meta;
                cur_chunk_meta.cluster_id = point_meta_data[i].cluster_id;
                cur_chunk_meta.num_points_in_leaf = num_points;
                cur_chunk_meta.offset = cur_offset;
                cur_offset = cur_offset + num_points * sizeof(ClusterPoint);
                chunk_meta_data.push_back(cur_chunk_meta);
                num_points = 0;
            }
        }

        all_chunk_meta.push_back(chunk_meta_data);
    }

    delete[] chunk;
    delete[] point_meta_data;

    dataset_file.close();

    ///////////////////////////
    // MERGE ALL CHUNK FILES //
    ///////////////////////////

    vector<uint32_t> leafs = find_all_leafs(index); // All cluster leaf ids in index
    vector<ClusterMeta> ecp_cluster_meta_data; // Meta data describing final database file of cluster assignments

    fstream cluster_file;
    cluster_file.open(ecp_dir_path + ECP_CLUSTERS_FILE_NAME, ios::out | ios::binary);
    assert(cluster_file.fail() == false); // Abort if file can not be opened

    for (auto leaf : leafs) // Go through each leaf in index tree
    {
        ClusterMeta cur_cluster_meta;
        cur_cluster_meta.cluster_id = leaf;
        cur_cluster_meta.offset = cluster_file.tellp(); // Remember starting position in binary file for current cluster id
        uint32_t cur_num_points = 0;

        for (int cur_chunk = 0; cur_chunk < num_chunks; cur_chunk++) // Search every chunk file for assignments to current leaf
        {
            ClusterMeta cur_leaf_meta;
            bool id_found = search_leaf_meta_data(all_chunk_meta[cur_chunk], leaf, cur_leaf_meta); // Check if current chunk file contains assignments to current leaf

            if (id_found) // If leaf has no assignments in that chunk skip it
            {
                ClusterPoint *merge_buffer{new ClusterPoint[cur_leaf_meta.num_points_in_leaf]{}}; // Merge buffer
                memset(merge_buffer, 0, cur_leaf_meta.num_points_in_leaf * sizeof(ClusterPoint)); // Reset merge buffer

                fstream chunk_file;
                chunk_file.open(ecp_dir_path + "ecp_chunk_" + to_string(cur_chunk) + ".bin", ios::in | ios::binary);
                assert(chunk_file.fail() == false); // Abort if file can not be opened
                chunk_file.seekg(cur_leaf_meta.offset, dataset_file.beg);
                chunk_file.read(reinterpret_cast<char *>(merge_buffer), cur_leaf_meta.num_points_in_leaf * sizeof(ClusterPoint));    // Read leaf part of chunk in buffer
                cluster_file.write(reinterpret_cast<char *>(merge_buffer), cur_leaf_meta.num_points_in_leaf * sizeof(ClusterPoint)); // Write leaf part from buffer in final clusters database
                cur_num_points = cur_num_points + cur_leaf_meta.num_points_in_leaf;
                chunk_file.close();

                delete[] merge_buffer;
            }
        }

        cur_cluster_meta.num_points_in_leaf = cur_num_points;
        ecp_cluster_meta_data.push_back(cur_cluster_meta); // Save meta data for current cluster
    }

    cluster_file.close();

    /////////////////////////////////////////////
    // WRITE FINAL CLUSTER META DATA TO BINARY //
    /////////////////////////////////////////////

    uint32_t num_leafs = leafs.size();
    fstream cluster_meta_file;
    string meta_data_file_path = ecp_dir_path + ECP_CLUSTER_META_FILE_NAME;
    cluster_meta_file.open(meta_data_file_path, ios::out | ios::binary);
    assert(cluster_file.fail() == false); // Abort if file can not be opened
    uint32_t binary_metric = metric;
    cluster_meta_file.write(reinterpret_cast<char *>(&binary_metric), sizeof(uint32_t)); // Write down which metric was used to create assignments
    cluster_meta_file.write(reinterpret_cast<char *>(&num_leafs), sizeof(uint32_t));

    for (uint32_t i = 0; i < num_leafs; i++)
    {
        uint32_t cur_id = ecp_cluster_meta_data[i].cluster_id;
        uint32_t cur_num_points_in_leaf = ecp_cluster_meta_data[i].num_points_in_leaf;
        uint32_t cur_offset = ecp_cluster_meta_data[i].offset;
        cluster_meta_file.write(reinterpret_cast<char *>(&cur_id), sizeof(uint32_t));
        cluster_meta_file.write(reinterpret_cast<char *>(&cur_num_points_in_leaf), sizeof(uint32_t));
        cluster_meta_file.write(reinterpret_cast<char *>(&cur_offset), sizeof(uint32_t));
    }
    cluster_meta_file.close();

    return;
}

/**
 * Find nearest leaf for given query point
 */
Node *find_nearest_leaf(DATATYPE *query, vector<Node> &nodes)
{
    Node *closest_cluster = get_closest_node(nodes, query);
    if (!closest_cluster->children.empty())
    {
        return find_nearest_leaf(query, closest_cluster->children);
    }
    return closest_cluster;
}

/**
 * Get closest node from given vector of nodes
 */
Node *get_closest_node(vector<Node> &nodes, DATATYPE *query)
{
    float max = numeric_limits<float>::max();
    Node *closest = nullptr;
    for (Node &node : nodes)
    {
        const float distance = distance::g_distance_function(query, &node.leader.descriptors[0], max);

        if (distance < max)
        {
            max = distance;
            closest = &node;
        }
    }
    return closest;
}

/**
 * Compare two cluster ids
 */
bool compare_cluster_id(PointMeta p1, PointMeta p2)
{
    if (p1.cluster_id < p2.cluster_id)
    {
        return true;
    }
    return false;
}

/**
 * Find all leafs in a given index
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
                leaf_ids.push_back(node.node_id);
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
 * Search if there is assignment data for given leaf in given chunk meta data
 */
bool search_leaf_meta_data(vector<ClusterMeta> chunk_meta, uint32_t leaf, ClusterMeta &cur_leaf_meta)
{
    for (ClusterMeta i : chunk_meta)
    {
        if (i.cluster_id == leaf)
        {
            cur_leaf_meta = i;
            return true;
        }
    }
    return false;
}