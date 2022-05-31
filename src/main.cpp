#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

/* Global variables */
vector<vector<int8_t>> vector_set;

/* Create the index */
int create_index()
{
    
    return 0;
}

/* Search k nearest neigbhors for a given query point q */
int query()
{
    return 0;
}

/* Read the binary file in a global vector of vectors */
int read_binary_file(string filename)
{
    ifstream dataset(filename, ios::in | ios::binary);

    if (!dataset)
    {
        cout << "Cannot open file!" << endl;
        return 1;
    }

    uint32_t num_points;
    uint32_t num_dimensions;

    dataset.read((char *)&num_points, sizeof(uint32_t));
    dataset.read((char *)&num_dimensions, sizeof(uint32_t));

    cout << "num_points:" << num_points << endl;
    cout << "num_dimensions:" << num_dimensions << endl;

    for (int i = 0; i < num_points; i++)
    {
        vector<int8_t> vector_point(num_dimensions);
        dataset.read(reinterpret_cast<char *>(vector_point.data()), sizeof(int8_t) * num_dimensions);
        vector_set.push_back(vector_point);
    }

    dataset.close();

    if (vector_set.size() != num_points)
    {
        cout << "Error while reading the binary file!" << endl;
    }

    vector_set.shrink_to_fit();
    cout << (int)vector_set[100000][99] << endl;
    return 0;
}

int main()
{
    // int L = 2;
    // int metric = 0;
    // int k = 2;
    // int b = 2;
    // int p = 1000;
    // int d = 128;
    // int r = 1000;
    // int qs = 15;

    // vector<vector<int>> S;
    // vector<vector<int>> q;

    read_binary_file("spacev1b_base.i8bin");

    // ofstream wf("test.bin", ios::out | ios::binary);

    // if (!wf)
    // {
    //     cout << "Cannot open file!" << endl;
    //     return 1;
    // }

    // uint32_t num = 4294967293;

    // wf.write((char *)&num, sizeof(int));

    // wf.close();

    // if (!wf.good())
    // {
    //     cout << "Error occurred at writing time!" << endl;
    //     return 1;
    // }

    // ifstream rf("test.bin", ios::in | ios::binary);

    // if (!rf)
    // {
    //     cout << "Cannot open file!" << endl;
    //     return 1;
    // }

    // uint32_t res;

    // rf.read((char *)&res, sizeof(int));

    // rf.close();

    // cout << res << endl;

    return 0;
}