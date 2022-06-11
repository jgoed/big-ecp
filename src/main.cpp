#include <fstream>
#include <iostream>
#include <math.h>
#include <random>
#include <string>
#include <sstream>
#include <vector>
#include <bits/stdc++.h>

using namespace std;

fstream dataset;
ofstream index_file;

uint32_t num_points;
uint32_t num_dimensions;
uint32_t num_leaders;

uint32_t num_nodes_in_index = 0;

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

vector<uint32_t> get_random_unique_indexes(uint32_t amount, uint32_t max_index)
{
    vector<uint32_t> collected_samples;
    collected_samples.reserve(amount);
    unordered_set<uint32_t> visited_samples;
    random_device random_seed;
    mt19937 generator(random_seed());
    for (int i = 0; i < amount; i++)
    {
        uniform_int_distribution<uint32_t> distribution(0, max_index); // To-from inclusive
        uint32_t cur_rnd_num = distribution(generator);

        unordered_set<uint32_t>::const_iterator iter = visited_samples.find(cur_rnd_num);
        if (iter == visited_samples.end()) // Not found
        {
            visited_samples.insert(cur_rnd_num);
            collected_samples.emplace_back(cur_rnd_num);
        }
        else // Found
        {
            visited_samples.insert(i);
            collected_samples.emplace_back(i);
        }
    }
    return collected_samples;
}

Point read_point_from_binary(uint32_t index)
{
    vector<int8_t> descriptors(num_dimensions);
    dataset.seekg((sizeof(num_points) + sizeof(num_dimensions) + index * num_dimensions * sizeof(int8_t)), dataset.beg);
    dataset.read(reinterpret_cast<char *>(descriptors.data()), sizeof(int8_t) * num_dimensions);
    Point point;
    point.id = index;
    point.descriptors = descriptors;
    return point;
}

Node read_node_from_binary(uint32_t index)
{
    Node node;
    node.points.push_back(read_point_from_binary(index));
    return node;
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

Node *get_closest_node_from_uncomplete_tree(vector<Node> &nodes, int8_t *query, int deepth)
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
    if (deepth > 1)
    {
        closest = get_closest_node_from_uncomplete_tree(closest->children, query, deepth - 1);
    }
    return closest;
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

bool is_leaf(Node &node)
{
    return node.children.empty();
}

void print_index_levels(vector<Node> &root)
{
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
            if (is_leaf(node))
            {
                cout << " [L: " << node.points.size() << "] ";
            }
            else
                cout << " [N: " << node.children.size() << "] ";
            for (unsigned int i = 0; i < node.children.size(); i++)
                q.push(node.children[i]);
            n--;
        }
        cout << "\n--------------\n";
    }
}

void write_node_to_binary(Node node)
{
    uint32_t cur_id = node.points[0].id;
    vector<int8_t> cur_descriptors = node.points.at(0).descriptors;
    uint32_t cur_num_children = node.children.size();
    vector<Node> cur_children = node.children;
    index_file.write(reinterpret_cast<char *>(&cur_id), sizeof(uint32_t));
    index_file.write(reinterpret_cast<char *>(cur_descriptors.data()), sizeof(int8_t) * node.points[0].descriptors.size());
    index_file.write(reinterpret_cast<char *>(&cur_num_children), sizeof(cur_num_children));
    num_nodes_in_index++;
}

int main()
{
    int L = 3;

    // Open binary file
    dataset.open("spacev1b_base.i8bin", ios::in | ios::binary);

    // Read metadata from binary file
    dataset.read((char *)&num_points, sizeof(uint32_t));
    dataset.read((char *)&num_dimensions, sizeof(uint32_t));

    // Calculate the overall number of leaders
    int desired_cluster_size = 512000;                                                                              // 512000 byte is default block size for SSDs
    num_leaders = ceil(num_points / (desired_cluster_size / (sizeof(int8_t) * num_dimensions + sizeof(uint32_t)))); // N/ts

    // Generate random leaders
    // random_leader_indexes.at(i) vector<uint32_t> random_leader_indexes = get_random_unique_indexes(num_leaders, num_points - 1);

    // Tree index structure
    vector<Node> tree;

    for (int cur_lvl = 1; cur_lvl <= L; cur_lvl++)
    {
        // Calculate number of leaders for current level
        int cur_lvl_leaders = ceil(pow(num_leaders, ((1.0 / L)) * (cur_lvl)));

        if (cur_lvl == 1) // If current level is top level
        {
            vector<Node> current_level;
            for (int i = 0; i < cur_lvl_leaders; i++)
            {
                current_level.push_back(read_node_from_binary(i));
            }
            tree.swap(current_level);
        }
        else // Every other level below top level
        {
            for (int i = 0; i < cur_lvl_leaders; i++)
            {
                Node current_node = read_node_from_binary(i);
                Node *clostest_node = get_closest_node_from_uncomplete_tree(tree, &current_node.points[0].descriptors[0], cur_lvl - 1);
                clostest_node->children.push_back(current_node);
            }
        }
    }

    // Add all points from input dataset to the index incl those duplicated in the index construction.
    for (uint32_t cur_index = 0; cur_index < num_points; cur_index++)
    {
        Point cur_point = read_point_from_binary(cur_index);
        auto *leaf = find_nearest_leaf(cur_point.descriptors.data(), tree);
        if (cur_index != leaf->points[0].id) // Prevent adding leader twice
        {
            leaf->points.emplace_back(cur_point);
        }
    }

    index_file.open("index.i8bin", ios::out | ios::binary);

    index_file.write(reinterpret_cast<char *>(&num_nodes_in_index), sizeof(uint32_t));

    queue<Node> q;
    for (auto &node : tree)
    {
        q.push(node);
    }
    while (!q.empty())
    {
        int n = q.size();
        while (n > 0)
        {
            Node node = q.front();
            q.pop();
            write_node_to_binary(node);
            // cout << "id: " << node.points[0].id << " , num_child: " << node.children.size() << " | ";
            if (!is_leaf(node))
            {
                for (unsigned int i = 0; i < node.children.size(); i++)
                {
                    q.push(node.children[i]);
                    // cout << "child_id: " << node.children[i].points[0].id << ", ";
                    write_node_to_binary(node.children[i]);
                }
            }
            // cout << endl;

            n--;
        }
    }
    index_file.seekp(0, index_file.beg);
    index_file.write(reinterpret_cast<char *>(&num_nodes_in_index), sizeof(uint32_t));
    index_file.close();

    int teste = 0;

    ifstream index_file_read;
    index_file_read.open("index.i8bin", ios::in | ios::binary);
    uint32_t num_nodes_to_read;
    index_file_read.read(reinterpret_cast<char *>(&num_nodes_to_read), sizeof(uint32_t));
    for (int i = 0; i < num_nodes_to_read; i++)
    {
        teste++;
    }
    cout << teste << endl;

    uint32_t testid;
    vector<int8_t> testvec(num_dimensions);
    uint32_t test_num_chil;
    // index_file_read.seekg(0, index_file_read.beg);
    index_file_read.read(reinterpret_cast<char *>(&testid), sizeof(testid));
    index_file_read.read(reinterpret_cast<char *>(testvec.data()), sizeof(int8_t) * num_dimensions);
    index_file_read.read(reinterpret_cast<char *>(&test_num_chil), sizeof(uint32_t));

    // print_index_levels(tree);

    return 0;
}