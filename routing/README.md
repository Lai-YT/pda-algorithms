# Routing

## 🧐 About

This subproject addresses the 2-layer channel routing problem by implementing the _Constraint Left-Edge algorithm_. Notably, the top and bottom channel boundaries may exhibit irregular rectilinear shapes.

> [!note]
> The Constraint Left-Edge algorithm does not incorporate the concept of dogleg, rendering it unsuitable for routing instances with circular vertical constraints.

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

The executable will be located in the root of the subproject and named `./Routing`.

## 🎈 Usage

To run the program, you can use the following command:

```
Usage: ./Routing IN OUT

Options:
    -h, --help       Prints this help message

Arguments:
    IN               The file to read the channel routing instance from
    OUT              The file to write the channel routing result to
```

### File Format

#### Input File Format

The input file should follow this format:

- Specify the rectilinear boundaries of the top and bottom. Note that top boundaries need not precede all bottom boundaries; they may be interleaved.
- After the boundaries, assign net ids corresponding to the pins in the top and bottom boundaries. Net ids on the top must be specified before the bottom. Pin with id 0 indicates an empty pin with no net connected to it

Here's a sample input format:

```
T1 0 3
T2 3 7
T0 7 11
B0 0 5
B1 5 11
1 2 3 0 4 3 4 0 5 7 5 4
6 5 6 7 1 0 7 7 8 2 8 2
```

> [!important]
> There should NOT be a newline at the end of the file.

#### Output File Format

The output file follows this format:

- The first line gives the number of extra routing tracks in the channel.
- Starting from the second line, every two lines indicate the track with the interval within which each net is routed in the order of the net id.

Here's a potential result based on the provided input:

```
Channel density: 5
Net 1
C4 0 4
Net 2
C2 1 11
Net 3
T0 2 5
Net 4
C5 4 11
Net 5
C1 1 10
Net 6
C3 0 2
Net 7
C3 3 9
Net 8
B0 8 10
```

> [!important]
> There will NOT be a newline at the end of the file.

## 🔧 Running Tests

Unfortunately, this subproject doesn't come with a built-in test infrastructure. However, you can discover sample input routing instances within the `test/` directory.

## ✍️ License

This project is licensed under the [MIT License](./LICENSE).
