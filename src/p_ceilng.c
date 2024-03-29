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
/// \brief Ceiling aninmation (lowering, crushing, raising)

#include "doomdef.h"
#include "p_local.h"
#include "r_main.h"
#include "s_sound.h"
#include "z_zone.h"
#include "d_netcmd.h"

// ==========================================================================
//                              CEILINGS
// ==========================================================================

// the list of ceilings moving currently, including crushers
int ceilmovesound = sfx_stnmov;

/** Moves a moving ceiling.
  *
  * \param ceiling Thinker for the ceiling to be moved.
  * \sa EV_DoCeiling
  * \todo Split up into multiple functions.
  */
void T_MoveCeiling(ceiling_t *ceiling)
{
	result_e res;
	boolean dontupdate = false;

	if (cv_objectplace.value)
		return;

	if (ceiling->delaytimer)
	{
		ceiling->delaytimer--;
		return;
	}

	switch (ceiling->direction)
	{
		case 0: // IN STASIS
			break;
		case 1: // UP
			res = T_MovePlane(ceiling->sector, ceiling->speed, ceiling->topheight, false,
				1, ceiling->direction);

			if (ceiling->type == bounceCeiling)
			{
				const fixed_t origspeed = FixedDiv(ceiling->origspeed,(ELEVATORSPEED/2));
				const fixed_t fs = abs(ceiling->sector->ceilingheight - lines[ceiling->texture].frontsector->ceilingheight);
				const fixed_t bs = abs(ceiling->sector->ceilingheight - lines[ceiling->texture].backsector->ceilingheight);
				if (fs < bs)
					ceiling->speed = FixedDiv(fs,25*FRACUNIT) + FRACUNIT/4;
				else
					ceiling->speed = FixedDiv(bs,25*FRACUNIT) + FRACUNIT/4;

				ceiling->speed = FixedMul(ceiling->speed,origspeed);
			}

			if (res == pastdest)
			{
				switch (ceiling->type)
	            {
					case instantMoveCeilingByFrontSector:
						ceiling->sector->ceilingpic = ceiling->texture;
						ceiling->sector->ceilingdata = NULL;
						ceiling->sector->ceilspeed = 0;
						P_RemoveThinker(&ceiling->thinker);
						dontupdate = true;
						break;
					case moveCeilingByFrontSector:
						if (ceiling->texture < -1) // chained linedef executing
							P_LinedefExecute(ceiling->texture + MAXSHORT + 2, NULL, NULL);
						if (ceiling->texture > -1) // flat changing
							ceiling->sector->ceilingpic = ceiling->texture;
						// don't break
					case raiseToHighest:
					case raiseCeilingByLine:
						ceiling->sector->ceilingdata = NULL;
						ceiling->sector->ceilspeed = 0;
						P_RemoveThinker(&ceiling->thinker);
						dontupdate = true;
						break;

					case fastCrushAndRaise:
					case crushAndRaise:
						ceiling->direction = -1;
						break;

					case bounceCeiling:
					{
						fixed_t dest = ceiling->topheight;

						if (dest == lines[ceiling->texture].frontsector->ceilingheight)
							dest = lines[ceiling->texture].backsector->ceilingheight;
						else
							dest = lines[ceiling->texture].frontsector->ceilingheight;

						if (dest < ceiling->sector->ceilingheight) // must move down
						{
							ceiling->direction = -1;
							ceiling->bottomheight = dest;
						}
						else // must move up
						{
							ceiling->direction = 1;
							ceiling->topheight = dest;
						}

						ceiling->delaytimer = ceiling->delay;

						// That's it. Do not set dontupdate, do not remove the thinker.
						break;
					}

					case bounceCeilingCrush:
					{
						fixed_t dest = ceiling->topheight;

						if (dest == lines[ceiling->texture].frontsector->ceilingheight)
						{
							dest = lines[ceiling->texture].backsector->ceilingheight;
							ceiling->speed = ceiling->origspeed = FixedDiv(abs(lines[ceiling->texture].dy),4*FRACUNIT); // return trip, use dy
						}
						else
						{
							dest = lines[ceiling->texture].frontsector->ceilingheight;
							ceiling->speed = ceiling->origspeed = FixedDiv(abs(lines[ceiling->texture].dx),4*FRACUNIT); // going frontways, use dx
						}

						if (dest < ceiling->sector->ceilingheight) // must move down
						{
							ceiling->direction = -1;
							ceiling->bottomheight = dest;
						}
						else // must move up
						{
							ceiling->direction = 1;
							ceiling->topheight = dest;
						}

						ceiling->delaytimer = ceiling->delay;

						// That's it. Do not set dontupdate, do not remove the thinker.
						break;
					}

					default:
						break;
				}
			}
			break;

		case -1: // DOWN
			res = T_MovePlane(ceiling->sector, ceiling->speed, ceiling->bottomheight,
				ceiling->crush, 1, ceiling->direction);

			if (ceiling->type == bounceCeiling)
			{
				const fixed_t origspeed = FixedDiv(ceiling->origspeed,(ELEVATORSPEED/2));
				const fixed_t fs = abs(ceiling->sector->ceilingheight - lines[ceiling->texture].frontsector->ceilingheight);
				const fixed_t bs = abs(ceiling->sector->ceilingheight - lines[ceiling->texture].backsector->ceilingheight);
				if (fs < bs)
					ceiling->speed = FixedDiv(fs,25*FRACUNIT) + FRACUNIT/4;
				else
					ceiling->speed = FixedDiv(bs,25*FRACUNIT) + FRACUNIT/4;
				ceiling->speed = FixedMul(ceiling->speed,origspeed);
			}

			if (res == pastdest)
			{
				switch (ceiling->type)
				{
					// make platform stop at bottom of all crusher strokes
					// except generalized ones, reset speed, start back up
					case crushAndRaise:
						ceiling->speed = CEILSPEED;
					case fastCrushAndRaise:
						ceiling->direction = 1;
						break;

					case instantMoveCeilingByFrontSector:
						ceiling->sector->ceilingpic = ceiling->texture;
						ceiling->sector->ceilingdata = NULL;
						ceiling->sector->ceilspeed = 0;
						P_RemoveThinker(&ceiling->thinker);
						dontupdate = true;
						break;

					case moveCeilingByFrontSector:
						if (ceiling->texture < -1) // chained linedef executing
							P_LinedefExecute(ceiling->texture + MAXSHORT + 2, NULL, NULL);
						if (ceiling->texture > -1) // flat changing
							ceiling->sector->ceilingpic = ceiling->texture;
						// don't break

					// in all other cases, just remove the active ceiling
					case lowerAndCrush:
					case lowerToLowest:
					case raiseToLowest:
					case lowerCeilingByLine:
						ceiling->sector->ceilingdata = NULL;
						ceiling->sector->ceilspeed = 0;
						P_RemoveThinker(&ceiling->thinker);
						dontupdate = true;
						break;
					case bounceCeiling:
					{
						fixed_t dest = ceiling->bottomheight;

						if (dest == lines[ceiling->texture].frontsector->ceilingheight)
							dest = lines[ceiling->texture].backsector->ceilingheight;
						else
							dest = lines[ceiling->texture].frontsector->ceilingheight;

						if (dest < ceiling->sector->ceilingheight) // must move down
						{
							ceiling->direction = -1;
							ceiling->bottomheight = dest;
						}
						else // must move up
						{
							ceiling->direction = 1;
							ceiling->topheight = dest;
						}

						ceiling->delaytimer = ceiling->delay;

						// That's it. Do not set dontupdate, do not remove the thinker.
						break;
					}

					case bounceCeilingCrush:
					{
						fixed_t dest = ceiling->bottomheight;

						if (dest == lines[ceiling->texture].frontsector->ceilingheight)
						{
							dest = lines[ceiling->texture].backsector->ceilingheight;
							ceiling->speed = ceiling->origspeed = FixedDiv(abs(lines[ceiling->texture].dy),4*FRACUNIT); // return trip, use dy
						}
						else
						{
							dest = lines[ceiling->texture].frontsector->ceilingheight;
							ceiling->speed = ceiling->origspeed = FixedDiv(abs(lines[ceiling->texture].dx),4*FRACUNIT); // going frontways, use dx
						}

						if (dest < ceiling->sector->ceilingheight) // must move down
						{
							ceiling->direction = -1;
							ceiling->bottomheight = dest;
						}
						else // must move up
						{
							ceiling->direction = 1;
							ceiling->topheight = dest;
						}

						ceiling->delaytimer = ceiling->delay;

						// That's it. Do not set dontupdate, do not remove the thinker.
						break;
					}

					default:
						break;
				}
			}
			else if (res == crushed)
			{
				switch (ceiling->type)
				{
					case crushAndRaise:
					case lowerAndCrush:
						ceiling->speed = FixedDiv(CEILSPEED,8*FRACUNIT);
						break;

					default:
						break;
				}
			}
		break;
	}
	if (!dontupdate)
		ceiling->sector->ceilspeed = ceiling->speed*ceiling->direction;
	else
		ceiling->sector->ceilspeed = 0;
}

/** Moves a ceiling crusher.
  *
  * \param ceiling Thinker for the crusher to be moved.
  * \sa EV_DoCrush
  */
void T_CrushCeiling(ceiling_t *ceiling)
{
	result_e res;

	if (cv_objectplace.value)
		return;

	switch (ceiling->direction)
	{
		case 0: // IN STASIS
			break;
		case 1: // UP
			if (ceiling->type == crushBothOnce)
			{
				// Move the floor
				T_MovePlane(ceiling->sector, ceiling->speed, ceiling->bottomheight-(ceiling->topheight-ceiling->bottomheight), false, 0, -ceiling->direction);
			}

			res = T_MovePlane(ceiling->sector, ceiling->speed, ceiling->topheight,
				false, 1, ceiling->direction);

			if (res == pastdest)
			{
				ceiling->direction = -1;
				ceiling->speed = ceiling->oldspeed*2;

				if (ceiling->type == crushCeilOnce
					|| ceiling->type == crushBothOnce)
				{
					// Remove
					switch(ceiling->type)
					{
						case crushCeilOnce:
							ceiling->sector->ceilspeed = 0;
							ceiling->sector->ceilingdata = NULL;
							break;
						case crushBothOnce:
							ceiling->sector->floorspeed = 0;
							ceiling->sector->ceilspeed = 0;
							ceiling->sector->ceilingdata = NULL;
							break;
						default:
							break;
					}
					P_RemoveThinker(&ceiling->thinker);
					return;
				}
			}
			break;

		case -1: // DOWN
			if (ceiling->type == crushBothOnce)
			{
				// Move the floor
				T_MovePlane(ceiling->sector, ceiling->speed, ceiling->bottomheight, ceiling->crush, 0, -ceiling->direction);
			}

			res = T_MovePlane(ceiling->sector, ceiling->speed, ceiling->bottomheight,
				ceiling->crush, 1, ceiling->direction);

			if (res == pastdest)
			{
				mobj_t *mp = (void *)&ceiling->sector->soundorg;
				ceiling->sector->soundorg.z = ceiling->sector->floorheight;
				S_StartSound(mp,sfx_pstop);
				ceiling->speed = ceiling->oldspeed/2;
				ceiling->direction = 1;
			}
			break;
	}

	if (ceiling->type == crushBothOnce)
		ceiling->sector->floorspeed = ceiling->speed*(-ceiling->direction);

	ceiling->sector->ceilspeed = ceiling->speed*ceiling->direction;
}

/** Starts a ceiling mover.
  *
  * \param line The source line.
  * \param type The type of ceiling movement.
  * \return 1 if at least one ceiling mover was started, 0 otherwise.
  * \sa EV_DoCrush, EV_DoFloor, EV_DoElevator, T_MoveCeiling
  */
int EV_DoCeiling(line_t *line, ceiling_e type)
{
	int rtn = 0, firstone = 1;
	long secnum = -1;
	sector_t *sec;
	ceiling_t *ceiling;

	if (cv_objectplace.value)
		return false;

	while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
	{
		sec = &sectors[secnum];

		if (sec->ceilingdata)
			continue;

		// new door thinker
		rtn = 1;
		ceiling = Z_Calloc(sizeof (*ceiling), PU_LEVSPEC, NULL);
		P_AddThinker (&ceiling->thinker);
		sec->ceilingdata = ceiling;
		ceiling->thinker.function.acp1 = (actionf_p1)T_MoveCeiling;
		ceiling->sector = sec;
		ceiling->crush = false;

		switch (type)
		{
			case fastCrushAndRaise:
				ceiling->crush = true;
				ceiling->topheight = sec->ceilingheight;
				ceiling->bottomheight = sec->floorheight + (8*FRACUNIT);
				ceiling->direction = -1;
				ceiling->speed = CEILSPEED * 2;
				break;

			case crushAndRaise:
				ceiling->crush = true;
				ceiling->topheight = sec->ceilingheight;
			case lowerAndCrush:
				ceiling->bottomheight = sec->floorheight;
				ceiling->bottomheight += 4*FRACUNIT;
				ceiling->direction = -1;
				ceiling->speed = line->dx;
				break;

			case raiseToHighest:
				ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
				ceiling->direction = 1;
				ceiling->speed = CEILSPEED;
				break;

			//SoM: 3/6/2000: Added Boom types
			case lowerToLowest:
				ceiling->bottomheight = P_FindLowestCeilingSurrounding(sec);
				ceiling->direction = -1;
				ceiling->speed = CEILSPEED;
				break;

			case raiseToLowest: // Graue 09-07-2004
				ceiling->topheight = P_FindLowestCeilingSurrounding(sec) - 4*FRACUNIT;
				ceiling->direction = 1;
				ceiling->speed = line->dx; // hack
				break;

			case lowerToLowestFast:
				ceiling->bottomheight = P_FindLowestCeilingSurrounding(sec);
				ceiling->direction = -1;
				ceiling->speed = FixedDiv(4*FRACUNIT,NEWTICRATERATIO*FRACUNIT);
				break;

			case instantRaise:
				ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
				ceiling->direction = 1;
				ceiling->speed = MAXINT/2;
				break;

			//  Linedef executor excellence
			case moveCeilingByFrontSector:
				ceiling->speed = P_AproxDistance(line->dx, line->dy);
				ceiling->speed = FixedDiv(ceiling->speed,8*FRACUNIT);
				if (line->frontsector->ceilingheight >= sec->ceilingheight) // Move up
				{
					ceiling->direction = 1;
					ceiling->topheight = line->frontsector->ceilingheight;
				}
				else // Move down
				{
					ceiling->direction = -1;
					ceiling->bottomheight = line->frontsector->ceilingheight;
				}

				// chained linedef executing ability
				if (line->flags & ML_BLOCKMONSTERS)
				{
					// only set it on ONE of the moving sectors (the smallest numbered)
					// and front side x offset must be positive
					if (firstone && sides[line->sidenum[0]].textureoffset > 0)
						ceiling->texture = (sides[line->sidenum[0]].textureoffset>>FRACBITS) - 32769;
					else
						ceiling->texture = -1;
				}

				// flat changing ability
				else if (line->flags & ML_NOCLIMB)
					ceiling->texture = line->frontsector->ceilingpic;
				else
					ceiling->texture = -1;
				break;

			// More linedef executor junk
			case instantMoveCeilingByFrontSector:
				ceiling->speed = MAXINT/2;

				if (line->frontsector->ceilingheight >= sec->ceilingheight) // Move up
				{
					ceiling->direction = 1;
					ceiling->topheight = line->frontsector->ceilingheight;
				}
				else // Move down
				{
					ceiling->direction = -1;
					ceiling->bottomheight = line->frontsector->ceilingheight;
				}
				ceiling->texture = line->frontsector->ceilingpic;
				break;

			case lowerCeilingByLine:
				ceiling->speed = FixedDiv(abs(line->dx),8*FRACUNIT);
				ceiling->direction = -1; // Move down
				ceiling->bottomheight = sec->ceilingheight - abs(line->dy);
				break;

			case raiseCeilingByLine:
				ceiling->speed = FixedDiv(abs(line->dx),8*FRACUNIT);
				ceiling->direction = 1; // Move up
				ceiling->topheight = sec->ceilingheight + abs(line->dy);
				break;

			case bounceCeiling:
				ceiling->speed = P_AproxDistance(line->dx, line->dy); // same speed as elevateContinuous
				ceiling->speed = FixedDiv(ceiling->speed,NEWTICRATERATIO*4*FRACUNIT);
				ceiling->origspeed = ceiling->speed;
				if (line->frontsector->ceilingheight >= sec->ceilingheight) // Move up
				{
					ceiling->direction = 1;
					ceiling->topheight = line->frontsector->ceilingheight;
				}
				else // Move down
				{
					ceiling->direction = -1;
					ceiling->bottomheight = line->frontsector->ceilingheight;
				}

				// Any delay?
				ceiling->delay = sides[line->sidenum[0]].textureoffset >> FRACBITS;
				ceiling->delaytimer = sides[line->sidenum[0]].rowoffset >> FRACBITS; // Initial delay

				ceiling->texture = (fixed_t)(line - lines); // hack: use texture to store sourceline number
				break;

			case bounceCeilingCrush:
				ceiling->speed = abs(line->dx); // same speed as elevateContinuous
				ceiling->speed = FixedDiv(ceiling->speed,NEWTICRATERATIO*4*FRACUNIT);
				ceiling->origspeed = ceiling->speed;
				if (line->frontsector->ceilingheight >= sec->ceilingheight) // Move up
				{
					ceiling->direction = 1;
					ceiling->topheight = line->frontsector->ceilingheight;
				}
				else // Move down
				{
					ceiling->direction = -1;
					ceiling->bottomheight = line->frontsector->ceilingheight;
				}

				// Any delay?
				ceiling->delay = sides[line->sidenum[0]].textureoffset >> FRACBITS;
				ceiling->delaytimer = sides[line->sidenum[0]].rowoffset >> FRACBITS; // Initial delay

				ceiling->texture = (fixed_t)(line - lines); // hack: use texture to store sourceline number
				break;

			default:
				break;

		}

		ceiling->tag = sec->tag;
		ceiling->type = type;
		firstone = 0;
	}
	return rtn;
}

/** Starts a ceiling crusher.
  *
  * \param line The source line.
  * \param type The type of ceiling, either ::crushAndRaise or
  *             ::fastCrushAndRaise.
  * \return 1 if at least one crusher was started, 0 otherwise.
  * \sa EV_DoCeiling, EV_DoFloor, EV_DoElevator, T_CrushCeiling
  */
int EV_DoCrush(line_t *line, ceiling_e type)
{
	int rtn = 0;
	long secnum = -1;
	sector_t *sec;
	ceiling_t *ceiling;

	while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
	{
		sec = &sectors[secnum];

		if (sec->ceilingdata)
			continue;

		// new door thinker
		rtn = 1;
		ceiling = Z_Calloc(sizeof (*ceiling), PU_LEVSPEC, NULL);
		P_AddThinker (&ceiling->thinker);
		sec->ceilingdata = ceiling;
		ceiling->thinker.function.acp1 = (actionf_p1)T_CrushCeiling;
		ceiling->sector = sec;
		ceiling->crush = true;
		ceiling->oldspeed = (R_PointToDist2(line->v2->x, line->v2->y, line->v1->x, line->v1->y)/16)/NEWTICRATERATIO;

		switch(type)
		{
			case fastCrushAndRaise: // Up and then down
				ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
				ceiling->direction = 1;
				ceiling->speed = ceiling->oldspeed;
				ceiling->bottomheight = sec->floorheight + FRACUNIT;
				break;
			case crushBothOnce:
				ceiling->topheight = sec->ceilingheight;
				ceiling->bottomheight = sec->floorheight + (sec->ceilingheight-sec->floorheight)/2;
				ceiling->direction = -1;
				ceiling->speed = ceiling->oldspeed*2;
				break;
			case crushCeilOnce:
			default: // Down and then up.
				ceiling->topheight = sec->ceilingheight;
				ceiling->direction = -1;
				ceiling->speed = ceiling->oldspeed*2;
				ceiling->bottomheight = sec->floorheight + FRACUNIT;
				break;
		}

		ceiling->tag = sec->tag;
		ceiling->type = type;
	}
	return rtn;
}
