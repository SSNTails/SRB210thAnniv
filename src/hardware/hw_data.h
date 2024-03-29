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
//
//-----------------------------------------------------------------------------
/// \file
/// \brief defines structures and exports for the standard 3D driver DLL used by Doom Legacy

#ifndef _HWR_DATA_
#define _HWR_DATA_

#if (defined (_WIN32) || defined (_WIN64)) && !defined (__CYGWIN__) && !defined (_XBOX)
//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif

#if defined (VID_X11) && !defined (SDL)
#include <GL/glx.h>
#endif

#include "../doomdef.h"
//THIS MUST DISAPPEAR!!!
#include "hw_glide.h"
#include "../screen.h"


// ==========================================================================
//                                                               TEXTURE INFO
// ==========================================================================

// grInfo.data holds the address of the graphics data cached in heap memory
//                NULL if the texture is not in Doom heap cache.
struct GlideMipmap_s
{
	GrTexInfo       grInfo;         //for TexDownloadMipMap
	FxU32           flags;
	unsigned short  height;
	unsigned short  width;
	unsigned int    downloaded;     // the dll driver have it in there cache ?

	struct GlideMipmap_s    *nextcolormap;
	const byte              *colormap;

	// opengl/glide
	struct GlideMipmap_s *nextmipmap;// glide  : the FIFO list of texture in the memory
	                                 //          _DO NOT TUCH IT_
	                                 // opengl : liste of all texture in opengl driver
	// glide only
	FxU32           cachepos;        //offset in hardware cache
	FxU32           mipmapSize;      //size of mipmap
};
typedef struct GlideMipmap_s GlideMipmap_t;


//
// Doom texture info, as cached for hardware rendering
//
struct GlideTexture_s
{
	GlideMipmap_t mipmap;
	float       scaleX;             //used for scaling textures on walls
	float       scaleY;
};
typedef struct GlideTexture_s GlideTexture_t;


// a cached patch as converted to hardware format, holding the original patch_t
// header so that the existing code can retrieve ->width, ->height as usual
// This is returned by W_CachePatchNum()/W_CachePatchName(), when rendermode
// is 'render_glide'. Else it returns the normal patch_t data.

struct GlidePatch_s
{
	// the 4 first fields come right away from the original patch_t
	short               width;
	short               height;
	short               leftoffset;     // pixels to the left of origin
	short               topoffset;      // pixels below the origin
	//
	float               max_s,max_t;
	lumpnum_t           patchlump;      // the software patch lump num for when the hardware patch
	                                   // was flushed, and we need to re-create it
	GlideMipmap_t       mipmap;
};
typedef struct GlidePatch_s GlidePatch_t;

#endif //_HWR_DATA_
