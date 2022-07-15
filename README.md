# big-ecp

This repository contains a version of the Extended Cluster Pruning algorithm, which is designed to participate in the Big-ANN-Benchmark.

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