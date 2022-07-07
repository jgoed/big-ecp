from __future__ import absolute_import
from benchmark.algorithms.base import BaseANN
from benchmark.datasets import DATASETS
import benchmark.algorithms.ecp_wrapper as ecp
import numpy as np

class Ecp(BaseANN):
    def __init__(self, metric, index_params):
        self.name = "ecp"
        if (index_params.get("L") == None):
            print("Error: Missing Parameter L")
            return
        if (index_params.get("DCS") == None):
            print("Error: Missing Parameter DCS")

        self._index_params = index_params
        self._metric = metric

        self.L = index_params.get("L")
        self.DCS = index_params.get("DCS")

    def track(self):
        return "T2"

    def fit(self, dataset):
        ds = DATASETS[dataset]()
        dataset_file_path = ds.get_dataset_fn()
        self.index_file_path = ecp.ecp_create_index(dataset_file_path, self.L, self.DCS)
        self.meta_data_file_path = ecp.ecp_assign_points_to_cluster(dataset_file_path, self.index_file_path, 500000)

    def load_index(self, dataset):
        return False

    def index_files_to_store(self, dataset):
        return False
    
    def query(self, X, k):
        queries = X.astype(np.float64)
        result = ecp.ecp_process_query(queries, self.index_file_path, self.meta_data_file_path, self.k, self.b, self.L)
        self.res = result

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
        self.L = query_args.get("L")