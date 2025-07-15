from .types import EvaluationParams, ChessPosition

class ChessEvaluator:
    """Python implementation of the chess evaluation function"""
    
    def __init__(self, params: EvaluationParams = None):
        self.params = params or EvaluationParams()
        
        # phase weights for game phase calculation
        self.phase_weights = [0, 1, 1, 2, 4, 0] # P, N, B, R, Q, K
        self.max_phase = (1 * 2 + 1 * 2 + 2 * 2 + 4 * 1) * 2 # 24
        
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

    def evaluate(self, position: ChessPosition) -> float:
        """Main evaluation function"""

        score = 0.0
        
        phase = self._calculate_phase(position)
        
        score += self._evaluate_material(position, phase)
        score += self._evaluate_castling(position, phase)
        score += self._evaluate_pawn_structure(position, phase)
        
        # return score from white's perspective
        return score if position.turn == 'w' else -score

    def _calculate_phase(self, position: ChessPosition) -> float:
        """Calculate game phase (1.0 = opening, 0.0 = endgame)"""

        phase = 0
        
        piece_types = ['P', 'N', 'B', 'R', 'Q']
        for i, piece_type in enumerate(piece_types):
            if i == 0: # skip pawns for phase calculation
                continue
            
            count = (position.get_piece_count('w', piece_type) + 
                    position.get_piece_count('b', piece_type))
            phase += self.phase_weights[i] * count
        
        if phase < 0:
            phase = 0
        
        return min(phase / self.max_phase, 1.0)
    
    def _evaluate_material(self, position: ChessPosition, phase: float) -> float:
        """Evaluate material balance and piece-square tables"""

        score = 0.0
        opening_score = 0.0
        endgame_score = 0.0
        
        piece_types = ['P', 'N', 'B', 'R', 'Q', 'K']
        
        for i, piece_type in enumerate(piece_types):
            # white pieces
            for square in position.white_pieces[piece_type]:
                score += self.params.material_value[i]
                opening_score += self.pst_opening[piece_type][square]
                endgame_score += self.pst_endgame[piece_type][square]
            
            # white pieces (mirror vertically)
            for square in position.black_pieces[piece_type]:
                score -= self.params.material_value[i]
                mirrored_square = square ^ 56  # vertical flip
                opening_score -= self.pst_opening[piece_type][mirrored_square]
                endgame_score -= self.pst_endgame[piece_type][mirrored_square]
        
        # blend opening and endgame scores
        blended_pst = opening_score * phase + endgame_score * (1.0 - phase)
        
        return score + blended_pst
    
    def _evaluate_castling(self, position: ChessPosition, phase: float) -> float:
        """Evaluate castling rights and castled positions"""

        score = 0.0
        
        # castling rights bonus
        if 'K' in position.castling or 'Q' in position.castling:
            score += self.params.castle_rights_bonus
        if 'k' in position.castling or 'q' in position.castling:
            score -= self.params.castle_rights_bonus
        
        # castled position detection
        if position.white_pieces['K']:
            wk_square = position.white_pieces['K'][0]
            if wk_square == 6 or wk_square == 2: # g1 or c1
                score += self.params.castled_position_bonus
        
        if position.black_pieces['K']:
            bk_square = position.black_pieces['K'][0]
            if bk_square == 62 or bk_square == 58: # g8 or c8
                score -= self.params.castled_position_bonus
        
        # blend by opening phase
        return score * phase
    
    def _evaluate_pawn_structure(self, position: ChessPosition, phase: float) -> float:
        """Evaluate pawn structure"""

        score = 0.0
        
        # white pawns
        score += self._analyze_pawn_weaknesses(position, 'w', phase)
        score += self._analyze_passed_pawns(position, 'w', phase)
        
        # black pawns
        score -= self._analyze_pawn_weaknesses(position, 'b', phase)
        score -= self._analyze_passed_pawns(position, 'b', phase)
        
        return score
    
    def _analyze_pawn_weaknesses(self, position: ChessPosition, color: str, phase: float) -> float:
        """Analyze pawn weaknesses for given color"""

        penalty = 0.0
        
        pawns = position.white_pieces['P'] if color == 'w' else position.black_pieces['P']
        
        # count pawns per file
        file_counts = [0] * 8
        for square in pawns:
            file_counts[square % 8] += 1
        
        for square in pawns:
            file = square % 8
            
            # isolated pawn penalty
            if self._is_isolated_pawn(position, color, file):
                isolated_penalty = self.params.isolated_pawn_penalty
                # worse in endgame
                isolated_penalty *= (1.0 + (1.0 - phase) * 0.5)
                penalty += isolated_penalty
            
            # doubled pawn penalty
            if file_counts[file] > 1:
                penalty += self.params.doubled_pawn_penalty
            
            # backward pawn penalty
            if self._is_backward_pawn(position, color, square):
                penalty += self.params.backward_pawn_penalty
        
        return -penalty
    
    def _analyze_passed_pawns(self, position: ChessPosition, color: str, phase: float) -> float:
        """Analyze passed pawns for given color"""

        bonus = 0.0
        
        pawns = position.white_pieces['P'] if color == 'w' else position.black_pieces['P']
        
        for square in pawns:
            if self._is_passed_pawn(position, color, square):
                rank = (square // 8) if color == 'w' else (7 - (square // 8))
                bonus += self._passed_pawn_value(rank, phase)
                
                # additional bonuses for advanced passed pawns
                if rank >= 5:
                    file = square % 8
                    
                    # connected passed pawn bonus
                    if self._has_adjacent_pawns(position, color, file):
                        bonus += self.params.connected_passed_pawn_bonus
                    
                    # protected passed pawn bonus
                    if self._is_protected_by_pawn(position, color, square):
                        bonus += self.params.protected_passed_pawn_bonus
        
        return bonus
    
    def _is_isolated_pawn(self, position: ChessPosition, color: str, file: int) -> bool:
        """Check if pawn on given file is isolated"""

        pawns = position.white_pieces['P'] if color == 'w' else position.black_pieces['P']
        
        # check adjacent files
        for square in pawns:
            pawn_file = square % 8
            if abs(pawn_file - file) == 1:
                return False
        
        return True
    
    def _is_backward_pawn(self, position: ChessPosition, color: str, square: int) -> bool:
        """Check if pawn is backward"""

        file = square % 8
        rank = square // 8
        
        own_pawns = position.white_pieces['P'] if color == 'w' else position.black_pieces['P']
        enemy_pawns = position.black_pieces['P'] if color == 'w' else position.white_pieces['P']
        
        # check if can be supported by adjacent pawns
        can_be_supported = False
        for adj_file in [file - 1, file + 1]:
            if 0 <= adj_file < 8:
                for pawn_square in own_pawns:
                    if pawn_square % 8 == adj_file:
                        pawn_rank = pawn_square // 8
                        if (color == 'w' and pawn_rank <= rank) or (color == 'b' and pawn_rank >= rank):
                            can_be_supported = True
                            break
        
        # check if blocked by enemy pawn
        front_square = square + 8 if color == 'w' else square - 8
        if not (0 <= front_square < 64):
            return False
        
        blocked_by_enemy = False
        for enemy_square in enemy_pawns:
            enemy_file = enemy_square % 8
            enemy_rank = enemy_square // 8
            
            # check if enemy pawn attacks the front square
            if abs(enemy_file - (front_square % 8)) == 1:
                if (color == 'w' and enemy_rank == front_square // 8 + 1) or (color == 'b' and enemy_rank == front_square // 8 - 1):
                    blocked_by_enemy = True
                    break
        
        return not can_be_supported and blocked_by_enemy
    
    def _is_passed_pawn(self, position: ChessPosition, color: str, square: int) -> bool:
        """Check if pawn is passed"""

        file = square % 8
        rank = square // 8
        
        enemy_pawns = position.black_pieces['P'] if color == 'w' else position.white_pieces['P']
        
        # check if any enemy pawns can stop this pawn
        for enemy_square in enemy_pawns:
            enemy_file = enemy_square % 8
            enemy_rank = enemy_square // 8
            
            # check if enemy pawn is in the passed pawn zone
            if abs(enemy_file - file) <= 1:
                if (color == 'w' and enemy_rank > rank) or (color == 'b' and enemy_rank < rank):
                    return False
        
        return True
    
    def _passed_pawn_value(self, rank: int, phase: float) -> float:
        """Calculate value of passed pawn based on rank"""

        base_value = self.params.base_values[rank]
        
        # more valuable in endgame
        endgame_multiplier = 1.0 + (1.0 - phase) * 1.5
        
        return base_value * endgame_multiplier
    
    def _has_adjacent_pawns(self, position: ChessPosition, color: str, file: int) -> bool:
        """Check if there are adjacent pawns"""

        return False # placeholder
    
    def _is_protected_by_pawn(self, position: ChessPosition, color: str, square: int) -> bool:
        """Check if pawn is protected by another pawn"""

        return False # placeholder