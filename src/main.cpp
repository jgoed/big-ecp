#include "datastructure.hpp"
#include "index.hpp"
#include "assignment.hpp"
//#include "query.hpp"

using namespace std;

int main()
{
    string index_file_path = create_index("spacev1b_base.i8bin", 3);

    assign_points_to_cluster("spacev1b_base.i8bin", index_file_path);

    // queryprocessing();

    return 0;
}