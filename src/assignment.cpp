#include "datastructure.hpp"
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

Cluster_meta search_leaf_meta_data(std::vector<Cluster_meta> chunk_meta, uint32_t leaf)
{
    for (Cluster_meta i : chunk_meta)
    {
        if (i.cluster_id == leaf)
        {
            return i;
        }
    }
    cout << "ID NOT FOUND" << endl;
}

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

bool compare_cluster_id(Point_meta p1, Point_meta p2)
{
    if (p1.cluster_id < p2.cluster_id)
    {
        return true;
    }
    return false;
}

Node *get_closest_node(std::vector<Node> &nodes, int8_t *query)
{
    float max = numeric_limits<float>::max();
    Node *closest = nullptr;
    for (Node &node : nodes)
    {
        const float distance = euclidean_distance(query, &node.leader.descriptors[0]);

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

int assign_points_to_cluster(string dataset_file_path, std::string ecp_dir_path, unsigned int chunk_size)
{
    // Read index from binary file
    vector<Node> index = load_index(ecp_dir_path + "ecp_index.bin");

    // Open given input dataset binary file
    fstream dataset_file;
    dataset_file.open(dataset_file_path, ios::in | ios::binary);

    // Read total number of points from binary file
    uint32_t num_points;
    dataset_file.read((char *)&num_points, sizeof(uint32_t));

    // Calculate total number of chunks needed
    uint32_t num_chunks = num_points / chunk_size;

    // Allocate memory buffer
    Binary_point *chunk{new Binary_point[chunk_size]{}};       // Buffer for data points
    Point_meta *point_meta_data{new Point_meta[chunk_size]{}}; // Meta data for each data point in buffer

    vector<vector<Cluster_meta>> all_chunk_meta;

    for (int cur_chunk = 0; cur_chunk < num_chunks; cur_chunk++)
    {
        memset(chunk, 0, chunk_size * sizeof(Binary_point));         // Reset buffer
        memset(point_meta_data, 0, chunk_size * sizeof(Point_meta)); // Reset meta data for buffer

        // Read chunk into buffer
        dataset_file.seekg((sizeof(uint32_t) + sizeof(uint32_t) + (cur_chunk * chunk_size * sizeof(Binary_point))), dataset_file.beg);
        dataset_file.read(reinterpret_cast<char *>(chunk), chunk_size * sizeof(Binary_point));

        // Asign all points from chunk to a cluster leaf
        for (unsigned int i = 0; i < chunk_size; i++)
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
        chunk_file.open(ecp_dir_path + "ecp_chunk_" + to_string(cur_chunk) + ".bin", ios::out | ios::binary);
        for (unsigned int i = 0; i < chunk_size; i++)
        {
            chunk_file.write(reinterpret_cast<char *>(&point_meta_data[i].cluster_id), sizeof(uint32_t));
            chunk_file.write(reinterpret_cast<char *>(&point_meta_data[i].point_id), sizeof(uint32_t));
            chunk_file.write(reinterpret_cast<char *>(&chunk[point_meta_data[i].buffer_position].descriptors), sizeof(Binary_point));
        }
        chunk_file.close();

        vector<Cluster_meta> chunk_meta_data;
        uint32_t num_points = 0;
        uint32_t cur_offset = 0;

        // Write chunk meta data to binary
        for (unsigned int i = 0; i < chunk_size; i++)
        {
            if (point_meta_data[i].cluster_id == point_meta_data[i + 1].cluster_id)
            {
                num_points++;
            }
            else
            {
                num_points++;
                Cluster_meta cur_chunk_meta;
                cur_chunk_meta.cluster_id = point_meta_data[i].cluster_id;
                cur_chunk_meta.num_points_in_leaf = num_points;
                cur_chunk_meta.offset = cur_offset;
                cur_offset = cur_offset + num_points * sizeof(Cluster_point);
                chunk_meta_data.push_back(cur_chunk_meta);
                num_points = 0;
            }
        }
        all_chunk_meta.push_back(chunk_meta_data);
    }

    delete[] chunk;
    delete[] point_meta_data;

    dataset_file.close();

    // Merge all chunks
    vector<uint32_t> leafs = find_all_leafs(index); // All cluster leaf ids in index
    vector<Cluster_meta> ecp_cluster_meta_data;     // Meta data describing final database file of cluster assignments

    fstream cluster_file;
    cluster_file.open(ecp_dir_path + "ecp_clusters.bin", ios::out | ios::binary);
    fstream chunk_file;

    // For every leaf in the index tree
    for (auto leaf : leafs)
    {
        Cluster_meta cur_cluster_meta;
        cur_cluster_meta.cluster_id = leaf;             // Remeber cluster id
        cur_cluster_meta.offset = cluster_file.tellp(); // Remember starting position in binary file for current cluster id
        uint32_t cur_num_points = 0;

        // Search every chunk file for all points assigned to current leaf
        for (int cur_chunk = 0; cur_chunk < num_chunks; cur_chunk++)
        {

            Cluster_meta cur_leaf_meta = search_leaf_meta_data(all_chunk_meta[cur_chunk], leaf);

            Cluster_point *merge_buffer{new Cluster_point[cur_leaf_meta.num_points_in_leaf]{}};
            memset(merge_buffer, 0, cur_leaf_meta.num_points_in_leaf * sizeof(Cluster_point));

            chunk_file.open(ecp_dir_path + "ecp_chunk_" + to_string(cur_chunk) + ".bin", ios::in | ios::binary);

            chunk_file.seekg(cur_leaf_meta.offset, dataset_file.beg);
            chunk_file.read(reinterpret_cast<char *>(merge_buffer), cur_leaf_meta.num_points_in_leaf * sizeof(Cluster_point));
            cluster_file.write(reinterpret_cast<char *>(merge_buffer), cur_leaf_meta.num_points_in_leaf * sizeof(Cluster_point));
            cur_num_points = cur_num_points + cur_leaf_meta.num_points_in_leaf;

            // // Search every point in chunk file
            // for (unsigned int i = 0; i < chunk_size; i++)
            // {
            //     chunk_file.read((char *)&cur_cluster_point, sizeof(Cluster_point));
            //     if (cur_cluster_point.cluster_id == leaf) // If point is assigned to current leaf
            //     {
            //         cluster_file.write(reinterpret_cast<char *>(&cur_cluster_point), sizeof(Cluster_point));
            //         cur_num_points++;
            //     }
            // }
            chunk_file.close();
            delete[] merge_buffer;
        }
        cur_cluster_meta.num_points_in_leaf = cur_num_points;
        ecp_cluster_meta_data.push_back(cur_cluster_meta); // Save meta data for current cluster
    }

    cluster_file.close();

    // Write cluster meta data to binary file
    uint32_t num_leafs = leafs.size();
    fstream cluster_meta_file;
    string meta_data_file_path = ecp_dir_path + "ecp_cluster_meta.bin";
    cluster_meta_file.open(meta_data_file_path, ios::out | ios::binary);
    cluster_meta_file.write(reinterpret_cast<char *>(&num_leafs), sizeof(uint32_t));
    for (unsigned int i = 0; i < num_leafs; i++)
    {
        uint32_t cur_id = ecp_cluster_meta_data.at(i).cluster_id;
        uint32_t cur_num_points_in_leaf = ecp_cluster_meta_data.at(i).num_points_in_leaf;
        uint32_t cur_offset = ecp_cluster_meta_data.at(i).offset;
        cluster_meta_file.write(reinterpret_cast<char *>(&cur_id), sizeof(uint32_t));
        cluster_meta_file.write(reinterpret_cast<char *>(&cur_num_points_in_leaf), sizeof(uint32_t));
        cluster_meta_file.write(reinterpret_cast<char *>(&cur_offset), sizeof(uint32_t));
    }
    cluster_meta_file.close();

    return 0;
}