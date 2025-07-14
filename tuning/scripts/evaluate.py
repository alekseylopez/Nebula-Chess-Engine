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