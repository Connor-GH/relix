#!/bin/sh

git clone https://github.com/ozkl/doomgeneric
cd doomgeneric/doomgeneric
find *.c -exec sed 's/true/DG_true/g' {} \;
find *.h -exec sed 's/true/DG_true/g' {} \;
find *.c -exec sed 's/false/DG_false/g' {} \;
find *.h -exec sed 's/false/DG_false/g' {} \;
echo "## Make sure to comment out the function that gives " \
"a compilation error related to floating point!"
