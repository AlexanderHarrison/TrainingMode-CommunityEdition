# Development

Thanks for considering contributing to TM-CE!
I really appreciate all the help.
Melee is a pretty complicated game, but that doesn't make TM-CE hard to contribute to!
Here are a few things you should know before contributing.

First thing, [please join the discord](https://discord.gg/2Khb8CVP7A).
If you have any questions, feel free to ping me (Aitch) in the dev-discussion channel.

## Compilation

We have completely overhauled the build process.
Now any developer can easily compile an iso from source with a simple script or makefile.

### Windows
1. [Install DevKitPro](https://github.com/devkitPro/installer/releases/latest). Install the Gamecube (aka PPC or PowerPC) package.
2. Drag your legally obtained SSBM v1.02 ISO on to the 'build_windows.bat' file. If all goes well, 'TM-CE.iso' will be created.
3. If you run into `could not create temporary file whilst copying ...` issues, use powershell instead of cmd.

### Linux / MacOS / WSL / MSYS2
This method is much faster to build than `build_windows.bat`, so this should be preferred if you can set it up.

1. [Install DevKitPro](https://devkitpro.org/wiki/Getting_Started#Unix-like_platforms). Install the Gamecube (gamecube-dev) package.
    - Ensure that `/opt/devkitpro/devkitPPC/bin/` is added to the PATH.
2. [Install Mono](https://www.mono-project.com/download/stable). Prefer installation through your package manager.
3. Install xdelta3. This should be simple to install through your package manager.
4. Run `make iso=path-to-melee.iso iso`. If all goes well, 'TM-CE.iso' will be created.
    - If the provided 'gecko' binary fails (possibly due to libc issues), you can compile your own binary from [here](https://github.com/JLaferri/gecko/releases/tag/v3.4.0). **YOU MUST USE VERSION 3.4.0 OF GECKO OR IT WILL SILENTLY FAIL**.
    - If the provided 'gc_fst' binary fails (possibly due to libc issues), you can compile your own binary from [here](https://github.com/AlexanderHarrison/gc_fst).

## Project Structure

There are three important directories to know about:
1. `src/`: this directory contains the source for the C events, as well as some setup code for the event in `events.c`.
2. `MexTK/`:
    - The `include/` subdirectory contains headers for internal melee functions. Calling these will call native ssbm code.
    - The `MexTK` binary is the compiler executable, which takes in C source and spits out a dat file.
    - The `.txt` files contain symbols that we want called by the m-ex system.
        For example, C events will want their `Event_Init` and `Event_Think` functions called.
        We only use the evFunction and cssFunction modes.
3. `ASM/`: This huge directory contains gecko codes for various things like UCF, old events, OSDs, etc.
Every file has a injection address at the start.
When the game boots up, it will overwrite the instruction at that address and replace it with a branch to the asm contained in the file.
The `.asm` files will be injected and run, while the `.s` files contain include macros and will not be assembled by gecko.

## Melee Stuff

### HSDRaw and Dat Files

[A dat file, or an HSD_Archive](https://github.com/doldecomp/melee/blob/master/src/sysdolphin/baselib/archive.c) is the file format for data in ssbm.
Everything is stored in dat files - models, animations, code, textures, etc. Only cutscenes and music are stored differently.

[You can open, view, and edit dat files with HSDRawViewer](https://github.com/Ploaj/HSDLib).

The `dat/` directory contains some of these files.
They contain event specific objects, mostly menu models with some random other data.

### Objects

- **GOBJ** - game object. This is a very generic object.
They can have a model, an update function (think function in melee), pointer to arbitrary data, etc.
Most everything is a GOBJ.
- **JOBJ/JOBJDesc** - joint object (models).
Each JOBJ has a sibling and a child, forming a tree of joints.
Each joint can have a DOBJ, forming a large tree of models.
Technically, HSDRaw only deals with JOBJDescs, as JOBJs are only created at runtime from a JOBJDesc.
However, it calls them JOBJs for whatever reason. So JOBJDescs in training mode are JOBJs in HSDraw.
This same pattern holds for a lot (but not all!) HSDRaw objects.
Almost every object in the training mode dat files are JOBJDescs (node will be 0x40 in length).
You'll need to right click on the node -> Open As -> JOBJ in HSDRaw in order to view the model.
- **DOBJ** - display object. These contain meshes, textures, a material, etc.
- **MOBJ** - material object. Lots of stuff here, but I don't know much about them.
- **HSD_Material**. This contains colouring information. Most objects are coloured by setting the diffuse field in these.
- **TOBJ** - texture object. Images that will be displayed on a mesh.
- **POBJ** - polygon object. Contains a mesh.

## How To Do Things

- If you want to alter an event written in C (easy):
    - The training lab, lcancel, ledgedash, wavedash, and powershield events are written in c.
    This makes them much easier to modify than the other events. Poke around in their source in `src/`.
- If you want to alter an event written in asm (big knowledge check):
    - You will need to know a bit of Power PC asm.
    - Read `ASM/Readme.md`
    - Go to `ASM/training-mode/Custom Events/Custom Event Code - Rewrite.asm` and search for the event you want to modify.
    - These will A LOT trickier to modify than the other events. Prefer making a new event or modifying the lab.
    - There are a lot of random loads from random offsets there. Grep for that address in MexTK/include to see where it points.
    Feel free to put a comment there indicating the source!
- If you want to make a new event (tricky, but super flexible):
    - Add a file and header to the `src/`.
    - Add the required compilation steps in `Makefile` and `build_windows.bat`. Follow the same structure as the other events. Be sure to use the evFunction mode. You can skip the dat copy if you don't have any models attached to the event (you won't), like the powershield event.
    - Add the `EventDesc` and `EventMatchData` structs to `events.c` and add a reference to them in the `General_Events`, `Minigames_Events`, or `Spacie_Events` array.
    - Implement the `Event_Init`, `Event_Update`, `Event_Think` methods and `Event_Menu` pointer in your c file. Poke around the other events to figure out how the data flows.
- If you want to create a new OSD (hard):
    - You will need to know a lot of Power PC asm.
    - You will need to find the function that does the processing of the value you want to measure (reach out to me if you're not sure how to find this).
    - Create and write an asm file for the new OSD in `ASM/training-mode/Onscreen Display/`.
    - Add the OSD to the OSD list in `ASM/training-mode/Globals.s`. OSD ids are weird, I don't know exactly how to do this.

## Debugging Tips
- Due to a deficiency in the MexTK headers, we cannot turn on warnings effectively, so be aware of that.
- Set `TM_DEBUG` to 2 in events.h to get OSReport statements on the screen.
- **Use the dolphin debugger!** Make sure you have the latest version of dolphin for debugging.
    - To set a breakpoint, use the `bp()` fn call in C or the `SetBreakpoint` macro in ASM (which will clobber r3). Then when you boot up dolphin, put a breakpoint on the `bp` symbol.
    - **Be sure to load GTME01.map with Symbols->Load Other Map File!**
