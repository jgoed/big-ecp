# big-ecp

## Structure

### main.cpp
- Starts three separate tasks:
    - Index creation
    - Point assignment
    - Query processing

### datastructure.cpp
- Contains structs and some variabls (maybe get rid of them) globaly needed

### distance.cpp
- Contains distance metric function
- More different version will be added in the future

### index.cpp
- Reads metadata from input dataset
- Calculates overall number of leaders for the index
- Generates random numbers (currently disabled for easier debugging)
- Builds index by going through each level
- Writes index to binary file

### assignment.cpp
- Loads index from binary file
- Allocates memory chunk buffer, currently in terms of how many points should be in one chunk
- Calculates how many chunks are needed for the whole dataset
- For every chunk: Resets buffer, reads new data into buffer from binary, assigns all points to closest leaf, sorts meta_data array after cluster_id(leaf_id), writes all data in binary chunk (cluster_id, point_id, descriptors)

### query.cpp
- ...