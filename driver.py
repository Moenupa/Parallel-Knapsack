"""
@author Meng Wang
@email mwang106@ur.rochester.edu
@create date 2024-02-22 15:27:35
@modify date 2024-02-22 15:27:35
@desc driver code to run experiments and plot results
"""

import os
import re
import subprocess
from glob import glob
from datetime import datetime
from typing import Tuple
from matplotlib import pyplot as plt
import pandas as pd


VERIFICATION_EPOCHS = 10
NUM_THREADS = [0, 1, 2, 4, 6, 8, 10, 12, 16, 32, 64]
INPUTS = glob('inputs/*.txt')
INPUTS.sort()
assert INPUTS, 'No input files found'


def execute(cmd: str) -> Tuple[str, str, str]:
    """    
    Returns:
    - str: num_threads in int value
    - str: result in int value
    - str: elapsed_time in float value
    """
    stdout = subprocess.check_output(cmd, shell=True).decode('utf-8')
    return re.findall(r'\d+(?:\.\d+)?', stdout)


def gen_log_timestamp() -> str:
    return f'log/{datetime.now():%Y%m%d_%H%M%S}.csv'


def run_experiment(input_file: str, logger: str, nums_threads: list[int] = NUM_THREADS,
                   verification_epochs: int = VERIFICATION_EPOCHS):
    # the same input file should ALWAYS generate the same result
    ground_truth = None
    
    # run the program with different number of threads * verification_epochs
    for n_threads in nums_threads:
        for _ in range(verification_epochs):
            command = f'bin/knapsack {n_threads} < {input_file}'
            threads, result, elapsed_time = execute(command)
            if ground_truth is None:
                ground_truth = result
            
            # if there is at least one mismatch, program fails!
            elif ground_truth != result:
                with open(logger, 'a') as f:
                    f.write(f'{input_file},{threads},{result},{elapsed_time}\n'
                            f'error,-1,-1,-1\n')
                assert False, f'Expected: {ground_truth} != Actual: {result} when running {command}'
            
            # if program runs correctly, record the result
            with open(logger, 'a') as f:
                f.write(f'{input_file},{threads},{result},{elapsed_time}\n')
                

def plot(logger: str):
    data = pd.read_csv(logger, names=['input', 'threads', 'result', 'elapsed_time'])
    data = data.groupby(['input', 'threads']).mean().reset_index()
    with open('average.txt', 'w') as f:
        data[data['threads'].isin({0, 1, 2, 4, 8, 16})].to_csv(f, index=False)

    for file, group in data.groupby('input'):
        group['speedup'] = group.iloc[0]['elapsed_time']/group['elapsed_time']
        title = os.path.basename(file)
        ax = group.plot(x='threads', y='speedup', title=title, legend=False, grid=True)
        ax.set_ylim(0, None)
        ax.set_ylabel('Speedup')
        ax.get_figure().savefig(f'res/{title}.png', dpi=300)


if __name__ == '__main__':
    os.makedirs('res', exist_ok=True)
    os.makedirs('log', exist_ok=True)
    
    
    log_path = gen_log_timestamp()
    for input_file in INPUTS:
        run_experiment(input_file, log_path)
    
    print(f'Experiment finished. Log file: {log_path}')

    plot(log_path)
    # plot("log/20240222_031503.csv")
            