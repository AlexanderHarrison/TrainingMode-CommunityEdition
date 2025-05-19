    # To be inserted at 8015cf74
    .include "../../../../Globals.s"
    .include "../../../../m-ex/Header.s"

# r3 = event
# r4 = score
# r5 = event score pointer

    .set REG_EventID, 3
    .set REG_EventScore, 4
    .set REG_EventScorePointer, 5
    .set REG_PageID, 6

    backup

    # temp move eventID
    mr 7, REG_EventID
    # Get Page ID in r3 for function call
    lbz r3, CurrentEventPage(REG_EventScorePointer)
    # Get event's score offset using function
    rtocbl r12, TM_GetPageEventOffset
    mr REG_PageID, r3
    #restore REG_EventID
    mr REG_EventID, r7
    # Multiply and add to score pointer
    mulli REG_PageID, REG_PageID, 4
    add REG_EventScorePointer, REG_PageID, REG_EventScorePointer
    # Now this events offset
    mulli REG_EventID, REG_EventID, 4
    add REG_EventScorePointer, REG_EventID, REG_EventScorePointer
    # Save score
    stw REG_EventScore, 0x1A70(REG_EventScorePointer)
    b Exit

Exit:
    restore
    blr
