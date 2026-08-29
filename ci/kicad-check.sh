#!/bin/sh
# Run KiCAD ERC and DRC on the latest version of each board under hw/.
#
# - Each board lives in hw/<board>/<version>/
# - Only the latest version (highest version-sorted directory name) is checked
# - Directories containing a .ci-kicad-ignore file are skipped
# - Only errors fail the check; warnings are ignored (--severity-error)
#
# Requires kicad-cli (KiCAD 8+). Run from the repository root:
#   ./ci/kicad-check.sh

set -eu

HW_DIR="${1:-hw}"
FAILED=0

check_file() {
    # $1 = erc|drc, $2 = sch|pcb, $3 = file
    report=$(mktemp)
    echo "--- $1: $3"
    if kicad-cli "$2" "$1" --severity-error --exit-code-violations -o "$report" "$3"; then
        echo "    OK"
    else
        echo "    FAILED"
        cat "$report"
        FAILED=1
    fi
    rm -f "$report"
}

for board_dir in "$HW_DIR"/*/; do
    [ -d "$board_dir" ] || continue
    board=$(basename "$board_dir")

    if [ -e "$board_dir/.ci-kicad-ignore" ]; then
        echo "=== $board: skipped (.ci-kicad-ignore)"
        continue
    fi

    # candidate versions: subdirectories with KiCAD files, not marked ignored
    latest=""
    for version_dir in $(printf '%s\n' "$board_dir"*/ | sort -V); do
        [ -d "$version_dir" ] || continue
        [ -e "$version_dir/.ci-kicad-ignore" ] && continue
        found=0
        for f in "$version_dir"*.kicad_sch "$version_dir"*.kicad_pcb; do
            [ -e "$f" ] && found=1 && break
        done
        [ "$found" = 1 ] && latest="$version_dir"
    done

    if [ -z "$latest" ]; then
        echo "=== $board: no checkable versions found, skipping"
        continue
    fi

    echo "=== $board: checking $latest"

    for sch in "$latest"*.kicad_sch; do
        [ -e "$sch" ] || continue
        check_file erc sch "$sch"
    done

    for pcb in "$latest"*.kicad_pcb; do
        [ -e "$pcb" ] || continue
        check_file drc pcb "$pcb"
    done
done

if [ "$FAILED" != 0 ]; then
    echo "KiCAD checks FAILED"
    exit 1
fi
echo "All KiCAD checks passed"
