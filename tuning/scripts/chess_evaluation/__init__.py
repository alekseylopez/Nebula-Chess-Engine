"""
Python scripts that use machine learning to tune Nebula's static evaluation function
"""

from .evaluate import EvaluationParams
from .evaluate import ChessPosition
from .evaluate import ChessEvaluator

__version__ = "0.1.0"
__all__ = ["EvaluationParams", "ChessPosition", "ChessEvaluator"]