#!/usr/bin/env python3
"""
sim.py - Consensus Module Simulator

Generates Byzantine fault datasets and visualizes consensus performance.

Usage:
    python3 sim.py generate     # Generate byzantine_noise.csv
    python3 sim.py test         # Run consensus against dataset
    python3 sim.py plot         # Visualize results

Copyright (c) 2026 William Murray
MIT License
"""

import csv
import sys
import random
import math

def generate_byzantine_dataset(filename="byzantine_noise.csv", samples=100):
    """
    Generate a Byzantine fault dataset.
    
    - S0: Ground truth (perfect)
    - S1: Ground truth + Gaussian noise
    - S2: The Liar (matches truth for first 10 samples, then drifts at 1.5x)
    """
    ground_truth = 100.0
    noise_std = 0.5
    drift_rate = 0.15  # Units per sample after initial period
    honest_period = 10
    
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['ts', 's0', 's1', 's2', 'ground_truth'])
        
        for i in range(samples):
            ts = i * 100  # 100ms intervals
            
            # S0: Perfect ground truth
            s0 = ground_truth
            
            # S1: Ground truth + noise
            s1 = ground_truth + random.gauss(0, noise_std)
            
            # S2: The Liar
            if i < honest_period:
                s2 = ground_truth + random.gauss(0, noise_std * 0.5)
            else:
                # Drift away at 1.5x rate
                drift = (i - honest_period) * drift_rate
                s2 = ground_truth + drift
            
            writer.writerow([ts, f"{s0:.3f}", f"{s1:.3f}", f"{s2:.3f}", f"{ground_truth:.3f}"])
    
    print(f"Generated {filename} with {samples} samples")
    print(f"  S0: Perfect ground truth ({ground_truth})")
    print(f"  S1: Ground truth + noise (σ={noise_std})")
    print(f"  S2: Honest for {honest_period} samples, then drifts at {drift_rate}/sample")

def load_dataset(filename="byzantine_noise.csv"):
    """Load the Byzantine dataset."""
    data = []
    with open(filename, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            data.append({
                'ts': int(row['ts']),
                's0': float(row['s0']),
                's1': float(row['s1']),
                's2': float(row['s2']),
                'ground_truth': float(row['ground_truth'])
            })
    return data

def simple_consensus_mvs(s0, s1, s2):
    """Mid-value selection consensus (Python reference implementation)."""
    values = sorted([s0, s1, s2])
    return values[1]  # Median

def simple_consensus_avg(s0, s1, s2):
    """Simple average (for comparison)."""
    return (s0 + s1 + s2) / 3.0

def test_consensus(filename="byzantine_noise.csv"):
    """Test consensus algorithms against the dataset."""
    data = load_dataset(filename)
    
    mvs_errors = []
    avg_errors = []
    
    print("\nTesting consensus algorithms against Byzantine liar:")
    print("=" * 70)
    print(f"{'Step':>5} | {'S0':>8} | {'S1':>8} | {'S2 (liar)':>10} | {'MVS':>8} | {'AVG':>8} | {'Truth':>8}")
    print("-" * 70)
    
    for i, row in enumerate(data[:20]):  # Show first 20
        mvs = simple_consensus_mvs(row['s0'], row['s1'], row['s2'])
        avg = simple_consensus_avg(row['s0'], row['s1'], row['s2'])
        truth = row['ground_truth']
        
        mvs_err = abs(mvs - truth)
        avg_err = abs(avg - truth)
        mvs_errors.append(mvs_err)
        avg_errors.append(avg_err)
        
        marker = " ← drift starts" if i == 10 else ""
        print(f"{i:5} | {row['s0']:8.2f} | {row['s1']:8.2f} | {row['s2']:10.2f} | "
              f"{mvs:8.2f} | {avg:8.2f} | {truth:8.2f}{marker}")
    
    # Process remaining
    for row in data[20:]:
        mvs = simple_consensus_mvs(row['s0'], row['s1'], row['s2'])
        avg = simple_consensus_avg(row['s0'], row['s1'], row['s2'])
        truth = row['ground_truth']
        mvs_errors.append(abs(mvs - truth))
        avg_errors.append(abs(avg - truth))
    
    print("-" * 70)
    print(f"\nResults over {len(data)} samples:")
    print(f"  Mid-Value Selection (MVS):")
    print(f"    Max error: {max(mvs_errors):.4f}")
    print(f"    Mean error: {sum(mvs_errors)/len(mvs_errors):.4f}")
    print(f"  Simple Average:")
    print(f"    Max error: {max(avg_errors):.4f}")
    print(f"    Mean error: {sum(avg_errors)/len(avg_errors):.4f}")
    
    # Check if MVS stayed within 2% of ground truth
    truth = data[0]['ground_truth']
    threshold = truth * 0.02
    mvs_within_threshold = all(e <= threshold for e in mvs_errors)
    
    print(f"\n  2% threshold = {threshold:.2f}")
    print(f"  MVS within 2%: {'✓ PASS' if mvs_within_threshold else '✗ FAIL'}")
    print(f"  AVG within 2%: {'✓ PASS' if all(e <= threshold for e in avg_errors) else '✗ FAIL'}")
    
    return mvs_within_threshold

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 sim.py [generate|test|plot]")
        print("  generate - Create byzantine_noise.csv")
        print("  test     - Run consensus test")
        sys.exit(1)
    
    cmd = sys.argv[1]
    
    if cmd == "generate":
        generate_byzantine_dataset()
    elif cmd == "test":
        success = test_consensus()
        sys.exit(0 if success else 1)
    elif cmd == "plot":
        print("Plot functionality requires matplotlib.")
        print("Run: pip install matplotlib pandas")
        print("Then modify this script to add plotting.")
    else:
        print(f"Unknown command: {cmd}")
        sys.exit(1)

if __name__ == "__main__":
    main()
