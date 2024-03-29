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
/// \brief WAD I/O functions, wad resource definitions (some)

#ifndef __W_WAD__
#define __W_WAD__

#ifdef HWRENDER
#include "hardware/hw_data.h"
#else
typedef void GlidePatch_t;
#endif

#ifdef __GNUG__
#pragma interface
#endif

// ==============================================================
//               WAD FILE STRUCTURE DEFINITIONS
// ==============================================================

// header of a wad file
typedef struct
{
	char identification[4]; // should be "IWAD" or "PWAD"
	ULONG numlumps; // how many resources
	ULONG infotableofs; // the 'directory' of resources
} wadinfo_t;

//  a memory entry of the wad directory
typedef struct
{
	ULONG position; // filelump_t filepos
	ULONG disksize; // filelump_t size
	char name[9]; // filelump_t name[]
	size_t size; // real (uncompressed) size
	int compressed; // i
} lumpinfo_t;

// =========================================================================
//                         DYNAMIC WAD LOADING
// =========================================================================

#define MAX_WADPATH 128
#define MAX_WADFILES 48 // maximum of wad files used at the same time
// (there is a max of simultaneous open files anyway, and this should be plenty)

#define lumpcache_t void *

typedef struct wadfile_s
{
	char *filename;
	lumpinfo_t *lumpinfo;
	lumpcache_t *lumpcache;
#ifdef HWRENDER
	GlidePatch_t *hwrcache; // patches are cached in renderer's native format
#endif
	USHORT numlumps; // this wad's number of resources
	FILE *handle;
	ULONG filesize; // for network
	unsigned char md5sum[16];
} wadfile_t;

#define WADFILENUM(lumpnum) (USHORT)((lumpnum)>>16) // wad flumpnum>>16) // wad file number in upper word
#define LUMPNUM(lumpnum) (USHORT)((lumpnum)&0xFFFF) // lump number for this pwad

extern USHORT numwadfiles;
extern wadfile_t *wadfiles[MAX_WADFILES];

// =========================================================================

void W_Shutdown(void);

// Load and add a wadfile to the active wad files, returns numbers of lumps, MAXSHORT on error
USHORT W_LoadWadFile(const char *filename);
void W_UnloadWadFile(USHORT num);

// W_InitMultipleFiles returns 1 if all is okay, 0 otherwise,
// so that it stops with a message if a file was not found, but not if all is okay.
int W_InitMultipleFiles(char **filenames);

const char *W_CheckNameForNumPwad(USHORT wad, USHORT lump);
const char *W_CheckNameForNum(lumpnum_t lumpnum);

USHORT W_CheckNumForNamePwad(const char *name, USHORT wad, USHORT startlump); // checks only in one pwad
lumpnum_t W_CheckNumForName(const char *name);
lumpnum_t W_GetNumForName(const char *name); // like W_CheckNumForName but I_Error on LUMPERROR

size_t W_LumpLengthPwad(USHORT wad, USHORT lump);
size_t W_LumpLength(lumpnum_t lumpnum);

size_t W_ReadLumpHeaderPwad(USHORT wad, USHORT lump, void *dest, size_t size, size_t offset);
size_t W_ReadLumpHeader(lumpnum_t lump, void *dest, size_t size, size_t offest); // read all or a part of a lump
void W_ReadLumpPwad(USHORT wad, USHORT lump, void *dest);
void W_ReadLump(lumpnum_t lump, void *dest);

void *W_CacheLumpNumPwad(USHORT wad, USHORT lump, int tag);
void *W_CacheLumpNum(lumpnum_t lump, int tag);

void *W_CacheLumpName(const char *name, int tag);
void *W_CachePatchName(const char *name, int tag);

#ifdef HWRENDER
//void *W_CachePatchNumPwad(USHORT wad, USHORT lump, int tag); // return a patch_t
void *W_CachePatchNum(lumpnum_t lumpnum, int tag); // return a patch_t
#else
//#define W_CachePatchNumPwad(wad, lump, tag) W_CacheLumpNumPwad(wad, lump, tag)
#define W_CachePatchNum(lumpnum, tag) W_CacheLumpNum(lumpnum, tag)
#endif

void W_VerifyFileMD5(USHORT wadfilenum, const char *matchmd5);

int W_VerifyNMUSlumps(const char *filename);

#endif // __W_WAD__
