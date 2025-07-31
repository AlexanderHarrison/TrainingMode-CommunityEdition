#! /usr/bin/env bash

set -Eeo pipefail

# ensure an iso was passed
if [ -z "${1}" ]; then
    echo "Usage:"
    echo "    $0 <path/to/vanilla/melee.iso> [release]"
    exit 1
fi
iso="${1}"

if [ "${2}" = "release" ]; then
    release=true
elif [ -n "${2}" ]; then
    mode="${2}"
fi

if [ ! -f "${iso}" ]; then
    echo "Error: path '${iso}' does not exist!"
    exit 1
fi

if [[ "$(uname)" =~ "MSYS" ]]; then
    gc_fst="bin/gc_fst.exe"
    hgecko="bin/hgecko.exe"
    hmex="bin/hmex.exe"
    xdelta="bin/xdelta.exe"
else
    gc_fst="bin/gc_fst"
    hgecko="bin/hgecko"
    hmex="bin/hmex"
    xdelta="xdelta3"
fi

# check if en or jp and use appropriate patch
header=$(${gc_fst} get-header "${iso}")
if [ "${header}" = "GALE01" ]; then
    patch="patch.xdelta"
elif [ "${header}" = "GALJ01" ]; then
    patch="patch_jp.xdelta"
else
    echo "Error: Invalid ISO '${iso}'"
    exit 1
fi

# fn to kill all processes everywhere
kill_all() {
    trap "kill -- -$$" EXIT
}

# fn to build mex executable
mex_build() {
    local sym="${1}"
    local out="${2}"
    local src="${3}"
    
    if [[ -n "${mode}" && "${mode}" != "${out}" ]]; then
        return
    fi
    
    if [ -n "${4}" ]; then
        local dat="-dat ${4}"
    else
        local dat=""
    fi
    
    if [ "${release}" = true ]; then
        local opt="-O2"
    else
        local opt="-DTM_DEBUG"
    fi
    
    warn="-Wall -Wextra -Wno-char-subscripts -Wno-builtin-declaration-mismatch -Wno-unused-parameter"
    ${hmex} -q -l "MexTK/melee.link" -f "${warn} ${opt}" -s "${sym}" -t "MexTK/${sym}.txt" -o "${out}" -i ${src} ${dat} || kill_all
    echo built ${out}
}

# make build directory if necessary
mkdir -p build

# compile code in parallel
mex_build "tmFunction" "build/eventMenu.dat" "src/events.c src/menu.c src/savestate_v1.c" "dats/eventMenu.dat" &
mex_build "cssFunction" "build/labCSS.dat" "src/lab_css.c" "dats/labCSS.dat" &
mex_build "evFunction" "build/lab.dat" "src/lab.c" "dats/lab.dat" &
mex_build "evFunction" "build/lcancel.dat" "src/lcancel.c" "dats/lcancel.dat" &
mex_build "evFunction" "build/ledgedash.dat" "src/ledgedash.c" "dats/ledgedash.dat" &
mex_build "evFunction" "build/wavedash.dat" "src/wavedash.c" "dats/wavedash.dat" &
mex_build "evFunction" "build/powershield.dat" "src/powershield.c" &
mex_build "evFunction" "build/edgeguard.dat" "src/edgeguard.c" &
mex_build "evFunction" "build/fc.dat" "src/fc.c" "dats/ledgedash.dat" &

# wait for compilation to finish
wait

# compile asm
if [[ -z "${mode}" || "${mode}" = "build/codes.gct" ]]; then
    ${hgecko} -q ASM build/codes.gct
    echo built build/codes.gct
fi

# build mex Start.dol
${gc_fst} read "${iso}" Start.dol build/ISOStart.dol
${xdelta} -dfs build/ISOStart.dol "Build TM Start.dol/${patch}" build/Start.dol

# copy iso over
if [ ! -f TM-CE.iso ]; then cp "${iso}" TM-CE.iso; fi

# add TM files to iso
${gc_fst} fs TM-CE.iso \
    delete MvHowto.mth \
    delete MvOmake15.mth \
    delete MvOpen.mth \
    insert TM/eventMenu.dat build/eventMenu.dat \
    insert TM/lab.dat build/lab.dat \
    insert TM/labCSS.dat build/labCSS.dat \
    insert TM/lcancel.dat build/lcancel.dat \
    insert TM/ledgedash.dat build/ledgedash.dat \
    insert TM/wavedash.dat build/wavedash.dat \
    insert TM/powershield.dat build/powershield.dat \
    insert TM/edgeguard.dat build/edgeguard.dat \
    insert TM/fc.dat build/fc.dat \
    insert codes.gct build/codes.gct \
    insert Start.dol build/Start.dol \
    insert opening.bnr opening.bnr
${gc_fst} set-header TM-CE.iso "GTME01" "Training Mode Community Edition"

echo "built TM-CE.iso"

# build release
if [ "${2}" = "release" ]; then
    ${xdelta} -fs "${iso}" -e "TM-CE.iso" "TM-CE/patch.xdelta"
    zip -r TM-CE.zip TM-CE/
    echo "built TM-CE.zip"
fi

