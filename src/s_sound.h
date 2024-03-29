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
/// \brief The not so system specific sound interface

#ifndef __S_SOUND__
#define __S_SOUND__

#include "sounds.h"
#include "m_fixed.h"

// mask used to indicate sound origin is player item pickup
#define PICKUP_SOUND 0x8000

extern consvar_t stereoreverse;
extern consvar_t cv_soundvolume, cv_digmusicvolume, cv_midimusicvolume;
extern consvar_t cv_numChannels;

#ifdef SNDSERV
extern consvar_t sndserver_cmd, sndserver_arg;
#endif
#ifdef MUSSERV
extern consvar_t musserver_cmd, musserver_arg;
#endif

extern CV_PossibleValue_t soundvolume_cons_t[];
//part of i_cdmus.c
extern consvar_t cd_volume, cdUpdate;

#if defined (macintosh) && !defined (SDL)
typedef enum
{
	music_normal,
	playlist_random,
	playlist_normal
} playmode_t;

extern consvar_t play_mode;
#endif

typedef enum
{
	SF_TOTALLYSINGLE =  1, // Only play one of these sounds at a time...GLOBALLY
	SF_NOMULTIPLESOUND =  2, // Like SF_NOINTERRUPT, but doesnt care what the origin is
	SF_OUTSIDESOUND  =  4, // Volume is adjusted depending on how far away you are from 'outside'
	SF_X4AWAYSOUND   =  8, // Hear it from 4x the distance away
	SF_X8AWAYSOUND   = 16, // Hear it from 8x the distance away
	SF_NOINTERRUPT   = 32, // Only play this sound if it isn't already playing on the origin
} soundflags_t;

// register sound vars and commands at game startup
void S_RegisterSoundStuff(void);

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume, allocates channel buffer, sets S_sfx lookup.
//
void S_Init(int sfxVolume, int digMusicVolume, int midiMusicVolume);

//
// Per level startup code.
// Kills playing sounds at start of level, determines music if any, changes music.
//
void S_StopSounds(void);
void S_ClearSfx(void);
void S_Start(void);

//
// Basically a W_GetNumForName that adds "ds" at the beginning of the string. Returns a lumpnum.
//
lumpnum_t S_GetSfxLumpNum(sfxinfo_t *sfx);

//
// Start sound for thing at <origin> using <sound_id> from sounds.h
//
void S_StartSound(const void *origin, sfxenum_t sound_id);

// Will start a sound at a given volume.
void S_StartSoundAtVolume(const void *origin, sfxenum_t sound_id, int volume);

// Stop sound for thing at <origin>
void S_StopSound(void *origin);

// Start music using <music_id> from sounds.h, and set whether looping
void S_ChangeMusic(musicenum_t music_num, int looping);

// Set Speed of Music
boolean S_SpeedMusic(float speed);

// Stops the music.
void S_StopMusic(void);

// Stop and resume music, during game PAUSE.
void S_PauseSound(void);
void S_ResumeSound(void);

//
// Updates music & sounds
//
void S_UpdateSounds(void);

fixed_t S_CalculateSoundDistance(fixed_t x1, fixed_t y1, fixed_t z1, fixed_t x2, fixed_t y2, fixed_t z2);

void S_SetDigMusicVolume(int volume);
void S_SetMIDIMusicVolume(int volume);
void S_SetSfxVolume(int volume);

int S_SoundPlaying(void *origin, sfxenum_t id);
void S_StartSoundName(void *mo, char *soundname);

void S_StopSoundByNum(sfxenum_t sfxnum);

#endif
