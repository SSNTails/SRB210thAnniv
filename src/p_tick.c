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
/// \brief Archiving: SaveGame I/O, Thinker, Ticker

#include "doomstat.h"
#include "g_game.h"
#include "p_local.h"
#include "z_zone.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "p_polyobj.h"

tic_t leveltime;

//
// THINKERS
// All thinkers should be allocated by Z_Calloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//

// Both the head and tail of the thinker list.
thinker_t thinkercap;

void Command_Numthinkers_f(void)
{
	int num;
	int count = 0;
	actionf_p1 action;
	thinker_t *think;

	if (COM_Argc() < 2)
	{
		CONS_Printf("numthinkers <#>: Count number of thinkers\n\t1: P_MobjThinker\n\t2: P_RainThinker\n\t3: P_SnowThinker\n\t4: P_NullPrecipThinker\n\t5: T_Friction\n\t6: T_Pusher\n\t7: P_RemoveThinkerDelayed\n");
		return;
	}

	num = atoi(COM_Argv(1));

	switch (num)
	{
		case 1:
			action = (actionf_p1)P_MobjThinker;
			CONS_Printf("Number of P_MobjThinker: ");
			break;
		case 2:
			action = (actionf_p1)P_RainThinker;
			CONS_Printf("Number of P_RainThinker: ");
			break;
		case 3:
			action = (actionf_p1)P_SnowThinker;
			CONS_Printf("Number of P_SnowThinker: ");
			break;
		case 4:
			action = (actionf_p1)P_NullPrecipThinker;
			CONS_Printf("Number of P_NullPrecipThinker: ");
			break;
		case 5:
			action = (actionf_p1)T_Friction;
			CONS_Printf("Number of T_Friction: ");
			break;
		case 6:
			action = (actionf_p1)T_Pusher;
			CONS_Printf("Number of T_Pusher: ");
			break;
		case 7:
			action = (actionf_p1)P_RemoveThinkerDelayed;
			CONS_Printf("Number of P_RemoveThinkerDelayed: ");
			break;
		default:
			CONS_Printf("That is not a valid number.\n");
			return;
	}

	for (think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if (think->function.acp1 != action)
			continue;

		count++;
	}

	CONS_Printf("%d\n", count);
}

void Command_CountMobjs_f(void)
{
	thinker_t *th;
	mobjtype_t i;
	int count;

	CONS_Printf("Count of active objects in level:\n");

	for (i = 0; i < NUMMOBJTYPES; i++)
	{
		count = 0;

		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			if (((mobj_t *)th)->type == i)
				count++;
		}

		CONS_Printf("%d: %d\n", i, count);
	}
	CONS_Printf("Done\n");
}

//
// P_InitThinkers
//
void P_InitThinkers(void)
{
	thinkercap.prev = thinkercap.next = &thinkercap;
}

//
// P_AddThinker
// Adds a new thinker at the end of the list.
//
void P_AddThinker(thinker_t *thinker)
{
	thinkercap.prev->next = thinker;
	thinker->next = &thinkercap;
	thinker->prev = thinkercap.prev;
	thinkercap.prev = thinker;

	thinker->references = 0;    // killough 11/98: init reference counter to 0
}

//
// killough 11/98:
//
// Make currentthinker external, so that P_RemoveThinkerDelayed
// can adjust currentthinker when thinkers self-remove.

static thinker_t *currentthinker;

//
// P_RemoveThinkerDelayed()
//
// Called automatically as part of the thinker loop in P_RunThinkers(),
// on nodes which are pending deletion.
//
// If this thinker has no more pointers referencing it indirectly,
// remove it, and set currentthinker to one node preceeding it, so
// that the next step in P_RunThinkers() will get its successor.
//
void P_RemoveThinkerDelayed(void *pthinker)
{
  thinker_t *thinker = pthinker;
  if (!thinker->references)
    {
      { /* Remove from main thinker list */
        thinker_t *next = thinker->next;
        /* Note that currentthinker is guaranteed to point to us,
         * and since we're freeing our memory, we had better change that. So
         * point it to thinker->prev, so the iterator will correctly move on to
         * thinker->prev->next = thinker->next */
        (next->prev = currentthinker = thinker->prev)->next = next;
      }
      Z_Free(thinker);
    }
}

//
// P_RemoveThinker
//
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//
// killough 4/25/98:
//
// Instead of marking the function with -1 value cast to a function pointer,
// set the function to P_RemoveThinkerDelayed(), so that later, it will be
// removed automatically as part of the thinker process.
//
void P_RemoveThinker(thinker_t *thinker)
{
  thinker->function.acp1 = P_RemoveThinkerDelayed;
}

/*
 * P_SetTarget
 *
 * This function is used to keep track of pointer references to mobj thinkers.
 * In Doom, objects such as lost souls could sometimes be removed despite
 * their still being referenced. In Boom, 'target' mobj fields were tested
 * during each gametic, and any objects pointed to by them would be prevented
 * from being removed. But this was incomplete, and was slow (every mobj was
 * checked during every gametic). Now, we keep a count of the number of
 * references, and delay removal until the count is 0.
 */

mobj_t *P_SetTarget(mobj_t **mop, mobj_t *targ)
{
  if (*mop)             // If there was a target already, decrease its refcount
    (*mop)->thinker.references--;
  if ((*mop = targ) != NULL)    // Set new target and if non-NULL, increase its counter
    targ->thinker.references++;
  return targ;
}

//
// P_RunThinkers
//
// killough 4/25/98:
//
// Fix deallocator to stop using "next" pointer after node has been freed
// (a Doom bug).
//
// Process each thinker. For thinkers which are marked deleted, we must
// load the "next" pointer prior to freeing the node. In Doom, the "next"
// pointer was loaded AFTER the thinker was freed, which could have caused
// crashes.
//
// But if we are not deleting the thinker, we should reload the "next"
// pointer after calling the function, in case additional thinkers are
// added at the end of the list.
//
// killough 11/98:
//
// Rewritten to delete nodes implicitly, by making currentthinker
// external and using P_RemoveThinkerDelayed() implicitly.
//
static inline void P_RunThinkers(void)
{
	for (currentthinker = thinkercap.next; currentthinker != &thinkercap; currentthinker = currentthinker->next)
	{
		if (currentthinker->function.acp1)
			currentthinker->function.acp1(currentthinker);
	}
}

//
// P_Ticker
//
void P_Ticker(void)
{
	int i;

	postimgtype = postimg_none;

	// Check for pause or menu up in single player
	if (paused || (!netgame && menuactive && !demoplayback))
	{
		objectsdrawn = 0;
		return;
	}

	P_MapStart();

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i])
			P_PlayerThink(&players[i]);

	// Keep track of how long they've been playing!
	totalplaytime++;

///////////////////////
//SPECIAL STAGE STUFF//
///////////////////////

	if (gamemap >= sstage_start && gamemap <= sstage_end)
	{
		boolean inwater = false;

		// Can't drown in a special stage
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			players[i].powers[pw_underwater] = players[i].powers[pw_spacetime] = 0;
		}

		if (sstimer < 15*TICRATE+6 && sstimer > 7 && mapheaderinfo[gamemap-1].speedmusic)
			S_SpeedMusic(1.4f);

		if (sstimer < 7 && sstimer > 0) // The special stage time is up!
		{
			sstimer = 0;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i])
				{
					players[i].exiting = (14*TICRATE)/5 + 1;
					players[i].pflags &= ~PF_GLIDING;
				}

				if (i == consoleplayer)
					S_StartSound(NULL, sfx_lose);
			}

			if (mapheaderinfo[gamemap-1].speedmusic)
				S_SpeedMusic(1.0f);
		}

		if (sstimer > 1) // As long as time isn't up...
		{
			ULONG ssrings = 0;
			// Count up the rings of all the players and see if
			// they've collected the required amount.
			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i])
				{
					ssrings += (players[i].mo->health-1);

					// If in water, deplete timer 6x as fast.
					if ((players[i].mo->eflags & MF_TOUCHWATER)
						|| (players[i].mo->eflags & MF_UNDERWATER))
						inwater = true;
				}

			if (ssrings >= totalrings && totalrings > 0)
			{
				S_StartSound(NULL, sfx_cgot); // Got the emerald!

				// Halt all the players
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i])
					{
						players[i].mo->momx = players[i].mo->momy = 0;
						players[i].exiting = (14*TICRATE)/5 + 1;
					}

				sstimer = 0;

				// Check what emeralds the player has so you know which one to award next.
				if (!(emeralds & EMERALD1))
				{
					emeralds |= EMERALD1;
					for (i = 0; i < MAXPLAYERS; i++)
						if (playeringame[i])
							P_SetMobjState(P_SpawnMobj(players[i].mo->x, players[i].mo->y, players[i].mo->z + players[i].mo->info->height, MT_GOTEMERALD), mobjinfo[MT_GOTEMERALD].spawnstate);
				}
				else if ((emeralds & EMERALD1) && !(emeralds & EMERALD2))
				{
					emeralds |= EMERALD2;
					for (i = 0; i < MAXPLAYERS; i++)
						if (playeringame[i])
							P_SetMobjState(P_SpawnMobj(players[i].mo->x, players[i].mo->y, players[i].mo->z + players[i].mo->info->height, MT_GOTEMERALD), mobjinfo[MT_GOTEMERALD].spawnstate+1);
				}
				else if ((emeralds & EMERALD2) && !(emeralds & EMERALD3))
				{
					emeralds |= EMERALD3;
					for (i = 0; i < MAXPLAYERS; i++)
						if (playeringame[i])
							P_SetMobjState(P_SpawnMobj(players[i].mo->x, players[i].mo->y, players[i].mo->z + players[i].mo->info->height, MT_GOTEMERALD), mobjinfo[MT_GOTEMERALD].spawnstate+2);
				}
				else if ((emeralds & EMERALD3) && !(emeralds & EMERALD4))
				{
					emeralds |= EMERALD4;
					for (i = 0; i < MAXPLAYERS; i++)
						if (playeringame[i])
							P_SetMobjState(P_SpawnMobj(players[i].mo->x, players[i].mo->y, players[i].mo->z + players[i].mo->info->height, MT_GOTEMERALD), mobjinfo[MT_GOTEMERALD].spawnstate+3);
				}
				else if ((emeralds & EMERALD4) && !(emeralds & EMERALD5))
				{
					emeralds |= EMERALD5;
					for (i = 0; i < MAXPLAYERS; i++)
						if (playeringame[i])
							P_SetMobjState(P_SpawnMobj(players[i].mo->x, players[i].mo->y, players[i].mo->z + players[i].mo->info->height, MT_GOTEMERALD), mobjinfo[MT_GOTEMERALD].spawnstate+4);
				}
				else if ((emeralds & EMERALD5) && !(emeralds & EMERALD6))
				{
					emeralds |= EMERALD6;
					for (i = 0; i < MAXPLAYERS; i++)
						if (playeringame[i])
							P_SetMobjState(P_SpawnMobj(players[i].mo->x, players[i].mo->y, players[i].mo->z + players[i].mo->info->height, MT_GOTEMERALD), mobjinfo[MT_GOTEMERALD].spawnstate+5);
				}
				else if ((emeralds & EMERALD6) && !(emeralds & EMERALD7))
				{
					emeralds |= EMERALD7;
					for (i = 0; i < MAXPLAYERS; i++)
						if (playeringame[i])
							P_SetMobjState(P_SpawnMobj(players[i].mo->x, players[i].mo->y, players[i].mo->z + players[i].mo->info->height, MT_GOTEMERALD), mobjinfo[MT_GOTEMERALD].spawnstate+6);
				}
			}

			// Decrement the timer
			if (inwater)
				sstimer -= 6;
			else
				sstimer--;
		}
	}
/////////////////////////////////////////

	if (runemeraldmanager)
		P_EmeraldManager(); // Power stone mode

	P_RunThinkers();

	// Run any "after all the other thinkers" stuff
	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i])
			P_PlayerAfterThink(&players[i]);

	P_UpdateSpecials();
	P_RespawnSpecials();

	if (cv_objectplace.value)
	{
		objectsdrawn = 0;
		P_MapEnd();
		return;
	}

	leveltime++;
	timeinmap++;

	if (countdowntimer)
	{
		countdowntimer--;
		if (countdowntimer <= 0)
		{
			countdowntimer = 0;
			countdowntimeup = true;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;

				if (!players[i].mo)
					continue;

				P_DamageMobj(players[i].mo, NULL, NULL, 10000);
			}
		}
	}

	if (countdown > 1)
		countdown--;

	if (countdown2)
		countdown2--;

	P_MapEnd();

//	Z_CheckMemCleanup();
}
