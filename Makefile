.PHONY: clean iso all release

DATS = build/ledgedash.dat build/wavedash.dat build/lcancel.dat build/labCSS.dat build/eventMenu.dat build/lab.dat build/powershield.dat build/edgeguard.dat

# find all .asm and .s files in the ASM dir. We have the escape the spaces, so we pipe to sed
ASM_FILES := $(shell find ASM -type f \( -name '*.asm' -o -name '*.s' \) | sed 's/ /\\ /g')
SHELL := /bin/bash

ifndef iso
$(error Error: INVALID ISO - run `make iso=path/to/vanilla/melee.iso TM-CE.iso`)
endif

# Use exes if we are on windows
UNAME=$(shell uname)
ifeq ($(findstring MSYS,$(UNAME)),MSYS)
	# Windows
	GC_FST=bin/gc_fst.exe
	HMEX=bin/hmex.exe
	XDELTA="Build TM Start.dol/xdelta.exe"
	GECKO=bin/hgecko.exe
else
	# Unix
	GC_FST=bin/gc_fst
	HMEX=bin/hmex
	XDELTA=xdelta3
	GECKO=bin/hgecko
endif

export PATH := $(DEVKITPPC)/bin:$(PATH)

HEADER := $(shell ${GC_FST} get-header '${iso}')
ifeq ($(HEADER), GALE01)
PATCH := patch.xdelta
else
ifeq ($(HEADER), GALJ01)
PATCH := patch_jp.xdelta
else
$(error Error: INVALID ISO - run `make iso=path/to/vanilla/melee iso`)
endif
endif

MEX_BUILD=$(HMEX) -q -l "MexTK/melee.link" -f "-w -fpermissive"

clean:
	rm -rf TM-CE/patch.xdelta
	rm -rf TM-CE.iso
	rm -rf ./build/

build/eventMenu.dat: src/events.c src/events.h src/menu.c src/menu.h src/savestate_v1.c src/savestate.h
	$(MEX_BUILD) -i "src/events.c" "src/menu.c" "src/savestate_v1.c" -s "tmFunction" -dat "dats/eventMenu.dat" -o "build/eventMenu.dat" -t "MexTK/tmFunction.txt"

build/lab.dat: src/lab.c src/lab.h src/lab_common.h src/events.h
	$(MEX_BUILD) -i "src/lab.c" -s "evFunction" -dat "dats/lab.dat" -o "build/lab.dat" -t "MexTK/evFunction.txt"

build/labCSS.dat: src/lab_css.c src/lab_common.h src/events.h
	$(MEX_BUILD) -i "src/lab_css.c" -s "cssFunction" -dat "dats/labCSS.dat" -o "build/labCSS.dat" -t "MexTK/cssFunction.txt"

build/lcancel.dat: src/lcancel.c src/lcancel.h src/events.h
	$(MEX_BUILD) -i "src/lcancel.c" -s "evFunction" -dat "dats/lcancel.dat" -o "build/lcancel.dat" -t "MexTK/evFunction.txt"

build/ledgedash.dat: src/ledgedash.c src/ledgedash.h src/events.h
	$(MEX_BUILD) -i "src/ledgedash.c" -s "evFunction" -dat "dats/ledgedash.dat" -o "build/ledgedash.dat" -t "MexTK/evFunction.txt"

build/wavedash.dat: src/wavedash.c src/wavedash.h src/events.h
	$(MEX_BUILD) -i "src/wavedash.c" -s "evFunction" -dat "dats/wavedash.dat" -o "build/wavedash.dat" -t "MexTK/evFunction.txt"

build/powershield.dat: src/powershield.c src/events.h
	$(MEX_BUILD) -i "src/powershield.c" -s "evFunction" -o "build/powershield.dat" -t "MexTK/evFunction.txt"

build/edgeguard.dat: src/edgeguard.c src/edgeguard.h src/events.h
	$(MEX_BUILD) -i "src/edgeguard.c" -s "evFunction" -o "build/edgeguard.dat" -t "MexTK/evFunction.txt"

build/codes.gct: $(ASM_FILES)
	$(GECKO) ASM build/codes.gct

build/Start.dol: | build
	${GC_FST} read '${iso}' Start.dol build/ISOStart.dol
	${XDELTA} -d -f -s build/ISOStart.dol "Build TM Start.dol/$(PATCH)" build/Start.dol

TM-CE.iso: build/Start.dol build/codes.gct $(DATS)
	if [[ ! -f TM-CE.iso ]]; then cp '${iso}' TM-CE.iso; fi
	${GC_FST} fs TM-CE.iso \
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
		insert codes.gct build/codes.gct \
		insert Start.dol build/Start.dol \
		insert opening.bnr opening.bnr
	${GC_FST} set-header TM-CE.iso "GTME01" "Training Mode Community Edition"

build:
	mkdir -p build

iso: TM-CE.iso

TM-CE.zip: TM-CE.iso
	${XDELTA} -f -s '${iso}' -e TM-CE.iso TM-CE/patch.xdelta
	zip -r TM-CE.zip TM-CE/

release: TM-CE.zip

all: iso release
