    # To be inserted at 800b6ff4 

    .include "Common.s"

    lwz r3, 0x4(r29) # internal char id
    cmpwi r3, 0xb    # Nana
    bne Exit
    CharRng_FetchSetting r3, CharRng_Setting_Nana_Throw, Exit
    cmpwi r3, 0 # 0 = default setting
    beq Exit

    cmpwi r3, 5
    beq FwdDownThrow         # value 5: skip zeroing, use raw f1 for coin flip

    lfd f31, -0x6F88(rtoc) # loads 0.25
    fsubs f1, f1, f1       # f1 = 0.0

    cmpwi r3, 1
    beq Fthrow
    cmpwi r3, 2
    beq Bthrow
    cmpwi r3, 3
    beq Uthrow
    cmpwi r3, 4
    beq Dthrow
    b Exit

FwdDownThrow:
    lfs f0, -0x6F94(rtoc)
    fcmpo cr0, f1, f0
    lfd f31, -0x6F88(rtoc)
    fsubs f1, f1, f1
    bge Fthrow
    b Dthrow

    # The CPU AI throws randomly based on a random float. Each throw has a 0.25
    # probablitity [0,.25)=up, [.25,.5)=down, [.5,.75)=fwd, [.75,1)=back
Bthrow:
    fadds f1, f1, f31
Fthrow:
    fadds f1, f1, f31
Dthrow:
    fadds f1, f1, f31
Uthrow:

Exit:
    fmr f31, f1 # original line
