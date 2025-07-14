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

class ChessPosition:
    """Represents a chess position from FEN string"""
    
    def __init__(self, fen: str):
        self.fen = fen
        parts = fen.split()
        self.board_str = parts[0]
        self.turn = parts[1]  # 'w' or 'b'
        self.castling = parts[2]  # KQkq
        self.en_passant = parts[3]
        self.halfmove = int(parts[4])
        self.fullmove = int(parts[5])
        
        # parse board into piece arrays
        self.white_pieces = self._parse_pieces('w')
        self.black_pieces = self._parse_pieces('b')
    
    def _parse_pieces(self, color: str) -> Dict[str, List[int]]:
        """Parse FEN board string to get piece positions"""

        pieces = {'P': [], 'N': [], 'B': [], 'R': [], 'Q': [], 'K': []}
        
        rank = 7
        file = 0
        
        for char in self.board_str:
            if char == '/':
                rank -= 1
                file = 0
            elif char.isdigit():
                file += int(char)
            else:
                square = rank * 8 + file
                piece_type = char.upper()
                
                if (color == 'w' and char.isupper()) or (color == 'b' and char.islower()):
                    pieces[piece_type].append(square)
                
                file += 1
        
        return pieces

    def get_piece_count(self, color: str, piece_type: str) -> int:
        """Get count of specific piece type for given color"""

        return len(self.white_pieces[piece_type]) if color == 'w' else len(self.black_pieces[piece_type])

class ChessEvaluator:
    """Python implementation of the chess evaluation function"""
    
    def __init__(self, params: EvaluationParams = None):
        self.params = params or EvaluationParams()
        
        # phase weights for game phase calculation
        self.phase_weights = [0, 1, 1, 2, 4, 0]  # P, N, B, R, Q, K
        self.max_phase = (1 * 2 + 1 * 2 + 2 * 2 + 4 * 1) * 2  # 24
        
        # opening PST
        self.pst_opening = {
            'P': [0, 0, 0, 0, 0, 0, 0, 0,
                  5, 10, 10, -20, -20, 10, 10, 5,
                  10, 10, 20, 30, 30, 20, 10, 10,
                  5, 5, 10, 50, 50, 10, 5, 5,
                  0, 0, 0, 60, 60, 0, 0, 0,
                  5, -5, -10, 20, 20, -10, -5, 5,
                  5, 10, 10, -20, -20, 10, 10, 5,
                  0, 0, 0, 0, 0, 0, 0, 0],
            
            'N': [-50, -40, -30, -30, -30, -30, -40, -50,
                  -40, -20, 0, 0, 0, 0, -20, -40,
                  -30, 0, 10, 15, 15, 10, 0, -30,
                  -30, 5, 15, 20, 20, 15, 5, -30,
                  -30, 0, 15, 20, 20, 15, 0, -30,
                  -30, 5, 10, 15, 15, 10, 5, -30,
                  -40, -20, 0, 5, 5, 0, -20, -40,
                  -50, -40, -30, -30, -30, -30, -40, -50],
            
            'B': [-20, -10, -10, -10, -10, -10, -10, -20,
                  -10, 0, 0, 0, 0, 0, 0, -10,
                  -10, 0, 5, 10, 10, 5, 0, -10,
                  -10, 5, 5, 10, 10, 5, 5, -10,
                  -10, 0, 10, 10, 10, 10, 0, -10,
                  -10, 10, 10, 10, 10, 10, 10, -10,
                  -10, 5, 0, 0, 0, 0, 5, -10,
                  -20, -10, -10, -10, -10, -10, -10, -20],
            
            'R': [0, 0, 0, 0, 0, 0, 0, 0,
                  -5, 0, 0, 0, 0, 0, 0, -5,
                  -5, 0, 0, 0, 0, 0, 0, -5,
                  -5, 0, 0, 0, 0, 0, 0, -5,
                  -5, 0, 0, 0, 0, 0, 0, -5,
                  -5, 0, 0, 0, 0, 0, 0, -5,
                  5, 10, 10, 10, 10, 10, 10, 5,
                  0, 0, 0, 5, 5, 0, 0, 0],
            
            'Q': [-20, -10, -10, -5, -5, -10, -10, -20,
                  -10, 0, 0, 0, 0, 0, 0, -10,
                  -10, 0, 5, 5, 5, 5, 0, -10,
                  -5, 0, 5, 5, 5, 5, 0, -5,
                  0, 0, 5, 5, 5, 5, 0, -5,
                  -10, 5, 5, 5, 5, 5, 0, -10,
                  -10, 0, 5, 0, 0, 0, 0, -10,
                  -20, -10, -10, -5, -5, -10, -10, -20],
            
            'K': [20, 30, 10, 0, 0, 10, 30, 20,
                  20, 20, 0, 0, 0, 0, 20, 20,
                  -10, -20, -20, -20, -20, -20, -20, -10,
                  -20, -30, -30, -40, -40, -30, -30, -20,
                  -30, -40, -40, -50, -50, -40, -40, -30,
                  -30, -40, -40, -50, -50, -40, -40, -30,
                  -30, -40, -40, -50, -50, -40, -40, -30,
                  -30, -40, -40, -50, -50, -40, -40, -30]
        }
        
        # endgame PST
        self.pst_endgame = {
            'P': [0, 0, 0, 0, 0, 0, 0, 0,
                  5, 10, 10, -20, -20, 10, 10, 5,
                  5, -5, -10, 0, 0, -10, -5, 5,
                  0, 0, 0, 20, 20, 0, 0, 0,
                  5, 5, 10, 25, 25, 10, 5, 5,
                  10, 10, 20, 30, 30, 20, 10, 10,
                  50, 50, 50, 50, 50, 50, 50, 50,
                  0, 0, 0, 0, 0, 0, 0, 0],
            
            'N': [-50, -40, -30, -30, -30, -30, -40, -50,
                  -40, -20, 0, 0, 0, 0, -20, -40,
                  -30, 0, 10, 15, 15, 10, 0, -30,
                  -30, 5, 15, 20, 20, 15, 5, -30,
                  -30, 0, 15, 20, 20, 15, 0, -30,
                  -30, 5, 10, 15, 15, 10, 5, -30,
                  -40, -20, 0, 5, 5, 0, -20, -40,
                  -50, -40, -30, -30, -30, -30, -40, -50],
            
            'B': [-20, -10, -10, -10, -10, -10, -10, -20,
                  -10, 0, 0, 0, 0, 0, 0, -10,
                  -10, 0, 5, 10, 10, 5, 0, -10,
                  -10, 5, 5, 10, 10, 5, 5, -10,
                  -10, 0, 10, 10, 10, 10, 0, -10,
                  -10, 10, 10, 10, 10, 10, 10, -10,
                  -10, 5, 0, 0, 0, 0, 5, -10,
                  -20, -10, -10, -10, -10, -10, -10, -20],
            
            'R': [0, 0, 5, 10, 10, 5, 0, 0,
                  0, 0, 5, 10, 10, 5, 0, 0,
                  0, 0, 5, 10, 10, 5, 0, 0,
                  0, 0, 5, 10, 10, 5, 0, 0,
                  0, 0, 5, 10, 10, 5, 0, 0,
                  0, 0, 5, 10, 10, 5, 0, 0,
                  25, 25, 25, 25, 25, 25, 25, 25,
                  0, 0, 5, 10, 10, 5, 0, 0],
            
            'Q': [-20, -10, -10, -5, -5, -10, -10, -20,
                  -10, 0, 0, 0, 0, 0, 0, -10,
                  -10, 0, 5, 5, 5, 5, 0, -10,
                  -5, 0, 5, 5, 5, 5, 0, -5,
                  0, 0, 5, 5, 5, 5, 0, -5,
                  -10, 5, 5, 5, 5, 5, 0, -10,
                  -10, 0, 5, 0, 0, 0, 0, -10,
                  -20, -10, -10, -5, -5, -10, -10, -20],
            
            'K': [-50, -40, -30, -20, -20, -30, -40, -50,
                  -30, -20, -10, 0, 0, -10, -20, -30,
                  -30, -10, 20, 30, 30, 20, -10, -30,
                  -30, -10, 30, 40, 40, 30, -10, -30,
                  -30, -10, 30, 40, 40, 30, -10, -30,
                  -30, -10, 20, 30, 30, 20, -10, -30,
                  -30, -30, 0, 0, 0, 0, -30, -30,
                  -50, -30, -30, -30, -30, -30, -30, -50]
        }