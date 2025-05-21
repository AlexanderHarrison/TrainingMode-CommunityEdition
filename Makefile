.PHONY: clean iso all release

dats = build/ledgedash.dat build/wavedash.dat build/lcancel.dat build/labCSS.dat build/eventMenu.dat build/lab.dat build/powershield.dat build/edgeguard.dat

# find all .asm and .s files in the ASM dir. We have the escape the spaces, so we pipe to sed
ASM_FILES := $(shell find ASM -type f \( -name '*.asm' -o -name '*.s' \) | sed 's/ /\\ /g')
SHELL := /bin/bash

MEX_BUILD=mono MexTK/MexTK.exe -ff -b "build" -q -ow -l "MexTK/melee.link" -op 2

ifndef iso
$(error Error: INVALID ISO - run `make iso=path/to/vanilla/melee iso`)
endif

HEADER := $(shell ./gc_fst get-header ${iso})
ifeq ($(HEADER), GALE01)
PATCH := patch.xdelta
else
ifeq ($(HEADER), GALJ01)
PATCH := patch_jp.xdelta
else
$(error Error: INVALID ISO - run `make iso=path/to/vanilla/melee iso`)
endif
endif

clean:
	rm -rf TM-CE/patch.xdelta
	rm -rf TM-CE.iso
	rm -rf ./build/

build/eventMenu.dat: src/events.c src/events.h
	cp "dats/eventMenu.dat" "build/eventMenu.dat" 
	$(MEX_BUILD) -i "src/events.c" -s "tmFunction" -dat "build/eventMenu.dat" -t "MexTK/tmFunction.txt"

build/lab.dat: src/lab.c src/lab.h src/lab_common.h src/events.h
	cp "dats/lab.dat" "build/lab.dat"
	$(MEX_BUILD) -i "src/lab.c" -s "evFunction" -dat "build/lab.dat" -t "MexTK/evFunction.txt"

build/labCSS.dat: src/lab_css.c src/lab_common.h src/events.h
	cp "dats/labCSS.dat" "build/labCSS.dat"
	$(MEX_BUILD) -i "src/lab_css.c" -s "cssFunction" -dat "build/labCSS.dat" -t "MexTK/cssFunction.txt"

build/lcancel.dat: src/lcancel.c src/lcancel.h src/events.h
	cp "dats/lcancel.dat" "build/lcancel.dat"
	$(MEX_BUILD) -i "src/lcancel.c" -s "evFunction" -dat "build/lcancel.dat" -t "MexTK/evFunction.txt"

build/ledgedash.dat: src/ledgedash.c src/ledgedash.h src/events.h
	cp "dats/ledgedash.dat" "build/ledgedash.dat"
	$(MEX_BUILD) -i "src/ledgedash.c" -s "evFunction" -dat "build/ledgedash.dat" -t "MexTK/evFunction.txt"

build/wavedash.dat: src/wavedash.c src/wavedash.h src/events.h
	cp "dats/wavedash.dat" "build/wavedash.dat"
	$(MEX_BUILD) -i "src/wavedash.c" -s "evFunction" -dat "build/wavedash.dat" -t "MexTK/evFunction.txt"

build/powershield.dat: src/powershield.c src/events.h
	$(MEX_BUILD) -i "src/powershield.c" -s "evFunction" -dat "build/powershield.dat" -t "MexTK/evFunction.txt"

build/edgeguard.dat: src/edgeguard.c src/edgeguard.h src/events.h
	$(MEX_BUILD) -i "src/edgeguard.c" -s "evFunction" -dat "build/edgeguard.dat" -t "MexTK/evFunction.txt"

build/codes.gct: Additional\ ISO\ Files/opening.bnr $(ASM_FILES)
	cd "Build TM Codeset" && ./gecko build
	cp Additional\ ISO\ Files/* build/

build/Start.dol: | build
	./gc_fst read ${iso} Start.dol build/Start.dol
	xdelta3 -d -f -s build/Start.dol "Build TM Start.dol/$(PATCH)" build/Start.dol

TM-CE.iso: build/Start.dol build/codes.gct $(dats)
	if [[ ! -f TM-CE.iso ]]; then cp ${iso} TM-CE.iso; fi
	./gc_fst fs TM-CE.iso \
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
		insert opening.bnr build/opening.bnr
	./gc_fst set-header TM-CE.iso "GTME01" "Training Mode Community Edition"

build:
	mkdir -p build

iso: TM-CE.iso

TM-CE.zip: TM-CE.iso
	xdelta3 -f -s ${iso} -e TM-CE.iso TM-CE/patch.xdelta
	zip -r TM-CE.zip TM-CE/

release: TM-CE.zip

all: iso release
