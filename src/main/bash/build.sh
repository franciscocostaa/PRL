#! /bin/bash

set -euxo pipefail

BASE_PATH="$(dirname "$0")/../../.."
cd "$BASE_PATH"

GREEN='\033[0;32m'
OFF='\033[0m'

rm -rf ".build"
rm -f "src/main/c/frontend/lexical-analysis/FlexScanner.c"
rm -f "src/main/c/frontend/lexical-analysis/FlexScanner.h"
rm -f "src/main/c/frontend/syntactic-analysis/BisonParser.c"
rm -f "src/main/c/frontend/syntactic-analysis/BisonParser.h"

cmake -S . -B .build
echo -e "${GREEN}CMake done.${OFF}"
cd .build
make
cd ..

echo -e "${GREEN}All done.${OFF}"
