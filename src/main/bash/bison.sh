#! /bin/bash

set -euxo pipefail

BASE_PATH="$(dirname "$0")/../../.."
cd "$BASE_PATH"

GREEN='\033[0;32m'
OFF='\033[0m'

if bison --help | grep --quiet -- "counterexamples"; then
	bison \
		-Wcounterexamples \
		-d "src/main/c/frontend/syntactic-analysis/BisonGrammar.y" \
		--output="src/main/c/frontend/syntactic-analysis/BisonParser.c"
else
	bison \
		-d "src/main/c/frontend/syntactic-analysis/BisonGrammar.y" \
		--output="src/main/c/frontend/syntactic-analysis/BisonParser.c"
fi

echo -e "${GREEN}Bison done.${OFF}"
