import numpy as np
from typing import Dict, List, Tuple
from dataclasses import dataclass

@dataclass
class EvaluationParams:
    """Parameters for the chess evaluation function"""
    
    # material values
    material_value: List[float] = None
    
    # castling bonuses
    castle_rights_bonus: float = 30.0
    castled_position_bonus: float = 75.0
    
    # pawn structure penalties
    isolated_pawn_penalty: float = 25.0
    doubled_pawn_penalty: float = 20.0
    backward_pawn_penalty: float = 15.0
    
    # passed pawn bonuses
    base_values: List[float] = None
    connected_passed_pawn_bonus: float = 15.0
    protected_passed_pawn_bonus: float = 10.0
    
    def __post_init__(self):
        if self.material_value is None:
            self.material_value = [100.0, 320.0, 330.0, 500.0, 900.0, 0.0]
        if self.base_values is None:
            self.base_values = [0.0, 10.0, 20.0, 40.0, 80.0, 150.0, 250.0, 0.0]