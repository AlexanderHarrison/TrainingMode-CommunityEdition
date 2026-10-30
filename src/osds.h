#ifndef OSDS_H
#define OSDS_H

void OSD_Think(GOBJ *event);

// OSD IDs
// Mirrored in Globals.s!!
// Be sure to append the ID to the LabOSD_IDs array in lab.h so you can update your OSD in the lab.
#define OSD_Wavedash 0
#define OSD_LCancel 1
#define OSD_ActOoS 3
#define OSD_Dashback 5
#define OSD_FighterSpecificTech 8
#define OSD_Powershield 9
#define OSD_SDI 10
#define OSD_LockoutTimers 12
#define OSD_RollAirdodgeInterrupt 13
#define OSD_BoostGrab 14
#define OSD_Miscellaneous 15
#define OSD_ActOoWait 16
#define OSD_ActOoAerial 17
#define OSD_ActOoAirborne 18
#define OSD_ActOoJumpSquat 19
#define OSD_Fastfall 20
#define OSD_FrameAdvantage 21
#define OSD_ComboCounter 22
#define OSD_GrabBreakout 24
#define OSD_Ledge 26
#define OSD_ActOoHitstun 28

// Special ID: always displays. Never overwrites another OSD.
#define OSD_Null (-1)

// Some OSD options contain multiple sub OSDs.
// We don't want those OSDs to overwrite each other,
// so we have some alt IDs for them to use.
#define OSD_FighterSpecificTechAlt 64

#endif

