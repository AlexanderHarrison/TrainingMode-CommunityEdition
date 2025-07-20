# To be inserted at 80142ae0

.include "Common.s"

# r3 = output, 0 is misfire, 1 is no misfire
# r31 contains Fighter

CharRng_FetchSetting r6, CharRng_Setting_Luigi_Misfire, RandomMisfire

cmpwi r6, 1
blt+ RandomMisfire
beq AlwaysMisfire

NeverMisfire:
  li r3, 1
  b Exit

RandomMisfire:
  # original line
  branchl r12, HSD_Randi
  b Exit

AlwaysMisfire:
  li r3, 0

Exit:
