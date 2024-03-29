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
/// \brief load and convert graphics to the hardware format

#include "hw_glob.h"
#include "hw_drv.h"

#include "../doomstat.h"    //gamemode
#include "../i_video.h"     //rendermode
#include "../r_data.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../v_video.h"
#include "../r_draw.h"

//Hurdler: 25/04/2000: used for new colormap code in hardware mode
//static byte *gr_colormap = NULL; // by default it must be NULL ! (because colormap tables are not initialized)
boolean firetranslucent = false;

// Values set after a call to HWR_ResizeBlock()
static int blocksize, blockwidth, blockheight;

int patchformat = GR_TEXFMT_AP_88; // use alpha for holes
int textureformat = GR_TEXFMT_P_8; // use chromakey for hole

// sprite, use alpha and chroma key for hole
static void HWR_DrawPatchInCache(GlideMipmap_t *mipmap,
	int pblockwidth, int pblockheight, int blockmodulo,
	int ptexturewidth, int ptextureheight,
	int originx, int originy, // where to draw patch in surface block
	const patch_t *realpatch, int bpp)
{
	int x, x1, x2;
	int col, ncols;
	fixed_t xfrac, xfracstep;
	fixed_t yfrac, yfracstep, position, count;
	fixed_t scale_y;
	RGBA_t colortemp;
	byte *dest;
	const byte *source;
	const column_t *patchcol;
	byte alpha;
	byte *block = mipmap->grInfo.data;

	x1 = originx;
	x2 = x1 + SHORT(realpatch->width);

	if (x1 < 0)
		x = 0;
	else
		x = x1;

	if (x2 > ptexturewidth)
		x2 = ptexturewidth;

	if (!ptexturewidth)
		return;

	col = x * pblockwidth / ptexturewidth;
	ncols = ((x2 - x) * pblockwidth) / ptexturewidth;

/*
	CONS_Printf("patch %dx%d texture %dx%d block %dx%d\n", SHORT(realpatch->width),
															SHORT(realpatch->height),
															ptexturewidth,
															textureheight,
															pblockwidth,pblockheight);
	CONS_Printf("      col %d ncols %d x %d\n", col, ncols, x);
*/

	// source advance
	xfrac = 0;
	if (x1 < 0)
		xfrac = -x1<<FRACBITS;

	xfracstep = (ptexturewidth << FRACBITS) / pblockwidth;
	yfracstep = (ptextureheight<< FRACBITS) / pblockheight;
	if (bpp < 1 || bpp > 4)
		I_Error("HWR_DrawPatchInCache: no drawer defined for this bpp (%d)\n",bpp);

	for (block += col*bpp; ncols--; block += bpp, xfrac += xfracstep)
	{
		patchcol = (const column_t *)((const byte *)realpatch
		 + LONG(realpatch->columnofs[xfrac>>FRACBITS]));

		scale_y = (pblockheight << FRACBITS) / ptextureheight;

		while (patchcol->topdelta != 0xff)
		{
			source = (const byte *)patchcol + 3;
			count  = ((patchcol->length * scale_y) + (FRACUNIT/2)) >> FRACBITS;
			position = originy + patchcol->topdelta;

			yfrac = 0;
			//yfracstep = (patchcol->length << FRACBITS) / count;
			if (position < 0)
			{
				yfrac = -position<<FRACBITS;
				count += (((position * scale_y) + (FRACUNIT/2)) >> FRACBITS);
				position = 0;
			}

			position = ((position * scale_y) + (FRACUNIT/2)) >> FRACBITS;
			if (position + count >= pblockheight)
				count = pblockheight - position;

			dest = block + (position*blockmodulo);
			while (count > 0)
			{
				byte texel;
				count--;

				texel = source[yfrac>>FRACBITS];

				if (firetranslucent && (transtables[(texel<<8)+0x40000]!=texel))
					alpha = 0x80;
				else
					alpha = 0xff;

				//Hurdler: not perfect, but better than holes
				if (texel == HWR_PATCHES_CHROMAKEY_COLORINDEX && (mipmap->flags & TF_CHROMAKEYED))
					texel = HWR_CHROMAKEY_EQUIVALENTCOLORINDEX;
				//Hurdler: 25/04/2000: now support colormap in hardware mode
				else if (mipmap->colormap)
					texel = mipmap->colormap[texel];

				// hope compiler will get this switch out of the loops (dreams...)
				// gcc do it ! but vcc not ! (why don't use cygwin gcc for win32 ?)
			   // Alam: SRB2 uses Mingw, HUGS
				switch (bpp)
				{
					case 2 : *((unsigned short *)dest) = (unsigned short)((alpha<<8) | texel);       break;
					case 3 : colortemp = V_GetColor(texel);
					         ((RGBA_t *)dest)->s.red   = colortemp.s.red;
					         ((RGBA_t *)dest)->s.green = colortemp.s.green;
					         ((RGBA_t *)dest)->s.blue  = colortemp.s.blue;
					         break;
					case 4 : *((RGBA_t *)dest) = V_GetColor(texel);
							  ((RGBA_t *)dest)->s.alpha = alpha;                   break;
					// default is 1
					default: *dest = texel;                                       break;
				}

				dest += blockmodulo;
				yfrac += yfracstep;
			}
			patchcol = (const column_t *)((const byte *)patchcol + patchcol->length + 4);
		}
	}
}


// resize the patch to be 3dfx compliant
// set : blocksize = blockwidth * blockheight  (no bpp used)
//       blockwidth
//       blockheight
//note :  8bit (1 byte per pixel) palettized format
static void HWR_ResizeBlock(int originalwidth, int originalheight,
	GrTexInfo *grInfo)
{
	//   Build the full textures from patches.
	static const GrLOD_t gr_lods[9] =
	{
		GR_LOD_LOG2_256,
		GR_LOD_LOG2_128,
		GR_LOD_LOG2_64,
		GR_LOD_LOG2_32,
		GR_LOD_LOG2_16,
		GR_LOD_LOG2_8,
		GR_LOD_LOG2_4,
		GR_LOD_LOG2_2,
		GR_LOD_LOG2_1
	};

	typedef struct
	{
		GrAspectRatio_t aspect;
		float           max_s;
		float           max_t;
	} booring_aspect_t;

	static const booring_aspect_t gr_aspects[8] =
	{
		{GR_ASPECT_LOG2_1x1, 255, 255},
		{GR_ASPECT_LOG2_2x1, 255, 127},
		{GR_ASPECT_LOG2_4x1, 255,  63},
		{GR_ASPECT_LOG2_8x1, 255,  31},

		{GR_ASPECT_LOG2_1x1, 255, 255},
		{GR_ASPECT_LOG2_1x2, 127, 255},
		{GR_ASPECT_LOG2_1x4,  63, 255},
		{GR_ASPECT_LOG2_1x8,  31, 255}
	};

	int     j,k;
	int     max,min;

	// find a power of 2 width/height
	//size up to nearest power of 2
	blockwidth = 1;
	while (blockwidth < originalwidth)
		blockwidth <<= 1;
	// scale down the original graphics to fit in 256
	if (blockwidth > 2048)
		blockwidth = 2048;
		//I_Error("3D GenerateTexture : too big");

	//size up to nearest power of 2
	blockheight = 1;
	while (blockheight < originalheight)
		blockheight <<= 1;
	// scale down the original graphics to fit in 256
	if (blockheight > 2048)
		blockheight = 2048;
		//I_Error("3D GenerateTexture : too big");

	// do the boring LOD stuff.. blech!
	if (blockwidth >= blockheight)
	{
		max = blockwidth;
		min = blockheight;
	}
	else
	{
		max = blockheight;
		min = blockwidth;
	}

	for (k = 2048, j = 0; k > max; j++)
		k>>=1;
	grInfo->smallLodLog2 = gr_lods[j];
	grInfo->largeLodLog2 = gr_lods[j];

	for (k = max, j = 0; k > min && j < 4; j++)
		k>>=1;
	// aspect ratio too small for 3Dfx (eg: 8x128 is 1x16 : use 1x8)
	if (j == 4)
	{
		j = 3;
		//CONS_Printf("HWR_ResizeBlock : bad aspect ratio %dx%d\n", blockwidth,blockheight);
		if (blockwidth < blockheight)
			blockwidth = max>>3;
		else
			blockheight = max>>3;
	}
	if (blockwidth < blockheight)
		j += 4;
	grInfo->aspectRatioLog2 = gr_aspects[j].aspect;

	blocksize = blockwidth * blockheight;

	//CONS_Printf("Width is %d, Height is %d\n", blockwidth, blockheight);
}


static const int format2bpp[16] =
{
	0, //0
	0, //1
	1, //2  GR_TEXFMT_ALPHA_8
	1, //3  GR_TEXFMT_INTENSITY_8
	1, //4  GR_TEXFMT_ALPHA_INTENSITY_44
	1, //5  GR_TEXFMT_P_8
	4, //6  GR_RGBA
	0, //7
	0, //8
	0, //9
	2, //10 GR_TEXFMT_RGB_565
	2, //11 GR_TEXFMT_ARGB_1555
	2, //12 GR_TEXFMT_ARGB_4444
	2, //13 GR_TEXFMT_ALPHA_INTENSITY_88
	2, //14 GR_TEXFMT_AP_88
};

static byte *MakeBlock(GlideMipmap_t *grMipmap)
{
	int bpp;
	byte *block;
	int i;

	bpp =  format2bpp[grMipmap->grInfo.format];
	block = Z_Malloc(blocksize*bpp, PU_STATIC, &(grMipmap->grInfo.data));

	switch (bpp)
	{
		case 1: memset(block, HWR_PATCHES_CHROMAKEY_COLORINDEX, blocksize); break;
		case 2:
				// fill background with chromakey, alpha = 0
				for (i = 0; i < blocksize; i++)
				//[segabor]
					*((unsigned short *)block+i) = ((0x00 <<8) | HWR_CHROMAKEY_EQUIVALENTCOLORINDEX);
				break;
		case 4: memset(block,0,blocksize*4); break;
	}

	return block;
}

//
// Create a composite texture from patches, adapt the texture size to a power of 2
// height and width for the hardware texture cache.
//
static void HWR_GenerateTexture(int texnum, GlideTexture_t *grtex)
{
	byte *block;
	texture_t *texture;
	texpatch_t *patch;
	patch_t *realpatch;

	int i;
	boolean skyspecial = false; //poor hack for Legacy large skies..

	texture = textures[texnum];

	// hack the Legacy skies..
	if (texture->name[0] == 'S' &&
	     texture->name[1] == 'K' &&
	     texture->name[2] == 'Y' &&
	     texture->name[4] == 0)
	{
		skyspecial = true;
		grtex->mipmap.flags = TF_WRAPXY; // don't use the chromakey for sky
	}
	else
		grtex->mipmap.flags = TF_CHROMAKEYED | TF_WRAPXY;

	HWR_ResizeBlock (texture->width, texture->height, &grtex->mipmap.grInfo);
	grtex->mipmap.width = (unsigned short)blockwidth;
	grtex->mipmap.height = (unsigned short)blockheight;
	grtex->mipmap.grInfo.format = textureformat;

	block = MakeBlock(&grtex->mipmap);

	if (skyspecial) //Hurdler: not efficient, but better than holes in the sky (and it's done only at level loading)
	{
		int j;
		RGBA_t col;

		col = V_GetColor(HWR_CHROMAKEY_EQUIVALENTCOLORINDEX);
		for (j = 0; j < blockheight; j++)
		{
			for (i = 0; i < blockwidth; i++)
			{
				block[4*(j*blockwidth+i)+0] = col.s.red;
				block[4*(j*blockwidth+i)+1] = col.s.green;
				block[4*(j*blockwidth+i)+2] = col.s.blue;
				block[4*(j*blockwidth+i)+3] = 0xff;
			}
		}
	}

	// Composite the columns together.
	for (i = 0, patch = texture->patches;
	 i < texture->patchcount;
	 i++, patch++)
	{
		realpatch = W_CacheLumpNum (patch->patch, PU_CACHE);
		HWR_DrawPatchInCache(&grtex->mipmap,
		                      blockwidth, blockheight,
		                      blockwidth*format2bpp[grtex->mipmap.grInfo.format],
		                      texture->width, texture->height,
		                      patch->originx, patch->originy,
		                      realpatch,
		                      format2bpp[grtex->mipmap.grInfo.format]);
	}
	 //Hurdler: not efficient at all but I don't remember exactly how HWR_DrawPatchInCache works :(
	if (format2bpp[grtex->mipmap.grInfo.format]==4)
	{
		for (i = 3; i < blocksize; i += 4)
		{
			if (block[i] == 0)
			{
				grtex->mipmap.flags |= TF_TRANSPARENT;
				break;
			}
		}
	}

	// make it purgable from zone memory
	// use PU_PURGELEVEL so we can Z_FreeTags all at once
	Z_ChangeTag (block, PU_HWRCACHE);

	grtex->scaleX = 1.0f/(texture->width*FRACUNIT);
	grtex->scaleY = 1.0f/(texture->height*FRACUNIT);
}

// grTex : Hardware texture cache info
//         .data : address of converted patch in heap memory
//                 user for Z_Malloc(), becomes NULL if it is purged from the cache
void HWR_MakePatch (const patch_t *patch, GlidePatch_t *grPatch, GlideMipmap_t *grMipmap)
{
	byte *block;
	int newwidth, newheight;

	// don't do it twice (like a cache)
	if (grMipmap->width == 0)
	{
		// save the original patch header so that the GlidePatch can be casted
		// into a standard patch_t struct and the existing code can get the
		// orginal patch dimensions and offsets.
		grPatch->width = SHORT(patch->width);
		grPatch->height = SHORT(patch->height);
		grPatch->leftoffset = SHORT(patch->leftoffset);
		grPatch->topoffset = SHORT(patch->topoffset);

		// find the good 3dfx size (boring spec)
		HWR_ResizeBlock (SHORT(patch->width), SHORT(patch->height), &grMipmap->grInfo);
		grMipmap->width = (unsigned short)blockwidth;
		grMipmap->height = (unsigned short)blockheight;

		// no wrap around, no chroma key
		grMipmap->flags = 0;
		// setup the texture info
		grMipmap->grInfo.format = patchformat;
	}
	else
	{
		blockwidth = grMipmap->width;
		blockheight = grMipmap->height;
		blocksize = blockwidth * blockheight;
	}

	Z_Free(grMipmap->grInfo.data);
	grMipmap->grInfo.data = NULL;

	block = MakeBlock(grMipmap);

	// no rounddown, do not size up patches, so they don't look 'scaled'
	newwidth  = min(SHORT(patch->width), blockwidth);
	newheight = min(SHORT(patch->height), blockheight);

	HWR_DrawPatchInCache(grMipmap,
	                      newwidth, newheight,
	                      blockwidth*format2bpp[grMipmap->grInfo.format],
	                      SHORT(patch->width), SHORT(patch->height),
	                      0, 0,
	                      patch,
	                      format2bpp[grMipmap->grInfo.format]);

	grPatch->max_s = (float)newwidth / (float)blockwidth;
	grPatch->max_t = (float)newheight / (float)blockheight;

	// Now that the texture has been built in cache, it is purgable from zone memory.
	Z_ChangeTag (block, PU_HWRCACHE);
}


// =================================================
//             CACHING HANDLING
// =================================================

static size_t gr_numtextures;
static GlideTexture_t *gr_textures; // for ALL Doom textures

void HWR_InitTextureCache(void)
{
	gr_numtextures = 0;
	gr_textures = NULL;
}

void HWR_FreeTextureCache(void)
{
	int i,j;
	// free references to the textures
	HWD.pfnClearMipMapCache();

	// free all hardware-converted graphics cached in the heap
	// our gool is only the textures since user of the texture is the texture cache
	if (Z_TagUsage(PU_HWRCACHE)) Z_FreeTags (PU_HWRCACHE, PU_HWRCACHE);
	// Alam: free the Z_Blocks before freeing it's users

	// free all skin after each level: must be done after pfnClearMipMapCache!
	for (j = 0; j < numwadfiles; j++)
		for (i = 0; i < wadfiles[j]->numlumps; i++)
		{
			GlidePatch_t *grpatch = &(wadfiles[j]->hwrcache[i]);
			while (grpatch->mipmap.nextcolormap)
			{
				GlideMipmap_t *grmip = grpatch->mipmap.nextcolormap;
				grpatch->mipmap.nextcolormap = grmip->nextcolormap;
				free(grmip);
			}
		}

	// now the heap don't have any 'user' pointing to our
	// texturecache info, we can free it
	if (gr_textures)
		free(gr_textures);
	gr_textures = NULL;
	gr_numtextures = 0;
}

void HWR_PrepLevelCache(size_t pnumtextures)
{
	// problem: the mipmap cache management hold a list of mipmaps.. but they are
	//           reallocated on each level..
	//sub-optimal, but 1) just need re-download stuff in hardware cache VERY fast
	//   2) sprite/menu stuff mixed with level textures so can't do anything else

	// we must free it since numtextures changed
	HWR_FreeTextureCache();

	gr_numtextures = pnumtextures;
	gr_textures = calloc(pnumtextures, sizeof (*gr_textures));
	if (gr_textures == NULL)
		I_Error("3D can't alloc gr_textures");
}

void HWR_SetPalette(RGBA_t *palette)
{
	//Hudler: 16/10/99: added for OpenGL gamma correction
	RGBA_t gamma_correction = {0x7F7F7F7F};

	//Hurdler 16/10/99: added for OpenGL gamma correction
	gamma_correction.s.red   = (byte)cv_grgammared.value;
	gamma_correction.s.green = (byte)cv_grgammagreen.value;
	gamma_correction.s.blue  = (byte)cv_grgammablue.value;
	HWD.pfnSetPalette(palette, &gamma_correction);

	// hardware driver will flush there own cache if cache is non paletized
	// now flush data texture cache so 32 bit texture are recomputed
	if (patchformat == GR_RGBA || textureformat == GR_RGBA)
		Z_FreeTags(PU_HWRCACHE, PU_HWRCACHE);
}

// --------------------------------------------------------------------------
// Make sure texture is downloaded and set it as the source
// --------------------------------------------------------------------------
GlideTexture_t *HWR_GetTexture(int tex)
{
	GlideTexture_t *grtex;
#ifdef PARANOIA
	if ((unsigned)tex >= gr_numtextures)
		I_Error(" HWR_GetTexture: tex >= numtextures\n");
#endif
	grtex = &gr_textures[tex];

	if (!grtex->mipmap.grInfo.data && !grtex->mipmap.downloaded)
		HWR_GenerateTexture(tex, grtex);

	HWD.pfnSetTexture(&grtex->mipmap);
	return grtex;
}


static void HWR_CacheFlat(GlideMipmap_t *grMipmap, lumpnum_t flatlumpnum)
{
	byte *block;
	size_t size, pflatsize;

	// setup the texture info
	grMipmap->grInfo.smallLodLog2 = GR_LOD_LOG2_64;
	grMipmap->grInfo.largeLodLog2 = GR_LOD_LOG2_64;
	grMipmap->grInfo.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
	grMipmap->grInfo.format = GR_TEXFMT_P_8;
	grMipmap->flags = TF_WRAPXY;

	size = W_LumpLength(flatlumpnum);

	switch (size)
	{
		case 4194304: // 2048x2048 lump
			pflatsize = 2048;
			break;
		case 1048576: // 1024x1024 lump
			pflatsize = 1024;
			break;
		case 262144:// 512x512 lump
			pflatsize = 512;
			break;
		case 65536: // 256x256 lump
			pflatsize = 256;
			break;
		case 16384: // 128x128 lump
			pflatsize = 128;
			break;
		case 1024: // 32x32 lump
			pflatsize = 32;
			break;
		default: // 64x64 lump
			pflatsize = 64;
			break;
	}
	grMipmap->width  = (unsigned short)pflatsize;
	grMipmap->height = (unsigned short)pflatsize;

	// the flat raw data needn't be converted with palettized textures
	block = Z_Malloc(W_LumpLength(flatlumpnum),
		PU_HWRCACHE, &grMipmap->grInfo.data);

	W_ReadLump(flatlumpnum, grMipmap->grInfo.data);
}


// Download a Doom 'flat' to the hardware cache and make it ready for use
void HWR_GetFlat(lumpnum_t flatlumpnum)
{
	GlideMipmap_t *grmip;

	grmip = &(wadfiles[WADFILENUM(flatlumpnum)]->hwrcache[LUMPNUM(flatlumpnum)].mipmap);

	if (!grmip->downloaded && !grmip->grInfo.data)
		HWR_CacheFlat(grmip, flatlumpnum);

	HWD.pfnSetTexture(grmip);
}

//
// HWR_LoadMappedPatch(): replace the skin color of the sprite in cache
//                          : load it first in doom cache if not already
//
static void HWR_LoadMappedPatch(GlideMipmap_t *grmip, GlidePatch_t *gpatch)
{
	if (!grmip->downloaded && !grmip->grInfo.data)
	{
		patch_t *patch = W_CacheLumpNum(gpatch->patchlump, PU_STATIC);
		HWR_MakePatch(patch, gpatch, grmip);

		Z_Free(patch);
	}

	HWD.pfnSetTexture(grmip);
}

// -----------------+
// HWR_GetPatch     : Download a patch to the hardware cache and make it ready for use
// -----------------+
void HWR_GetPatch(GlidePatch_t *gpatch)
{
	// is it in hardware cache
	if (!gpatch->mipmap.downloaded && !gpatch->mipmap.grInfo.data)
	{
		// load the software patch, PU_STATIC or the Z_Malloc for hardware patch will
		// flush the software patch before the conversion! oh yeah I suffered
		patch_t *ptr = W_CacheLumpNum(gpatch->patchlump, PU_STATIC);
		HWR_MakePatch(ptr, gpatch, &gpatch->mipmap);

		// this is inefficient.. but the hardware patch in heap is purgeable so it should
		// not fragment memory, and besides the REAL cache here is the hardware memory
		Z_Free(ptr);
	}

	HWD.pfnSetTexture(&gpatch->mipmap);
}


// -------------------+
// HWR_GetMappedPatch : Same as HWR_GetPatch for sprite color
// -------------------+
void HWR_GetMappedPatch(GlidePatch_t *gpatch, const byte *colormap)
{
	GlideMipmap_t *grmip, *newmip;

	if (colormap == colormaps || colormap == NULL)
	{
		// Load the default (green) color in doom cache (temporary?) AND hardware cache
		HWR_GetPatch(gpatch);
		return;
	}

	// search for the mimmap
	// skip the first (no colormap translated)
	for (grmip = &gpatch->mipmap; grmip->nextcolormap; )
	{
		grmip = grmip->nextcolormap;
		if (grmip->colormap == colormap)
		{
			HWR_LoadMappedPatch(grmip, gpatch);
			return;
		}
	}
	// not found, create it!
	// If we are here, the sprite with the current colormap is not already in hardware memory

	//BP: WARNING: don't free it manually without clearing the cache of harware renderer
	//              (it have a liste of mipmap)
	//    this malloc is cleared in HWR_FreeTextureCache
	//    (...) unfortunately z_malloc fragment alot the memory :(so malloc is better
	newmip = calloc(1, sizeof *newmip);
	grmip->nextcolormap = newmip;

	newmip->colormap = colormap;
	HWR_LoadMappedPatch(newmip, gpatch);
}

static const int picmode2GR[] =
{
	GR_TEXFMT_P_8,                // PALETTE
	0,                            // INTENSITY          (unsupported yet)
	GR_TEXFMT_ALPHA_INTENSITY_88, // INTENSITY_ALPHA    (corona use this)
	0,                            // RGB24              (unsupported yet)
	GR_RGBA,                      // RGBA32             (opengl only)
};

static void HWR_DrawPicInCache(byte *block, int pblockwidth, int pblockheight,
	int blockmodulo, pic_t *pic, int bpp)
{
	int i,j;
	fixed_t posx, posy, stepx, stepy;
	byte *dest, *src, texel;
	int picbpp;
	RGBA_t col;

	stepy = ((int)SHORT(pic->height)<<FRACBITS)/pblockheight;
	stepx = ((int)SHORT(pic->width)<<FRACBITS)/pblockwidth;
	picbpp = format2bpp[picmode2GR[pic->mode]];
	posy = 0;
	for (j = 0; j < pblockheight; j++)
	{
		posx = 0;
		dest = &block[j*blockmodulo];
		src = &pic->data[(posy>>FRACBITS)*SHORT(pic->width)*picbpp];
		for (i = 0; i < pblockwidth;i++)
		{
			switch (pic->mode)
			{ // source bpp
				case PALETTE :
					texel = src[(posx+FRACUNIT/2)>>FRACBITS];
					switch (bpp)
					{ // destination bpp
						case 1 :
							*dest++ = texel; break;
						case 2 :
							*(USHORT *)dest = (USHORT)(texel | 0xff00);
							dest +=2;
							break;
						case 3 :
							col = V_GetColor(texel);
							((RGBA_t *)dest)->s.red   = col.s.red;
							((RGBA_t *)dest)->s.green = col.s.green;
							((RGBA_t *)dest)->s.blue  = col.s.blue;
							dest += 3;
							break;
						case 4 :
							*(RGBA_t *)dest = V_GetColor(texel);
							dest += 4;
							break;
					}
					break;
				case INTENSITY :
					*dest++ = src[(posx+FRACUNIT/2)>>FRACBITS];
					break;
				case INTENSITY_ALPHA : // assume dest bpp = 2
					*(USHORT*)dest = *((short *)src + ((posx+FRACUNIT/2)>>FRACBITS));
					dest += 2;
					break;
				case RGB24 :
					break;  // not supported yet
				case RGBA32 : // assume dest bpp = 4
					dest += 4;
					*(ULONG *)dest = *((ULONG *)src + ((posx+FRACUNIT/2)>>FRACBITS));
					break;
			}
			posx += stepx;
		}
		posy += stepy;
	}
}

// -----------------+
// HWR_GetPic       : Download a Doom pic (raw row encoded with no 'holes')
// Returns          :
// -----------------+
GlidePatch_t *HWR_GetPic(lumpnum_t lumpnum)
{
	GlidePatch_t *grpatch;

	grpatch = &(wadfiles[lumpnum>>16]->hwrcache[lumpnum & 0xffff]);

	if (!grpatch->mipmap.downloaded && !grpatch->mipmap.grInfo.data)
	{
		pic_t *pic;
		byte *block;
		size_t len;
		int newwidth, newheight;

		pic = W_CacheLumpNum(lumpnum, PU_STATIC);
		grpatch->width = SHORT(pic->width);
		grpatch->height = SHORT(pic->height);
		len = W_LumpLength(lumpnum) - sizeof (pic_t);

		grpatch->leftoffset = 0;
		grpatch->topoffset = 0;

		// find the good 3dfx size (boring spec)
		HWR_ResizeBlock (grpatch->width, grpatch->height, &grpatch->mipmap.grInfo);
		grpatch->mipmap.width = (unsigned short)blockwidth;
		grpatch->mipmap.height = (unsigned short)blockheight;

		if (pic->mode == PALETTE)
			grpatch->mipmap.grInfo.format = textureformat; // can be set by driver
		else
			grpatch->mipmap.grInfo.format = picmode2GR[pic->mode];

		Z_Free(grpatch->mipmap.grInfo.data);

		// allocate block
		block = MakeBlock(&grpatch->mipmap);

		// no rounddown, do not size up patches, so they don't look 'scaled'
		newwidth  = min(SHORT(pic->width),blockwidth);
		newheight = min(SHORT(pic->height),blockheight);

		if (grpatch->width  == blockwidth &&
			grpatch->height == blockheight &&
			format2bpp[grpatch->mipmap.grInfo.format] == format2bpp[picmode2GR[pic->mode]])
		{
			// no conversion needed
			memcpy(grpatch->mipmap.grInfo.data, pic->data,len);
		}
		else
			HWR_DrawPicInCache(block, newwidth, newheight,
			                   blockwidth*format2bpp[grpatch->mipmap.grInfo.format],
			                   pic,
			                   format2bpp[grpatch->mipmap.grInfo.format]);

		Z_ChangeTag(pic, PU_CACHE);
		Z_ChangeTag(block, PU_HWRCACHE);

		grpatch->mipmap.flags = 0;
		grpatch->max_s = (float)newwidth  / (float)blockwidth;
		grpatch->max_t = (float)newheight / (float)blockheight;
	}
	HWD.pfnSetTexture(&grpatch->mipmap);
	//CONS_Printf("picloaded at %x as texture %d\n",grpatch->mipmap.grInfo.data, grpatch->mipmap.downloaded);

	return grpatch;
}
