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

ifstream index_file_read;
ifstream cluster_metadata_read;
ifstream cluster_file_read;
uint32_t already_read = 0;

Node load_node()
{
    already_read++;
    Node node;
    uint32_t read_node_id;
    uint32_t read_point_id;
    vector<int8_t> read_descriptors(num_dimensions);
    uint32_t read_num_children;
    index_file_read.read(reinterpret_cast<char *>(&read_node_id), sizeof(uint32_t));
    index_file_read.read(reinterpret_cast<char *>(&read_point_id), sizeof(uint32_t));
    index_file_read.read(reinterpret_cast<char *>(read_descriptors.data()), sizeof(int8_t) * num_dimensions);
    index_file_read.read(reinterpret_cast<char *>(&read_num_children), sizeof(uint32_t));
    node.id = read_node_id;
    Point point;
    point.id = read_point_id;
    point.descriptors = read_descriptors;
    node.points.push_back(point);
    for (int i = 0; i < read_num_children; i++)
    {
        node.children.push_back(load_node());
    }
    return node;
}

int queryprocessing()
{
    // Read index from binary file
    index_file_read.open("index.i8bin", ios::in | ios::binary);
    uint32_t num_nodes_to_read;
    index_file_read.read(reinterpret_cast<char *>(&num_nodes_to_read), sizeof(uint32_t));
    vector<Node> read_tree;
    for (already_read; already_read < num_nodes_to_read;)
    {
        read_tree.push_back(load_node());
    }

    // Read cluster meta data from binary file
    cluster_metadata_read.open("cluster_metadata_0.i8bin", ios::in | ios::binary);
    uint32_t num_clu_node_to_read;
    cluster_metadata_read.read(reinterpret_cast<char *>(&num_clu_node_to_read), sizeof(uint32_t));
    vector<tuple<uint32_t, uint32_t>> meta;
    for (int i = 0; i < num_clu_node_to_read; i++)
    {
        uint32_t id;
        uint32_t n_points;
        cluster_metadata_read.read(reinterpret_cast<char *>(&id), sizeof(uint32_t));
        cluster_metadata_read.read(reinterpret_cast<char *>(&n_points), sizeof(uint32_t));
        meta.push_back(make_tuple(id, n_points));
    }

    return 0;
}