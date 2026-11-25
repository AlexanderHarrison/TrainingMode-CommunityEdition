.include "../../../Globals.s"

.set CharRng_Setting_Peach_Item, 0x0
.set CharRng_Setting_Peach_FSmash, 0x1
.set CharRng_Setting_Luigi_Misfire, 0x2
.set CharRng_Setting_GnW_Hammer, 0x3
.set CharRng_Setting_Nana_Throw, 0x4
.set CharRng_Setting_Nana_Pummel_Enable, 0x5
.set CharRng_Setting_Nana_Pummel_Min, 0x6
.set CharRng_Setting_Nana_Pummel_Max, 0x7

.macro CharRng_FetchSetting reg, setting, default_branch
load \reg, EventVars_Ptr
lwz \reg, 0x0(\reg)
lwz \reg, EventVars_RNGControl(\reg)
lbz \reg, \setting(\reg)
.endm
