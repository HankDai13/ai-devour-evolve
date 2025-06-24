import torch
import pygame
import gymnasium as gym
import stable_baselines3
import numpy as np

def verify():
    """
    Verifies that all necessary libraries for the AI training environment are installed correctly.
    """

    try:
        print("Checking library versions...")

        # 1. Check PyTorch and CUDA support (most important!)
        print(f"  [OK] PyTorch version: {torch.__version__}")

        is_cuda_available = torch.cuda.is_available()
        print(f"  [INFO] CUDA available: {is_cuda_available}")

        if is_cuda_available:
            gpu_count = torch.cuda.device_count()
            gpu_name = torch.cuda.get_device_name(0)
            print(f"       -> Found {gpu_count} GPU device(s).")
            print(f"       -> Device 0 name: {gpu_name}")
        else:
            print("       [WARNING] No CUDA-capable GPU detected. Training will run on CPU.")

        # 2. Check Pygame
        print(f"  [OK] Pygame version: {pygame.version.ver}")

        # 3. Check Gymnasium
        print(f"  [OK] Gymnasium version: {gym.__version__}")

        # 4. Check Stable Baselines3
        print(f"  [OK] Stable-Baselines3 version: {stable_baselines3.__version__}")

        # 5. Check NumPy
        print(f"  [OK] NumPy version: {np.__version__}")

        print("\nCongratulations! The Python environment is correctly set up.")
        if is_cuda_available:
            print("GPU is ready for accelerated training!")
        else:
            print("Training will be performed on CPU. Consider checking your GPU drivers.")

    except ImportError as e:
        print(f"\nError] A required library could not be imported: {e}")
        print("Please ensure you're running this script in the correct Conda environment and that dependencies are installed.")
    except Exception as e:
        print(f"\n[Error] An unexpected error occurred: {e}")

if __name__ == "__main__":
    verify()
