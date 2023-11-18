#ifndef FLOORPLAN_ARG_H_
#define FLOORPLAN_ARG_H_

#include <getopt.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace floorplan {

struct Argument {
  std::string in;
  std::string out;
  bool area_only;
};

inline void Usage(const char* prog_name) {
  // clang-format off
  std::cerr << "Usage: " << prog_name << " [-ah] IN OUT\n";
  std::cerr << '\n';
  std::cerr << "Options:\n";
  std::cerr << "    -a, --area-only  Outputs only the area\n";
  std::cerr << "    -h, --help       Prints this help message\n";
  std::cerr << '\n';
  std::cerr << "Arguments:\n";
  std::cerr << "    IN               The file to read the constraint and blocks from\n";
  std::cerr << "    OUT              The file to write the floorplanning result to\n";
  // clang-format on
}

inline struct option long_options[] = {
    {"area-only", no_argument, 0, 'a'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0},
};

inline Argument HandleArguments(int argc, char** argv) {
  auto arg = Argument{};

  // Handle options
  int c;
  while ((c = getopt_long(argc, argv, "ah", long_options, nullptr)) != -1) {
    switch (c) {
      case 'a':
        arg.area_only = true;
        break;
      case 'h':
        Usage(argv[0]);
        std::exit(EXIT_SUCCESS);
      default:
        Usage(argv[0]);
        std::exit(EXIT_FAILURE);
    }
  }

  // Handle arguments
  if (argc < optind + 2) {
    std::cerr << argv[0] << ": not enough arguments\n";
    Usage(argv[0]);
    std::exit(EXIT_FAILURE);
  }
  arg.in = std::string{argv[optind++]};
  arg.out = std::string{argv[optind++]};

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

}  // namespace floorplan

#endif  // FLOORPLAN_ARG_H_
