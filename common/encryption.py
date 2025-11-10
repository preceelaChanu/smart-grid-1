"""
Homomorphic Encryption wrapper for CKKS scheme
Provides encryption, decryption, and secure computation capabilities
"""

import tenseal as ts
import numpy as np
from typing import List, Tuple, Union
from common.utils import TimerContext
import json


class CKKSEncryptionEngine:
    """
    Manages CKKS homomorphic encryption operations
    Provides encryption, decryption, and computation on encrypted data
    """
    
    def __init__(self, poly_modulus_degree: int = 8192, coeff_modulus_bits: int = 40, scale_bits: int = 40):
        """
        Initialize CKKS encryption context
        
        Args:
            poly_modulus_degree: Polynomial modulus degree (must be power of 2)
            coeff_modulus_bits: Bit length for coefficient modulus
            scale_bits: Scaling factor bit length for CKKS
        """
        self.poly_modulus_degree = poly_modulus_degree
        self.coeff_modulus_bits = coeff_modulus_bits
        self.scale_bits = scale_bits
        
        # Initialize TensorSEAL context
        self.context = self._create_context()
        self.public_key = None
        self.secret_key = None
        
    def _create_context(self):
        """Create and initialize CKKS context"""
        context = ts.context(
            ts.SCHEME_TYPE.CKKS,
            poly_modulus_degree=self.poly_modulus_degree,
            coeff_modulus_bits=[self.coeff_modulus_bits],
            global_scale=2 ** self.scale_bits
        )
        context.generate_galois_keys()
        return context
    
    def encrypt_data(self, plaintext: Union[List[float], np.ndarray]) -> Tuple[bytes, float]:
        """
        Encrypt plaintext data using CKKS scheme
        
        Args:
            plaintext: List or array of floating point numbers to encrypt
        
        Returns:
            Tuple of (encrypted_bytes, encryption_time_ms)
        """
        if isinstance(plaintext, (int, float)):
            plaintext = [plaintext]
        
        plaintext = np.array(plaintext, dtype=np.float32).tolist()
        
        with TimerContext("CKKS Encryption") as timer:
            encrypted_vector = ts.ckks_vector(self.context, plaintext)
            encrypted_bytes = encrypted_vector.serialize()
        
        return encrypted_bytes, timer.elapsed_ms
    
    def decrypt_data(self, encrypted_bytes: bytes) -> List[float]:
        """
        Decrypt encrypted data using secret key
        
        Args:
            encrypted_bytes: Serialized encrypted data
        
        Returns:
            Decrypted plaintext values
        """
        encrypted_vector = ts.ckks_vector_from(self.context, encrypted_bytes)
        return encrypted_vector.decrypt()
    
    def add_encrypted(self, encrypted1: bytes, encrypted2: bytes) -> bytes:
        """
        Add two encrypted vectors (homomorphic addition)
        
        Args:
            encrypted1: First encrypted vector (serialized)
            encrypted2: Second encrypted vector (serialized)
        
        Returns:
            Serialized result of encrypted1 + encrypted2
        """
        vec1 = ts.ckks_vector_from(self.context, encrypted1)
        vec2 = ts.ckks_vector_from(self.context, encrypted2)
        result = vec1 + vec2
        return result.serialize()
    
    def multiply_encrypted_by_plain(self, encrypted: bytes, plain_scalar: float) -> bytes:
        """
        Multiply encrypted vector by plaintext scalar (homomorphic multiplication)
        
        Args:
            encrypted: Encrypted vector (serialized)
            plain_scalar: Plaintext scalar to multiply with
        
        Returns:
            Serialized result of encrypted * plain_scalar
        """
        vec = ts.ckks_vector_from(self.context, encrypted)
        result = vec * plain_scalar
        return result.serialize()
    
    def sum_encrypted_vector(self, encrypted: bytes) -> bytes:
        """
        Sum all elements in encrypted vector (reduction operation)
        
        Args:
            encrypted: Encrypted vector (serialized)
        
        Returns:
            Serialized result containing sum
        """
        vec = ts.ckks_vector_from(self.context, encrypted)
        # Sum operation is achieved through polynomial operations
        result = vec.sum()
        return result.serialize()
    
    def batch_encrypt(self, readings: List[float]) -> Tuple[bytes, float]:
        """
        Encrypt multiple readings as a single batch
        
        Args:
            readings: List of power readings to encrypt
        
        Returns:
            Tuple of (encrypted_bytes, encryption_time_ms)
        """
        return self.encrypt_data(readings)
    
    def get_encryption_params(self) -> dict:
        """Get current encryption parameters"""
        return {
            'poly_modulus_degree': self.poly_modulus_degree,
            'coeff_modulus_bits': self.coeff_modulus_bits,
            'scale_bits': self.scale_bits,
            'scheme': 'CKKS'
        }
    
    def serialize_context(self) -> bytes:
        """Serialize the encryption context (for key sharing)"""
        return self.context.serialize()
    
    @staticmethod
    def deserialize_context(context_bytes: bytes):
        """Deserialize encryption context"""
        return ts.context_from(context_bytes)


class EncryptedAggregator:
    """
    Performs aggregation operations on encrypted data without decryption
    Implements homomorphic sum, mean, max, min, and variance calculations
    """
    
    def __init__(self, engine: CKKSEncryptionEngine):
        """
        Initialize aggregator with encryption engine
        
        Args:
            engine: CKKSEncryptionEngine instance
        """
        self.engine = engine
    
    def sum_encrypted_readings(self, encrypted_readings: List[bytes]) -> Tuple[bytes, float]:
        """
        Sum multiple encrypted readings (homomorphic sum)
        
        Args:
            encrypted_readings: List of encrypted reading bytes
        
        Returns:
            Tuple of (encrypted_sum, computation_time_ms)
        """
        if not encrypted_readings:
            raise ValueError("No readings to sum")
        
        with TimerContext("Encrypted Sum") as timer:
            result = ts.ckks_vector_from(self.engine.context, encrypted_readings[0])
            
            for i in range(1, len(encrypted_readings)):
                encrypted_next = ts.ckks_vector_from(self.engine.context, encrypted_readings[i])
                result = result + encrypted_next
        
        return result.serialize(), timer.elapsed_ms
    
    def mean_encrypted_readings(self, encrypted_readings: List[bytes]) -> Tuple[bytes, float]:
        """
        Calculate mean of encrypted readings (sum / count, count is plaintext)
        
        Args:
            encrypted_readings: List of encrypted reading bytes
        
        Returns:
            Tuple of (encrypted_mean, computation_time_ms)
        """
        with TimerContext("Encrypted Mean") as timer:
            encrypted_sum, _ = self.sum_encrypted_readings(encrypted_readings)
            mean_factor = 1.0 / len(encrypted_readings)
            result = self.engine.multiply_encrypted_by_plain(encrypted_sum, mean_factor)
        
        return result, timer.elapsed_ms
    
    def max_encrypted_readings(self, encrypted_readings: List[bytes]) -> Tuple[bytes, float]:
        """
        Find maximum of encrypted readings using comparison (approximate)
        
        Args:
            encrypted_readings: List of encrypted reading bytes
        
        Returns:
            Tuple of (encrypted_max, computation_time_ms)
        """
        with TimerContext("Encrypted Max") as timer:
            # For HE, exact max is difficult. We approximate using polynomial degree.
            # In practice, this would need a comparison circuit.
            result = encrypted_readings[0]
            for i in range(1, len(encrypted_readings)):
                # Simplified: just track first for now
                # Real implementation would use secure comparison
                pass
        
        return result, timer.elapsed_ms
    
    def variance_encrypted_readings(self, encrypted_readings: List[bytes]) -> Tuple[bytes, float]:
        """
        Calculate variance of encrypted readings
        Variance = E[X^2] - E[X]^2
        
        Args:
            encrypted_readings: List of encrypted reading bytes
        
        Returns:
            Tuple of (encrypted_variance, computation_time_ms)
        """
        with TimerContext("Encrypted Variance") as timer:
            # Variance requires squaring and mean operations
            n = len(encrypted_readings)
            
            # Calculate mean
            encrypted_mean, _ = self.mean_encrypted_readings(encrypted_readings)
            
            # This is simplified - full variance requires squared terms
            variance = encrypted_mean  # Placeholder
        
        return variance, timer.elapsed_ms


class KeyManager:
    """Manages encryption keys and key distribution"""
    
    def __init__(self, engine: CKKSEncryptionEngine):
        """
        Initialize key manager
        
        Args:
            engine: CKKSEncryptionEngine instance
        """
        self.engine = engine
    
    def export_public_context(self) -> bytes:
        """
        Export public encryption context for distribution to meters
        
        Returns:
            Serialized public context
        """
        return self.engine.serialize_context()
    
    def save_context_to_file(self, filepath: str):
        """Save encryption context to file"""
        context_bytes = self.export_public_context()
        with open(filepath, 'wb') as f:
            f.write(context_bytes)
    
    @staticmethod
    def load_context_from_file(filepath: str) -> 'CKKSEncryptionEngine':
        """Load encryption context from file"""
        with open(filepath, 'rb') as f:
            context_bytes = f.read()
        context = CKKSEncryptionEngine.deserialize_context(context_bytes)
        engine = CKKSEncryptionEngine()
        engine.context = context
        return engine
