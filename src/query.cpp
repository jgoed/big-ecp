#include "datastructure.hpp"
#include "index.hpp"

using namespace std;

int process_query(string query_file_path, string index_file_path)
{
    // Read index from binary file
    vector<Node> index = load_index(index_file_path);

    return 0;
}