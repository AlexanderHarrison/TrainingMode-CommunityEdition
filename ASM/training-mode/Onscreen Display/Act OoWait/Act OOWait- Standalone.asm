    # To be inserted at 8000551c
    .include "../../../Globals.s"
    .include "../../../m-ex/Header.s"

    .set playerdata, 31
    .set player, 30
    .set FramesSince, 28
    .set ASBeforeWait, 27

    backupall

    # Get Player Pointers
    mr player, r3
    lwz playerdata, 0x2c(player)

CheckForFollower:
    mr r3, playerdata
    branchl r12, 0x80005510
    cmpwi r3, 0x1
    beq Exit

    # Calculate Frames Since Wait and Get AS Before Wait
    li r5, 0                                    # Loop Count
    li r4, TM_PrevASStart

WaitSearchLoop:
    mulli r3, r5, 0x2
    add r3, r3, r4
    lhzx r3, r3, playerdata
    cmpwi r3, ASID_Wait
    beq WaitSearchExit_Wait
    cmpwi r3, ASID_Landing
    beq WaitSearchExit_Landing
    addi r5, r5, 1
    cmpwi r5, 6
    bge Exit
    b WaitSearchLoop
    
WaitSearchExit_Landing:
    li FramesSince, -3
    b WaitSearchExit
    
WaitSearchExit_Wait:
    li FramesSince, 0

WaitSearchExit:
    # Get AS Before Wait
    addi r3, r5, 1
    mulli r3, r3, 0x2
    add r3, r3, r4
    lhzx ASBeforeWait, r3, playerdata
    # Get Frames In Wait
    li r4, TM_FramesInPrevASStart               # Frame Count Start

FrameCountLoop:
    cmpwi r5, 0
    beq FrameCountLoopFinish
    mulli r3, r5, 0x2
    add r3, r3, r4
    lhzx r3, r3, playerdata
    add FramesSince, r3, FramesSince
    subi r5, r5, 1
    b FrameCountLoop

FrameCountLoopFinish:
    lhz r3, TM_FramesInPrevASStart(playerdata)  # Frames spent in Wait
    add FramesSince, r3, FramesSince            # Get Total Frames Since

    # Check If Under 13 Frames
    cmpwi FramesSince, 13
    bgt Exit

    # Only Coming from Throws, Aerial Landing, and Teching/Getups
    # Aerial Landing
    cmpwi ASBeforeWait, 0x46
    blt NotThrow
    cmpwi ASBeforeWait, 0x4A
    bgt NotThrow
    b ComingFromWhitelist

NotThrow:
    # Throws
    cmpwi ASBeforeWait, 0xDB
    blt NotAerialLanding
    cmpwi ASBeforeWait, 0xDE
    bgt NotAerialLanding
    b ComingFromWhitelist

NotAerialLanding:
    # Teching/Getups
    # Aerial Landing
    cmpwi ASBeforeWait, 0xB7
    blt NotTeching
    cmpwi ASBeforeWait, 0xC9
    bgt NotTeching
    b ComingFromWhitelist

NotTeching:
    # Wavedash
    cmpwi ASBeforeWait, ASID_LandingFallSpecial
    bne NotWavedash
    b ComingFromWhitelist

NotWavedash:
    cmpwi ASBeforeWait, ASID_AttackAirN
    blt NotAerial
    cmpwi ASBeforeWait, ASID_AttackAirLe
    blt NotAerial
    b ComingFromWhitelist
    
NotAerial:
    b Exit

ComingFromWhitelist:
SpawnText:
    # Change Text Color
    cmpwi FramesSince, 0x1
    bne RedText

GreenText:
    load r3, 0x8dff6eff                         # green
    b StoreTextColor

RedText:
    load r3, 0xffa2baff

StoreTextColor:
    stw r3, 0x80(sp)

    # Create Text
    li r3, 5                                    # Message Kind
    lbz r4, 0xC(playerdata)                     # Message Queue
    li r5, MSGCOLOR_WHITE
    bl TechText
    mflr r6
    mr r7, FramesSince
    Message_Display

ChangeColor:
    lwz r3, 0x2C(r3)
    lwz r3, MsgData_Text(r3)
    li r4, 1
    addi r5, sp, 0x80
    branchl r12, Text_ChangeTextColor

    b Exit

###################
## TEXT CONTENTS ##
###################

TechText:
    blrl
    .string "Act OoWait\nFrame: %d"
    .align 2

##############################

Exit:
    restoreall
    blr
