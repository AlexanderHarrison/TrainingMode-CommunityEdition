#!/usr/bin/env bash

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
XDELTA_PATCH_PATH="$SCRIPT_DIR/patch.xdelta"

echo Hi fellow linux users! Consider contributing to TM-CE if you have a bit of free time. - Aitch

if command -v xdelta3 > /dev/null; then
    XDELTA_CMD="xdelta3"
elif command -v xdelta > /dev/null; then
    XDELTA_CMD="xdelta"
else
    echo "ERROR: xdelta is not installed. Please install xdelta or xdelta3 through your package manager."
    exit 1
fi

if [[ -z "${1}" ]]; then
    echo "ERROR: ISO was not passed"
    exit 1
fi

if [[ ! -f "${1}" ]]; then
    echo "ERROR: ISO '${1}' is not a valid file"
    exit 1
fi

if ! ${XDELTA_CMD} -f -d -s "${1}" "${XDELTA_PATCH_PATH}" TM-CE.iso; then 
    echo "ERROR: The ISO '${1}' is not a valid v1.02 NTSC melee iso"
    exit 1
fi

echo "############ TM-CE.iso has been successfully created ######################"

if [[ ! -t 0 ]]; then
    echo "Press Enter to continue..."
    read -r
fi
