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
- ...

### query.cpp
- ...