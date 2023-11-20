import argparse
from dataclasses import dataclass
from random import randint
from typing import Final, List, Tuple


@dataclass
class Block:
    width: int
    height: int


def gen_blocks(num_of_blocks: int) -> List[Block]:
    """
    The generated blocks has their width and height uniformly distributed in the
    range of [30, 120] with the stride of 10.
    """
    RANGE_DIV_10: Final[Tuple[int, int]] = (3, 12)
    blocks: List[Block] = []
    for _ in range(num_of_blocks):
        width = randint(*RANGE_DIV_10) * 10
        height = randint(*RANGE_DIV_10) * 10
        blocks.append(Block(width, height))
    return blocks


def gen_area_constraint() -> Tuple[float, float]:
    """
    The generated lower bound is [0.3, 0.8] and the upper bound is [1.2, 3.3].
    """
    LOWER_BOUND_X_100: Tuple[int, int] = (30, 80)
    UPPER_BOUND_X_100: Tuple[int, int] = (120, 330)
    lower_bound: float = randint(*LOWER_BOUND_X_100) / 100
    upper_bound: float = randint(*UPPER_BOUND_X_100) / 100
    return (lower_bound, upper_bound)


def output_blocks(constraint: Tuple[float, float], blocks: List[Block]) -> None:
    lines: List[str] = []
    lines.append(f"{constraint[0]} {constraint[1]}\n")
    for i, block in enumerate(blocks):
        lines.append(f"b{i} {block.width} {block.height}\n")
    # The expected format does not allow the end of file newline.
    # This is removed by intention.
    lines[-1] = lines[-1].rstrip("\n")

    with open(f"in{len(blocks)}.txt", mode="w+") as f:
        f.writelines(lines)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        description="""
Generates a list of Blocks to the file {num_of_blocks}.txt.
The format is as the following:

    ```
    <lower bound of the aspect ratio> <upper bound of the aspect ratio>
    (<block name> <block width> <block height>)+
    ```
""",
    )

    def int_greater_than_one(value: str) -> int:
        int_value = int(value)
        if int_value <= 1:
            raise argparse.ArgumentTypeError(
                f"{int_value} is an int with value less than or equal to 1"
            )
        return int_value

    parser.add_argument(
        "num_of_blocks",
        type=int_greater_than_one,
    )
    args: argparse.Namespace = parser.parse_args()
    output_blocks(gen_area_constraint(), gen_blocks(args.num_of_blocks))
