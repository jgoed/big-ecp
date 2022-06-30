#include "datastructure.hpp"
#include "index.hpp"
#include "assignment.hpp"
#include "query.hpp"

using namespace std;

int main()
{
    string dataset_file_path = "spacev1b_base.i8bin";
    int L = 3;
    string query_file_path = "query.i8bin";

    string index_file_path = create_index(dataset_file_path, L);
    string meta_data_file_path = assign_points_to_cluster(dataset_file_path, index_file_path, 5000000);
    process_query(query_file_path, index_file_path, meta_data_file_path);

    return 0;
}