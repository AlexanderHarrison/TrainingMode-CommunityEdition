    # To be inserted at 8024d92c
    .include "../../../Globals.s"

    backup

    # This checks and stores if triggers were pressed this frame in r27 (L), r28 (R)
    # We need to only process trigger events on press, not hold.
    # But a lot of people DONT have hard press on triggers, only analog press. And they complained a lot.
    # So we need to check analog input as well as hard input and combine the two.
    # And because analog triggers don't register the down field in HSD_Pad, we need to do the tracking ourselves.
    # And we need to process EVERY pad! Because the combined pad used for menus (0x80479C30) doesn't store analog triggers!
    # What a pain!

    li r27, 0 # triggerl was just pressed
    li r28, 0 # triggerr was just pressed

    # put maximum analog values in r27, r28
    li r25, 0
    load r26, 0x804c1fac # master pad array
    CheckAnalogTriggerLoop:
        lbz r3, 0x1C(r26)
        cmpw r3, r27
        blt EndNewMaxLTrigger
            mr r27, r3
    EndNewMaxLTrigger:
    
        lbz r3, 0x1D(r26)
        cmpw r3, r28
        blt EndNewMaxRTrigger
            mr r28, r3
    EndNewMaxRTrigger:
    
        addi r25, r25, 1
        addi r26, r26, 0x44
        cmpwi r25, 4
        bne CheckAnalogTriggerLoop
    
    bl EndStoreTriggers
    .byte 0 # prev analog L
    .byte 0 # prev analog R
    .align 2
EndStoreTriggers:
    mflr r3
    
    # r27 = new_l >= thresh && old_l < thresh
    lbz r4, 0x0(r3)
    stb r27, 0x0(r3)
    mr r5, r27
    li r27, 0
    cmpwi r4, AnalogTriggerThreshold
    bge EndTriggerL
    cmpwi r5, AnalogTriggerThreshold
    blt EndTriggerL
    li r27, 1
EndTriggerL:

    # r28 = new_r >= thresh && old_r < thresh
    lbz r4, 0x1(r3)
    stb r28, 0x1(r3)
    mr r5, r28
    li r28, 0
    cmpwi r4, AnalogTriggerThreshold
    bge EndTriggerR
    cmpwi r5, AnalogTriggerThreshold
    blt EndTriggerR
    li r28, 1
EndTriggerR:

    load r3, 0x80479cf0 # combined pad
    lwz r4, 0xC(r3) # down
    
    andi. r0, r4, 0x40
    beq EndTriggerLHard
    li r27, 1
EndTriggerLHard:
    
    andi. r0, r4, 0x20
    beq EndTriggerRHard
    li r28, 1
EndTriggerRHard:

    bl StoreHelpGObjPtr
    .long 0
    .long 0
    .long 0
StoreHelpGObjPtr:
    mflr r29
    lwz r4, 0x0(r29)
    cmpwi r4, 0
    beq NotInHelp

    lbz r4, 0x8(r29)
    cmpwi r4, 0
    beq CheckCloseHelp
    subi r4, r4, 1
    stb r4, 0x8(r29)
    lwz r4, 0xC(r3)
    andi. r4, r4, 0x20
    beq CheckCloseHelp
    li r28, 0

CheckCloseHelp:
    lwz r4, 0x8(r3) # down
    or r4, r4, r27
    or r4, r4, r28
    cmpwi r4, 0
    bne CloseHelp
    lwz r4, 0x18(r3) # stick/cstick x/y
    cmpwi r4, 0
    bne CloseHelp
    b exit
    
NotInHelp:
    cmpwi r27, 0
    bne OpenFDD

    cmpwi r28, 0
    bne OpenHelp

CheckToSwitchPage:
    li r3, 4
    branchl r12, Inputs_GetPlayerRapidInputs
    # Check For Left
    li r5, -1
    rlwinm. r0, r3, 0, 25, 25
    bne SwitchPage
    # Check For Right
    li r5, 1
    rlwinm. r0, r3, 0, 24, 24
    bne SwitchPage
    b exit

OpenFDD:
    # PLAY SFX
    li r3, 1
    branchl r4, SFX_MenuCommonSound

    # SET FLAG IN RULES STRUCT
    li r0, 3                    # 3 = frame data from event toggle
    load r3, 0x804a04f0
    stb r0, 0x0011(r3)

    # SET SOMETHING
    li r0, 5
    sth r0, -0x4AD8(r13)

    # BACKUP CURRENT EVENT ID
    lwz r3, -0x4A40(r13)
    lwz r5, 0x002C(r3)
    lbz r3, 0x0(r5)
    lwz r4, 0x4(r5)
    add r3, r3, r4
    lwz r4, MemcardData(r13)
    stb r3, 0x0535(r4)

    # LOAD RSS
    branchl r3, 0x80237410

    # REMOVE EVENT THINK FUNCTION
    lwz r3, -0x3E84(r13)
    branchl r12, GObj_Destroy

    b exit
    
CloseHelp:
    lwz r3, 0x0(r29)
    branchl r12, GObj_Destroy
    lwz r3, 0x4(r29)
    branchl r12, Text_RemoveText
    li r3, 0
    stw r3, 0x0(r29)
    stw r3, 0x4(r29)
    b exit
    
OpenHelp:
    lwz r4, 0xC(r3)
    andi. r4, r4, 0x20
    bne OpenHelp_Create
    li r4, 15
    stb r4, 0x8(r29)
OpenHelp_Create:
    li r3, 0
    li r4, 0
    li r5, 0
    branchl r12, GObj_Create
    stw r3, 0x0(r29)
    bl HelpGX_Ptr
    mflr r4
    li r5, 17
    li r6, 0
    branchl r12, GObj_AddGXLink

    li r3, 0
    li r4, 1
    branchl r12, Text_CreateTextStruct
    mr r23, r3
    stw r3, 0x4(r29)

    # use kerning
    li r4, 0x1
    stb r4, 0x49(r23)

    # black text
    lis r4, 0x1010
    ori r4, r4, 0x10FF
    stw r4, 0x30(r23)

    # white bg
    lis r4, 0xE0E0
    ori r4, r4, 0xE0FF
    stw r4, 0x2C(r23)

    bl EndHelpInfo
    .float 90 # text x pos
    .float 350 # text y pos
    .float 0.0306 # canvas x scale
    .float 0.0306 # canvas y scale
    .float -9.69 # canvas x pos
    .float -7 # canvas y pos
    .float 2 # text scale
    .float 1 # subtext scale
    .float 115 # subtext x pos
    .float 430 # subtext y pos
EndHelpInfo:
    mflr r24

    # scale canvas
    lfs f1, 0x8(r24)
    lfs f2, 0xC(r24)
    stfs f1, 0x24(r23)
    stfs f2, 0x28(r23)

    # move canvas
    lfs f1, 0x10(r24)
    lfs f2, 0x14(r24)
    stfs f1, 0x0(r23)
    stfs f2, 0x4(r23)

    bl EndHelpText
    .string "TM-CE Tips"
    .align 2
EndHelpText:
    mflr r4

    # init text
    lfs f1, 0x0(r24) # x offset
    lfs f2, 0x4(r24) # y offset
    branchl r12, Text_InitializeSubtext

    mr r4, r3
    mr r3, r23
    lfs f1, 0x18(r24) # x scale
    lfs f2, 0x18(r24) # y scale
    branchl r12, Text_UpdateSubtextSize

bl EndHelpSubText
    .string "Tutorial Video Series"
    .align 2
EndHelpSubText:
    mflr r4

    # init subtext
    mr r3, r23
    lfs f1, 0x20(r24) # x offset
    lfs f2, 0x24(r24) # y offset
    branchl r12, Text_InitializeSubtext

    mr r4, r3
    mr r3, r23
    lfs f1, 0x1C(r24) # x scale
    lfs f2, 0x1C(r24) # y scale
    branchl r12, Text_UpdateSubtextSize

    b exit

SwitchPage:
    # Get number of pages
    rtocbl r12, TM_GetPageNum
    mr r6, r3
    # Change page
    lwz r4, MemcardData(r13)
    lbz r3, CurrentEventPage(r4)
    add r3, r3, r5
    stb r3, CurrentEventPage(r4)

# Check if within page bounds
SwitchPage_CheckHigh:
    cmpw r3, r6
    ble SwitchPage_CheckLow
    # Stay on current page
    subi r3, r3, 1
    stb r3, CurrentEventPage(r4)
    b exit

SwitchPage_CheckLow:
    cmpwi r3, 0
    bge SwitchPage_ChangePage
    # Stay on current page
    li r3, 0
    stb r3, CurrentEventPage(r4)
    b exit

SwitchPage_ChangePage:
    # Get Page Name string
    lwz r3, MemcardData(r13)
    lbz r3, CurrentEventPage(r3)
    rtocbl r12, TM_GetPageName
    # Update Page Name
    mr r5, r3
    lwz r3, -0x4EB4(r13)
    li r4, 0
    branchl r12, Text_UpdateSubtextContents

    # Reset cursor to 0, 0
    lwz r5, -0x4A40(r13)
    lwz r5, 0x002C(r5)
    li r3, 0
    stw r3, 0x0004(r5)          # Selection Number
    stb r3, 0(r5)               # Page Number

# Redraw Event Text
SwitchPage_DrawEventTextInit:
    li r29, 0                   # loop count
    lwz r3, 0x0004(r5)          # Selection Number
    lbz r4, 0(r5)               # Page Number
    add r28, r3, r4

SwitchPage_DrawEventTextLoop:
    mr r3, r29
    add r4, r29, r28
    branchl r12, 0x8024d15c
    addi r29, r29, 1
    cmpwi r29, 9
    blt SwitchPage_DrawEventTextLoop

    # Redraw Event Description
    lwz r3, -0x4A40(r13)
    mr r4, r28
    branchl r12, 0x8024d7e0

    # Update High Score
    lwz r3, -0x4A40(r13)
    li r4, 0
    branchl r12, 0x8024d5b0

    # Update cursor position
    # Get Texture Data
    lwz r3, -0x4A40(r13)
    lwz r3, 0x0028(r3)
    addi r4, sp, 0x40
    li r5, 11
    li r6, -1
    crclr 6
    branchl r12, 0x80011e24
    lwz r3, 0x40(sp)
    # Change Y offset?
    li r0, 0
    stw r0, 0x3C(r3)
    # DirtySub
    branchl r12, HSD_JObjSetMtxDirtySub

    # Play SFX
    li r3, 2
    branchl r12, SFX_MenuCommonSound
    
    b exit

#######################################

HelpGX_Ptr:
    blrl
    
HelpGX:
    backup
    
    bl QRData
    # 37 squares on each side
    .long 0x18AE33C, 0x3EBF3BA, 0xAF9142D6, 0xDD448A26
    .long 0x655A245A, 0xE02D513E, 0xCC5ADAF8, 0x5555550
    .long 0x1FE8309A, 0xFF7D156E, 0x898EF548, 0xDFE14117
    .long 0x127251C4, 0x7BCC9765, 0xDCA7E6DB, 0x5F3E3731
    .long 0x5E26316F, 0x9885FAAF, 0xBD51D980, 0xD095D3D4
    .long 0xDBFF1461, 0x2C521334, 0x5454FBF5, 0xED023351
    .long 0x95BF1517, 0x65AB0082, 0xD7809260, 0xA19B07D5
    .long 0x55A771F2, 0x139E8795, 0xD4301AD0, 0x289F2A9C
    .long 0xE72686A8, 0x4201FF87, 0xB173B006, 0xF023D5CF
    .long 0xB13FCCEF, 0x45DAC900, 0x4A2FAD85, 0xEC916FFE
    .long 0x65E0FB84, 0xCFEA6017, 0x3A672F7F
QRData:
    .set qr, r31
    mflr qr

    cmpwi r4, 2
    bne HelpGX_Exit

    bl Attrs
    .float 210 # start x
    .float 50 # start y
    .float 6 # square size
    .long 0x101010ff # black
    .long 0xe0e0e0ff # white
    .float 3
    .float 40
Attrs:
    .set attrs, r30
    mflr attrs

    li r3, 0x80 # GX_QUADS
    li r4, 0
    li r5, 2728
    branchl r12, GXBegin

    .set x, f1
    .set y, f2
    .set x_min, f3
    .set y_min, f4
    .set square_size, f5
    .set x_end, f6
    .set y_end, f7
    .set x_i, r3
    .set y_i, r4
    .set bit, r5
    .set bit_i, r6
    .set gxpipe, r7
    .set color, r8
    
    lfs x_min, 0x0(attrs)
    lfs y_min, 0x4(attrs)
    lfs square_size, 0x8(attrs)
    load gxpipe, GXPipe
    
    # draw background and border
    fmr x, x_min
    fmr y, y_min
    lfs f8, 0x14(attrs) # 3
    fmuls f8, f8, square_size
    fsubs x, x_min, f8
    fsubs y, y_min, f8
    lfs f8, 0x18(attrs) # 40
    fmuls f8, f8, square_size
    fadds x_end, x_min, f8
    fadds y_end, y_min, f8
    lwz color, 0x10(attrs)
    stfs x, 0(gxpipe)
    stfs y, 0(gxpipe)
    stw color, 0(gxpipe)
    stfs x_end, 0(gxpipe)
    stfs y, 0(gxpipe)
    stw color, 0(gxpipe)
    stfs x_end, 0(gxpipe)
    stfs y_end, 0(gxpipe)
    stw color, 0(gxpipe)
    stfs x, 0(gxpipe)
    stfs y_end, 0(gxpipe)
    stw color, 0(gxpipe)

    lwz color, 0xC(attrs)
    li bit_i, 31

    fmr y, y_min
    li y_i, 0
HelpGX_NextRow:
    fmr x, x_min
    li x_i, 0
    
    HelpGX_NextCol:
        # find vtx colour from bit
        cmpwi bit_i, 0
        bge HelpGX_EndIncWord
            addi qr, qr, 4
            li bit_i, 31
        HelpGX_EndIncWord:

        # check if bit is set
        lwz bit, 0x0(qr)
        srw bit, bit, bit_i
        andi. bit, bit, 1
        bne HelpGX_EndDraw

        fadds x_end, x, square_size
        fadds y_end, y, square_size

        # write black to pipe
        stfs x, 0(gxpipe)
        stfs y, 0(gxpipe)
        stw color, 0(gxpipe)
        stfs x_end, 0(gxpipe)
        stfs y, 0(gxpipe)
        stw color, 0(gxpipe)
        stfs x_end, 0(gxpipe)
        stfs y_end, 0(gxpipe)
        stw color, 0(gxpipe)
        stfs x, 0(gxpipe)
        stfs y_end, 0(gxpipe)
        stw color, 0(gxpipe)

        HelpGX_EndDraw:

        addi bit_i, bit_i, -1
        addi x_i, x_i, 1
        fadds x, x, square_size
        cmpwi x_i, 37
        bne HelpGX_NextCol

    addi y_i, y_i, 1
    fadds y, y, square_size
    cmpwi y_i, 37
    bne HelpGX_NextRow

HelpGX_Exit:
    restore
    blr

#######################################

exit:
    restore
    li r0, 16
