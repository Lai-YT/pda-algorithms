# Floorplan

## 🧐 About

This subproject conducts [slicing floorplanning](https://en.wikipedia.org/wiki/Floorplan_(microelectronics)#Sliceable_floorplans) using the slicing tree structure and the simulated annealing algorithm.

> [!note]
> - Optimization focuses on chip area, disregarding net wirelength.
> - Assumes the lower-left corner of the chip is at the origin (0,0) with no required space (channel) between blocks.

## 🏁 Getting Started

### Prerequisites

To build and run this subproject, you'll need:

- A C++17 compatible compiler (defaults to _g++_)
- [GNU Make](https://www.gnu.org/software/make/)

### Compilation

Use the following commands to compile the project with optimizations:

```sh
make release
```

If you're using a different compiler, you can specify it like this:

```sh
make release CXX=<compiler>
```

The executable will be located in the root of the subproject and named `Floorplan`.

## 🎈 Usage

To run the program, you can use the following command:

```
Usage: ./Floorplan [-ah] IN OUT

Options:
    -a, --area-only  Outputs only the area
    -h, --help       Prints this help message

Arguments:
    IN               The file to read the constraint and blocks from
    OUT              The file to write the floorplanning result to
```

### File Format

#### Input File Format

The input file should have the following format:

```
<lower bound of the aspect ratio> <upper bound of the aspect ratio>
<block name> <block width> <block height>
<block name> <block width> <block height>+
```

It starts with the _lower bound_ and _upper bound_ of the aspect ration constraint, followed by the description of 2 or more _blocks_. The description of each block contains the _block name_, followed by the _width_ and _height_ of the block.
Here's a sample input for the format of a circuit with 4 blocks.

```
0.5 2.0
b1 40 50
b2 60 50
b3 60 50
b4 40 50
```

#### Output File Format

The output file has the following format:

```
A = <chip area>
R = <aspect ratio>
<block name> <x> <y>
[<block name> <x> <y>]+
```

It starts with the overall _chip area_ and the _aspect ratio_ of the
resulting floorplan, and the bottom-left _coordinate_ of each block.
Here's a possible floorplan based on the above input:

```
A = 10000
R = 1.0
b1 0 50
b2 40 50
b3 0 0
b4 60 0
```

## 🔧 Running Tests

Unfortunately, this subproject doesn't come with a built-in test infrastructure. However, you can use the following scripts to assist with testing and profiling:

- [heap_profile.sh](./test/heap_profile.sh): Profiles heap usage with [Massif](https://valgrind.org/docs/manual/ms-manual.html).
- [gen.py](./test/gen.py): Generates the input block file with a customized number of blocks.

## 🎉 Reference

- D. F. Wong and C. L. Liu, "A New Algorithm for Floorplan Design," 23rd ACM/IEEE Design Automation Conference, Las Vegas, NV, USA, 1986, pp. 101-107, doi: 10.1109/DAC.1986.1586075.

## ✍️ License

This project is licensed under the [MIT License](./LICENSE).
