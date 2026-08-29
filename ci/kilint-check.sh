#!/bin/sh
# Run kilint on the board named by BOFFF.md.
#
# The path comes from the board.repository_path field in BOFFF.md, so the
# current board is the only one linted; older boards under hw/ predate these
# conventions and are not checked.
#
# Rule configuration lives in .kilint.yml at the repository root.
#
# Requires kilint (https://github.com/romkey/kilint). Run from the repo root:
#   ./ci/kilint-check.sh

set -eu

BOFFF="${1:-BOFFF.md}"

if [ ! -f "$BOFFF" ]; then
    echo "kilint-check: $BOFFF not found"
    exit 1
fi

# board.repository_path: hw/<board>/<version>
path=$(sed -n 's/^[[:space:]]*repository_path:[[:space:]]*//p' "$BOFFF" \
       | head -1 | tr -d "\"'" | sed 's/[[:space:]]*$//')

if [ -z "$path" ]; then
    echo "kilint-check: no repository_path in $BOFFF"
    exit 1
fi

if [ ! -d "$path" ]; then
    echo "kilint-check: repository_path '$path' from $BOFFF is not a directory"
    exit 1
fi

echo "=== linting $path (from $BOFFF)"
kilint lint "$path"
