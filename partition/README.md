# Partition

## 🧐 About

This subproject performs bi-partitioning on a network consisting of a set of cells connected by a set of nets using the **Fiduccia-Mattheyses Algorithm**. It's an iterative mincut heuristic with **linear-time** complexity.

> [!note]
> The size of all cells are fixed to 1.

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

The executable will be located in the root of the subproject and named `Lab1`.

## 🎈 Usage

To run the program, you can use the following command:

```sh
./Lab1 IN OUT
```

- `IN`: Name of the input net connection file.
- `OUT`: Name of the output partition result file.

### File Format

#### Input File Format

The input file should have the following format:

```
<balance factor>
NET <net name> [<cell name>]+ ;
```

It starts with the balance factor, followed by descriptions of nets. Each net description contains the keyword `NET`, followed by the net name and a list of connected cells, ending with `;`.
Here's a sample input for a circuit with 3 nets and 6 cells:

```
0.3
NET n1 c2 c3 c4 ; NET n2 c3 c6 ;
NET n3 c3 c5 c6 ;
NET n4 c1 c3 c5 c6 ;
NET n5 c2 c4 ; NET n6 c4 c6 ;
NET n7 c5 c6 ;
```

#### Output File Format

The output file has the following format:

```
Cutsize = <number>
G1 <size>
[<Cell Name>]+ ;
G2 <size>
[<Cell Name>]+ ;
```

It starts with the cut size of the partition, followed by the size of the 1st block along with the cells in it, and then the 2nd block with its cells.
Here's a possible partition based on the above input:

```
Cutsize = 2
G1 2
c2 c4 ;
G2 4
c3 c6 c5 c1 ;
```

## 🔧 Running Tests

Unfortunately, this subproject doesn't come with a built-in test infrastructure. However, you can use the following scripts to assist with testing and profiling:

- [heap_profile.sh](./test/heap_profile.sh): Profiles heap usage with [Massif](https://valgrind.org/docs/manual/ms-manual.html).
- [gen.py](./test/gen.py): Generates the input net connection file with a customized number of cells and nets.

## 🎉 Reference

- C. M. Fiduccia and R. M. Mattheyses, "A Linear-Time Heuristic for Improving Network Partitions," 19th Design Automation Conference, Las Vegas, NV, USA, 1982, pp. 175-181, doi: 10.1109/DAC.1982.1585498.

## ✍️ License

This project is licensed under the [MIT License](./LICENSE).
