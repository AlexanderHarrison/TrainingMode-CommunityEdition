# To be inserted at 800b6dec

.include "Common.s"

lwz r3, 0x4(r29)
cmpwi r3, 0xb
bne Exit

CharRng_FetchSetting r3, CharRng_Setting_Nana_Pummel_Enable, Exit
cmpwi r3, 0
beq Exit

lwz r4, 0x94(r31)

CharRng_FetchSetting r3, CharRng_Setting_Nana_Pummel_Min, Exit
cmpw r4, r3
blt ForceContinue

CharRng_FetchSetting r5, CharRng_Setting_Nana_Pummel_Max, Exit
cmpw r4, r5
bge ForceStop

subf r3, r4, r5
addi r3, r3, 1
branchl r12, HSD_Randi
cmpwi r3, 0
beq ForceStop
b ForceContinue

ForceContinue:
    fsubs f1, f1, f1
    b Exit

ForceStop:
    lfd f1, -0x6FD8(rtoc)

Exit:
    lwz r3, 0x94(r31)
