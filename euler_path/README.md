# Euler Path

## 🧐 About

This subproject focuses on identifying _[Hamilton Paths](https://en.wikipedia.org/wiki/Hamiltonian_path)_ within the HSPICE netlist. Given that finding Hamilton Paths is an NP-complete problem, the subproject employs a simple rotation-extension heuristic. The netlist may consist of multiple paths, which are subsequently connected through [dummy nodes](https://qr.ae/pK9WFU) to create a unified path. The primary objective of discovering Hamilton Paths is to enhance diffusion sharing. However, in this subproject, the emphasis is on ensuring the identification of at least one path rather than maximizing sharing or minimizing wire length.

> [!note]
> In CMOS circuit design, solving the problem of finding an optimal logic gate ordering to maximize diffusion sharing is akin to identifying an [Euler Path](https://en.wikipedia.org/wiki/Eulerian_path#Applications). Although the requirement is to visit each CMOS gate once, allowing multiple traversals of edges, I posit that this problem is effectively a Hamilton Path problem.

## 🏁 Getting Started

### Prerequisites

To build and run this subproject, you'll need:

- A C++17 compatible compiler (defaults to _g++_)
- [GNU Make](https://www.gnu.org/software/make/)
- (optional) [Flex](https://github.com/westes/flex) and [Bison](https://www.gnu.org/software/bison/) if you modify the functionality of the lexer and the parser

### Compilation

Use the following commands to compile the project with optimizations:

```sh
make release
# Or simply, `make`. It's the default target.
```

If you're using a different compiler, you can specify it like this:

```sh
make release CXX=<compiler>
```

The executable will be located in the root of the subproject and named `Lab3`.

## 🎈 Usage

To run the program, you can use the following command:

```
Usage: ./Lab3 IN OUT

Options:
    -h, --help       Prints this help message

Arguments:
    IN               The netlist to find euler path on
    OUT              The file to write the path result to
```

### File Format

#### Input File Format

The lexer and parser support a restricted subset of the HSPICE netlist.

– The first line gives the circuit name and the inputs/outputs.
– The rest lines describe the instance name of each NMOS/PMOS and its interconnections, which starts with the instance name, followed by the net names connected to the drain, gate, source, and substrate of the transistor, MOS type (PMOS or NMOS), channel width, channel length, and fin number.

Here's a sample input format for a circuit with 3 PMOS and 3 NMOS (3 CMOS):

```
.subckt two n1 n2 n5 vdd vss
mM1 n3 n1 vdd vdd pmos_rvt w=81.0n  l=20n nfin=3
mM2 n4 n2 vdd vdd pmos_rvt w=81.0n  l=20n nfin=3
mM3 n6 n5 n7  n7  pmos_rvt w=81.0n  l=20n nfin=3
mM4 n3 n1 vss vss nmos_rvt w=162.0n l=20n nfin=6
mM5 n3 n2 vss vss nmos_rvt w=162.0n l=20n nfin=6
mM6 n6 n5 n8  n8  nmos_rvt w=162.0n l=20n nfin=6
.ENDS
```

> [!important]
> There should NOT be a newline at the end of the file.

#### Output File Format

The output file follows this format:

– The first line gives the total HPWL of all nets in the SPICE netlist.
– The second and third lines shows the Euler path of the PMOS network in terms of instance names and net names, respectively.
– The fourth and fifth lines shows the Euler path of the NMOS network in terms of instance names and net names, respectively.

Here's a potential result of the path based on the provided input:

```
450
M1 M2 Dummy M3
n3 n1 vdd n2 n4 Dummy n7 n5 n6
M4 M5 Dummy M6
n3 n1 vss n2 n3 Dummy n6 n5 n8
```

> [!important]
> There will NOT be a newline at the end of the file.

## 🔧 Running Tests

Unfortunately, this subproject doesn't come with a built-in test infrastructure. However, you can discover sample input netlists within the `test/` directory.

## 🎉 Reference

- [LeechLattice](https://mathoverflow.net/users/125498/leechlattice), "Which are good algorithms for finding Hamiltonian path (not necessarily a circle) up to now?," URL (version: 2019-04-12): https://mathoverflow.net/q/327893

## ✍️ License

This project is licensed under the [MIT License](./LICENSE).
