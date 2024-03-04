// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2006 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//----------------------------------------------------------------------------
//
// Polyobjects
//
// Movable segs like in Hexen, but more flexible due to application of
// dynamic binary space partitioning theory.
//
// haleyjd 02/16/06
//
//----------------------------------------------------------------------------

#ifndef POLYOBJ_H__
#define POLYOBJ_H__

#include "m_dllist.h"
#include "p_mobj.h"
#include "r_defs.h"

// haleyjd: temporary define
#ifdef POLYOBJECTS
//
// Defines
//

// haleyjd: use zdoom-compatible doomednums

#define POLYOBJ_ANCHOR_DOOMEDNUM     760
#define POLYOBJ_SPAWN_DOOMEDNUM      761
#define POLYOBJ_SPAWNCRUSH_DOOMEDNUM 762

#define POLYOBJ_START_LINE    20
#define POLYOBJ_EXPLICIT_LINE 21
#define POLYINFO_SPECIALNUM   22

typedef enum
{
	POF_CLIPLINES         = 0x1,       ///< Test against lines for collision
	POF_CLIPPLANES        = 0x2,       ///< Test against tops and bottoms for collision
	POF_SOLID             = 0x3,       ///< Clips things.
	POF_TESTHEIGHT        = 0x4,       ///< Test line collision with heights
	POF_RENDERSIDES       = 0x8,       ///< Renders the sides.
	POF_RENDERTOP         = 0x10,      ///< Renders the top..
	POF_RENDERBOTTOM      = 0x20,      ///< Renders the bottom.
	POF_RENDERPLANES      = 0x30,      ///< Renders top and bottom.
	POF_RENDERALL         = 0x38,      ///< Renders everything.
	POF_INVERT            = 0x40,      ///< Inverts collision (like a cage).
	POF_INVERTPLANES      = 0x80,      ///< Render inside planes.
	POF_INVERTPLANESONLY  = 0x100,     ///< Only render inside planes.
} polyobjflags_e;

//
// Polyobject Structure
//

typedef struct polyobj_s
{
	mdllistitem_t link; // for subsector links; must be first

	int id;    // numeric id
	int first; // for hashing: index of first polyobject in this hash chain
	int next;  // for hashing: next polyobject in this hash chain

	int mirror; // numeric id of a mirroring polyobject

	size_t segCount;        // number of segs in polyobject
	size_t numSegsAlloc;    // number of segs allocated
	struct seg_s **segs; // the segs, a reallocating array.

	size_t numVertices;            // number of vertices (generally == segCount)
	size_t numVerticesAlloc;       // number of vertices allocated
	vertex_t *origVerts; // original positions relative to spawn spot
	vertex_t *tmpVerts;  // temporary vertex backups for rotation
	vertex_t **vertices; // vertices this polyobject must move

	size_t numLines;          // number of linedefs (generally <= segCount)
	size_t numLinesAlloc;     // number of linedefs allocated
	struct line_s **lines; // linedefs this polyobject must move

	degenmobj_t spawnSpot; // location of spawn spot
	vertex_t    centerPt;  // center point
	fixed_t zdist;                // viewz distance for sorting
	angle_t angle;                // for rotation
	boolean attached;             // if true, is attached to a subsector

	fixed_t blockbox[4]; // bounding box for clipping
	boolean linked;      // is linked to blockmap
	size_t validcount;      // for clipping: prevents multiple checks
	int damage;          // damage to inflict on stuck things
	fixed_t thrust;      // amount of thrust to put on blocking objects
	int flags;           // Flags for this polyobject

	thinker_t *thinker;  // pointer to a thinker affecting this polyobj

	boolean isBad; // a bad polyobject: should not be rendered/manipulated
} polyobj_t;

//
// Polyobject Blockmap Link Structure
//

typedef struct polymaplink_s
{
	mdllistitem_t link; // for blockmap links
	polyobj_t *po;      // pointer to polyobject
} polymaplink_t;

//
// Polyobject Special Thinkers
//

typedef struct polyrotate_s
{
	thinker_t thinker; // must be first

	int polyObjNum;    // numeric id of polyobject (avoid C pointers here)
	int speed;         // speed of movement per frame
	int distance;      // distance to move
} polyrotate_t;

typedef struct polymove_s
{
	thinker_t thinker;  // must be first

	int polyObjNum;     // numeric id of polyobject
	int speed;          // resultant velocity
	fixed_t momx;       // x component of speed along angle
	fixed_t momy;       // y component of speed along angle
	int distance;       // total distance to move
	unsigned int angle; // angle along which to move
} polymove_t;

typedef struct polyslidedoor_s
{
	thinker_t thinker;      // must be first

	int polyObjNum;         // numeric id of affected polyobject
	int delay;              // delay time
	int delayCount;         // delay counter
	int initSpeed;          // initial speed
	int speed;              // speed of motion
	int initDistance;       // initial distance to travel
	int distance;           // current distance to travel
	unsigned int initAngle; // intial angle
	unsigned int angle;     // angle of motion
	unsigned int revAngle;  // reversed angle to avoid roundoff error
	fixed_t momx;           // x component of speed along angle
	fixed_t momy;           // y component of speed along angle
	boolean closing;        // if true, is closing
} polyslidedoor_t;

typedef struct polyswingdoor_s
{
	thinker_t thinker; // must be first

	int polyObjNum;    // numeric id of affected polyobject
	int delay;         // delay time
	int delayCount;    // delay counter
	int initSpeed;     // initial speed
	int speed;         // speed of rotation
	int initDistance;  // initial distance to travel
	int distance;      // current distance to travel
	boolean closing;   // if true, is closing
} polyswingdoor_t;

//
// Line Activation Data Structures
//

typedef struct polyrotdata_s
{
	int polyObjNum;   // numeric id of polyobject to affect
	int direction;    // direction of rotation
	int speed;        // angular speed
	int distance;     // distance to move
	boolean overRide; // if true, will override any action on the object
} polyrotdata_t;

typedef struct polymovedata_s
{
	int polyObjNum;     // numeric id of polyobject to affect
	fixed_t distance;   // distance to move
	fixed_t speed;      // linear speed
	angle_t angle;      // angle of movement
	boolean overRide;   // if true, will override any action on the object
} polymovedata_t;

// polyobject door types
typedef enum
{
	POLY_DOOR_SLIDE,
	POLY_DOOR_SWING,
} polydoor_e;

typedef struct polydoordata_s
{
	int polyObjNum;     // numeric id of polyobject to affect
	int doorType;       // polyobj door type
	int speed;          // linear or angular speed
	angle_t angle;      // for slide door only, angle of motion
	int distance;       // distance to move
	int delay;          // delay time after opening
} polydoordata_t;

//
// Functions
//

polyobj_t *Polyobj_GetForNum(int id);
void Polyobj_InitLevel(void);
void Polyobj_MoveOnLoad(polyobj_t *po, angle_t angle, fixed_t x, fixed_t y);
boolean P_PointInsidePolyobj(polyobj_t *po, fixed_t x, fixed_t y);
void Polyobj_GetInfo(short tag, int *polyID, int *mirrorID, unsigned short *exparg);

// thinkers (needed in p_saveg.c)
void T_PolyObjRotate(polyrotate_t *);
void T_PolyObjMove  (polymove_t *);
void T_PolyDoorSlide(polyslidedoor_t *);
void T_PolyDoorSwing(polyswingdoor_t *);
void T_PolyObjFlag  (polymove_t *);

int EV_DoPolyDoor(polydoordata_t *);
int EV_DoPolyObjMove(polymovedata_t *);
int EV_DoPolyObjRotate(polyrotdata_t *);
int EV_DoPolyObjFlag(struct line_s *);


//
// External Variables
//

extern polyobj_t *PolyObjects;
extern int numPolyObjects;
extern polymaplink_t **polyblocklinks; // polyobject blockmap

#endif // ifdef POLYOBJECTS

#endif

// EOF
