    # To be inserted at 803a684c
    .include "../../Globals.s"
    .include "../../../m-ex/Header.s"

    .set LoopCount, 31
    .set Dictionary, 30
    .set CurrentDictOffset, 12

# Runs When the ASCII Value is 0x85 or Greater
# Katagana Starts at Dictionary Offset 0xB0

    backup

    # Search custom symbol table
    bl CustomSymbolDictionary
    mflr Dictionary
    li LoopCount, 0

Loop:
    mulli CurrentDictOffset, LoopCount, 0x3
    add CurrentDictOffset, CurrentDictOffset, Dictionary
    lbz r11, 0x0(CurrentDictOffset)
    cmpwi r11, 0xFF
    beq Exit
    cmpw r10, r11
    beq CharacterFound

IncLoop:
    addi LoopCount, LoopCount, 1
    b Loop

CharacterFound:
    # If overriding 0xA
    lbz r10, 0x0(CurrentDictOffset) # Check Matched Character
    cmpwi r10, 0x0A
    bne CharacterFound_Resume
    # Store 0x3 to the menu text string
    li r10, 0x03
    stbx r10, r3, r9
    # increment r9 by 1 to get next offset in menu text string
    addi r9, r9, 1
    # branch to 803a6b70, will increment r5 and r6 by 1 to get next offset in ASCII string and loop
    restore
    branch r12, 0x803a6b70

CharacterFound_Resume:
    lbz r10, 0x1(CurrentDictOffset) # Load Override Character
    lbz r12, 0x2(CurrentDictOffset) # Load Override Character
    restore
    mr r31, r12
    load r0, 0x8040c8c0
    branch r12, 0x803a6b08

CustomSymbolDictionary:
    blrl
    .byte 0x21, 0x81, 0x49          # Exclamation Point
    .byte 0x2f, 0x81, 0x5E          # Slash
    .byte 0x2e, 0x81, 0x44          # Period
    .byte 0x3d, 0x81, 0x81          # Equals Sign
    .byte 0x2b, 0x81, 0x7B          # Plus Sign
    .byte 0x25, 0x81, 0x93          # Percent Sign
    .byte 0x0A, 0x03, 0x00          # New Line
    .byte 0x27, 0x81, 0x66          # Apostrophe
    .byte 0x3e, 0x81, 0x84          # Greater than
    .byte 0x3c, 0x81, 0x83          # Less than
    .byte 0x5F, 0x81, 0x51          # Underscore
    .byte 0x5E, 0x81, 0x4F          # Carrot
    .byte 0x3F, 0x81, 0x48          # Question mark
    .byte 0x7c, 0x81, 0x62          # | symbol
    .byte 0x28, 0x81, 0x69          # Open Parenthesis
    .byte 0x29, 0x81, 0x6A          # Close Parenthesis
    .byte 0xFF
    .align 2

Exit:
    restore
    load r0, 0x8040c8c0
    cmplwi r10, 32

    /*
    # Katagana code
    cmpwi r10, 0x85
    blt original

    addi r31, r10, 0x2b
    branch r12, 0x803a6b38

original:
    lbzu r31, 0x0001(r6)
    */
