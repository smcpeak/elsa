#!/bin/sh
# Use ccparse to pretty-print.

# This is something I do fairly often during development.

if [ "x$1" = "x" ]; then
  echo "usage: $0 [-xc] file.{c,cc}"
  exit 2
fi

# Make sure GCC likes it.
gcc-check "$@" || exit

# Now use ccparse.
./ccparse.exe --pretty-print "$@"

# EOF
