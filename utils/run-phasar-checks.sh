#!/bin/sh
#
# author: Philipp Schubert
#
# Ensure the overall code quality and the look and feel of the PhASAR project.
# Run some very useful clang-tidy checks and clang-format on the code base.
#

# exclude external projects from clang tidy checks
cp .clang-tidy-ignore external/googletest/.clang-tidy
cp .clang-tidy-ignore external/json/.clang-tidy
cp .clang-tidy-ignore external/WALi-OpenNWA/.clang-tidy

echo "Run clang-tidy ..."

run-clang-tidy.py -p build/ -header-filter='phasar*.h' -fix

echo "Run clang-format ..."
./utils/run-clang-format.py

rm external/googletest/.clang-tidy
rm external/json/.clang-tidy
rm external/WALi-OpenNWA/.clang-tidy

exit 0
