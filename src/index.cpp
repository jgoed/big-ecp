#include "datastructure.hpp"
#include "distance.hpp"
#include "index.hpp"

#include <math.h>
#include <random>

using namespace std;

Node load_node(fstream &index_file, uint32_t &read_nodes)
{
    read_nodes++;
    Node node;
    uint32_t num_children;
    index_file.read(reinterpret_cast<char *>(&node.id), sizeof(uint32_t));
    index_file.read(reinterpret_cast<char *>(&node.leader.id), sizeof(uint32_t));
    index_file.read(reinterpret_cast<char *>(node.leader.descriptors), global_point_size);
    index_file.read(reinterpret_cast<char *>(&num_children), sizeof(uint32_t));
    for (int i = 0; i < (int)num_children; i++)
    {
        node.children.push_back(load_node(index_file, read_nodes));
    }
    return node;
}

vector<Node> load_index(string index_file_path)
{
    fstream index_file;
    index_file.open(index_file_path, ios::in | ios::binary);
    uint32_t num_nodes_to_read = 0;
    index_file.read(reinterpret_cast<char *>(&num_nodes_to_read), sizeof(uint32_t));
    vector<Node> index;
    uint32_t read_nodes = 0;
    for (read_nodes; read_nodes < num_nodes_to_read;)
    {
        index.push_back(load_node(index_file, read_nodes));
    }
    index_file.close();
    return index;
}

void save_node(fstream &index_file, Node node, uint32_t &num_nodes_in_index)
{
    num_nodes_in_index++;
    index_file.write(reinterpret_cast<char *>(&node.id), sizeof(uint32_t));
    index_file.write(reinterpret_cast<char *>(&node.leader.id), sizeof(uint32_t));
    index_file.write(reinterpret_cast<char *>(node.leader.descriptors), global_point_size);
    uint32_t cur_num_children = node.children.size();
    index_file.write(reinterpret_cast<char *>(&cur_num_children), sizeof(uint32_t));
    for (int i = 0; i < (int)node.children.size(); i++)
    {
        save_node(index_file, node.children[i], num_nodes_in_index);
    }
}

Node *get_closest_node_from_uncomplete_index(vector<Node> &uncomplete_index, int8_t *query, int deepth)
{
    float max = numeric_limits<float>::max();
    Node *closest = nullptr;
    for (Node &node : uncomplete_index)
    {
        const float distance = euclidean_distance(query, &node.leader.descriptors[0]);
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

Node create_node(fstream &dataset_file, uint32_t position, uint32_t &unique_node_id)
{
    Node node;
    node.id = unique_node_id;
    unique_node_id++;
    node.leader.id = position;
    dataset_file.seekg((sizeof(uint32_t) + sizeof(uint32_t) + position * global_point_size), dataset_file.beg);
    dataset_file.read(reinterpret_cast<char *>(node.leader.descriptors), global_point_size);
    return node;
}

vector<uint32_t> create_random_unique_numbers(int amount, uint32_t max_number)
{
    vector<uint32_t> collected_samples;
    collected_samples.reserve(amount);
    unordered_set<uint32_t> visited_samples;
    random_device random_seed;
    mt19937 generator(random_seed());
    for (int i = 0; i < amount; i++)
    {
        uniform_int_distribution<uint32_t> distribution(0, max_number);
        uint32_t cur_rnd_num = distribution(generator);
        unordered_set<uint32_t>::const_iterator iter = visited_samples.find(cur_rnd_num);
        if (iter == visited_samples.end())
        {
            visited_samples.insert(cur_rnd_num);
            collected_samples.emplace_back(cur_rnd_num);
        }
        else
        {
            visited_samples.insert(i);
            collected_samples.emplace_back(i);
        }
    }
    return collected_samples;
}

string create_index(string dataset_file_path, string ecp_dir_path, int L, int desired_cluster_size)
{
    // Open given input dataset binary file
    fstream dataset_file;
    dataset_file.open(dataset_file_path, ios::in | ios::binary);

    // Read metadata from binary file
    uint32_t num_points = 0;
    dataset_file.read((char *)&num_points, sizeof(uint32_t));            // Total number of points with n demensions
    dataset_file.read((char *)&global_num_dimensions, sizeof(uint32_t)); // Total number of dimensions for one point
    global_point_size = sizeof(uint8_t) * global_num_dimensions;

    // Calculate the overall number of leaders
    //int desired_cluster_size = 512000;                                                                         // 512000 byte is default block size for SSDs
    int num_leaders = ceil(num_points / (desired_cluster_size / (global_point_size + sizeof(uint32_t)))); // N/ts

    // Generate random leaders
    // NOTE: Currently not used due to make debugging easier, otherwise use this below: random_leader_ids.at(i)
    // vector<uint32_t> random_leader_ids = create_random_unique_numbers(num_leaders, num_points - 1);

    // Create index structure
    vector<Node> index;
    uint32_t unique_node_id = 0;

    // Go through each level
    for (int cur_lvl = 1; cur_lvl <= L; cur_lvl++)
    {
        // Calculate number of leaders for current level
        int cur_lvl_leaders = ceil(pow(num_leaders, ((1.0 / L)) * (cur_lvl)));
        // Go through each leader of current level
        for (int i = 0; i < cur_lvl_leaders; i++)
        {
            if (cur_lvl == 1)
            {
                index.push_back(create_node(dataset_file, i, unique_node_id));
            }
            else
            {
                Node current_node = create_node(dataset_file, i, unique_node_id);
                Node *clostest_node = get_closest_node_from_uncomplete_index(index, &current_node.leader.descriptors[0], cur_lvl - 1);
                clostest_node->children.push_back(current_node);
            }
        }
    }
    dataset_file.close();

    // Write index to binary file
    uint32_t num_nodes_in_index = 0;
    string index_file_path = ecp_dir_path + "ecp_index.bin";
    fstream index_file;
    index_file.open(index_file_path, ios::out | ios::binary);
    index_file.write(reinterpret_cast<char *>(&num_nodes_in_index), sizeof(uint32_t));
    for (auto &node : index)
    {
        save_node(index_file, node, num_nodes_in_index);
    }
    index_file.seekp(0, index_file.beg);
    // Write total number of nodes in index at first position in binary file
    index_file.write(reinterpret_cast<char *>(&num_nodes_in_index), sizeof(uint32_t));
    index_file.close();

    return index_file_path;
}