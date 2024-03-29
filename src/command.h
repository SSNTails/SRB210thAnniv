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
/// \brief Deals with commands from console input, scripts, and remote server

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdio.h>
#include "doomtype.h"

//===================================
// Command buffer & command execution
//===================================

typedef void (*com_func_t)(void);

void COM_AddCommand(const char *name, com_func_t func);

size_t COM_Argc(void);
const char *COM_Argv(size_t arg); // if argv > argc, returns empty string
char *COM_Args(void);
size_t COM_CheckParm(const char *check); // like M_CheckParm :)

// match existing command or NULL
const char *COM_CompleteCommand(const char *partial, int skips);

// insert at queu (at end of other command)
void COM_BufAddText(const char *text);

// insert in head (before other command)
void COM_BufInsertText(const char *text);

// Execute commands in buffer, flush them
void COM_BufExecute(void);

// setup command buffer, at game tartup
void COM_Init(void);

// ======================
// Variable sized buffers
// ======================

typedef struct vsbuf_s
{
	boolean allowoverflow; // if false, do a I_Error
	boolean overflowed; // set to true if the buffer size failed
	byte *data;
	size_t maxsize;
	size_t cursize;
} vsbuf_t;

void VS_Alloc(vsbuf_t *buf, size_t initsize);
void VS_Free(vsbuf_t *buf);
void VS_Clear(vsbuf_t *buf);
void *VS_GetSpace(vsbuf_t *buf, size_t length);
void VS_Write(vsbuf_t *buf, const void *data, size_t length);
void VS_Print(vsbuf_t *buf, const char *data); // strcats onto the sizebuf

//==================
// Console variables
//==================
// console vars are variables that can be changed through code or console,
// at RUN TIME. They can also act as simplified commands, because a func-
// tion can be attached to a console var, which is called whenever the
// variable is modified (using flag CV_CALL).

// flags for console vars

typedef enum
{
	CV_SAVE = 1,   // save to config when quit game
	CV_CALL = 2,   // call function on change
	CV_NETVAR = 4, // send it when change (see logboris.txt at 12-4-2000)
	CV_NOINIT = 8, // dont call function when var is registered (1st set)
	CV_FLOAT = 16, // the value is fixed 16 : 16, where unit is FRACUNIT
	               // (allow user to enter 0.45 for ex)
	               // WARNING: currently only supports set with CV_Set()
	CV_NOTINNET = 32,     // some varaiable can't be changed in network but is not netvar (ex: splitscreen)
	CV_MODIFIED = 64,     // this bit is set when cvar is modified
	CV_SHOWMODIF = 128,   // say something when modified
	CV_SHOWMODIFONETIME = 256, // same but will be reset to 0 when modified, set in toggle
	CV_NOSHOWHELP = 512,  // Don't show variable in the HELP list Tails 08-13-2002
	CV_HIDEN = 1024,      // variable is not part of the cvar list so cannot be accessed by the console
	                      // can only be set when we have the pointer to hit
	                      // used on the menu
} cvflags_t;

typedef struct CV_PossibleValue_s
{
	int value;
	const char *strvalue;
} CV_PossibleValue_t;

typedef struct consvar_s //NULL, NULL, 0, NULL, NULL |, 0, NULL, NULL, 0, 0, NULL
{
	const char *name;
	const char *defaultvalue;
	int flags;            // flags see cvflags_t above
	CV_PossibleValue_t *PossibleValue; // table of possible values
	void (*func)(void);   // called on change, if CV_CALL set
	int value;            // for int and fixed_t
	const char *string;   // value in string
	char *zstring;        // Either NULL or same as string.
	                      // If non-NULL, must be Z_Free'd later.
	unsigned short netid; // used internaly : netid for send end receive
	                      // used only with CV_NETVAR
	char changed;         // has variable been changed by the user? 0 = no, 1 = yes
	struct consvar_s *next;
} consvar_t;

extern CV_PossibleValue_t CV_OnOff[];
extern CV_PossibleValue_t CV_YesNo[];
extern CV_PossibleValue_t CV_Unsigned[];
extern CV_PossibleValue_t CV_Natural[];
// register a variable for use at the console
void CV_RegisterVar(consvar_t *variable);

// sets changed to 0 for every console variable
void CV_ClearChangedFlags(void);

// returns the name of the nearest console variable name found
const char *CV_CompleteVar(char *partial, int skips);

// equivalent to "<varname> <value>" typed at the console
void CV_Set(consvar_t *var, const char *value);

// expands value to a string and calls CV_Set
void CV_SetValue(consvar_t *var, int value);

// avoids calling the function if it is CV_CALL
void CV_StealthSetValue(consvar_t *var, int value);
void CV_StealthSet(consvar_t *var, const char *value);

// it a setvalue but with a modulo at the maximum
void CV_AddValue(consvar_t *var, int increment);

// write all CV_SAVE variables to config file
void CV_SaveVariables(FILE *f);

// load/save gamesate (load and save option and for network join in game)
void CV_SaveNetVars(byte **p);
void CV_LoadNetVars(byte **p);

#endif // __COMMAND_H__
