# big-ecp

This repository contains a version of the Extended Cluster Pruning algorithm, which is designed to participate in the Big-ANN-Benchmark.

# Source code roadmap
<pre><font color="#2A7BDE"><b>.</b></font>
├── <font color="#2A7BDE"><b>benchmark</b></font> // All files needed by benchmark
│   ├── algos.yaml
│   ├── <font color="#33DA7A"><b>Dockerfile.ecp</b></font>
│   └── ecp-t2.py
├── CMakeLists.txt // Top level CMake file
├── <font color="#2A7BDE"><b>include</b></font>
│   └── <font color="#2A7BDE"><b>ecp</b></font>
│       └── ecp.hpp // Include header of ecp library
├── LICENSE
├── <font color="#2A7BDE"><b>main</b></font>
│   ├── CMakeLists.txt
│   └── main.cpp // Main function for debugging using ecp library
├── README.md
├── <font color="#2A7BDE"><b>scripts</b></font>
│   ├── <font color="#33DA7A"><b>build.sh</b></font> // Build project
│   ├── <font color="#33DA7A"><b>configure.sh</b></font> // Configures project
│   ├── <font color="#33DA7A"><b>copy_files.sh</b></font> // Copy necessary files to benchmark folder
│   ├── <font color="#33DA7A"><b>docker_clean.sh</b></font> // Remove all docker images and containers
│   └── <font color="#33DA7A"><b>gen_wrapper.sh</b></font> // Generate python wrapper for ecp library
├── <font color="#2A7BDE"><b>src</b></font> // Main folder containing all src files for ecp library
│   ├── assignment.cpp // Assigne all points to clusters and save in binary file
│   ├── assignment.hpp
│   ├── CMakeLists.txt
│   ├── datastructures.cpp // Global defines, structs and variables
│   ├── datastructures.hpp
│   ├── distance.cpp // Different distance calculation functions
│   ├── distance.hpp
│   ├── ecp.cpp
│   ├── index.cpp // Create index and save in binary file
│   ├── index.hpp
│   ├── query.cpp // Search knns for set of queries
│   └── query.hpp
└── <font color="#2A7BDE"><b>swig</b></font> // SWIG files to create python wrapper
    ├── CMakeLists.txt
    └── ecp.i
</pre>

# Install
Note: Ubuntu 22.04 has been used during development. Other linux distributions might need different instructions.

1. Install required packages
```
sudo apt install cmake cpp gcc swig
```

2. Build project
    - Use CMakeLists.txt in root directory with your favored IDE
    - Or use `scripts/configure.sh` and `scripts/build.sh`

# Usage
## Standalone
The algorithm code from `src/` is complied as a C++ shared library, which is used by `main/mainc.pp`.

## Big-ANN-Benchmark
However since the main goal of this repository is to make eCP participating in the Big-ANN-Benchmark there is also a python wrapper (`swig/`) for the C++ shared library created.

To use the algorithm in the benchmark both repositories ([https://github.com/harsha-simhadri/big-ann-benchmarks](https://github.com/harsha-simhadri/big-ann-benchmarks) and this) must be placed in the same directory.

Also the some steps of big-ann-benchmarks must be followed, which includes installation, creating a first small dataset and complete a test run with DiskANN.

After that one can use `scripts/copy_files` to copy the necessary files from `benchmark/` into `big-ann-benchmarks`.

Finally the algorithm can be installed in the benchmark and used with the followign commands:

```
python install.py --algorithm ecp
python run.py --algorithm --dataset msspacev-1B
```