from __future__ import absolute_import
import ecp_wrapper as ecp
import numpy as np
import os
from benchmark.algorithms.base import BaseANN
from benchmark.datasets import DATASETS

class Ecp(BaseANN):
    def __init__(self, metric, index_params):
        self.name = "ecp"
        if (index_params.get("L") == None):
            print("Error: Missing Parameter L")
            return
        if (index_params.get("DCS") == None):
            print("Error: Missing Parameter DCS")
            return
        if (index_params.get("NC") == None):
            print("Error: Missing Parameter NC")
            return
        if (index_params.get("METRIC") == None):
            print("Error: Missing Parameter METRIC")
            return

        self._index_params = index_params
        self._metric = metric

        self.L = index_params.get("L")
        self.DCS = index_params.get("DCS")
        self.NC = index_params.get("NC")
        self.METRIC = index_params.get("METRIC")

    def track(self):
        return "T2"

    def create_index_dir(self, dataset):
        self.index_name = f"L{self.L}_DCS{self.DCS}_NC{self.NC}_METRIC{self.METRIC}"
        index_dir = os.path.join(os.getcwd(), "data", "indices")
        os.makedirs(index_dir, mode=0o777, exist_ok=True)
        index_dir = os.path.join(index_dir, "T2")
        os.makedirs(index_dir, mode=0o777, exist_ok=True)
        index_dir = os.path.join(index_dir, self.__str__())
        os.makedirs(index_dir, mode=0o777, exist_ok=True)
        index_dir = os.path.join(index_dir, dataset.short_name())
        os.makedirs(index_dir, mode=0o777, exist_ok=True)
        index_dir = os.path.join(index_dir, self.index_name)
        os.makedirs(index_dir, mode=0o777, exist_ok=True)
        return index_dir

    def fit(self, dataset):
        ds = DATASETS[dataset]()
        self.index_dir = self.create_index_dir(ds) + "/"
        dataset_file_path = ds.get_dataset_fn()
        self.index_file_path = ecp.ecp_create_index(dataset_file_path, self.index_dir, self.L, self.DCS, self.METRIC)
        self.meta_data_file_path = ecp.ecp_assign_points_to_cluster(dataset_file_path, self.index_dir, self.NC)

    def load_index(self, dataset):
        return False

    def index_files_to_store(self, dataset):
        return False
    
    def query(self, X, k):
        queries = X.astype(np.float64)
        result = ecp.ecp_process_query(queries, self.index_dir, self.k, self.b, self.L)
        self.res = result
        stats = dict({"dist_comps": ecp.ecp_get_distance_calculation_count()})
        self.stats = stats
        print(self.stats["dist_comps"])

    def get_results(self):
        """
        Helper method to convert query results of k-NN search.
        If there are nq queries, returns a (nq, k) array of integers
        representing the indices of the k-NN for each query.
        """
        return self.res

    def set_query_arguments(self, query_args):
        self._query_args = query_args
        self.k = query_args.get("k")
        self.b = query_args.get("b")

    def get_additional(self):
        """
        Retrieve additional results.
        Return a dictionary with metrics
        and corresponding measured values.
        The following additional metrics are supported:
        `mean_latency` in microseconds, if this applies to your algorithm.
        Skip if your algorithm batches query processing.
        `latency_999` is the 99.9pc latency in microseconds, if this applies
        to your algorithm. Skip if your algorithm batches query processing.
        `dist_comps` is the total number of points in the base set
        to which a query was compared.
        `mean_ssd_ios` is the average number of SSD I/Os per query for T2 algorithms.
        """
        return self.stats