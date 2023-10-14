#!/usr/bin/env sh

PROG=./Lab1
OUT=heap.prof

help() {
  echo "Usage: ${0} [-h] [-o FILE] IN"
  echo ""
  echo "    Runs ${PROG} on IN with the inspection of Massif."
  echo "    The result of the partition is discarded."
  echo ""
  echo "Arguments:"
  echo "    IN        Name of the input net connection file"
  echo ""
  echo "Options:"
  echo "    -o FILE   Place the profile output into FILE"
  echo "              (default: ${OUT})"
  echo "    -h        Print this help message"
}

# Handle options.
# https://stackoverflow.com/questions/192249/how-do-i-parse-command-line-arguments-in-bash

# A POSIX variable.
# Reset in case getopts has been used previously in the shell.
OPTIND=1

while getopts "ho:" opt; do
  case "$opt" in
  h)
    help
    exit 0
    ;;
  o)
    OUT=$OPTARG
    ;;
  *)
    help
    exit 1
    ;;
  esac
done

shift $((OPTIND - 1))

# Handle non-option arguments.
if [ $# -ne 1 ]; then
  help
  exit 1
fi

IN="$1"

# Make sure Valgrind is installed.
# https://stackoverflow.com/questions/592620/how-can-i-check-if-a-program-exists-from-a-bash-script
if
  ! command -v valgrind --tool=massif >/dev/null 2>&1
then
  echo "This tool requires valgrind. Install it please, and then run this tool again."
  exit 1
fi

# Finally, run the program with the heap profiler.
make clean && make profile &&
  valgrind --tool=massif --time-unit=B --massif-out-file=tmp $PROG "$IN" /dev/null &&
  ms_print tmp >"$OUT"
rm -f tmp
