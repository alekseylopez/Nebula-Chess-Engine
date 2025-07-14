"""
Python scripts that use machine learning to tune Nebula's static evaluation function
"""

from .evaluate import ChessEvaluator
from .evaluate import ChessPosition

__version__ = "0.1.0"
__all__ = ["ChessEvaluator", "ChessPosition"]