#include "datastructure.hpp"
#include "distance.hpp"
#include "index.hpp"

#include <math.h>
#include <random>

using namespace std;

/**
 * Load node from given index binary file into memory
 */
Node load_node(fstream &index_file, uint32_t &read_nodes)
{
    read_nodes++;
    Node node;
    uint32_t num_children;
    index_file.read(reinterpret_cast<char *>(&node.id), sizeof(uint32_t));
    index_file.read(reinterpret_cast<char *>(&node.leader.id), sizeof(uint32_t));
    index_file.read(reinterpret_cast<char *>(node.leader.descriptors), sizeof(DATATYPE) * DIMENSIONS);
    index_file.read(reinterpret_cast<char *>(&num_children), sizeof(uint32_t));
    for (uint32_t i = 0; i < num_children; i++)
    {
        node.children.push_back(load_node(index_file, read_nodes));
    }
    return node;
}

/**
 * Load index from given index binary file into memory
 */
vector<Node> load_index(string index_file_path)
{
    fstream index_file;
    index_file.open(index_file_path, ios::in | ios::binary);
    uint32_t num_nodes_to_read = 0;
    index_file.read(reinterpret_cast<char *>(&num_nodes_to_read), sizeof(uint32_t));
    vector<Node> index;
    uint32_t read_nodes = 0;
    while (read_nodes < num_nodes_to_read)
    {
        index.push_back(load_node(index_file, read_nodes));
    }
    index_file.close();
    return index;
}

/**
 * Write node from in memory index in index binary file
 */
void save_node(fstream &index_file, Node node, uint32_t &num_nodes_in_index)
{
    num_nodes_in_index++;
    index_file.write(reinterpret_cast<char *>(&node.id), sizeof(uint32_t));
    index_file.write(reinterpret_cast<char *>(&node.leader.id), sizeof(uint32_t));
    index_file.write(reinterpret_cast<char *>(node.leader.descriptors), sizeof(DATATYPE) * DIMENSIONS);
    uint32_t cur_num_children = node.children.size();
    index_file.write(reinterpret_cast<char *>(&cur_num_children), sizeof(uint32_t));
    for (auto child : node.children)
    {
        save_node(index_file, child, num_nodes_in_index);
    }
}

/**
 * Get the closest node from uncomplete tree for given query point
 */
Node *get_closest_node_from_uncomplete_index(vector<Node> &uncomplete_index, int8_t *query, int deepth)
{
    float max = numeric_limits<float>::max();
    Node *closest = nullptr;
    for (Node &node : uncomplete_index)
    {
        const float distance = distance::g_distance_function(query, &node.leader.descriptors[0], max);
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

/**
 * Read point from input dataset binary file and create node for index with it
 */
Node create_node(fstream &dataset_file, uint32_t position, uint32_t &unique_node_id)
{
    Node node;
    node.id = unique_node_id;
    unique_node_id++;
    node.leader.id = position;
    dataset_file.seekg((sizeof(uint32_t) + sizeof(uint32_t) + position * sizeof(DATATYPE) * DIMENSIONS), dataset_file.beg);
    dataset_file.read(reinterpret_cast<char *>(node.leader.descriptors), sizeof(DATATYPE) * DIMENSIONS);
    return node;
}

/**
 * Create given amount of random numbers
 */
vector<uint32_t> create_random_unique_numbers(uint32_t amount, uint32_t max_number)
{
    vector<uint32_t> collected_samples;
    collected_samples.reserve(amount);
    unordered_set<uint32_t> visited_samples;
    random_device random_seed;
    mt19937 generator(random_seed());
    for (uint32_t i = 0; i < amount; i++)
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

/**
 * Create index for given dataset
 */
int create_index(string dataset_file_path, string ecp_dir_path, int L, int desired_cluster_size, int metric)
{
    fstream dataset_file;
    dataset_file.open(dataset_file_path, ios::in | ios::binary); // Open given input dataset binary file

    uint32_t num_points = 0;
    uint32_t num_dimensions = 0;
    dataset_file.read((char *)&num_points, sizeof(uint32_t));              // Total number of points with n dimensions
    dataset_file.read((char *)&num_dimensions, sizeof(uint32_t)); // Total number of dimensions for one point

    assert (num_dimensions == DIMENSIONS);

    auto cur_metric = static_cast<distance::Metric>(metric);
    distance::set_distance_function(cur_metric); // Set distance function globally

    fstream existing_index_file(ecp_dir_path + ECP_INDEX_FILE_NAME);
    if (existing_index_file.is_open())
    {
        cout << "ECP: NOT CREATING NEW INDEX, USING EXISTING FROM " << ecp_dir_path << endl;
        existing_index_file.close();
        dataset_file.close();
        return 0;
    }

    uint32_t num_leaders = ceil(num_points / (desired_cluster_size / (sizeof(DATATYPE) * DIMENSIONS + sizeof(uint32_t)))); // Calculate overall number of leaders

    vector<uint32_t> random_leader_ids = create_random_unique_numbers(num_leaders, num_points - 1);

    vector<Node> index; // Create index structure
    uint32_t unique_node_id = 0;

    for (uint32_t cur_lvl = 1; cur_lvl <= (uint32_t)L; cur_lvl++) // Go through each level
    {
        uint32_t cur_lvl_leaders = ceil(pow(num_leaders, ((1.0 / L)) * (cur_lvl))); // Calculate number of leaders for current level

        for (uint32_t i = 0; i < cur_lvl_leaders; i++) // Go through each leader of current level
        {
            if (cur_lvl == 1) // Top level
            {
                index.push_back(create_node(dataset_file, random_leader_ids[i], unique_node_id));
            }
            else // All other levels
            {
                Node current_node = create_node(dataset_file, random_leader_ids[i], unique_node_id);
                Node *clostest_node = get_closest_node_from_uncomplete_index(index, &current_node.leader.descriptors[0], cur_lvl - 1);
                clostest_node->children.push_back(current_node);
            }
        }
    }

    dataset_file.close(); // Close input dataset binary file

    uint32_t num_nodes_in_index = 0;
    string index_file_path = ecp_dir_path + ECP_INDEX_FILE_NAME; // Create path to index binary file
    fstream index_file;
    index_file.open(index_file_path, ios::out | ios::binary);                          // Open index binary file
    index_file.write(reinterpret_cast<char *>(&num_nodes_in_index), sizeof(uint32_t)); // Write placeholder for total number of nodes in index included
    for (auto &node : index)                                                           // Go through each node in index
    {
        save_node(index_file, node, num_nodes_in_index); // Write node in index binary file
    }
    index_file.seekp(0, index_file.beg);
    index_file.write(reinterpret_cast<char *>(&num_nodes_in_index), sizeof(uint32_t)); // Write total number of nodes in index at first position in binary file
    index_file.close();

    return 0;
}