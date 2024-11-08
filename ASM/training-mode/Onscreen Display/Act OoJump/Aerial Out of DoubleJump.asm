    # To be inserted at 800cc570
    .include "../../Globals.s"

    .set playerdata, 30
    .set player, 29

    backup

    mr r4, r29  # Branch to Interrupt Check With Interrupt Bool in r3 and player in r4
    branchl r12, 0x8000550c

Exit:
    restore
    cmpwi r3, 0
