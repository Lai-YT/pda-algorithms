"""
Generates a list of `n` Cells and `m` Nets.
"""

import argparse
import math
import random
from dataclasses import dataclass
from typing import List

Cell = int


@dataclass
class Net:
    cells: List[Cell]


def gen(num_of_cells: int, num_of_nets: int) -> None:
    cells: List[Cell] = [i for i in range(num_of_cells)]
    nets: List[Net] = [Net([]) for _ in range(num_of_nets)]
    # It meaningless if a net has connection less than 2.
    # In average, a net connects x cells.
    x: float = num_of_nets / num_of_cells + 2
    # We would like < 2 be outside of 3 sigmas.
    sigma: float = (x - 2) / 3
    for n in nets:
        # randomly determine how many cells this net should have
        k: int = 0
        while k <= 2:
            k = math.ceil(random.gauss(x, sigma))
        # Note that `sample` generates unique sequence.
        for c in random.sample(cells, k=k):
            n.cells.append(c)
    with open(f"{num_of_cells}_{num_of_nets}.txt", mode="w+") as f:
        # balanced factor is fixed to 0.5
        f.write("0.5\n")
        for i in range(num_of_nets):
            line = []
            line.append(f"NET n{i}")
            for c in nets[i].cells:
                line.append(f" c{c}")
            line.append(" ;\n")
            f.writelines(line)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("num_of_cells", type=int)
    parser.add_argument("num_of_nets", type=int)
    args: argparse.Namespace = parser.parse_args()
    gen(args.num_of_cells, args.num_of_nets)
