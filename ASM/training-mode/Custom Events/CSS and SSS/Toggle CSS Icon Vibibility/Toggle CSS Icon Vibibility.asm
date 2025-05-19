    # To be inserted at 80264578
    .include "../../../../Globals.s"
    .include "../../../../m-ex/Header.s"

    backup

    .set REG_GObj, 31
    .set REG_Data, 30
    .set REG_Whitelist, 29

    # Ensure this is event mode
    load r3, SceneController
    lbz r3, Scene.CurrentMajor(r3)
    cmpwi r3, Scene.EventMode
    bne Original

    # Create GObj
    li r3, 4
    li r4, 5
    li r5, 0
    branchl r12, GObj_Create
    mr REG_GObj, r3

    # Create Data
    li r3, 8
    branchl r12, HSD_MemAlloc
    mr REG_Data, r3

    # Attach Data
    mr r3, REG_GObj
    li r4, 14
    load r5, HSD_Free
    mr r6, REG_Data
    branchl r12, GObj_AddUserData

    # Attach Process
    bl ToggleCSSIcon
    mflr r4
    li r5, 2                    # Priority (after CSS_BigFunctionMonitorInputsAndMore)
    branchl r12, GObj_AddProc

    # Store Whitelist to GObj Data
    # Get Current Event Number
    lwz r4, MemcardData(r13)
    lbz r3, 0x0535(r4)
    # Get Current Page
    lbz r4, CurrentEventPage(r4)
    rtocbl r12, TM_GetEventCharList
    stw r3, 0x0(REG_Data)
    mr REG_Whitelist, r3
    # Check if exists
    cmpwi REG_Whitelist, -1
    beq Original

    # Make P1 Undecided
    lwz r20, CSS_Data(r13)      # CSS Match Info
    addi r4, r20, 0x70          # Get Player Info Start
    lhz r5, 0x0(r20)            # Get Player in control of CSS
    subi r5, r5, 1              # Zero index cause Yasuyuki Nagashima is dumb
    mulli r5, r5, 36
    add r4, r4, r5              # r4 now contains the players info
    # Check if 0x21(N/A)
    li r3, 0x21
    stb r3, 0x0(r4)
    b Original

#######################
ToggleCSSIcon:
    blrl
    .set REG_Whitelist, 31

    backup

    # Get Whitelist
    lwz r3, 0x2C(r3)
    lwz REG_Whitelist, 0x0(r3)
    # Check if whitelist exists
    cmpwi REG_Whitelist, -1
    beq ToggleCSSIcon_Exit

ToggleCSSIcon_Start:
    # Get P1's Cursor Data
    load r3, CSS_CursorPointers # Load CSS Cursor Pointers
    lwz r5, 0x0(r3)             # Get P1's Cursor Data

    # Check if holding a puck
    lbz r3, 0x5(r5)             # Check hand state
    cmpwi r3, 0x1
    bne ToggleCSSIcon_HoldingP1Puck
    # Check if holding P1's puck
    lbz r3, 0x6(r5)
    cmpwi r3, 0x0
    beq ToggleCSSIcon_HoldingP1Puck
    # Check if holding CPU's puck
    cmpwi r3, 0x1
    beq ToggleCSSIcon_HoldingCPUPuck
    b ToggleCSSIcon_Exit

ToggleCSSIcon_HoldingP1Puck:
    mr r3, REG_Whitelist
    bl ToggleCSSIcon_ToggleSelectedIcons
    b ToggleCSSIcon_Exit

ToggleCSSIcon_HoldingCPUPuck:
    addi r3, REG_Whitelist, 0x4 # CPU List is at 0x4
    bl ToggleCSSIcon_ToggleSelectedIcons
    b ToggleCSSIcon_Exit

ToggleCSSIcon_Exit:
    restore
    blr

# ******************************#

############################
## Show Whitelisted Chars ##
############################
ToggleCSSIcon_ToggleSelectedIcons:
    .set REG_IconDataStart, 31
    .set REG_Whitelist, 30
    .set REG_IconData, 29
    .set REG_VisibilityBool, 28
    .set REG_LoopCount, 27

    backup

ToggleCSSIcon_ToggleSelectedIconsInit:
    # Init Stuff
    lwz REG_Whitelist, 0x0(r3)
    li REG_LoopCount, 25
    load REG_IconDataStart, 0x803f0b24
    b ToggleCSSIcon_ToggleSelectedIconsIncLoop

ToggleCSSIcon_ToggleSelectedIconsLoop:
    # Get Icons Data
    mulli r3, REG_LoopCount, 0x1C
    add REG_IconData, r3, REG_IconDataStart
    # Check if icon should be visible
    li r3, 1
    slw r0, r3, REG_LoopCount
    and. r0, r0, REG_Whitelist
    bne ToggleCSSIcon_ToggleSelectedIcons_Display

ToggleCSSIcon_ToggleSelectedIcons_Hide:
    li REG_VisibilityBool, 0
    b 0x8

ToggleCSSIcon_ToggleSelectedIcons_Display:
    li REG_VisibilityBool, 1
    # Set Invisible
    # Get JObj
    lwz r3, -0x49E0(r13)
    addi r4, sp, 0x80
    lbz r5, 0x5(REG_IconData)
    crclr 6
    li r6, -1
    branchl r12, 0x80011e24
    # Toggle invisibility flag
    lwz r3, 0x80(sp)
    li r5, 1
    cmpwi REG_VisibilityBool, 0
    beq 0x8
    li r5, 0
    lwz r4, 0x14(r3)
    rlwimi r4, r5, 4, 27, 27
    stw r4, 0x14(r3)
    # Set interactable
    li r3, 0
    cmpwi REG_VisibilityBool, 0
    beq 0x8
    li r3, 2
    stb r3, 0x2(REG_IconData)

ToggleCSSIcon_ToggleSelectedIconsIncLoop:
    subi REG_LoopCount, REG_LoopCount, 1
    cmpwi REG_LoopCount, 0
    bge ToggleCSSIcon_ToggleSelectedIconsLoop

ToggleCSSIcon_ToggleSelectedIconsExit:
    restore
    blr

# ******************************#
Original:
    restore
    lbz r4, -0x3E57(r13)
