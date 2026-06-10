    # To be inserted at 80099d7c
    .include "../../../Globals.s"
    .include "../../../m-ex/Header.s"

    .set REG_FighterData, 29
    .set REG_FighterGObj, 27
    .set text, 28
    .set REG_Text, 30

    backup

    # Get Pointers
    mr REG_FighterGObj, r3
    lwz REG_FighterData, 0x2C(REG_FighterGObj)

    # CHECK IF OSD IS ENABLED
    li r0, OSD.Wavedash             # wavedash ID
    lwz r4, MemcardData(r13)
    lwz r4, 0x1F24(r4)
    li r3, 1
    slw r0, r3, r0
    and. r0, r0, r4
    beq Exit

    # Check if fighter is a subcharacter
    mr r3, REG_FighterData
    branchl r12, 0x80005510
    cmpwi r3, 0x1
    beq Exit

    # CHECK IF IT WAS A WAVEDASH (landed within 20 frames)
    lhz r3, TM_FramesinCurrentAS(REG_FighterData)
    cmpwi r3, 20
    bgt Exit

    # Get wavedash angle
    lfs f1, TM_AirdodgeAngle(REG_FighterData)
    bl GetWavedashAngle
    stfs f1, 0x80(sp)

    # Get frames in jump
    lhz r7, TM_FramesinOneASAgo(REG_FighterData)

    # Get short or full hop
    bl ShortHopString
    mflr r8
    lwz r9, TM_ShortOrFullHop(REG_FighterData)
    cmpwi r9, 1
    bgt PrintMessage_NoHopType
    beq EndShortOrFullHop
    bl FullHopString
    mflr r8
EndShortOrFullHop:

    # Get frames since jump release
    li r9, 0
JumpInputsLoop:
    mulli r10, r9, 8
    add r10, r10, REG_FighterData

    lhz r11, TM_Inputs(r10) # buttons
    andi. r11, r11, 0xC00 # x or y
    bne EndJumpInputsLoop

    addi r10, r10, 3
    lbz r11, TM_Inputs(r10) # stick y
    extsb r11, r11
    cmpwi r11, 24
    bgt EndJumpInputsLoop

    # no jump input this frame
    addi r9, r9, 1
    cmpwi r9, 32
    blt JumpInputsLoop

    # Out of input history with no jump input - wavedash probably took longer than 32 frames.
    # Nothing we can do here unless we increase the input history range.

    b PrintMessage_NoHopType

EndJumpInputsLoop:
    lbz r10, 0x685(REG_FighterData) # timer_jump
    sub r9, r10, r9 # get jump held frames
    addi r9, r9, 1 # 1-index
    b PrintMessage

PrintMessage:
    li r3, 0                        # Message Kind
    lbz r4, 0xC(REG_FighterData)    # Message Queue
    li r5, MSGCOLOR_WHITE
    bl Wavedash_String
    mflr r6
    Message_Display
    lwz r3, 0x2C(r3)
    lwz REG_Text, MsgData_Text(r3)
    b CheckSetHopTypeColor

PrintMessage_NoHopType:
    li r3, 0                        # Message Kind
    lbz r4, 0xC(REG_FighterData)    # Message Queue
    li r5, MSGCOLOR_WHITE
    bl Wavedash_String_NoHopType
    mflr r6
    Message_Display
    lwz r3, 0x2C(r3)
    lwz REG_Text, MsgData_Text(r3)
    b SetTimingColor

CheckSetHopTypeColor:
    lwz r3, TM_ShortOrFullHop(REG_FighterData)
    cmpwi r3, 0
    beq SetTimingColor
    bl Floats
    mflr r4
    addi r5, r4, 0xC
    # Change Color
    mr r3, REG_Text                 # text pointer
    li r4, 2
    branchl r12, Text_ChangeTextColor

SetTimingColor:
    # Adjust Timing color
    lhz r3, TM_FramesinOneASAgo(REG_FighterData)
    cmpwi r3, 1
    bne CheckSetAngleColor
    bl Floats
    mflr r4
    addi r5, r4, 0xC
    # Change Color
    mr r3, REG_Text                 # text pointer
    li r4, 0
    branchl r12, Text_ChangeTextColor

CheckSetAngleColor:
    # Adjust Angle color
    # Get Angle and Float Pointer
    bl Floats
    mflr r4
    lfs f1, 0x80(sp)
    # Check For Perfect Angle
    lfs f2, 0x0(r4)
    fcmpo cr0, f1, f2
    blt Exit
    lfs f2, 0x4(r4)
    fcmpo cr0, f1, f2
    blt PerfectAngle
    lfs f2, 0x8(r4)
    fcmpo cr0, f1, f2
    bgt Exit

OKAngle:
    addi r5, r4, 0x10
    b ChangeAngleColor

PerfectAngle:
    addi r5, r4, 0xC
    b ChangeAngleColor

ChangeAngleColor:
    # Change Color
    mr r3, REG_Text                 # text pointer
    li r4, 1
    branchl r12, Text_ChangeTextColor

    b Exit

#########################
## Get Wavedash Angle ##
#########################

GetWavedashAngle:
    backup

    # Convert angle to degrees
    lfs f2, -0x3D10(rtoc)           # 0.017453
    fdivs f2, f1, f2
    # Multiply by 10 to preserve 1 decimal point
    li r3, 10
    bl IntToFloat
    fmuls f2, f1, f2
    # Cast to int to floor
    fctiwz f1, f2
    stfd f1, -0x4(sp)
    lwz r3, 0x0(sp)
    # Cast back to float
    bl IntToFloat
    # Divide by 10 to get decimal point back
    fmr f2, f1
    li r3, 10
    bl IntToFloat
    fdivs f2, f2, f1
    # Normalize to 0-90, definitely could have handled this better but its w/e
    li r3, 0
    bl IntToFloat
    fcmpo cr0, f2, f1
    bge TopQuadrant

BottomQuadrant:
    li r3, -90
    bl IntToFloat
    fcmpo cr0, f2, f1
    bgt Quadrant4

Quadrant3:
    li r3, 180
    bl IntToFloat
    fadds f2, f1, f2
    b SaveAngle

Quadrant4:
    fneg f2, f2
    b SaveAngle

TopQuadrant:
    li r3, 90
    bl IntToFloat
    fcmpo cr0, f2, f1
    bgt Quadrant2

Quadrant1:
    b SaveAngle

Quadrant2:
    fneg f2, f2
    li r3, 180
    bl IntToFloat
    fadds f2, f1, f2
    b SaveAngle

SaveAngle:
    fmr f1, f2
    restore
    blr

#########################

Wavedash_String:
    blrl
    .string "Wavedash Frame: %d\nAngle: %2.1f\n%s: %df"
    .align 2

Wavedash_String_NoHopType:
    blrl
    .string "Wavedash Frame: %d\nAngle: %2.1f"
    .align 2

ShortHopString:
    blrl
    .string "Short Hop"
    .align 2

FullHopString:
    blrl
    .string "Full Hop"
    .align 2

Floats:
    blrl
    .float 16.8                     # Great Angle Min
    .float 23.7                     # Great Angle Max
    .float 30.5                     # Good Angle Max
    .long 0x8dff6eff                # Perfect Angle
    .long 0xfff000ff                # Great Angle

##############################

IntToFloat:
    mflr r0
    stw r0, 0x4(r1)
    stwu r1, -0x100(r1)             # make space for 12 registers
    stmw r20, 0x8(r1)
    stfs f2, 0x38(r1)

    lis r0, 0x4330
    lfd f2, -0x6758(rtoc)
    xoris r3, r3, 0x8000
    stw r0, 0xF0(sp)
    stw r3, 0xF4(sp)
    lfd f1, 0xF0(sp)
    fsubs f1, f1, f2                # Convert To Float

    lfs f2, 0x38(r1)
    lmw r20, 0x8(r1)
    lwz r0, 0x104(r1)
    addi r1, r1, 0x100              # release the space
    mtlr r0
    blr

##############################

Exit:
    mr r3, REG_FighterGObj
    restore

    lwz r4, -0x514C(r13)
