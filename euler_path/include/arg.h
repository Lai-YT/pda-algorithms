#ifndef EULER_PATH_ARG_H_
#define EULER_PATH_ARG_H_

#include <getopt.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace euler {

constexpr int kNumberOfArguments = 2;

struct Argument {
  std::string in;
  std::string out;
};

inline void Usage(const char* prog_name) {
  // clang-format off
  std::cerr << "Usage: " << prog_name << " IN OUT\n";
  std::cerr << '\n';
  std::cerr << "Options:\n";
  std::cerr << "    -h, --help       Prints this help message\n";
  std::cerr << '\n';
  std::cerr << "Arguments:\n";
  std::cerr << "    IN               The netlist to find euler path on\n";
  std::cerr << "    OUT              The file to write the path result to\n";
  // clang-format on
}

inline struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0},
};

inline Argument HandleArguments(int argc, char** argv) {
  auto arg = Argument{};

  // Handle options
  int c;
  while ((c = getopt_long(argc, argv, "h", long_options, nullptr)) != -1) {
    switch (c) {
      case 'h':
        Usage(argv[0]);
        std::exit(EXIT_SUCCESS);
      default:
        Usage(argv[0]);
        std::exit(EXIT_FAILURE);
    }
  }

  // Handle arguments
  if (argc < optind + kNumberOfArguments) {
    std::cerr << argv[0] << ": not enough arguments\n";
    Usage(argv[0]);
    std::exit(EXIT_FAILURE);
  }
  arg.in = argv[optind++];
  arg.out = argv[optind++];

  if (optind != argc) {
    std::cerr << argv[0] << ": unknown arguments --";
    while (optind != argc) {
      std::cerr << ' ' << argv[optind++];
    }
    std::cerr << '\n';
    Usage(argv[0]);
    std::exit(EXIT_FAILURE);
  }

  return arg;
}

}  // namespace euler

#endif  // EULER_PATH_ARG_H_
