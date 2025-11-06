#include "reversal.h"

void Exit(GOBJ *menu) 
{
    stc_match->state = 3;
    Match_EndVS();
}

void Event_Init(GOBJ *gobj)
{

}

void Event_Think(GOBJ *event)
{

}

EventMenu *Event_Menu = &Menu_Main;