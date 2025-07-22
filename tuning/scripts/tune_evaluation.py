import numpy as np
import pandas as pd
import json
from dataclasses import dataclass
from typing import List, Dict, Tuple
import matplotlib.pyplot as plt
from scipy.optimize import minimize
from chess_evaluation.evaluate import ChessEvaluator, ChessPosition
from chess_evaluation.types import EvaluationParams

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
    
    def evaluate_position_with_params(self, fen: str, params: Dict) -> float:
        """Evaluate a position using given parameters"""

        # create evaluation parameters object
        eval_params = EvaluationParams(
            material_value=params['material_value'],
            castle_rights_bonus=params['castle_rights_bonus'],
            castled_position_bonus=params['castled_position_bonus'],
            isolated_pawn_penalty=params['isolated_pawn_penalty'],
            doubled_pawn_penalty=params['doubled_pawn_penalty'],
            backward_pawn_penalty=params['backward_pawn_penalty'],
            connected_passed_pawn_bonus=params['connected_passed_pawn_bonus'],
            protected_passed_pawn_bonus=params['protected_passed_pawn_bonus'],
            base_values=params['base_values']
        )
        
        # create evaluator with these parameters
        evaluator = ChessEvaluator(eval_params)
        position = ChessPosition(fen)
        
        return evaluator.evaluate(position)
    
    def compute_loss(self, params_vector: np.ndarray, positions: List[Tuple[str, float]]) -> float:
        """Compute the loss function for a batch of positions"""

        params = self._vector_to_params(params_vector)
        total_loss = 0.0
        
        for fen, actual_result in positions:
            eval_score = self.evaluate_position_with_params(fen, params)
            predicted_result = self.sigmoid(eval_score)
            
            # cross-entropy loss
            total_loss -= actual_result * np.log(max(predicted_result, 1e-10)) + (1 - actual_result) * np.log(max(1 - predicted_result, 1e-10))
        
        # add L2 regularization
        regularization = self.config.regularization * np.sum(params_vector ** 2)
        
        return total_loss / len(positions) + regularization
    
    def compute_gradient(self, params_vector: np.ndarray, positions: List[Tuple[str, float]]) -> np.ndarray:
        """Compute gradients using finite differences"""
        
        epsilon = 1e-6
        gradients = np.zeros_like(params_vector)
        
        for i in range(len(params_vector)):
            params_plus = params_vector.copy()
            params_minus = params_vector.copy()
            
            params_plus[i] += epsilon
            params_minus[i] -= epsilon
            
            loss_plus = self.compute_loss(params_plus, positions)
            loss_minus = self.compute_loss(params_minus, positions)
            
            gradients[i] = (loss_plus - loss_minus) / (2 * epsilon)
        
        return gradients

    def train_batch(self, positions: List[Tuple[str, float]]) -> Tuple[float, float]:
        """Train on a batch of positions"""
        
        loss = self.compute_loss(self.param_vector, positions)
        gradients = self.compute_gradient(self.param_vector, positions)
        
        # update parameters using gradient descent
        self.param_vector -= self.config.learning_rate * gradients
        
        # compute accuracy
        correct = 0
        total = 0
        params = self._vector_to_params(self.param_vector)
        
        for fen, actual_result in positions:
            try:
                eval_score = self.evaluate_position_with_params(fen, params)
                predicted_result = self.sigmoid(eval_score)
                
                # consider prediction correct if within 0.1 of actual
                if abs(predicted_result - actual_result) < 0.1:
                    correct += 1
                total += 1
            except:
                continue
        
        accuracy = correct / total if total > 0 else 0.0
        
        return loss, accuracy
    
    def train(self, train_positions: List[Tuple[str, float]], 
              val_positions: List[Tuple[str, float]] = None) -> Dict:
        """Train the evaluation function"""
        print(f"Training on {len(train_positions)} positions")
        
        for epoch in range(self.config.epochs):
            # shuffle training data
            np.random.shuffle(train_positions)
            
            # train in batches
            epoch_losses = []
            epoch_accuracies = []
            
            for i in range(0, len(train_positions), self.config.batch_size):
                batch = train_positions[i:i + self.config.batch_size]
                loss, accuracy = self.train_batch(batch)
                epoch_losses.append(loss)
                epoch_accuracies.append(accuracy)
            
            # average metrics for epoch
            avg_loss = np.mean(epoch_losses)
            avg_accuracy = np.mean(epoch_accuracies)
            
            self.history['loss'].append(avg_loss)
            self.history['accuracy'].append(avg_accuracy)
            
            # validation
            val_loss = None
            val_accuracy = None
            if val_positions:
                val_loss = self.compute_loss(self.param_vector, val_positions)
                
                # compute validation accuracy
                correct = 0
                total = 0
                params = self._vector_to_params(self.param_vector)
                
                for fen, actual_result in val_positions:
                    try:
                        eval_score = self.evaluate_position_with_params(fen, params)
                        predicted_result = self.sigmoid(eval_score)
                        
                        if abs(predicted_result - actual_result) < 0.1:
                            correct += 1
                        total += 1
                    except:
                        continue
                
                val_accuracy = correct / total if total > 0 else 0.0
            
            # print progress
            if epoch % 10 == 0:
                print(f"Epoch {epoch}: Loss={avg_loss:.4f}, Acc={avg_accuracy:.4f}", end="")
                if val_loss is not None:
                    print(f", Val Loss={val_loss:.4f}, Val Acc={val_accuracy:.4f}")
                else:
                    print()
        
        # update final parameters
        self.params = self._vector_to_params(self.param_vector)
        
        return {
            'final_params': self.params,
            'history': self.history
        }
    
    def plot_training_history(self):
        """Plot training history"""
        
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
        
        ax1.plot(self.history['loss'])
        ax1.set_title('Training Loss')
        ax1.set_xlabel('Epoch')
        ax1.set_ylabel('Loss')
        ax1.grid(True)
        
        ax2.plot(self.history['accuracy'])
        ax2.set_title('Training Accuracy')
        ax2.set_xlabel('Epoch')
        ax2.set_ylabel('Accuracy')
        ax2.grid(True)
        
        plt.tight_layout()
        plt.show()

def load_training_data(train_file: str, val_file: str = None) -> Tuple[List[Tuple[str, float]], List[Tuple[str, float]]]:
    """Load training and validation data from CSV files"""

    # load training data
    train_df = pd.read_csv(train_file)
    train_positions = [(row['fen'], row['result']) for _, row in train_df.iterrows()]
    
    # load validation data if provided
    val_positions = []
    if val_file:
        val_df = pd.read_csv(val_file)
        val_positions = [(row['fen'], row['result']) for _, row in val_df.iterrows()]
    
    return train_positions, val_positions

def main():
    """Main training script"""

    # load data
    train_positions, val_positions = load_training_data(
        '../data/training_positions.csv', 
        '../data/validation_positions.csv'
    )
    
    print(f"Loaded {len(train_positions)} training positions")
    print(f"Loaded {len(val_positions)} validation positions")
    
    # create tuning configuration
    config = TuningConfig(
        learning_rate=0.001,
        batch_size=100,
        epochs=200,
        k_factor=1.2,
        regularization=0.0001
    )
    
    # create tuner
    tuner = EvaluationTuner(config)
    
    # gradient descent training
    print("\n=== Training with Gradient Descent ===")
    gd_results = tuner.train(train_positions, val_positions)
    
    print("\nFinal parameters from gradient descent:")
    for key, value in gd_results['final_params'].items():
        print(f"{key}: {value}")
    
    # plot training history
    tuner.plot_training_history()
    
    # save results
    with open('tuning_results.json', 'w') as f:
        json.dump({
            'gradient_descent': gd_results
        }, f, indent=2)
    
    print("\nResults saved to tuning_results.json")

if __name__ == "__main__":
    main()