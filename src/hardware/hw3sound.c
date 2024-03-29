// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2001 by DooM Legacy Team.
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
/// \brief Hardware 3D sound general code

#include "../doomdef.h"

#ifdef HW3SOUND

#include "../i_sound.h"
#include "../s_sound.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../g_game.h"
#include "../tables.h"
#include "../sounds.h"
#include "../r_main.h"
#include "../r_things.h"
#include "../m_random.h"
#include "../p_local.h"
#include "hw3dsdrv.h"
#include "hw3sound.h"

#define ANGLE2DEG(x) (((double)(x)) / ((double)ANG45/45))
#define NORM_PITCH 128
#define NORM_PRIORITY 64
#define NORM_SEP 128
//Alam_GBC: MPS, not MPF!
#define TPS(x) ((float)(x)/(float)TICRATE)

struct hardware3ds_s hw3ds_driver;

typedef struct
{
	fixed_t x, y, z;
	angle_t angle;
} listener_t;


typedef struct source_s
{
	sfxinfo_t       *sfxinfo;
	const void      *origin;
	int             handle;     // Internal source handle
	channel_type_t  type;       // Sound type (attack, scream, etc)
} source_t;


typedef struct ambient_sdata_s
{
	source3D_data_t left;
	source3D_data_t right;
} ambient_sdata_t;


typedef struct ambient_source_s
{
	source_t    left;
	source_t    right;
} ambient_source_t;

// Static sources
// This sources always creates
static source_t p_attack_source;        // Player attack source
static source_t p_attack_source2;        // Player2 attack source
static source_t p_scream_source;        // Player scream source
static source_t p_scream_source2;        // Player2 scream source


static ambient_source_t ambient_source; // Ambinet sound sources

//  abuse???
//static source3D_data_t  p_attack_sdata; // ?? Now just holds precalculated player attack source data
//static source3D_data_t  p_default_sdata;// ?? ---- // ---- // ---- // ---- player scream source data

static ambient_sdata_t  ambient_sdata;  // Precalculated datas of an ambient sound sources

// Stack of dynamic sources
static source_t      *sources       = NULL;     // Much like original channels
static int           num_sources    = 0;

// Current mode of 3D sound system
// Default is original (stereo) mode
int                  hws_mode       = HWS_DEFAULT_MODE;

//=============================================================================
void HW3S_SetSourcesNum(void)
{
	int i;

	// Allocating the internal channels for mixing
	// (the maximum number of sounds rendered
	// simultaneously) within zone memory.
	if (sources)
		HW3S_StopSounds();
	Z_Free(sources);

	if (cv_numChannels.value <= STATIC_SOURCES_NUM)
		I_Error("HW3S_SetSourcesNum: Number of sound sources cannot be less than %d\n", STATIC_SOURCES_NUM + 1);

	num_sources = cv_numChannels.value - STATIC_SOURCES_NUM;

	sources = (source_t *) Z_Malloc(num_sources * sizeof (*sources), PU_STATIC, 0);

	// Free all channels for use
	for (i = 0; i < num_sources; i++)
	{
		sources[i].sfxinfo = NULL;
		sources[i].type = CT_NORMAL;
		sources[i].handle = -1;
	}
}


//=============================================================================
static void HW3S_KillSource(int snum)
{
	source_t *  s = &sources[snum];

	if (s->sfxinfo)
	{
		HW3DS.pfnStopSource(s->handle);
		HW3DS.pfnKillSource(s->handle);
		s->handle = -1;
		s->sfxinfo->usefulness--;
		s->origin = NULL;
		s->sfxinfo = NULL;
	}
}


//=============================================================================
/*
static void HW3S_StopSource(int snum)
{
	source_t * s = &sources[snum];

	if (s->sfxinfo)
	{
		// stop the sound playing
		HW3DS.pfnStopSource(s->handle);
	}
}
*/


//=============================================================================
void HW3S_StopSound(void *origin)
{
	int snum;

	for (snum = 0; snum < num_sources; snum++)
	{
		if (sources[snum].sfxinfo && sources[snum].origin == origin)
		{
			HW3S_KillSource(snum);
			break;
		}
	}
}

void HW3S_StopSoundByNum(sfxenum_t sfxnum)
{
	int snum;

	for (snum = 0; snum < num_sources; snum++)
	{
		if (sources[snum].sfxinfo == &S_sfx[sfxnum])
		{
			HW3S_KillSource(snum);
			break;
		}
	}
}

//=============================================================================
void HW3S_StopSounds(void)
{
	int snum;

	// kill all playing sounds at start of level
	//  (trust me - a good idea)
	for (snum = 0; snum < num_sources; snum++)
		if (sources[snum].sfxinfo)
			HW3S_KillSource(snum);

	// Also stop all static sources
	HW3DS.pfnStopSource(p_attack_source.handle);
	HW3DS.pfnStopSource(p_attack_source2.handle);
	HW3DS.pfnStopSource(p_scream_source.handle);
	HW3DS.pfnStopSource(p_scream_source2.handle);
	HW3DS.pfnStopSource(ambient_source.left.handle);
	HW3DS.pfnStopSource(ambient_source.right.handle);
}


//=============================================================================
static int HW3S_GetSource(const void *origin, sfxinfo_t *sfxinfo, boolean splitsound)
{
	//
	//   If none available, return -1.  Otherwise source #.
	//   source number to use

	int         snum;
	source_t *  src;
	int sep = NORM_SEP, pitch = NORM_PITCH, volume = 255;

	mobj_t *listenmobj;
	listener_t listener  = {0,0,0,0};

	if (splitsound)
		listenmobj = players[displayplayer].mo;
	else
		listenmobj = players[secondarydisplayplayer].mo;

	if (splitsound && cv_chasecam2.value)
	{
		listener.x = camera2.x;
		listener.y = camera2.y;
		listener.z = camera2.z;
		listener.angle = camera2.angle;
	}
	else if (!splitsound && cv_chasecam.value)
	{
		listener.x = camera.x;
		listener.y = camera.y;
		listener.z = camera.z;
		listener.angle = camera.angle;
	}
	else if (listenmobj)
	{
		listener.x = listenmobj->x;
		listener.y = listenmobj->y;
		listener.z = listenmobj->z;
		listener.angle = listenmobj->angle;
	}

	// Find an open source
	for (snum = 0, src = sources; snum < num_sources; src++, snum++)
	{
		if (!src->sfxinfo)
			break;

		if (origin && src->origin ==  origin)
		{
			HW3S_KillSource(snum);
			break;
		}
	}

	// Check to see if it is audible
	if (origin && origin != listenmobj)
	{
		int rc;
		rc = S_AdjustSoundParams(listenmobj, origin, &volume, &sep, &pitch, sfxinfo);
		if (!rc)
			return -1;
	}

	// None available
	if (snum == num_sources)
	{
		// Look for lower priority
		for (snum = 0, src = sources; snum < num_sources; src++, snum++)
			if (src->sfxinfo->priority >= sfxinfo->priority)
				break;

		if (snum == num_sources)
		{
			// FUCK!  No lower priority.  Sorry, Charlie.
			return -1;
		}
		else
		{
			// Otherwise, kick out lower priority
			HW3S_KillSource(snum);
		}
	}
	return snum;
}


//=============================================================================
static void HW3S_FillSourceParameters
                              (const mobj_t     *origin,
                               source3D_data_t  *data,
                               channel_type_t   c_type)
{
	fixed_t x, y, z;

	if (origin && origin != players[displayplayer].mo)
	{
		memset(data, 0, sizeof (source3D_data_t));

		data->max_distance = MAX_DISTANCE;
		data->min_distance = MIN_DISTANCE;

		data->pos.momx = TPS(FIXED_TO_FLOAT(origin->momx));
		data->pos.momy = TPS(FIXED_TO_FLOAT(origin->momy));
		data->pos.momz = TPS(FIXED_TO_FLOAT(origin->momz));

		x = origin->x;
		y = origin->y;
		z = origin->z;

		if (c_type == CT_ATTACK)
		{
			const angle_t an = origin->angle >> ANGLETOFINESHIFT;
			x += FixedMul(16*FRACUNIT, FINECOSINE(an));
			y += FixedMul(16*FRACUNIT, FINESINE(an));
			z += origin->height >> 1;
		}

		else if (c_type == CT_SCREAM)
			z += origin->height - (5 * FRACUNIT);

		data->pos.x = FIXED_TO_FLOAT(x);
		data->pos.y = FIXED_TO_FLOAT(y);
		data->pos.z = FIXED_TO_FLOAT(z);
	}
}

#define HEADER_SIZE 8
//==============================================================
/*
static void make_outphase_sfx(void *dest, void *src, int size)
{
	signed char    *s = (signed char *)src + HEADER_SIZE, *d = (signed char *)dest + HEADER_SIZE;

	memcpy(dest, src, HEADER_SIZE);
	size -= HEADER_SIZE;

	while (size--)
		*d++ = -(*s++);
}
*/

//int HW3S_Start3DSound(const void *origin, source3D_data_t *source_parm, cone_def_t *cone_parm, channel_type_t channel, int sfx_id, int vol, int pitch);
//=============================================================================
int HW3S_I_StartSound(const void *origin_p, source3D_data_t *source_parm, channel_type_t c_type, sfxenum_t sfx_id, int volume, int pitch, int sep)
{
	sfxinfo_t       *sfx;
	const mobj_t    *origin = (const mobj_t *)origin_p;
	source3D_data_t source3d_data;
	int             s_num = 0;
	source_t        *source = NULL;
	mobj_t *listenmobj = players[displayplayer].mo;
	mobj_t *listenmobj2 = NULL;
	listener_t listener  = {0,0,0,0};
	listener_t listener2 = {0,0,0,0};

	if (splitscreen) listenmobj2 = players[secondarydisplayplayer].mo;

	if (nosound)
		return -1;

	if (cv_chasecam.value)
	{
		listener.x = camera.x;
		listener.y = camera.y;
		listener.z = camera.z;
		listener.angle = camera.angle;
	}
	else if (listenmobj)
	{
		listener.x = listenmobj->x;
		listener.y = listenmobj->y;
		listener.z = listenmobj->z;
		listener.angle = listenmobj->angle;
	}
	else if (origin)
		return -1;

	if (listenmobj2)
	{
		if (cv_chasecam2.value)
		{
			listener2.x = camera2.x;
			listener2.y = camera2.y;
			listener2.z = camera2.z;
			listener2.angle = camera2.angle;
		}
		else
		{
			listener2.x = listenmobj2->x;
			listener2.y = listenmobj2->y;
			listener2.z = listenmobj2->z;
			listener2.angle = listenmobj2->angle;
		}
	}

	sfx = &S_sfx[sfx_id];

	if (sfx->skinsound!=-1 && origin && origin->skin)
	{
		// it redirect player sound to the sound in the skin table
		sfx_id = ((skin_t *)origin->skin)->soundsid[sfx->skinsound];
		sfx    = &S_sfx[sfx_id];
	}

	if (!sfx->data)
		sfx->data = HW3S_GetSfx(sfx);

	// judgecutor 08-16-2002
	// Sound pitching for both Doom and Heretic
#if 0
	if (cv_rndsoundpitch.value)
	{
		/*if (gamemode != heretic)
		{
			if (sfx_id >= sfx_sawup && sfx_id <= sfx_sawhit)
				pitch += 8 - (M_Random()&15);
			else if (sfx_id != sfx_itemup && sfx_id != sfx_tink)
				pitch += 16 - (M_Random()&31);
		}
		else*/
			pitch = 128 + (M_Random() & 7) - (M_Random() & 7);
	}
#endif

	if (pitch < 0)
		pitch = NORMAL_PITCH;

	if (pitch > 255)
		pitch = 255;

	if (sep < 0)
		sep = 128;

	if (splitscreen && listenmobj2) // Copy the sound for the split player
	{
		if (c_type != CT_NORMAL && origin && (origin == listenmobj2))
		{
			if (c_type == CT_ATTACK)
			{
				if (origin == listenmobj2)
					source = &p_attack_source;
				else
					source = &p_attack_source2;
			}
			else
			{
				if (origin == listenmobj2)
					source = &p_scream_source;
				else
					source = &p_scream_source2;
			}

			if (source->sfxinfo != sfx)
			{
				HW3DS.pfnStopSource(source->handle);
				source->handle = HW3DS.pfnReloadSource(source->handle, sfx->volume);
				//CONS_Printf("PlayerSound data reloaded\n");
			}
		}
		else if (c_type == CT_AMBIENT)
		{
	//        sfx_data_t  outphased_sfx;

			if (ambient_source.left.sfxinfo != sfx)
			{
				HW3DS.pfnStopSource(ambient_source.left.handle);
				HW3DS.pfnStopSource(ambient_source.right.handle);

				// judgecutor:
				// Outphased sfx's temporarily not used!!!
	/*
					outphased_sfx.data = Z_Malloc(sfx_data.length, PU_STATIC, 0);
					make_outphase_sfx(outphased_sfx.data, sfx_data.data, sfx_data.length);
					outphased_sfx.length = sfx_data.length;
					outphased_sfx.id = sfx_data.id;
	*/
				ambient_source.left.handle = HW3DS.pfnReloadSource(ambient_source.left.handle, (u_int)sfx->length);
				//ambient_source.right.handle = HW3DS.pfnReloadSource(ambient_source.right.handle, &outphased_sfx);
				ambient_source.right.handle = HW3DS.pfnReloadSource(ambient_source.right.handle, (u_int)sfx->length);
				ambient_source.left.sfxinfo = ambient_source.right.sfxinfo = sfx;
				//Z_Free(outphased_sfx.data);
			}

			HW3DS.pfnUpdateSourceParms(ambient_source.left.handle, volume, -1);
			HW3DS.pfnUpdateSourceParms(ambient_source.right.handle, volume, -1);

			if (sfx->usefulness++ < 0)
				sfx->usefulness = -1;

			// Ambient sound is special case
			HW3DS.pfnStartSource(ambient_source.left.handle);
			HW3DS.pfnStartSource(ambient_source.right.handle);
		}
		else
		{
			s_num = HW3S_GetSource(origin, sfx, true);

			if (s_num  < 0)
			{
				//CONS_Printf("No free source, aborting\n");
				return -1;
			}

			source = &sources[s_num];

			if (origin && c_type == CT_NORMAL)
			{
				if (!source_parm)
				{
					source_parm = &source3d_data;
					HW3S_FillSourceParameters(origin, source_parm, c_type);
				}

				source->handle = HW3DS.pfnAddSource(source_parm, (u_int)sfx->length);
			}
			else
				source->handle = HW3DS.pfnAddSource(NULL, (u_int)sfx->length);
		}

		// increase the usefulness
		if (sfx->usefulness++ < 0)
			sfx->usefulness = -1;

		source->sfxinfo = sfx;
		source->origin = origin;
		HW3DS.pfnStartSource(source->handle);
	}

	if (c_type != CT_NORMAL && origin && (origin == listenmobj))
	{
		if (c_type == CT_ATTACK)
		{
			if (origin == listenmobj)
				source = &p_attack_source;
			else
				source = &p_attack_source2;
		}
		else
		{
			if (origin == listenmobj)
				source = &p_scream_source;
			else
				source = &p_scream_source2;
		}

		if (source->sfxinfo != sfx)
		{
			HW3DS.pfnStopSource(source->handle);
			source->handle = HW3DS.pfnReloadSource(source->handle, sfx->volume);
			//CONS_Printf("PlayerSound data reloaded\n");
		}
	}
	else if (c_type == CT_AMBIENT)
	{
//        sfx_data_t  outphased_sfx;

		if (ambient_source.left.sfxinfo != sfx)
		{
			HW3DS.pfnStopSource(ambient_source.left.handle);
			HW3DS.pfnStopSource(ambient_source.right.handle);

			// judgecutor:
			// Outphased sfx's temporarily not used!!!
/*
				outphased_sfx.data = Z_Malloc(sfx_data.length, PU_STATIC, 0);
				make_outphase_sfx(outphased_sfx.data, sfx_data.data, sfx_data.length);
				outphased_sfx.length = sfx_data.length;
				outphased_sfx.id = sfx_data.id;
*/
			ambient_source.left.handle = HW3DS.pfnReloadSource(ambient_source.left.handle, (u_int)sfx->length);
			//ambient_source.right.handle = HW3DS.pfnReloadSource(ambient_source.right.handle, &outphased_sfx);
			ambient_source.right.handle = HW3DS.pfnReloadSource(ambient_source.right.handle, (u_int)sfx->length);
			ambient_source.left.sfxinfo = ambient_source.right.sfxinfo = sfx;
			//Z_Free(outphased_sfx.data);
		}

		HW3DS.pfnUpdateSourceParms(ambient_source.left.handle, volume, -1);
		HW3DS.pfnUpdateSourceParms(ambient_source.right.handle, volume, -1);

		if (sfx->usefulness++ < 0)
			sfx->usefulness = -1;

		// Ambient sound is special case
		HW3DS.pfnStartSource(ambient_source.left.handle);
		HW3DS.pfnStartSource(ambient_source.right.handle);
		return -1;
	}
	else
	{
		s_num = HW3S_GetSource(origin, sfx, false);

		if (s_num  < 0)
		{
			//CONS_Printf("No free source, aborting\n");
			return -1;
		}

		source = &sources[s_num];

		if (origin && c_type == CT_NORMAL)
		{
			if (!source_parm)
			{
				source_parm = &source3d_data;
				HW3S_FillSourceParameters(origin, source_parm, c_type);
			}

			source->handle = HW3DS.pfnAddSource(source_parm, (u_int)sfx->length);
		}
		else
			source->handle = HW3DS.pfnAddSource(NULL, (u_int)sfx->length);

	}

	// increase the usefulness
	if (sfx->usefulness++ < 0)
		sfx->usefulness = -1;

	source->sfxinfo = sfx;
	source->origin = origin;
	HW3DS.pfnStartSource(source->handle);
	return s_num;

}


// Start normal sound
//=============================================================================
void HW3S_StartSound(const void *origin, sfxenum_t sfx_id)
{
	HW3S_I_StartSound(origin, NULL, CT_NORMAL, sfx_id, 255, NORMAL_PITCH, NORMAL_SEP);
}


//=============================================================================
void S_StartAttackSound(const void *origin, sfxenum_t sfx_id)
{
	if (hws_mode != HWS_DEFAULT_MODE)
		HW3S_I_StartSound(origin, NULL, CT_ATTACK, sfx_id, 255, NORMAL_PITCH, NORMAL_SEP);
	else
		S_StartSound(origin, sfx_id);
}

void S_StartScreamSound(const void *origin, sfxenum_t sfx_id)
{
	if (hws_mode != HWS_DEFAULT_MODE)
		HW3S_I_StartSound(origin, NULL, CT_SCREAM, sfx_id, 255, NORMAL_PITCH, NORMAL_SEP);
	else
		S_StartSound(origin, sfx_id);
}

#if 0 /// NOTE: not used
void S_StartAmbientSound(sfxenum_t sfx_id, int volume)
{
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		volume += 30;
		if (volume > 255)
			volume = 255;

		HW3S_I_StartSound(NULL, NULL, CT_AMBIENT, sfx_id, volume, NORMAL_PITCH, NORMAL_SEP);
	}
	else
		S_StartSoundAtVolume(NULL, sfx_id, volume);
}
#endif

FUNCMATH static inline float AmbientPos(angle_t an)
{
	const fixed_t fm = FixedMul((fixed_t)(MIN_DISTANCE * FRACUNIT), FINESINE(an>>ANGLETOFINESHIFT));
	return FIXED_TO_FLOAT(fm);
}


//=============================================================================
int HW3S_Init(I_Error_t FatalErrorFunction, snddev_t *snd_dev)
{
	int                succ;
	source3D_data_t    source_data;

	if (HW3DS.pfnStartup(FatalErrorFunction, snd_dev))
	{
		// Creating player sources
		memset(&source_data, 0, sizeof (source_data));

		// Attack source
		source_data.head_relative = 1;
		source_data.pos.y = 16;
		source_data.pos.z = -FIXED_TO_FLOAT(mobjinfo[MT_PLAYER].height >> 1);
		source_data.min_distance = MIN_DISTANCE;
		source_data.max_distance = MAX_DISTANCE;
		source_data.permanent = 1;

		p_attack_source.sfxinfo = NULL;

		memcpy(&p_attack_source2, &p_attack_source, sizeof (source_t));

		p_attack_source.handle = HW3DS.pfnAddSource(&source_data, NUMSFX);
		p_attack_source2.handle = HW3DS.pfnAddSource(&source_data, NUMSFX);

		// Scream source
		source_data.pos.y = 0;
		source_data.pos.z = 0;

		p_scream_source.sfxinfo = NULL;

		memcpy(&p_scream_source2, &p_scream_source, sizeof (source_t));

		p_scream_source.handle = HW3DS.pfnAddSource(&source_data, NUMSFX);
		p_scream_source2.handle = HW3DS.pfnAddSource(&source_data, NUMSFX);

		//FIXED_TO_FLOAT(mobjinfo[MT_PLAYER].height - (5 * FRACUNIT));

		// Ambient sources (left and right) at 210 and 330 degree
		// relative to listener
		memset(&ambient_sdata, 0, sizeof (ambient_sdata));

		ambient_sdata.left.head_relative = 1;
		ambient_sdata.left.pos.x = ambient_sdata.left.pos.y = AmbientPos(ANG180 + ANG90/3);
		ambient_sdata.left.max_distance = MAX_DISTANCE;
		ambient_sdata.left.min_distance = MIN_DISTANCE;

		ambient_sdata.left.permanent = 1;

		memcpy(&ambient_sdata.right, &ambient_sdata.left, sizeof (source3D_data_t));

		ambient_sdata.right.pos.x = -ambient_sdata.left.pos.x;
		ambient_source.left.handle = HW3DS.pfnAddSource(&ambient_sdata.left, NUMSFX);
		ambient_source.right.handle = HW3DS.pfnAddSource(&ambient_sdata.right, NUMSFX);

		succ = p_attack_source.handle > -1 && p_scream_source.handle > -1 &&
			p_attack_source2.handle > -1 && p_scream_source2.handle > -1 &&
			ambient_source.left.handle > -1 && ambient_source.right.handle > -1;

		//CONS_Printf("Player handles: attack %d, default %d\n", p_attack_source.handle, p_scream_source.handle);
		return succ;
	}
	return 0;
}




//=============================================================================
int HW3S_GetVersion(void)
{
	return HW3DS.pfnGetHW3DSVersion();
}


//=============================================================================
void HW3S_BeginFrameUpdate(void)
{
	if (hws_mode != HWS_DEFAULT_MODE)
		HW3DS.pfnBeginFrameUpdate();
}


//=============================================================================
void HW3S_EndFrameUpdate(void)
{
	if (hws_mode != HWS_DEFAULT_MODE)
		HW3DS.pfnEndFrameUpdate();
}



//=============================================================================
int HW3S_SoundIsPlaying(int handle)
{
	return HW3DS.pfnIsPlaying(handle);
}

int HW3S_SoundPlaying(void *origin, sfxenum_t id)
{
	int         snum;

	for (snum = 0; snum < num_sources; snum++)
	{
		if (origin &&  sources[snum].origin ==  origin)
			return 1;
		if (id != NUMSFX && (size_t)(sources[snum].sfxinfo - S_sfx) == (size_t)id)
			return 1;
	}
	return 0;
}

//=============================================================================
static void HW3S_UpdateListener(mobj_t *listener)
{
	listener_data_t data;

	if (!listener || !listener->player)
		return;

	if (camera.chase)
	{
		data.x = FIXED_TO_FLOAT(camera.x);
		data.y = FIXED_TO_FLOAT(camera.y);
		data.z = FIXED_TO_FLOAT(camera.z + camera.height - (5 * FRACUNIT));

		data.f_angle = ANGLE2DEG(camera.angle);
		data.h_angle = ANGLE2DEG(camera.aiming);

		data.momx = TPS(FIXED_TO_FLOAT(camera.momx));
		data.momy = TPS(FIXED_TO_FLOAT(camera.momy));
		data.momz = TPS(FIXED_TO_FLOAT(camera.momz));
	}
	else
	{
		data.x = FIXED_TO_FLOAT(listener->x);
		data.y = FIXED_TO_FLOAT(listener->y);
		data.z = FIXED_TO_FLOAT(listener->z + listener->height - (5 * FRACUNIT));

		data.f_angle = ANGLE2DEG(listener->angle);
		data.h_angle = ANGLE2DEG(listener->player->aiming);

		data.momx = TPS(FIXED_TO_FLOAT(listener->momx));
		data.momy = TPS(FIXED_TO_FLOAT(listener->momy));
		data.momz = TPS(FIXED_TO_FLOAT(listener->momz));
	}
	HW3DS.pfnUpdateListener(&data, 1);
}

static void HW3S_UpdateListener2(mobj_t *listener)
{
	listener_data_t data;

	if (!listener || !listener->player)
	{
		HW3DS.pfnUpdateListener(NULL, 2);
		return;
	}

	if (camera2.chase)
	{
		data.x = FIXED_TO_FLOAT(camera2.x);
		data.y = FIXED_TO_FLOAT(camera2.y);
		data.z = FIXED_TO_FLOAT(camera2.z + camera2.height - (5 * FRACUNIT));

		data.f_angle = ANGLE2DEG(camera2.angle);
		data.h_angle = ANGLE2DEG(camera2.aiming);

		data.momx = TPS(FIXED_TO_FLOAT(camera2.momx));
		data.momy = TPS(FIXED_TO_FLOAT(camera2.momy));
		data.momz = TPS(FIXED_TO_FLOAT(camera2.momz));
	}
	else
	{
		data.x = FIXED_TO_FLOAT(listener->x);
		data.y = FIXED_TO_FLOAT(listener->y);
		data.z = FIXED_TO_FLOAT(listener->z + listener->height - (5 * FRACUNIT));

		data.f_angle = ANGLE2DEG(listener->angle);
		data.h_angle = ANGLE2DEG(listener->player->aiming);

		data.momx = TPS(FIXED_TO_FLOAT(listener->momx));
		data.momy = TPS(FIXED_TO_FLOAT(listener->momy));
		data.momz = TPS(FIXED_TO_FLOAT(listener->momz));
	}

	HW3DS.pfnUpdateListener(&data, 2);
}

void HW3S_SetSfxVolume(int volume)
{
	HW3DS.pfnSetGlobalSfxVolume(volume);
}


static void HW3S_Update3DSource(source_t *src)
{
	source3D_data_t data;

	HW3S_FillSourceParameters(src->origin, &data, src->type);
	HW3DS.pfnUpdate3DSource(src->handle, &data.pos);

}

void HW3S_UpdateSources(void)
{
	mobj_t *listener = players[displayplayer].mo;
	mobj_t *listener2 = NULL;
	source_t    *src;
	int audible, snum, volume, sep, pitch;

	if (splitscreen) listener2 = players[secondarydisplayplayer].mo;

	HW3S_UpdateListener2(listener2);
	HW3S_UpdateListener(listener);

	for (snum = 0, src = sources; snum < num_sources; src++, snum++)
	{
		if (src->sfxinfo)
		{
			if (HW3DS.pfnIsPlaying(src->handle))
			{
				if (src->origin)
				{
					// initialize parameters
					volume = 255; // 8 bits internal volume precision
					pitch = NORM_PITCH;
					sep = NORM_SEP;

					// check non-local sounds for distance clipping
					//  or modify their params
					if (src->origin && listener != src->origin && !(listener2 && src->origin == listener2))
					{
						int audible2;
						int volume2 = volume, sep2 = sep, pitch2 = pitch;
						audible = S_AdjustSoundParams(listener, src->origin, &volume, &sep, &pitch,
							src->sfxinfo);

						if (listener2)
						{
							audible2 = S_AdjustSoundParams(listener2,
								src->origin, &volume2, &sep2, &pitch2, src->sfxinfo);
							if (audible2 && (!audible || (audible && volume2 > volume)))
							{
								audible = true;
								volume = volume2;
								sep = sep2;
								pitch = pitch2;
							}
						}

						if (audible)
							HW3S_Update3DSource(src); // Update positional sources
						else
							HW3S_KillSource(snum); //Kill it!
					}
				}
			}
			else
			{
				// Source allocated but stopped. Kill.
				HW3S_KillSource(snum);
			}
		}
	}
}

void HW3S_Shutdown(void)
{
	HW3DS.pfnShutdown();
}

void *HW3S_GetSfx(sfxinfo_t *sfx)
{
	sfx_data_t      sfx_data;

	if (sfx->lumpnum == LUMPERROR)
		sfx->lumpnum = S_GetSfxLumpNum (sfx);

	sfx_data.length = W_LumpLength(sfx->lumpnum);
	sfx_data.data = Z_Malloc(sfx_data.length, PU_SOUND, &sfx->data);
	W_ReadLump(sfx->lumpnum, sfx_data.data);
	sfx_data.priority = sfx->priority;

	sfx->length = HW3DS.pfnAddSfx(&sfx_data);

	Z_ChangeTag(sfx->data, PU_CACHE);

	return sfx_data.data;
}

void HW3S_FreeSfx(sfxinfo_t *sfx)
{
	if (sfx->length > 0)
		HW3DS.pfnKillSfx((u_int)sfx->length);
	sfx->length = 0;

	sfx->lumpnum = LUMPERROR;

	Z_Free(sfx->data);
	sfx->data = NULL;
}

#endif
