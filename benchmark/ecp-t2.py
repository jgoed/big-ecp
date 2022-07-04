from __future__ import absolute_import
import benchmark.algorithms.ecp_wrapper

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

        self._index_params = index_params
        self._metric = metric

        self.L = index_params.get("L")
        self.DCS = index_params.get("DCS")

    def track(self):
        return "T2"

    def fit(self, dataset):
        ds = DATASETS[dataset]()
        dataset_file_path = ds.get_dataset_fn()
        print("FIT")
        #ecp_wrapper.ecp_create_index()

    def load_index(self, dataset):
        return False

    def index_files_to_store(self, dataset):
        return False
    
    def query(self, X, k):
        return False

    def set_query_arguments(self, query_args):
        print("SET QUERY ARGS")