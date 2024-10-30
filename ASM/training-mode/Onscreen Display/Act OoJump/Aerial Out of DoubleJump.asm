    # To be inserted at 800cc570
    # (.fn ftCo_JumpAerial_IASA in asm/melee/ft/ft_0C88.s)
    .include "../../Globals.s"

    .set playerdata, 30
    .set player, 29

##########################################################
## 804a1f5c -> 804a1fd4 = Static Stock Icon Text Struct ##
## Is 0x80 long and is zero'd at the start ##
## of every VS Match ##
## Store Text Info here ##
##########################################################

    backup

    lwz playerdata, 0x2c(player)

    # Check For Interrupt
    cmpwi r3, 0x0
    beq Exit

    # CHECK IF ENABLED
    li r0, OSD.ActOoJump                     # OSD Menu ID
    # lwz r4, -0xdbc(rtoc) #get frame data toggle bits
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

    lhz r7, 0x2408(playerdata)
    cmpwi r7, 0x1
    bne RedText

GreenText:
    load r5, MSGCOLOR_GREEN
    b DisplayText

RedText:
    load r5, MSGCOLOR_RED

DisplayText:
    li r3, OSD.ActOoJump        # message kind
    lbz r4, 0xC(playerdata)     # message queue
    bl Text
    mflr r6
    Message_Display
    b Exit

###################
## TEXT CONTENTS ##
###################

Text:
    blrl
    .string "Aerial OoDoubleJump\nFrame %d"
    .align 2

##############################

Exit:
    restore
    cmpwi r3, 0
