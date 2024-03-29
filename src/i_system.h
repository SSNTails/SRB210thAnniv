// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
/// \brief System specific interface stuff.

#ifndef __I_SYSTEM__
#define __I_SYSTEM__

#include "d_ticcmd.h"
#include "d_event.h"

#ifdef __GNUG__
#pragma interface
#endif

/**	\brief max quit functions
*/
#define MAX_QUIT_FUNCS     16


/**	\brief Graphic system had started up
*/
extern byte graphics_started;

/**	\brief Keyboard system is up and run
*/
extern byte keyboard_started;

/**	\brief	The I_GetFreeMem function

	\param	total	total memory in the system

	\return	free memory in the system
*/
ULONG I_GetFreeMem(ULONG *total);

/**	\brief  Called by D_SRB2Loop, returns current time in tics.
*/
tic_t I_GetTime(void);

/**	\brief	The I_Sleep function

	\return	void
*/
void I_Sleep(void);

/**	\brief Get events

	Called by D_SRB2Loop,
	called before processing each tic in a frame.
	Quick syncronous operations are performed here.
	Can call D_PostEvent.
*/
void I_GetEvent(void);

/**	\brief Asynchronous interrupt functions should maintain private queues
	that are read by the synchronous functions
	to be converted into events.
*/
void I_OsPolling(void);

// Either returns a null ticcmd,
// or calls a loadable driver to build it.
// This ticcmd will then be modified by the gameloop
// for normal input.

/**	\brief Input for the first player
*/
ticcmd_t *I_BaseTiccmd(void);

/**	\brief Input for the sencond player
*/
ticcmd_t *I_BaseTiccmd2(void);

/**	\brief Called by M_Responder when quit is selected, return exit code 0
*/
void I_Quit(void) FUNCNORETURN;

/**	\brief	Allocates from low memory under dos,
	just mallocs under unix

	\param	length	how much memory
	\return	memory from low memory under dos
*/
byte *I_AllocLow(int length);

typedef enum
{
	EvilForce = -1,
	//Constant
	ConstantForce = 0,
	//Ramp
	RampForce,
	//Periodics
	SquareForce,
	SineForce,
	TriangleForce,
	SawtoothUpForce,
	SawtoothDownForce,
	//MAX
	NumberofForces,
} FFType;

typedef struct JoyFF_s
{
	long ForceX; ///< The X of the Force's Vel
	long ForceY; ///< The Y of the Force's Vel
	//All
	unsigned long Duration; ///< The total duration of the effect, in microseconds
	long Gain; //< /The gain to be applied to the effect, in the range from 0 through 10,000.
	//All, CONSTANTFORCE -10,000 to 10,000
	long Magnitude; ///< Magnitude of the effect, in the range from 0 through 10,000.
	//RAMPFORCE
	long Start; ///< Magnitude at the start of the effect, in the range from -10,000 through 10,000.
	long End; ///< Magnitude at the end of the effect, in the range from -10,000 through 10,000.
	//PERIODIC
	long Offset; ///< Offset of the effect.
	unsigned long Phase; ///< Position in the cycle of the periodic effect at which playback begins, in the range from 0 through 35,999
	unsigned long Period; ///< Period of the effect, in microseconds.
} JoyFF_t;

/**	\brief	Forcefeedback for the first joystick

	\param	Type   what kind of Effect
	\param	Effect Effect Info

	\return	void
*/

void I_Tactile(FFType Type, const JoyFF_t *Effect);

/**	\brief	Forcefeedback for the second joystick

	\param	Type   what kind of Effect
	\param	Effect Effect Info

	\return	void
*/
void I_Tactile2(FFType Type, const JoyFF_t *Effect);

/**	\brief to set up the first joystick scale
*/
void I_JoyScale(void);

/**	\brief to set up the second joystick scale
*/
void I_JoyScale2(void);

// Called by D_SRB2Main.

/**	\brief to startup the first joystick
*/
void I_InitJoystick(void);

/**	\brief to startup the second joystick
*/
void I_InitJoystick2(void);

/**	\brief return the number of joystick on the system
*/
int I_NumJoys(void);

/**	\brief	The *I_GetJoyName function

	\param	joyindex	which joystick

	\return	joystick name
*/
const char *I_GetJoyName(int joyindex);

/**	\brief	write a message to stderr (use before I_Quit) for when you need to quit with a msg, but need
 the return code 0 of I_Quit();

	\param	error	message string

	\return	void
*/
void I_OutputMsg(const char *error, ...) FUNCPRINTF;

/**	\brief Startup the first mouse
*/
void I_StartupMouse(void);

/**	\brief Startup the second mouse
*/
void I_StartupMouse2(void);

/**	\brief keyboard startup, shutdown, handler
*/
void I_StartupKeyboard(void);

/**	\brief  setup timer irq and user timer routine.
*/
void I_StartupTimer(void);

/**	\brief sample quit function
*/
typedef void (*quitfuncptr)();

/**	\brief	add a list of functions to call at program cleanup

	\param	(*func)()	funcction to call at program cleanup

	\return	void
*/
void I_AddExitFunc(void (*func)());

/**	\brief	The I_RemoveExitFunc function

	\param	(*func)()	function to remove from the list

	\return	void
*/
void I_RemoveExitFunc(void (*func)());

/**	\brief Setup signal handler, plus stuff for trapping errors and cleanly exit.
*/
int I_StartupSystem(void);

/**	\brief Shutdown systems
*/
void I_ShutdownSystem(void);

/**	\brief	The I_GetDiskFreeSpace function

	\param	freespace	a INT64 pointer to hold the free space amount

	\return	void
*/
void I_GetDiskFreeSpace(INT64 *freespace);

/**	\brief find out the user's name
*/
char *I_GetUserName(void);

/**	\brief	The I_mkdir function

	\param	dirname	string of mkidr
	\param	unixright	unix right

	\return status of new folder
*/
int I_mkdir(const char *dirname, int unixright);

typedef struct {
	int FPU        : 1; ///< FPU availabile
	int CPUID      : 1; ///< CPUID instruction
	int RDTSC      : 1; ///< RDTSC instruction
	int MMX        : 1; ///< MMX features
	int MMXExt     : 1; ///< MMX Ext. features
	int CMOV       : 1; ///< Pentium Pro's "cmov"
	int AMD3DNow   : 1; ///< 3DNow features
	int AMD3DNowExt: 1; ///< 3DNow! Ext. features
	int SSE        : 1; ///< SSE features
	int SSE2       : 1; ///< SSE2 features
	int SSE3       : 1; ///< SSE3 features
	int IA64       : 1; ///< Running on IA64
	int AMD64      : 1; ///< Running on AMD64
	int AltiVec    : 1; ///< AltiVec features
	int FPPE       : 1; ///< floating-point precision error
	int PFC        : 1; ///< TBD?
	int cmpxchg    : 1; ///< ?
	int cmpxchg16b : 1; ///< ?
	int cmp8xchg16 : 1; ///< ?
	int FPE        : 1; ///< FPU Emu
	int DEP        : 1; ///< Data excution prevent
	int PPCMM64    : 1; ///< PowerPC Movemem 64bit ok?
	int ALPHAbyte  : 1; ///< ?
	int PAE        : 1; ///< Physical Address Extension
	int CPUs       : 8;
} CPUInfoFlags;


/**	\brief Info about CPU
		\return CPUInfo in bits
*/
const CPUInfoFlags *I_CPUInfo(void);

/**	\brief Find main WAD
		\return path to main WAD
*/
const char *I_LocateWad(void);

/**	\brief First Joystick's events
*/
void I_GetJoystickEvents(void);

/**	\brief Second Joystick's events
*/
void I_GetJoystick2Events(void);

/**	\brief Mouses events
*/
void I_GetMouseEvents(void);

char *I_GetEnv(const char *name);

int I_PutEnv(char *variable);

#endif
