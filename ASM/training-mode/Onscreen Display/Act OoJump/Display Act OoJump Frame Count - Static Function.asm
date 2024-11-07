    # To be inserted at 8000550c
    .include "../../Globals.s"
    .include "../../../m-ex/Header.s"

    .set playerdata, 31
    .set player, 30

    backup

    # This static function needs to store Interrupt Bool to r3 and store player to r4 before being called
    mr player, r4
    lwz playerdata, 0x2c(player)

    # Check For Interrupt
    cmpwi r3, 0x0
    beq Exit

    # CHECK IF ENABLED
    li r0, OSD.ActOoJump
    lwz r4, -0x77C0(r13)
    lwz r4, 0x1F24(r4)
    li r3, 1
    slw r0, r3, r0
    and. r0, r0, r4
    beq Exit

CheckForFollower:
    mr r3, playerdata
    branchl r12, 0x80005510
    cmpwi r3, 0x1
    beq Exit

CheckForPreviousFrame:
    lhz r7, TM_FramesinOneASAgo(playerdata)
    cmpwi r7, 0x1
    bne RedText

GreenText:
    load r5, MSGCOLOR_GREEN
    b CheckForDoubleJump

RedText:
    load r5, MSGCOLOR_RED

CheckForDoubleJump:
    lwz r3, 0x10(playerdata) # load current action state ID
    cmpwi r3, ASID_JumpAerialF
    beq DoubleJump
    cmpwi r3, ASID_JumpAerialB
    beq DoubleJump

CheckForAerial:
    cmpwi r3, ASID_AttackAirN
    blt CheckForSpecialMove
    cmpwi r3, ASID_AttackAirLw
    bgt CheckForSpecialMove
    b Aerial

CheckForSpecialMove:
    cmpwi r3, ASID_BarrelCannonWait
    ble Exit
    b SpecialMove

DoubleJump:
    li r3, OSD.Miscellaneous    # message kind / Originally OSD.ActOoJump is natural, but use other ID to show at the same time as Aerial OoJump for fighters with DoubleJump cancel techniques
    bl DoubleJumpText
    mflr r6
    b DisplayText

Aerial:
    li r3, OSD.ActOoJump        # message kind
    bl AerialText
    mflr r6
    b DisplayText

SpecialMove:
    li r3, OSD.ActOoJump        # message kind
    bl SpecialMoveText
    mflr r6

DisplayText:
    lbz r4, 0xC(playerdata)     # message queue
    Message_Display
    b Exit

###################
## TEXT CONTENTS ##
###################

DoubleJumpText:
    blrl
    .string "DoubleJump OoJump\nFrame %d"
    .align 2

AerialText:
    blrl
    .string "Aerial OoJump\nFrame %d"
    .align 2

SpecialMoveText:
    blrl
    .string "Special OoJump\nFrame %d"
    .align 2

##############################

Exit:
    restore
    blr
