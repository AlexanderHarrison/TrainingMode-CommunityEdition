    # To be inserted at 801beb8c
    .include "../../../../Globals.s"

    backup

    mr r4, r3
    lwz r3, MemcardData(r13)
    lbz r3, CurrentEventPage(r3)
    rtocbl r12, TM_GetScoreType

    restore
    blr
