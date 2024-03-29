// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief handle mouse/keyboard/joystick inputs,
///	maps inputs to game controls (forward, spin, jump...)

#include "doomdef.h"
#include "doomstat.h"
#include "g_input.h"
#include "keys.h"
#include "hu_stuff.h" // need HUFONT start & end
#include "keys.h"
#include "d_net.h"
#include "console.h"

#define MAXMOUSESENSITIVITY 100 // sensitivity steps

static CV_PossibleValue_t mousesens_cons_t[] = {{1, "MIN"}, {MAXMOUSESENSITIVITY, "MAX"}, {0, NULL}};
static CV_PossibleValue_t onecontrolperkey_cons_t[] = {{1, "One"}, {2, "Several"}, {0, NULL}};

// mouse values are used once
consvar_t cv_mousesens = {"mousesens", "10", CV_SAVE, mousesens_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mlooksens = {"mlooksens", "10", CV_SAVE, mousesens_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mousesens2 = {"mousesens2", "10", CV_SAVE, mousesens_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mlooksens2 = {"mlooksens2", "10", CV_SAVE, mousesens_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_controlperkey = {"controlperkey", "One", CV_SAVE, onecontrolperkey_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_allowautoaim = {"allowautoaim", "Yes", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

int mousex, mousey;
int mlooky; // like mousey but with a custom sensitivity for mlook

int mouse2x, mouse2y, mlook2y;

// joystick values are repeated
int joyxmove[JOYAXISSET], joyymove[JOYAXISSET], joy2xmove[JOYAXISSET], joy2ymove[JOYAXISSET];

// current state of the keys: true if pushed
byte gamekeydown[NUMINPUTS];

// two key codes (or virtual key) per game control
int gamecontrol[num_gamecontrols][2];
int gamecontrolbis[num_gamecontrols][2]; // secondary splitscreen player

typedef struct
{
	byte time;
	byte state;
	byte clicks;
} dclick_t;
static dclick_t mousedclicks[MOUSEBUTTONS];
static dclick_t joydclicks[JOYBUTTONS + JOYHATS*4];
static dclick_t mouse2dclicks[MOUSEBUTTONS];
static dclick_t joy2dclicks[JOYBUTTONS + JOYHATS*4];

// protos
static byte G_CheckDoubleClick(byte state, dclick_t *dt);

//
// Remaps the inputs to game controls.
//
// A game control can be triggered by one or more keys/buttons.
//
// Each key/mousebutton/joybutton triggers ONLY ONE game control.
//
void G_MapEventsToControls(event_t *ev)
{
	int i;
	byte flag;

	switch (ev->type)
	{
		case ev_keydown:
			if (ev->data1 < NUMINPUTS)
				gamekeydown[ev->data1] = 1;
#ifdef PARANOIA
			else
				CONS_Printf("Bad downkey input %d\n",ev->data1);
#endif
			break;

		case ev_keyup:
			if (ev->data1 < NUMINPUTS)
				gamekeydown[ev->data1] = 0;
#ifdef PARANOIA
			else
				CONS_Printf("Bad upkey input %d\n",ev->data1);
#endif
			break;

		case ev_mouse: // buttons are virtual keys
			mousex = (int)(ev->data2*((cv_mousesens.value*cv_mousesens.value)/110.0f + 0.1f));
			mousey = (int)(ev->data3*((cv_mousesens.value*cv_mousesens.value)/110.0f + 0.1f));

			// for now I use the mlook sensitivity just for mlook,
			// instead of having a general mouse y sensitivity.
			mlooky = (int)(ev->data3*((cv_mlooksens.value*cv_mlooksens.value)/110.0f + 0.1f));
			break;

		case ev_joystick: // buttons are virtual keys
			i = ev->data1;
			if (i >= JOYAXISSET)
				break;
			if (ev->data2 != MAXINT) joyxmove[i] = ev->data2;
			if (ev->data3 != MAXINT) joyymove[i] = ev->data3;
			break;

		case ev_joystick2: // buttons are virtual keys
			i = ev->data1;
			if (i >= JOYAXISSET)
				break;
			if (ev->data2 != MAXINT) joy2xmove[i] = ev->data2;
			if (ev->data3 != MAXINT) joy2ymove[i] = ev->data3;
			break;

		case ev_mouse2: // buttons are virtual keys
			mouse2x = (int)(ev->data2*((cv_mousesens2.value*cv_mousesens2.value)/110.0f + 0.1f));
			mouse2y = (int)(ev->data3*((cv_mousesens2.value*cv_mousesens2.value)/110.0f + 0.1f));

			// for now I use the mlook sensitivity just for mlook,
			// instead of having a general mouse y sensitivity.
			mlook2y = (int)(ev->data3*((cv_mlooksens.value*cv_mlooksens.value)/110.0f + 0.1f));
			break;

		default:
			break;
	}

	// ALWAYS check for mouse & joystick double-clicks even if no mouse event
	for (i = 0; i < MOUSEBUTTONS; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_MOUSE1+i], &mousedclicks[i]);
		gamekeydown[KEY_DBLMOUSE1+i] = flag;
	}

	for (i = 0; i < JOYBUTTONS + JOYHATS*4; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_JOY1+i], &joydclicks[i]);
		gamekeydown[KEY_DBLJOY1+i] = flag;
	}

	for (i = 0; i < MOUSEBUTTONS; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_2MOUSE1+i], &mouse2dclicks[i]);
		gamekeydown[KEY_DBL2MOUSE1+i] = flag;
	}

	for (i = 0; i < JOYBUTTONS + JOYHATS*4; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_2JOY1+i], &joy2dclicks[i]);
		gamekeydown[KEY_DBL2JOY1+i] = flag;
	}
}

//
// General double-click detection routine for any kind of input.
//
static byte G_CheckDoubleClick(byte state, dclick_t *dt)
{
	if (state != dt->state && dt->time > 1)
	{
		dt->state = state;
		if (state)
			dt->clicks++;
		if (dt->clicks == 2)
		{
			dt->clicks = 0;
			return true;
		}
		else
			dt->time = 0;
	}
	else
	{
		dt->time++;
		if (dt->time > 20)
		{
			dt->clicks = 0;
			dt->state = 0;
		}
	}
	return false;
}

typedef struct
{
	int keynum;
	const char *name;
} keyname_t;

static keyname_t keynames[] =
{
	{KEY_SPACE, "SPACE"},
	{KEY_CAPSLOCK, "CAPS LOCK"},
	{KEY_ENTER, "ENTER"},
	{KEY_TAB, "TAB"},
	{KEY_ESCAPE, "ESCAPE"},
	{KEY_BACKSPACE, "BACKSPACE"},

	{KEY_NUMLOCK, "NUMLOCK"},
	{KEY_SCROLLLOCK, "SCROLLLOCK"},

	// bill gates keys
#ifndef _arch_dreamcast
	{KEY_LEFTWIN, "LEFTWIN"},
	{KEY_RIGHTWIN, "RIGHTWIN"},
	{KEY_MENU, "MENU"},
#endif

	{KEY_LSHIFT, "LSHIFT"},
	{KEY_RSHIFT, "RSHIFT"},
	{KEY_LSHIFT, "SHIFT"},
	{KEY_LCTRL, "LCTRL"},
	{KEY_RCTRL, "RCTRL"},
	{KEY_LCTRL, "CTRL"},
	{KEY_LALT, "LALT"},
	{KEY_RALT, "RALT"},
	{KEY_LALT, "ALT"},

	// keypad keys
	{KEY_KPADSLASH, "KEYPAD /"},
	{KEY_KEYPAD7, "KEYPAD 7"},
	{KEY_KEYPAD8, "KEYPAD 8"},
	{KEY_KEYPAD9, "KEYPAD 9"},
	{KEY_MINUSPAD, "KEYPAD -"},
	{KEY_KEYPAD4, "KEYPAD 4"},
	{KEY_KEYPAD5, "KEYPAD 5"},
	{KEY_KEYPAD6, "KEYPAD 6"},
	{KEY_PLUSPAD, "KEYPAD +"},
	{KEY_KEYPAD1, "KEYPAD 1"},
	{KEY_KEYPAD2, "KEYPAD 2"},
	{KEY_KEYPAD3, "KEYPAD 3"},
	{KEY_KEYPAD0, "KEYPAD 0"},
	{KEY_KPADDEL, "KEYPAD ."},

	// extended keys (not keypad)
	{KEY_HOME, "HOME"},
	{KEY_UPARROW, "UP ARROW"},
	{KEY_PGUP, "PGUP"},
	{KEY_LEFTARROW, "LEFT ARROW"},
	{KEY_RIGHTARROW, "RIGHT ARROW"},
	{KEY_END, "END"},
	{KEY_DOWNARROW, "DOWN ARROW"},
	{KEY_PGDN, "PGDN"},
	{KEY_INS, "INS"},
	{KEY_DEL, "DEL"},

	// other keys
	{KEY_F1, "F1"},
	{KEY_F2, "F2"},
	{KEY_F3, "F3"},
	{KEY_F4, "F4"},
	{KEY_F5, "F5"},
	{KEY_F6, "F6"},
	{KEY_F7, "F7"},
	{KEY_F8, "F8"},
	{KEY_F9, "F9"},
	{KEY_F10, "F10"},
	{KEY_F11, "F11"},
	{KEY_F12, "F12"},

	// virtual keys for mouse buttons and joystick buttons
	{KEY_MOUSE1+0,"MOUSE1"},
	{KEY_MOUSE1+1,"MOUSE2"},
	{KEY_MOUSE1+2,"MOUSE3"},
#if !defined (_arch_dreamcast) && !defined (_XBOX) && !defined (_PSP_)
	{KEY_MOUSE1+3,"MOUSE4"},
	{KEY_MOUSE1+4,"MOUSE5"},
	{KEY_MOUSE1+5,"MOUSE6"},
	{KEY_MOUSE1+6,"MOUSE7"},
	{KEY_MOUSE1+7,"MOUSE8"},
#endif
	{KEY_2MOUSE1+0,"SEC_MOUSE2"}, // BP: sorry my mouse handler swap button 1 and 2
	{KEY_2MOUSE1+1,"SEC_MOUSE1"},
	{KEY_2MOUSE1+2,"SEC_MOUSE3"},
#if !defined (_arch_dreamcast) && !defined (_XBOX) && !defined (_PSP)
	{KEY_2MOUSE1+3,"SEC_MOUSE4"},
	{KEY_2MOUSE1+4,"SEC_MOUSE5"},
	{KEY_2MOUSE1+5,"SEC_MOUSE6"},
	{KEY_2MOUSE1+6,"SEC_MOUSE7"},
	{KEY_2MOUSE1+7,"SEC_MOUSE8"},
#endif
	{KEY_MOUSEWHEELUP, "Wheel 1 UP"},
	{KEY_MOUSEWHEELDOWN, "Wheel 1 Down"},
	{KEY_2MOUSEWHEELUP, "Wheel 2 UP"},
	{KEY_2MOUSEWHEELDOWN, "Wheel 2 Down"},

#ifdef DC
	{KEY_JOY1+0, "JOYC"},
	{KEY_JOY1+1, "JOYB"},
	{KEY_JOY1+2, "JOYA"},
	{KEY_JOY1+3, "JOYS"},
	{KEY_JOY1+4, "JOYZ"},
	{KEY_JOY1+5, "JOYY"},
	{KEY_JOY1+6, "JOYX"},
	{KEY_JOY1+7, "JOYD"},
#elif defined (_XBOX)
	{KEY_JOY1+0, "JOYA"},
	{KEY_JOY1+1, "JOYB"},
	{KEY_JOY1+2, "JOYX"},
	{KEY_JOY1+3, "JOYY"},
	{KEY_JOY1+4, "JOYG"},
	{KEY_JOY1+5, "JOYW"},
	{KEY_JOY1+6, "JOYL"},
	{KEY_JOY1+7, "JOYR"},
	{KEY_JOY1+8, "JOYS"},
	{KEY_JOY1+9, "JOYN"},
	{KEY_JOY1+10,"JOYW"},
	{KEY_JOY1+11,"JOYE"},
#elif defined (_PSP_)
	{KEY_JOY1+0, "TRIANGLE"},
	{KEY_JOY1+1, "CIRCLE"  },
	{KEY_JOY1+2, "CROSS"   },
	{KEY_JOY1+3, "SQUARE"  },
	{KEY_JOY1+4, "LTRIGGER"},
	{KEY_JOY1+5, "RTRIGGER"},
	{KEY_JOY1+6, "SELECT"  },
	{KEY_JOY1+7, "START"   },
	{KEY_JOY1+8, "HOME"    },
	{KEY_JOY1+9, "HOLD"    },
#elif defined (GP2X)
	{KEY_JOY1+0, "JOYA"},
	{KEY_JOY1+1, "JOYY"},
	{KEY_JOY1+2, "JOYB"},
	{KEY_JOY1+3, "JOYX"},
	{KEY_JOY1+4, "JOYL"},
	{KEY_JOY1+5, "JOYR"},
	{KEY_JOY1+6, "JOYVOLUP"},
	{KEY_JOY1+7, "JOYVOLDOWN"},
	{KEY_JOY1+8, "JOYSELECT"},
#else
	{KEY_JOY1+0, "JOY1"},
	{KEY_JOY1+1, "JOY2"},
	{KEY_JOY1+2, "JOY3"},
	{KEY_JOY1+3, "JOY4"},
	{KEY_JOY1+4, "JOY5"},
	{KEY_JOY1+5, "JOY6"},
	{KEY_JOY1+6, "JOY7"},
	{KEY_JOY1+7, "JOY8"},
	{KEY_JOY1+8, "JOY9"},
#endif
#if !defined (_arch_dreamcast) && !defined (_XBOX) && !defined (_PSP_)
	// we use up to 32 buttons in DirectInput
	{KEY_JOY1+9, "JOY10"},
	{KEY_JOY1+10, "JOY11"},
	{KEY_JOY1+11, "JOY12"},
	{KEY_JOY1+12, "JOY13"},
	{KEY_JOY1+13, "JOY14"},
	{KEY_JOY1+14, "JOY15"},
	{KEY_JOY1+15, "JOY16"},
	{KEY_JOY1+16, "JOY17"},
	{KEY_JOY1+17, "JOY18"},
	{KEY_JOY1+18, "JOY19"},
	{KEY_JOY1+19, "JOY20"},
	{KEY_JOY1+20, "JOY21"},
	{KEY_JOY1+21, "JOY22"},
	{KEY_JOY1+22, "JOY23"},
	{KEY_JOY1+23, "JOY24"},
	{KEY_JOY1+24, "JOY25"},
	{KEY_JOY1+25, "JOY26"},
	{KEY_JOY1+26, "JOY27"},
	{KEY_JOY1+27, "JOY28"},
	{KEY_JOY1+28, "JOY29"},
	{KEY_JOY1+29, "JOY30"},
	{KEY_JOY1+30, "JOY31"},
	{KEY_JOY1+31, "JOY32"},
#endif
	// the DOS version uses Allegro's joystick support
	{KEY_HAT1+0, "HATUP"},
	{KEY_HAT1+1, "HATDOWN"},
	{KEY_HAT1+2, "HATLEFT"},
	{KEY_HAT1+3, "HATRIGHT"},
#if !defined (_XBOX) && !defined (_PSP_)
	{KEY_HAT1+4, "HATUP2"},
	{KEY_HAT1+5, "HATDOWN2"},
	{KEY_HAT1+6, "HATLEFT2"},
	{KEY_HAT1+7, "HATRIGHT2"},
#ifndef _arch_dreamcast
	{KEY_HAT1+8, "HATUP3"},
	{KEY_HAT1+9, "HATDOWN3"},
	{KEY_HAT1+10, "HATLEFT3"},
	{KEY_HAT1+11, "HATRIGHT3"},
	{KEY_HAT1+12, "HATUP4"},
	{KEY_HAT1+13, "HATDOWN4"},
	{KEY_HAT1+14, "HATLEFT4"},
	{KEY_HAT1+15, "HATRIGHT4"},
#endif
#endif

	{KEY_DBLMOUSE1+0, "DBLMOUSE1"},
	{KEY_DBLMOUSE1+1, "DBLMOUSE2"},
	{KEY_DBLMOUSE1+2, "DBLMOUSE3"},
#if !defined (_arch_dreamcast) && !defined (_XBOX) && !defined (_PSP_)
	{KEY_DBLMOUSE1+3, "DBLMOUSE4"},
	{KEY_DBLMOUSE1+4, "DBLMOUSE5"},
	{KEY_DBLMOUSE1+5, "DBLMOUSE6"},
	{KEY_DBLMOUSE1+6, "DBLMOUSE7"},
	{KEY_DBLMOUSE1+7, "DBLMOUSE8"},
#endif
	{KEY_DBL2MOUSE1+0, "DBLSEC_MOUSE2"}, // BP: sorry my mouse handler swap button 1 and 2
	{KEY_DBL2MOUSE1+1, "DBLSEC_MOUSE1"},
	{KEY_DBL2MOUSE1+2, "DBLSEC_MOUSE3"},
#if !defined (_arch_dreamcast) && !defined (_XBOX) && !defined (_PSP_)
	{KEY_DBL2MOUSE1+3, "DBLSEC_MOUSE4"},
	{KEY_DBL2MOUSE1+4, "DBLSEC_MOUSE5"},
	{KEY_DBL2MOUSE1+5, "DBLSEC_MOUSE6"},
	{KEY_DBL2MOUSE1+6, "DBLSEC_MOUSE7"},
	{KEY_DBL2MOUSE1+7, "DBLSEC_MOUSE8"},
#endif

#ifdef DC
	{KEY_DBLJOY1+0, "DBLJOYC"},
	{KEY_DBLJOY1+1, "DBLJOYB"},
	{KEY_DBLJOY1+2, "DBLJOYA"},
	{KEY_DBLJOY1+3, "DBLJOYS"},
	{KEY_DBLJOY1+4, "DBLJOYZ"},
	{KEY_DBLJOY1+5, "DBLJOYY"},
	{KEY_DBLJOY1+6, "DBLJOYX"},
	{KEY_DBLJOY1+7, "DBLJOYD"},
#elif defined (_XBOX)
	{KEY_DBLJOY1+0, "DBLJOYA"},
	{KEY_DBLJOY1+1, "DBLJOYB"},
	{KEY_DBLJOY1+2, "DBLJOYX"},
	{KEY_DBLJOY1+3, "DBLJOYY"},
	{KEY_DBLJOY1+4, "DBLJOYG"},
	{KEY_DBLJOY1+5, "DBLJOYW"},
	{KEY_DBLJOY1+6, "DBLJOYL"},
	{KEY_DBLJOY1+7, "DBLJOYR"},
	{KEY_DBLJOY1+8, "DBLJOYS"},
	{KEY_DBLJOY1+9, "DBLJOYN"},
	{KEY_DBLJOY1+10,"DBLJOYW"},
	{KEY_DBLJOY1+11,"DBLJOYE"},
#elif defined (_PSP_)
	{KEY_DBLJOY1+0, "DBLTRIANGLE"},
	{KEY_DBLJOY1+1, "DBLCIRCLE"  },
	{KEY_DBLJOY1+2, "DBLCROSS"   },
	{KEY_DBLJOY1+3, "DBLSQUARE"  },
	{KEY_DBLJOY1+4, "DBLLTRIGGER"},
	{KEY_DBLJOY1+5, "DBLRTRIGGER"},
	{KEY_DBLJOY1+6, "DBLSELECT"  },
	{KEY_DBLJOY1+7, "DBLSTART"   },
	{KEY_DBLJOY1+8, "DBLHOME"    },
	{KEY_DBLJOY1+9, "DBLHOLD"    },
#else
	{KEY_DBLJOY1+0, "DBLJOY1"},
	{KEY_DBLJOY1+1, "DBLJOY2"},
	{KEY_DBLJOY1+2, "DBLJOY3"},
	{KEY_DBLJOY1+3, "DBLJOY4"},
	{KEY_DBLJOY1+4, "DBLJOY5"},
	{KEY_DBLJOY1+5, "DBLJOY6"},
	{KEY_DBLJOY1+6, "DBLJOY7"},
	{KEY_DBLJOY1+7, "DBLJOY8"},
#endif
#if !defined (_arch_dreamcast) && !defined (_XBOX) && !defined (_PSP_)
	{KEY_DBLJOY1+8, "DBLJOY9"},
	{KEY_DBLJOY1+9, "DBLJOY10"},
	{KEY_DBLJOY1+10, "DBLJOY11"},
	{KEY_DBLJOY1+11, "DBLJOY12"},
	{KEY_DBLJOY1+12, "DBLJOY13"},
	{KEY_DBLJOY1+13, "DBLJOY14"},
	{KEY_DBLJOY1+14, "DBLJOY15"},
	{KEY_DBLJOY1+15, "DBLJOY16"},
	{KEY_DBLJOY1+16, "DBLJOY17"},
	{KEY_DBLJOY1+17, "DBLJOY18"},
	{KEY_DBLJOY1+18, "DBLJOY19"},
	{KEY_DBLJOY1+19, "DBLJOY20"},
	{KEY_DBLJOY1+20, "DBLJOY21"},
	{KEY_DBLJOY1+21, "DBLJOY22"},
	{KEY_DBLJOY1+22, "DBLJOY23"},
	{KEY_DBLJOY1+23, "DBLJOY24"},
	{KEY_DBLJOY1+24, "DBLJOY25"},
	{KEY_DBLJOY1+25, "DBLJOY26"},
	{KEY_DBLJOY1+26, "DBLJOY27"},
	{KEY_DBLJOY1+27, "DBLJOY28"},
	{KEY_DBLJOY1+28, "DBLJOY29"},
	{KEY_DBLJOY1+29, "DBLJOY30"},
	{KEY_DBLJOY1+30, "DBLJOY31"},
	{KEY_DBLJOY1+31, "DBLJOY32"},
#endif
	{KEY_DBLHAT1+0, "DBLHATUP"},
	{KEY_DBLHAT1+1, "DBLHATDOWN"},
	{KEY_DBLHAT1+2, "DBLHATLEFT"},
	{KEY_DBLHAT1+3, "DBLHATRIGHT"},
#if !defined (_XBOX) && !defined (_PSP_)
	{KEY_DBLHAT1+4, "DBLHATUP2"},
	{KEY_DBLHAT1+5, "DBLHATDOWN2"},
	{KEY_DBLHAT1+6, "DBLHATLEFT2"},
	{KEY_DBLHAT1+7, "DBLHATRIGHT2"},
#ifndef _arch_dreamcast
	{KEY_DBLHAT1+8, "DBLHATUP3"},
	{KEY_DBLHAT1+9, "DBLHATDOWN3"},
	{KEY_DBLHAT1+10, "DBLHATLEFT3"},
	{KEY_DBLHAT1+11, "DBLHATRIGHT3"},
	{KEY_DBLHAT1+12, "DBLHATUP4"},
	{KEY_DBLHAT1+13, "DBLHATDOWN4"},
	{KEY_DBLHAT1+14, "DBLHATLEFT4"},
	{KEY_DBLHAT1+15, "DBLHATRIGHT4"},
#endif
#endif

#ifdef DC
	{KEY_2JOY1+0, "SEC_JOYC"},
	{KEY_2JOY1+1, "SEC_JOYB"},
	{KEY_2JOY1+2, "SEC_JOYA"},
	{KEY_2JOY1+3, "SEC_JOYS"},
	{KEY_2JOY1+4, "SEC_JOYZ"},
	{KEY_2JOY1+5, "SEC_JOYY"},
	{KEY_2JOY1+6, "SEC_JOYX"},
	{KEY_2JOY1+7, "SEC_JOYD"},
#elif defined (_XBOX)
	{KEY_2JOY1+0, "SEC_JOYA"},
	{KEY_2JOY1+1, "SEC_JOYB"},
	{KEY_2JOY1+2, "SEC_JOYX"},
	{KEY_2JOY1+3, "SEC_JOYY"},
	{KEY_2JOY1+4, "SEC_JOYG"},
	{KEY_2JOY1+5, "SEC_JOYW"},
	{KEY_2JOY1+6, "SEC_JOYL"},
	{KEY_2JOY1+7, "SEC_JOYR"},
	{KEY_2JOY1+8, "SEC_JOYS"},
	{KEY_2JOY1+9, "SEC_JOYN"},
	{KEY_2JOY1+10,"SEC_JOYW"},
	{KEY_2JOY1+11,"SEC_JOYE"},
#elif defined (_PSP_)
	{KEY_2JOY1+0, "TRIANGLE"},
	{KEY_2JOY1+1, "CIRCLE"  },
	{KEY_2JOY1+2, "CROSS"   },
	{KEY_2JOY1+3, "SQUARE"  },
	{KEY_2JOY1+4, "LTRIGGER"},
	{KEY_2JOY1+5, "RTRIGGER"},
	{KEY_2JOY1+6, "SELECT"  },
	{KEY_2JOY1+7, "START"   },
	{KEY_2JOY1+8, "HOME"    },
	{KEY_2JOY1+9, "HOLD"    },
#else
	{KEY_2JOY1+0, "SEC_JOY1"},
	{KEY_2JOY1+1, "SEC_JOY2"},
	{KEY_2JOY1+2, "SEC_JOY3"},
	{KEY_2JOY1+3, "SEC_JOY4"},
	{KEY_2JOY1+4, "SEC_JOY5"},
	{KEY_2JOY1+5, "SEC_JOY6"},
	{KEY_2JOY1+6, "SEC_JOY7"},
	{KEY_2JOY1+7, "SEC_JOY8"},
#endif
#if !defined (_arch_dreamcast) && !defined (_XBOX) && !defined (_XBOX_)
	// we use up to 32 buttons in DirectInput
	{KEY_2JOY1+8, "SEC_JOY9"},
	{KEY_2JOY1+9, "SEC_JOY10"},
	{KEY_2JOY1+10, "SEC_JOY11"},
	{KEY_2JOY1+11, "SEC_JOY12"},
	{KEY_2JOY1+12, "SEC_JOY13"},
	{KEY_2JOY1+13, "SEC_JOY14"},
	{KEY_2JOY1+14, "SEC_JOY15"},
	{KEY_2JOY1+15, "SEC_JOY16"},
	{KEY_2JOY1+16, "SEC_JOY17"},
	{KEY_2JOY1+17, "SEC_JOY18"},
	{KEY_2JOY1+18, "SEC_JOY19"},
	{KEY_2JOY1+19, "SEC_JOY20"},
	{KEY_2JOY1+20, "SEC_JOY21"},
	{KEY_2JOY1+21, "SEC_JOY22"},
	{KEY_2JOY1+22, "SEC_JOY23"},
	{KEY_2JOY1+23, "SEC_JOY24"},
	{KEY_2JOY1+24, "SEC_JOY25"},
	{KEY_2JOY1+25, "SEC_JOY26"},
	{KEY_2JOY1+26, "SEC_JOY27"},
	{KEY_2JOY1+27, "SEC_JOY28"},
	{KEY_2JOY1+28, "SEC_JOY29"},
	{KEY_2JOY1+29, "SEC_JOY30"},
	{KEY_2JOY1+30, "SEC_JOY31"},
	{KEY_2JOY1+31, "SEC_JOY32"},
#endif
	// the DOS version uses Allegro's joystick support
	{KEY_2HAT1+0,  "SEC_HATUP"},
	{KEY_2HAT1+1,  "SEC_HATDOWN"},
	{KEY_2HAT1+2,  "SEC_HATLEFT"},
	{KEY_2HAT1+3,  "SEC_HATRIGHT"},
#if !defined (_XBOX) && !defined (_PSP_)
	{KEY_2HAT1+4, "SEC_HATUP2"},
	{KEY_2HAT1+5, "SEC_HATDOWN2"},
	{KEY_2HAT1+6, "SEC_HATLEFT2"},
	{KEY_2HAT1+7, "SEC_HATRIGHT2"},
#ifndef _arch_dreamcast
	{KEY_2HAT1+8, "SEC_HATUP3"},
	{KEY_2HAT1+9, "SEC_HATDOWN3"},
	{KEY_2HAT1+10, "SEC_HATLEFT3"},
	{KEY_2HAT1+11, "SEC_HATRIGHT3"},
	{KEY_2HAT1+12, "SEC_HATUP4"},
	{KEY_2HAT1+13, "SEC_HATDOWN4"},
	{KEY_2HAT1+14, "SEC_HATLEFT4"},
	{KEY_2HAT1+15, "SEC_HATRIGHT4"},
#endif
#endif

#ifdef DC
	{KEY_DBL2JOY1+0, "DBLSEC_JOYC"},
	{KEY_DBL2JOY1+1, "DBLSEC_JOYB"},
	{KEY_DBL2JOY1+2, "DBLSEC_JOYA"},
	{KEY_DBL2JOY1+3, "DBLSEC_JOYS"},
	{KEY_DBL2JOY1+4, "DBLSEC_JOYZ"},
	{KEY_DBL2JOY1+5, "DBLSEC_JOYY"},
	{KEY_DBL2JOY1+6, "DBLSEC_JOYX"},
	{KEY_DBL2JOY1+7, "DBLSEC_JOYD"},
#elif defined (_XBOX)
	{KEY_DBL2JOY1+0, "DBLSEC_JOYA"},
	{KEY_DBL2JOY1+1, "DBLSEC_JOYB"},
	{KEY_DBL2JOY1+2, "DBLSEC_JOYX"},
	{KEY_DBL2JOY1+3, "DBLSEC_JOYY"},
	{KEY_DBL2JOY1+4, "DBLSEC_JOYG"},
	{KEY_DBL2JOY1+5, "DBLSEC_JOYW"},
	{KEY_DBL2JOY1+6, "DBLSEC_JOYL"},
	{KEY_DBL2JOY1+7, "DBLSEC_JOYR"},
	{KEY_DBL2JOY1+8, "DBLSEC_JOYS"},
	{KEY_DBL2JOY1+9, "DBLSEC_JOYN"},
	{KEY_DBL2JOY1+10,"DBLSEC_JOYW"},
	{KEY_DBL2JOY1+11,"DBLSEC_JOYE"},
#elif defined (_PSP_)
	{KEY_DBL2JOY1+0, "DBLTRIANGLE"},
	{KEY_DBL2JOY1+1, "DBLCIRCLE"  },
	{KEY_DBL2JOY1+2, "DBLCROSS"   },
	{KEY_DBL2JOY1+3, "DBLSQUARE"  },
	{KEY_DBL2JOY1+4, "DBLLTRIGGER"},
	{KEY_DBL2JOY1+5, "DBLRTRIGGER"},
	{KEY_DBL2JOY1+6, "DBLSELECT"  },
	{KEY_DBL2JOY1+7, "DBLSTART"   },
	{KEY_DBL2JOY1+8, "DBLHOME"    },
	{KEY_DBL2JOY1+9, "DBLHOLD"    },
#else
	{KEY_DBL2JOY1+0, "DBLSEC_JOY1"},
	{KEY_DBL2JOY1+1, "DBLSEC_JOY2"},
	{KEY_DBL2JOY1+2, "DBLSEC_JOY3"},
	{KEY_DBL2JOY1+3, "DBLSEC_JOY4"},
	{KEY_DBL2JOY1+4, "DBLSEC_JOY5"},
	{KEY_DBL2JOY1+5, "DBLSEC_JOY6"},
	{KEY_DBL2JOY1+6, "DBLSEC_JOY7"},
	{KEY_DBL2JOY1+7, "DBLSEC_JOY8"},
#endif
#if !defined (_arch_dreamcast) && !defined (_XBOX) && !defined (_PSP_)
	{KEY_DBL2JOY1+8, "DBLSEC_JOY9"},
	{KEY_DBL2JOY1+9, "DBLSEC_JOY10"},
	{KEY_DBL2JOY1+10, "DBLSEC_JOY11"},
	{KEY_DBL2JOY1+11, "DBLSEC_JOY12"},
	{KEY_DBL2JOY1+12, "DBLSEC_JOY13"},
	{KEY_DBL2JOY1+13, "DBLSEC_JOY14"},
	{KEY_DBL2JOY1+14, "DBLSEC_JOY15"},
	{KEY_DBL2JOY1+15, "DBLSEC_JOY16"},
	{KEY_DBL2JOY1+16, "DBLSEC_JOY17"},
	{KEY_DBL2JOY1+17, "DBLSEC_JOY18"},
	{KEY_DBL2JOY1+18, "DBLSEC_JOY19"},
	{KEY_DBL2JOY1+19, "DBLSEC_JOY20"},
	{KEY_DBL2JOY1+20, "DBLSEC_JOY21"},
	{KEY_DBL2JOY1+21, "DBLSEC_JOY22"},
	{KEY_DBL2JOY1+22, "DBLSEC_JOY23"},
	{KEY_DBL2JOY1+23, "DBLSEC_JOY24"},
	{KEY_DBL2JOY1+24, "DBLSEC_JOY25"},
	{KEY_DBL2JOY1+25, "DBLSEC_JOY26"},
	{KEY_DBL2JOY1+26, "DBLSEC_JOY27"},
	{KEY_DBL2JOY1+27, "DBLSEC_JOY28"},
	{KEY_DBL2JOY1+28, "DBLSEC_JOY29"},
	{KEY_DBL2JOY1+29, "DBLSEC_JOY30"},
	{KEY_DBL2JOY1+30, "DBLSEC_JOY31"},
	{KEY_DBL2JOY1+31, "DBLSEC_JOY32"},
#endif
	{KEY_DBL2HAT1+0, "DBLSEC_HATUP"},
	{KEY_DBL2HAT1+1, "DBLSEC_HATDOWN"},
	{KEY_DBL2HAT1+2, "DBLSEC_HATLEFT"},
	{KEY_DBL2HAT1+3, "DBLSEC_HATRIGHT"},
#if !defined (_XBOX) && !defined (_PSP_)
	{KEY_DBL2HAT1+4, "DBLSEC_HATUP2"},
	{KEY_DBL2HAT1+5, "DBLSEC_HATDOWN2"},
	{KEY_DBL2HAT1+6, "DBLSEC_HATLEFT2"},
	{KEY_DBL2HAT1+7, "DBLSEC_HATRIGHT2"},
#ifndef _arch_dreamcast
	{KEY_DBL2HAT1+8, "DBLSEC_HATUP3"},
	{KEY_DBL2HAT1+9, "DBLSEC_HATDOWN3"},
	{KEY_DBL2HAT1+10, "DBLSEC_HATLEFT3"},
	{KEY_DBL2HAT1+11, "DBLSEC_HATRIGHT3"},
	{KEY_DBL2HAT1+12, "DBLSEC_HATUP4"},
	{KEY_DBL2HAT1+13, "DBLSEC_HATDOWN4"},
	{KEY_DBL2HAT1+14, "DBLSEC_HATLEFT4"},
	{KEY_DBL2HAT1+15, "DBLSEC_HATRIGHT4"},
#endif
#endif

};

static const char *gamecontrolname[num_gamecontrols] =
{
	"nothing", // a key/button mapped to gc_null has no effect
	"forward",
	"backward",
	"strafe",
	"straferight",
	"strafeleft",
	"turnleft",
	"turnright",
	"weaponnext",
	"weaponprev",
	"fire",
	"firenormal",
	"lightdash",
	"use",
	"taunt",
	"camleft",
	"camright",
	"camreset",
	"lookup",
	"lookdown",
	"centerview",
	"mouseaiming",
	"talkkey",
	"teamtalkkey",
	"scores",
	"jump",
	"console",
	"pause",
};

#define NUMKEYNAMES (sizeof (keynames)/sizeof (keyname_t))

//
// Detach any keys associated to the given game control
// - pass the pointer to the gamecontrol table for the player being edited
void G_ClearControlKeys(int (*setupcontrols)[2], int control)
{
	setupcontrols[control][0] = KEY_NULL;
	setupcontrols[control][1] = KEY_NULL;
}

//
// Returns the name of a key (or virtual key for mouse and joy)
// the input value being an keynum
//
const char *G_KeynumToString(int keynum)
{
	static char keynamestr[8];

	unsigned int j;

	// return a string with the ascii char if displayable
	if (keynum > ' ' && keynum <= 'z' && keynum != KEY_CONSOLE)
	{
		keynamestr[0] = (char)keynum;
		keynamestr[1] = '\0';
		return keynamestr;
	}

	// find a description for special keys
	for (j = 0; j < NUMKEYNAMES; j++)
		if (keynames[j].keynum == keynum)
			return keynames[j].name;

	// create a name for unknown keys
	sprintf(keynamestr, "KEY%d", keynum);
	return keynamestr;
}

int G_KeyStringtoNum(const char *keystr)
{
	unsigned int j;

	if (!keystr[1] && keystr[0] > ' ' && keystr[0] <= 'z')
		return keystr[0];

	for (j = 0; j < NUMKEYNAMES; j++)
		if (!stricmp(keynames[j].name, keystr))
			return keynames[j].keynum;

	if (strlen(keystr) > 3)
		return atoi(&keystr[3]);

	return 0;
}

#ifdef DC
void G_Controldefault(void)
{
	gamecontrol[gc_forward    ][0] = KEY_HAT1+0; //Up
	gamecontrol[gc_forward    ][1] = KEY_UPARROW;
	gamecontrol[gc_backward   ][0] = KEY_HAT1+1; //Down
	gamecontrol[gc_backward   ][1] = KEY_DOWNARROW;
	gamecontrol[gc_strafe     ][0] = KEY_RALT;
	gamecontrol[gc_strafe     ][1] = KEY_MOUSE1+1;
	//gamecontrol[gc_straferight][0] = '[';
	//gamecontrol[gc_strafeleft ][0] = ']';
	gamecontrol[gc_turnleft   ][0] = KEY_HAT1+2; //Left
	gamecontrol[gc_turnleft   ][1] = KEY_LEFTARROW;
	gamecontrol[gc_turnright  ][0] = KEY_HAT1+3; //Right
	gamecontrol[gc_turnright  ][1] = KEY_RIGHTARROW;
	gamecontrol[gc_weaponnext ][0] = ']';
	gamecontrol[gc_weaponprev ][0] = '[';
	gamecontrol[gc_fire       ][0] = KEY_JOY1+6; //X
	gamecontrol[gc_fire       ][1] = KEY_RCTRL;
	gamecontrol[gc_firenormal ][0] = KEY_JOY1+5; //Y
	gamecontrol[gc_firenormal ][1] = ';';
	gamecontrol[gc_lightdash  ][0] = '\'';
	gamecontrol[gc_use        ][0] = KEY_JOY1+1; //B
	gamecontrol[gc_use        ][1] = '.';
	gamecontrol[gc_taunt      ][0] = ',';
	gamecontrol[gc_camleft    ][0] = 'o';
	gamecontrol[gc_camright   ][0] = 'p';
	gamecontrol[gc_camreset   ][0] = 'c';
	gamecontrol[gc_lookup     ][0] = KEY_PGUP;
	gamecontrol[gc_lookdown   ][0] = KEY_PGDN;
	gamecontrol[gc_centerview ][0] = KEY_END;
	gamecontrol[gc_mouseaiming][0] = 's';
	gamecontrol[gc_talkkey    ][0] = 't';
	gamecontrol[gc_teamkey    ][0] = 'y';
	gamecontrol[gc_scores     ][0] = KEY_TAB;
	gamecontrol[gc_jump       ][0] = KEY_JOY1+2; //A
	gamecontrol[gc_jump       ][1] = '/';
	gamecontrol[gc_console    ][0] = KEY_CONSOLE;
	gamecontrol[gc_console    ][1] = KEY_F5;
	//gamecontrolbis
	gamecontrolbis[gc_forward   ][0] = KEY_2HAT1+0;
	gamecontrolbis[gc_forward   ][1] = 'w';
	gamecontrolbis[gc_backward  ][0] = KEY_2HAT1+1;
	gamecontrolbis[gc_backward  ][1] = 's';
	gamecontrolbis[gc_turnleft  ][0] = KEY_2HAT1+2;
	gamecontrolbis[gc_turnleft  ][1] = 'a';
	gamecontrolbis[gc_turnright ][0] = KEY_2HAT1+3;
	gamecontrolbis[gc_turnright ][1] = 'd';
	gamecontrolbis[gc_weaponnext][0] = 't';
	gamecontrolbis[gc_weaponprev][0] = 'r';
	gamecontrolbis[gc_fire      ][0] = KEY_2JOY1+6; //X
	gamecontrolbis[gc_firenormal][0] = KEY_2JOY1+5; //Y
	gamecontrolbis[gc_use       ][0] = KEY_2JOY1+1; //B
	gamecontrolbis[gc_jump      ][0] = KEY_2JOY1+2; //A
	//gamecontrolbis[gc_straferight][0] = 'x';
	//gamecontrolbis[gc_strafeleft ][0] = 'z';
}
#elif defined (_PSP_)
void G_Controldefault(void)
{
	gamecontrol[gc_forward    ][0] = KEY_HAT1+0; // Up
	gamecontrol[gc_backward   ][0] = KEY_HAT1+1; // Down
	gamecontrol[gc_turnleft   ][0] = KEY_HAT1+2; // Left
	gamecontrol[gc_turnright  ][0] = KEY_HAT1+3; // Right
	gamecontrol[gc_strafeleft ][0] = KEY_JOY1+4; // L
	gamecontrol[gc_straferight][0] = KEY_JOY1+5; // R
	gamecontrol[gc_lightdash  ][0] = KEY_JOY1+0; // Triangle
	gamecontrol[gc_use        ][0] = KEY_JOY1+1; // Circle
	gamecontrol[gc_taunt      ][0] = KEY_JOY1+6; // Select
	gamecontrol[gc_camreset   ][0] = KEY_JOY1+3; // Square
	gamecontrol[gc_centerview ][0] = KEY_JOY1+9; // Hold
	gamecontrol[gc_pause      ][0] = KEY_JOY1+8; // Start
	gamecontrol[gc_jump       ][0] = KEY_JOY1+2; // Cross
}
#elif defined (GP2X)
void G_Controldefault(void)
{
	gamecontrol[gc_fire       ][0] = KEY_JOY1+0; //A
	gamecontrol[gc_forward    ][0] = KEY_JOY1+1; //Y
	gamecontrol[gc_jump       ][0] = KEY_JOY1+2; //B
	gamecontrol[gc_use        ][0] = KEY_JOY1+3; //X
	gamecontrol[gc_strafeleft ][0] = KEY_JOY1+4; //L
	gamecontrol[gc_straferight][0] = KEY_JOY1+5; //R
	gamecontrol[gc_lookup     ][0] = KEY_JOY1+6; //U
	gamecontrol[gc_lookdown   ][0] = KEY_JOY1+7; //D
	gamecontrol[gc_pause      ][0] = KEY_JOY1+8; //S
}
#else
void G_Controldefault(void)
{
	gamecontrol[gc_forward    ][0] = KEY_UPARROW;
	gamecontrol[gc_forward    ][1] = KEY_MOUSE1+2;
	gamecontrol[gc_backward   ][0] = KEY_DOWNARROW;
	gamecontrol[gc_strafe     ][0] = KEY_RALT;
	gamecontrol[gc_strafe     ][1] = KEY_MOUSE1+1;
	gamecontrol[gc_straferight][0] = '[';
	gamecontrol[gc_strafeleft ][0] = ']';
	gamecontrol[gc_turnleft   ][0] = KEY_LEFTARROW;
	gamecontrol[gc_turnright  ][0] = KEY_RIGHTARROW;
	gamecontrol[gc_fire       ][0] = KEY_RCTRL;
	gamecontrol[gc_fire       ][1] = KEY_MOUSE1+0;
	gamecontrol[gc_firenormal ][0] = ';';
	gamecontrol[gc_lightdash  ][0] = '\'';
	gamecontrol[gc_use        ][0] = '.';
	gamecontrol[gc_taunt      ][0] = ',';
	gamecontrol[gc_camleft    ][0] = 'o';
	gamecontrol[gc_camright   ][0] = 'p';
	gamecontrol[gc_camreset   ][0] = 'c';
	gamecontrol[gc_lookup     ][0] = KEY_PGUP;
	gamecontrol[gc_lookdown   ][0] = KEY_PGDN;
	gamecontrol[gc_centerview ][0] = KEY_END;
	gamecontrol[gc_mouseaiming][0] = 's';
	gamecontrol[gc_talkkey    ][0] = 't';
	gamecontrol[gc_teamkey    ][0] = 'y';
	gamecontrol[gc_scores     ][0] = KEY_TAB;
	gamecontrol[gc_jump       ][0] = '/';
	gamecontrol[gc_console    ][0] = KEY_CONSOLE;
	gamecontrol[gc_pause      ][0] = KEY_PAUSE;
}
#endif

void G_SaveKeySetting(FILE *f)
{
	int i;

	for (i = 1; i < num_gamecontrols; i++)
	{
		fprintf(f, "setcontrol \"%s\" \"%s\"", gamecontrolname[i],
			G_KeynumToString(gamecontrol[i][0]));

		if (gamecontrol[i][1])
			fprintf(f, " \"%s\"\n", G_KeynumToString(gamecontrol[i][1]));
		else
			fprintf(f, "\n");
	}

	for (i = 1; i < num_gamecontrols; i++)
	{
		fprintf(f, "setcontrol2 \"%s\" \"%s\"", gamecontrolname[i],
			G_KeynumToString(gamecontrolbis[i][0]));

		if (gamecontrolbis[i][1])
			fprintf(f, " \"%s\"\n", G_KeynumToString(gamecontrolbis[i][1]));
		else
			fprintf(f, "\n");
	}
}

void G_CheckDoubleUsage(int keynum)
{
	if (cv_controlperkey.value == 1)
	{
		int i;
		for (i = 0; i < num_gamecontrols; i++)
		{
			if (gamecontrol[i][0] == keynum)
				gamecontrol[i][0] = KEY_NULL;
			if (gamecontrol[i][1] == keynum)
				gamecontrol[i][1] = KEY_NULL;
			if (gamecontrolbis[i][0] == keynum)
				gamecontrolbis[i][0] = KEY_NULL;
			if (gamecontrolbis[i][1] == keynum)
				gamecontrolbis[i][1] = KEY_NULL;
		}
	}
}

static void setcontrol(int (*gc)[2], int na)
{
	int numctrl;
	const char *namectrl;
	int keynum;

	namectrl = COM_Argv(1);
	for (numctrl = 0; numctrl < num_gamecontrols && stricmp(namectrl, gamecontrolname[numctrl]);
		numctrl++)
		;
	if (numctrl == num_gamecontrols)
	{
		CONS_Printf("Control '%s' unknown\n", namectrl);
		return;
	}
	keynum = G_KeyStringtoNum(COM_Argv(2));
	G_CheckDoubleUsage(keynum);
	gc[numctrl][0] = keynum;

	if (na == 4)
		gc[numctrl][1] = G_KeyStringtoNum(COM_Argv(3));
	else
		gc[numctrl][1] = 0;
}

void Command_Setcontrol_f(void)
{
	int na;

	na = (int)COM_Argc();

	if (na != 3 && na != 4)
	{
		CONS_Printf("setcontrol <controlname> <keyname> [<2nd keyname>]\n");
		return;
	}

	setcontrol(gamecontrol, na);
}

void Command_Setcontrol2_f(void)
{
	int na;

	na = (int)COM_Argc();

	if (na != 3 && na != 4)
	{
		CONS_Printf("setcontrol2 <controlname> <keyname> [<2nd keyname>]\n");
		return;
	}

	setcontrol(gamecontrolbis, na);
}
