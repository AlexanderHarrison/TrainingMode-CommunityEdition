#ifndef MEX_H_PRELOAD
#define MEX_H_PRELOAD

#include "structs.h"
#include "datatypes.h"

#define PRELOADFLAG_FTCOSTUME (1 << 0) // 0x01, fighter costume files
#define PRELOADFLAG_FTDAT (1 << 1)     // 0x02, main fighter PlX dat files
#define PRELOADFLAG_EFFECT (1 << 2)    // 0x04, effect files
#define PRELOADFLAG_FTANIM (1 << 3)    // 0x08, fighter animations
#define PRELOADFLAG_ALLOC (1 << 4)     // 0x10, special allocs like fod reflection
#define PRELOADFLAG_x20 (1 << 5)       // 0x20, unk
#define PRELOADFLAG_PERSIST (1 << 6)   // 0x40, IfAll / EfMnData / EfCoData / ItCo
#define PRELOADFLAG_x80 (1 << 7)       // 0x80, LbRb

typedef enum PreloadEntryLoadState // 0x0 of PreloadEntry
{
    PRELOADENTRYSTATE_NONE,
    PRELOADENTRYSTATE_QUEUED,
    PRELOADENTRYSTATE_LOADING,
    PRELOADENTRYSTATE_LOADED,
    PRELOADENTRYSTATE_ACCESSED,
} PreloadEntryState;

typedef enum PreloadEntryInitKind // 0x3 of PreloadEntry
{
    PRELOADENTRYINITKIND_NOINIT,     // do not initialize the file when accessed
    PRELOADENTRYINITKIND_REQINIT,    // request the file be initialized upon being accessed
    PRELOADENTRYINITKIND_INITALIZED, // file has been initialized
} PreloadEntryInitKind;

typedef enum PreloadHeapKind // 0x2 of PreloadEntry
{
    PRELOADHEAPKIND_MAINDRAM,         // main DRAM heap
    PRELOADHEAPKIND_MAINARAM,         // main ARAM heap
    PRELOADHEAPKIND_LBRB,             // seems to only contain LbRb.dat
    PRELOADHEAPKIND_PERSIST,          // IfAll, ItCo, EfMnData, LbRb
    PRELOADHEAPKIND_FIGHTERCACHE,     // fighter files like main dat, costume, and effects. also stage and special allocations (entrynum 2000+)
    PRELOADHEAPKIND_FIGHTERANIMCACHE, // fighter animations

    // custom heap that is resized dynamically to keep the major scene permenantly loaded.
    // i dont even think its fair to consider this a preload heap because it doesnt
    // adhere to the same rules as the others. its not created in the same way and
    // will crash without some hacky asm that constantly enables it so thegame
    // doesnt attempt to free or recreate it. this is because the preload heap "library"
    // isnt initialized in time when major scene files need to be loaded
    // (it steals its memory from PRELOADHEAPKIND_MAINDRAM).
    PRELOADHEAPKIND_MEXMAJORSCENE,

} PreloadHeapKind;

typedef enum PreloadFileKind // 0x1 of PreloadEntry
{
    PRELOADFILEKIND_ALLOC,   // special alloc, code @ 80017c7c will skip loading a file
    PRELOADFILEKIND_1,       // these will always result in an assertion?? 800180dc
    PRELOADFILEKIND_DEFAULT, // will only init the archive
    PRELOADFILEKIND_EFFECT,  // will init particle bank and get a symbol from it @ 80018110
    PRELOADFILEKIND_STAGE,   // will init get stage symbol and init its ptcl bank @ 80018130
    PRELOADFILEKIND_NUM,     // m-ex uses this as a custom file_kind. its main purpose to not be marked as unneeded every time the preload table is updated (cant preload arbitrary files in vanilla melee)
} PreloadFileKind;

struct PreloadHeapDesc // array of these are now located in m-ex injection @ 80015f88
{
    int id;
    int location;
    int pri;
    int size;
};

struct PreloadHeap //
{
    int heap_index;   // 0x0, is a heap ID returned by OSCreateHeap, is = -1 if the heap doesnt use OSCreateHeap
    void *handle;     // 0x4, name evidenced by asset @ 80014f10
    void *addr_start; // 0x8
    int size;         // 0xC
    int pri;          // 0x10. 0 will allocate using OSCreateHeap, 1 will create heap using 80014e24. 2 = use main heap AND handles?? has something to do with how the heap is built. check m-ex injection @ 800159b4. is referenced @ 80015d0c when freeing a preloaded file. if this is 1 it will properly free up the preload entry
    int x14;          // 0x14
    int is_disable;   // 0x18
};

struct PreloadHeapLookup
{
    // these dont seem to change for the entirety of runtime
    // i guess they're referenced when rebuilding heaps between scenes
    void *dram_addr_start; // 0x0
    void *dram_addr_end;   // 0x4
    void *aram_addr_start; // 0x8
    void *aram_addr_end;   // 0xC

    PreloadHeap heap_arr[7]; // 0x10, indexed by PreloadHeapKind
};

struct PreloadChar
{
    int kind;
    u8 costume;
};
struct PreloadTable
{
    int x0;
    void *x4;
    u8 x8;
    int stage;               // 0xc
    PreloadChar fighters[8]; // 0x10
    int defrag_num;          // 0x50
};

struct PreloadAllocData
{
    PreloadAllocData *next;
    void *alloc; // 0x4
    int size;    // 0x8
};
struct PreloadEntry // scene changes wait for asyncronous disc reads to finish if they are logged as a PreloadEntry!
{
    u8 load_state;                 // 0x0, 0 = none, 1 = queued, 2 = loading, 3 = loaded, 4 = persistently loaded (IfAll, etc)
    u8 file_kind;                  // 0x1, PreloadFileKind, alters the initialization behavior when the file is accessed with Preload_GetArchive/Preload_GetArchiveByEntrynum.its the value @ 0x0 in the tables relocated at m-ex injection 800186a8. 0 is used by special allocs (wont actually load anything, will auto mark as loaded), 2 is used by fighter files and ifall/itco/lbrb. 3 is used by efcodata and efmndata. 4 is used by stage files
    s8 heap_kind;                  // 0x2, these heap kinds are defined in m-ex injection 80015f88 and in the header as PreloadHeapKind
    u8 init_kind;                  // 0x3, 0, do not init archive, 1 = init archive when accessed with Preload_GetArchive/Preload_GetArchiveByEntrynum. 0 = fighter anims/special allocs (dont initialize), 1 = fighter files/itco/ifall/efco/efmn (queued for initialization), 2 = lbrb/ifall/stage (initialized)
    u8 flags;                      // 0x4, indicates the purpose of the file. referenced when waiting for files using Preload_CheckIfFilesAreLoading. costume files are 0x1, plX files are 0x2, effect files are 0x4, anim files are 0x8, stage files and allocs are 0x10, IfAll/EfMnData/EfCoData/ItCo are 0x40, LbRb is 0x80
    u16 file_entry_num;            // 0x6, if this is greater than 2000 its a special allocation (like fountain reflection (2001) or pokemon stadium transformation (200?))
    s16 status;                    // 0x8, is -9999 when marked for unload, 9999 when needed
    int file_size;                 // 0xc
    PreloadAllocData *file_data;   // 0x10, this gets allocated @ 80017c00
    PreloadAllocData *header_data; // 0x14
    int x18;                       // 0x18
};

struct Preload
{
    // int heap_kind;          // 0x0, copied from current MinorScene struct,
    PreloadTable queued;    // 0x4, modify this to request files to preload
    PreloadTable updated;   // 0x?
    PreloadEntry entry[80]; // 0xac
};

static PreloadHeapLookup *stc_preload_heaps_lookup = (void *)0x804dfb64;

/*** Functions ***/
void Preload_Update();                                                                                                                                               // checks for new files to preload and begins loading them synchronously
void Preload_Invalidate();                                                                                                                                           // invalidates cache, this is usually used from scene prep on CSS's
Preload *Preload_GetTable();                                                                                                                                         // gets preload table
PreloadEntry *Preload_CreateEntry(PreloadFileKind file_kind, char *filename, int r5, int heap_kind, int file_size, int is_init_archive, int r9, int flags, int r11); // creates a preload entry for any given file/ (file_kind is 0 for stuff like fod reflection, r5 = 4, r9 = 7) 80017740
PreloadEntry *Preload_CreateEntryByEntrynum(int file_kind, int entrynum, int r5, int heap_kind, int file_size, int is_init_archive, int r9, int flags, int r11);     // creates a preload entry for any given file/ (file_kind is 0 for stuff like fod reflection, r5 = 4, r9 = 7) 80017740
void Preload_RequestLoadEntry(int preload_entry_index);                                                                                                              //
void Preload_FreeEntry(int preload_entry_index);                                                                                                                     // 800174e8
HSD_Archive *Preload_GetArchive_Init_ReturnSymbols(int unk, char *filename, void *symbol_return, ...);                                                               // variadic args are symbol strings, terminate the variadic args with a 0
HSD_Archive *Preload_GetArchive(char *filename);                                                                                                                     //
HSD_Archive *Preload_GetArchiveByEntrynum(int entrynum);                                                                                                             // will wait if the file isnt loaded yet
int Preload_CheckFileStatus(int entrynum);                                                                                                                           // 0 = not loading, 1 = loading, 2 = loaded
int Preload_CheckIfFilesAreLoading(int preload_flags);                                                                                                               // use PRELOADFLAG_ definitions
void Preload_FreeUnneededInitializedArchivesInHeap(PreloadHeapKind heap_kind);                                                                                       // arg is heap_kind in PreloadEntry
void Preload_Wait(int preload_flags);                                                                                                                                // will wait until all preload entries with the flags specified are finished loading before returning, use PRELOADFLAG_ definitions
void Preload_LoadFighter(CharacterKind c_kind, int costume);
void *Preload_AllocFromHeap(PreloadHeapKind heap_kind, int size);
void Preload_FreeToHeap(void *ptr, int size);
#endif
