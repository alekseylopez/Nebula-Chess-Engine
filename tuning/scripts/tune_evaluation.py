import numpy as np
import pandas as pd
import json
from dataclasses import dataclass
from typing import List, Dict, Tuple
import matplotlib.pyplot as plt
from scipy.optimize import minimize

@dataclass
class TuningConfig:
    """Configuration for the tuning process"""

    learning_rate: float = 0.01
    batch_size: int = 1000
    epochs: int = 100
    k_factor: float = 1.2  # sigmoid scaling factor
    regularization: float = 0.001

class EvaluationTuner:
    def __init__(self, config: TuningConfig):
        self.config = config
        self.history = {'loss': [], 'accuracy': []}
        
        # initialize parameters to initial values
        self.params = {
            'material_value': [100, 320, 330, 500, 900, 0],
            'castle_rights_bonus': 30,
            'castled_position_bonus': 75,
            'isolated_pawn_penalty': 25,
            'doubled_pawn_penalty': 20,
            'backward_pawn_penalty': 15,
            'connected_passed_pawn_bonus': 15,
            'protected_passed_pawn_bonus': 10,
            'base_values': [0, 10, 20, 40, 80, 150, 250, 0]
        }
        
        self.param_vector = self._params_to_vector()
    
    def _params_to_vector(self) -> np.ndarray:
        """Convert parameter dictionary to a flat vector"""
        
        vector = []
        vector.extend(self.params['material_value'])
        vector.append(self.params['castle_rights_bonus'])
        vector.append(self.params['castled_position_bonus'])
        vector.append(self.params['isolated_pawn_penalty'])
        vector.append(self.params['doubled_pawn_penalty'])
        vector.append(self.params['backward_pawn_penalty'])
        vector.append(self.params['connected_passed_pawn_bonus'])
        vector.append(self.params['protected_passed_pawn_bonus'])
        vector.extend(self.params['base_values'])
        return np.array(vector, dtype=np.float32)
    
    def _vector_to_params(self, vector: np.ndarray) -> Dict:
        """Convert flat vector back to parameter dictionary"""

        params = {}
        idx = 0
        
        params['material_value'] = vector[idx:idx + 6].tolist()
        idx += 6
        params['castle_rights_bonus'] = float(vector[idx])
        idx += 1
        params['castled_position_bonus'] = float(vector[idx])
        idx += 1
        params['isolated_pawn_penalty'] = float(vector[idx])
        idx += 1
        params['doubled_pawn_penalty'] = float(vector[idx])
        idx += 1
        params['backward_pawn_penalty'] = float(vector[idx])
        idx += 1
        params['connected_passed_pawn_bonus'] = float(vector[idx])
        idx += 1
        params['protected_passed_pawn_bonus'] = float(vector[idx])
        idx += 1
        params['base_values'] = vector[idx:idx + 8].tolist()
        
        return params
    
    def sigmoid(self, x: float) -> float:
        """Sigmoid function for converting evaluation to win probability"""

        return 1.0 / (1.0 + np.exp(-x * self.config.k_factor / 400.0))