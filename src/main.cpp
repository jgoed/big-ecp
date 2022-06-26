#include "datastructure.hpp"
#include "index.hpp"
#include "assignment.hpp"
#include "query.hpp"

using namespace std;

int main()
{
    string dataset_file_path = "spacev1b_base.i8bin";
    string query_file_path = "private_query_30k.bin";

    string index_file_path = create_index(dataset_file_path, 3);

    assign_points_to_cluster(dataset_file_path, index_file_path, 125000);

    process_query(query_file_path, index_file_path);

    return 0;
}