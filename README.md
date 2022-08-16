# big-ecp

This repository contains a version of the Extended Cluster Pruning algorithm mostly based on the previous work of [this](https://github.com/fremartini/eCP) repository, which is designed to participate in [Big-ANN-Benchmarks](https://github.com/harsha-simhadri/big-ann-benchmarks). This implementation can also be used in a standalone mode for developing and debugging.

<br>

# Install and usage with Big-ANN-Benchmarks

The following steps were tested under a clean install of Ubuntu 18.04.6 LTS x86_64. Other version or distributions might also work, but perhaps need some adjustments. <br>

Necessary steps to use big-ecp in Big-ANN-Benchmarks:
1. Prepare Big-ANN-Benchmarks
2. Install big-ecp in Big-ANN-Benchmarks
3. Run big-ecp with Big-ANN-Benchmarks
4. Configure big-ecp for various dataset and scenarios

<br>

## Prepare Big-ANN-Benchmarks

If Big-ANN-Benchmarks undergoes changes in the future, the steps described here may lose their validity.

1. Clone repository
```
git clone https://github.com/harsha-simhadri/big-ann-benchmarks.git
cd big-ann-benchmarks
```

2. Install Python 3.6 and pip3
```
sudo apt install python3.6-dev python3-pip
```

3. Install requirements
```
pip3 install -r requirements.txt
```

4. Install docker by following instructions [here](https://docs.docker.com/engine/install/ubuntu/). You might also want to follow the post-install steps for running docker in non-root user mode.

5. Install diskann base line algorithm, create small dataset, run test, change permissions for result directory and plot results.
```
python3 install.py --algorithm diskann
python3 create_dataset.py --dataset random-xs
python3 run.py --algorithm diskann-t2 --dataset random-xs
chmod -R 777 results/
python3 plot.py --dataset random-xs
```

<br>

## Install big-ecp in Big-ANN-Benchmarks

Note: Both repository must be placed in the same directory (for example ~/).

1. Clone repository
```
git clone https://github.com/jgoed/big-ecp.git
cd big-ecp
```

2. Clean existing docker containers and copy benchmark files to Big-ANN-Benchmarks
```
cd scripts/
./docker_clean.sh
./copy_files.sh
```

3. Change to Big-ANN-Benchmarks directory and install big-ecp
```
cd big-ann-benchmarks/
python3 install.py --algorithm ecp
```

<br>

## Use big-ecp with Big-ANN-Benchmarks
1. Create dataset
```
python3 create_dataset.py --dataset msspacev-10M
```

2. Run benchmark
```
python3 run.py --algorithm ecp-t2 --dataset msspacev-10M
```

3. Plot results
```
chmod -R 777 results/
python3 plot.py --dataset msspacev-10M
```

<br>

## Configure big-ecp for various dataset and scenarios

1. The implementation must be correctly configured to the dataset to be used. The following configurations can be made via `#define` in `src/datastructure.hpp`:
    - Change datatype of input dataset
    - Change number of dimension of data points
    - Change various file names created by the implementation
    - Select between multi-threading or single core executaion
    - Select if random leaders are used or not

2. Various paramter of the algorithm can be configured in `benchmark/algos.yaml`.

For a detailed explanation of the datatypes and dimension of the datasets and the `algos.yaml` file please check [Big-ANN-Benchmarks](https://github.com/harsha-simhadri/big-ann-benchmarks).

<br>

# Standalone install and usage

1. Install required packages
```
sudo apt install cmake cpp gcc swig
```

2. Build project
    - Use CMakeLists.txt in root directory with your favored IDE
    - Or use `scripts/configure.sh` and `scripts/build.sh`

3. Run `main/main.cpp`

The algorithm code from `src/` is complied as a C++ shared library, which is used by `main/main.cpp`. One needs to provide files like datasets or groundtruth which are otherwise provided by Big-ANN-Benchmarks.