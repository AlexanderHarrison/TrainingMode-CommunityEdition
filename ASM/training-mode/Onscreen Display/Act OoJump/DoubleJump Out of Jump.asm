    # To be inserted at 800cb400
    .include "../../Globals.s"

    .set playerdata, 31
    .set player, 30

    backup

    mr r4, r30  # Branch to Interrupt Check With Interrupt Bool in r3 and player in r4
    branchl r12, 0x8000550c

Exit:
    restore
    cmpwi r3, 0
