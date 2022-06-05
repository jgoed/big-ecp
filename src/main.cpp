#include <fstream>
#include <iostream>
#include <math.h>
#include <random>
#include <string>
#include <sstream>
#include <vector>
#include <bits/stdc++.h>

using namespace std;

uint32_t num_points;
uint32_t num_dimensions;
uint32_t num_leaders;

struct Point
{
    uint32_t id;
    vector<int8_t> descriptors;
};

struct Node
{
    vector<Node> children;
    vector<Point> points;
};

vector<uint32_t> get_random_unique_indexes(int amount, uint32_t container_size)
{
    vector<uint32_t> collected_samples;
    collected_samples.reserve(amount);

    unordered_set<uint32_t> visited_samples;
    random_device random_seed;        // Will be used to obtain a seed for the random number engine.
    mt19937 generator(random_seed()); // Standard mersenne_twister_engine seeded with rd().
    int start = container_size - amount;

    for (int j = start; j < container_size; ++j)
    {
        uniform_int_distribution<uint32_t> distribution(0, j); // To-from inclusive.
        uint32_t t = distribution(generator);

        unordered_set<uint32_t>::const_iterator iter = visited_samples.find(t);
        if (iter == visited_samples.end()) // Not found.
        {
            visited_samples.insert(t);
            collected_samples.emplace_back(t);
        }
        else // Found
        {
            visited_samples.insert(j);
            collected_samples.emplace_back(j);
        }
    }
    return collected_samples;
}

vector<vector<unsigned>> generate_leaders_indexes(size_t num_points, unsigned L)
{
    // Indexes picked randomly from input dataset used as leaders for each level
    vector<vector<unsigned>> random_leader_indexes(L);

    // Computing random indexes top-down, leader_indexes[0] is top level.
    for (unsigned cur_lvl = 1; cur_lvl <= L; cur_lvl++)
    {
        // Calculate level sizes (i.e. how many clusters for level L)
        int leaders_per_lvl = ceil(pow(num_leaders, ((1.0 / L)) * cur_lvl));
        // Pick random leaders for current level
        random_leader_indexes[cur_lvl - 1].reserve(leaders_per_lvl);
        random_leader_indexes[cur_lvl - 1] = get_random_unique_indexes(leaders_per_lvl, num_points);
    }

    return random_leader_indexes;
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

Node *get_closest_node(vector<Node> &nodes, int8_t *query)
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

int main()
{
    int L = 3;

    // Open dataset
    fstream dataset("spacev1b_base.i8bin", ios::in | ios::binary);
    if (!dataset)
    {
        cout << "Cannot open file!" << endl;
        return 1;
    }

    // Read metadata from binary file
    dataset.read((char *)&num_points, sizeof(uint32_t));
    dataset.read((char *)&num_dimensions, sizeof(uint32_t));

    // Calculate the overall number of leaders
    int desired_cluster_size = 512000;                                                                              // 512000 byte is default block size for SSDs
    num_leaders = ceil(num_points / (desired_cluster_size / (sizeof(int8_t) * num_dimensions + sizeof(uint32_t)))); // N/ts

    // leaders per lvl
    // int leaders_per_lvl = ceil(pow(num_leaders, ((1.0 / L)) * cur_lvl));

    // Generate random leaders
    // TODO: Not the right amount per level and all different INLCUDE FROM ABOVE AND MAKE SIMPLER AS VECTOR<int>
    // const auto random_leader_indexes = generate_leaders_indexes(num_points, L);
    vector<uint32_t> random_leader_indexes = get_random_unique_indexes(num_leaders, num_points);

    vector<Node> upper_level;

    for (int cur_lvl = 1; cur_lvl <= L; cur_lvl++)
    {
        int leaders_per_lvl = ceil(pow(num_leaders, ((1.0 / L)) * cur_lvl));
        vector<Node> current_level;

        for (int i = 0; i < leaders_per_lvl; i++)
        {
            // Read descriptors at index
            vector<int8_t> current_descriptors(num_dimensions);
            dataset.seekg((sizeof(num_points) + sizeof(num_dimensions) + random_leader_indexes.at(i) * num_dimensions * sizeof(int8_t)), dataset.beg);
            dataset.read(reinterpret_cast<char *>(current_descriptors.data()), sizeof(int8_t) * num_dimensions);

            // Create point with descriptors
            Point current_point;
            current_point.id = random_leader_indexes.at(i);
            current_point.descriptors = current_descriptors;

            // Create node with point
            Node current_node;
            current_node.points.push_back(current_point);
            if (cur_lvl == 1)
            {
                current_level.push_back(current_node);
            }

            // For every level after top
            if (upper_level.size() != 0)
            {
                // // Add all nodes from current level as children of upper level
                // for (auto node : current_level)
                // {
                //     auto clostest_node = get_closest_node(upper_level, &node.points[0].descriptors[0]);
                //     clostest_node->children.push_back(node);
                // }
            }
        }
        // upper_level.swap(current_level);
        break;
    }

    return 0;
}