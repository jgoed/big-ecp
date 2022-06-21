#include "datastructure.hpp"
#include "index.hpp"

#include <fstream>
#include <math.h>
#include <random>

using namespace std;

fstream dataset;
uint32_t num_points = 0;
uint32_t num_leaders = 0;
uint32_t unique_node_id = 0;
uint32_t num_nodes_in_index = 0;
fstream ecp_index;

/**
 * Generate a given amount of random unique numbers from 0 to max_id
 * @param amount Total amount of random unique numbers
 * @param max_number Highest possible random unique number
 */
vector<uint32_t> get_random_unique_numbers(uint32_t amount, uint32_t max_number)
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

/**
 * Read one point at a given position from binary file
 * @param position Position of the point to read
 */
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

/**
 * Read one node with one point at a given position from binary file
 * @param position Position of the node to read
 */
Node read_node_from_binary(uint32_t position)
{
    Node node;
    node.id = unique_node_id; // Assign every node a unique id
    unique_node_id++;
    node.points.push_back(read_point_from_binary(position));
    return node;
}

/**
 * Calculate euclidean distance for two points in n dimensional space
 * @param a Point A
 * @param b Point B
 */
float euclidean_distance(const int8_t *a, const int8_t *b)
{
    float sums[] = {0.0, 0.0, 0.0, 0.0};
    for (unsigned int i = 0; i < num_dimensions; ++i)
    {
        float delta = a[i] - b[i];
        sums[i % 4] += delta * delta; // Skip square_root because exact distance is not needed
    }
    return sums[0] + sums[1] + sums[2] + sums[3];
}

/**
 * Get the closest node from uncomplete index tree with a given deepth for a given query point
 * @param uncomplete_index Uncomplete index to search for closest node
 * @param query Query point
 * @param deepth Deepth of uncomplete index
 */
Node *get_closest_node_from_uncomplete_index(vector<Node> &uncomplete_index, int8_t *query, int deepth)
{
    float max = numeric_limits<float>::max();
    Node *closest = nullptr;
    for (Node &node : uncomplete_index)
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

/**
 * Write a node and all its children to binary file
 * @param node Node to write to binary file
 */
void save_node(Node node)
{
    num_nodes_in_index++;
    ecp_index.write(reinterpret_cast<char *>(&node.id), sizeof(uint32_t));
    ecp_index.write(reinterpret_cast<char *>(&node.points.at(0).id), sizeof(uint32_t));
    ecp_index.write(reinterpret_cast<char *>(node.points.at(0).descriptors.data()), sizeof(int8_t) * num_dimensions);
    uint32_t cur_num_children = node.children.size();
    ecp_index.write(reinterpret_cast<char *>(&cur_num_children), sizeof(uint32_t));
    // If node has children
    for (int i = 0; i < node.children.size(); i++)
    {
        save_node(node.children[i]);
    }
}

/**
 * Create an ecp index tree from given dataset with given amount of levels
 * @param dataset_file_path File path to input dataset
 * @param L Amount of levels defingin the deepth of the tree
 */
int create_index(string dataset_file_path, int L)
{
    // Open given input dataset binary file
    dataset.open(dataset_file_path, ios::in | ios::binary);

    // Read metadata from binary file
    dataset.read((char *)&num_points, sizeof(uint32_t));     // Total number of points with n demensions
    dataset.read((char *)&num_dimensions, sizeof(uint32_t)); // Total number of dimensions for one point

    // Calculate the overall number of leaders
    int desired_cluster_size = 512000;                                                                              // 512000 byte is default block size for SSDs
    num_leaders = ceil(num_points / (desired_cluster_size / (sizeof(int8_t) * num_dimensions + sizeof(uint32_t)))); // N/ts

    // Generate random leaders
    // NOTE: Currently not used due to make debugging easier, otherwise use this below: random_leader_ids.at(i)
    // vector<uint32_t> random_leader_ids = get_random_unique_numbers(num_leaders, num_points - 1);

    // Create index structure
    vector<Node> index;
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

    // Write index to binary file
    ecp_index.open("ecp_index_" + dataset_file_path, ios::out | ios::binary);
    ecp_index.write(reinterpret_cast<char *>(&num_nodes_in_index), sizeof(uint32_t));
    for (auto &node : index)
    {
        save_node(node);
    }
    ecp_index.seekp(0, ecp_index.beg);
    // Write total number of nodes in index at first position in binary file
    ecp_index.write(reinterpret_cast<char *>(&num_nodes_in_index), sizeof(uint32_t));
    ecp_index.close();

    return 0;
}