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
/// \brief Commonly used routines
///
///	Default Config File.
///	PCX Screenshots.
///	File i/o

#ifdef __GNUC__
#include <unistd.h>
#endif
// Extended map support.
#include <ctype.h>

#ifdef HAVE_PNG
 #include "png.h"
 #ifdef PNG_WRITE_SUPPORTED
  #define USE_PNG // Only actually use PNG if write is supported.
  // See hardware/hw_draw.c for a similar check to this one.
 #endif
#endif

#ifdef HAVE_MNG
#define MNG_NO_INCLUDE_JNG
#define MNG_SUPPORT_WRITE
#define MNG_ACCESS_CHUNKS
#include "libmng.h"
#endif

#include "doomdef.h"
#include "g_game.h"
#include "m_misc.h"
#include "hu_stuff.h"
#include "v_video.h"
#include "z_zone.h"
#include "g_input.h"
#include "i_video.h"
#include "d_main.h"
#include "m_argv.h"
#include "i_system.h"

#ifdef _WIN32_WCE
#include "sdl/SRB2CE/cehelp.h"
#endif

#ifdef _XBOX
#include "sdl/SRB2XBOX/xboxhelp.h"
#endif

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

static CV_PossibleValue_t screenshot_cons_t[] = {{0, "Default"}, {1, "HOME"}, {2, "SRB2"}, {3, "CUSTOM"}, {0, NULL}};
consvar_t cv_screenshot_option = {"screenshot_option", "Default", CV_SAVE, screenshot_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_screenshot_folder = {"screenshot_folder", "", CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
boolean moviemode = false; // In snapshots files or in 1 MNG file

/** Returns the map number for a map identified by the last two characters in
  * its name.
  *
  * \param first  The first character after MAP.
  * \param second The second character after MAP.
  * \return The map number, or 0 if no map corresponds to these characters.
  * \sa G_BuildMapName
  */
int M_MapNumber(char first, char second)
{
	if (isdigit(first))
	{
		if (isdigit(second))
			return ((int)first - '0') * 10 + ((int)second - '0');
		return 0;
	}

	if (!isalpha(first))
		return 0;
	if (!isalnum(second))
		return 0;

	return 100 + ((int)tolower(first) - 'a') * 36 + (isdigit(second) ? ((int)second - '0') :
		((int)tolower(second) - 'a') + 10);
}

// ==========================================================================
//                         FILE INPUT / OUTPUT
// ==========================================================================

//
// FIL_WriteFile
//
#ifndef O_BINARY
#define O_BINARY 0
#endif

/** Writes out a file.
  *
  * \param name   Name of the file to write.
  * \param source Memory location to write from.
  * \param length How many bytes to write.
  * \return True on success, false on failure.
  */
boolean FIL_WriteFile(char const *name, const void *source, size_t length)
{
	FILE *handle = NULL;
	size_t count;

	//if (FIL_WriteFileOK(name))
		handle = fopen(name, "w+b");

	if (!handle)
		return false;

	count = fwrite(source, 1, length, handle);
	fclose(handle);

	if (count < length)
		return false;

	return true;
}

/** Reads in a file, appending a zero byte at the end.
  *
  * \param name   Filename to read.
  * \param buffer Pointer to a pointer, which will be set to the location of a
  *               newly allocated buffer holding the file's contents.
  * \return Number of bytes read, not counting the zero byte added to the end,
  *         or 0 on error.
  */
size_t FIL_ReadFile(char const *name, byte **buffer)
{
	FILE *handle = NULL;
	size_t count, length;
	byte *buf;

	if (FIL_ReadFileOK(name))
		handle = fopen(name, "rb");

	if (!handle)
		return 0;

	fseek(handle,0,SEEK_END);
	length = ftell(handle);
	fseek(handle,0,SEEK_SET);

	buf = Z_Malloc(length + 1, PU_STATIC, NULL);
	count = fread(buf, 1, length, handle);
	fclose(handle);

	if (count < length)
	{
		Z_Free(buf);
		return 0;
	}

	// append 0 byte for script text files
	buf[length] = 0;

	*buffer = buf;
	return length;
}

/** Check if the filename exists
  *
  * \param name   Filename to check.
  * \return true if file exists, false if it doesn't.
  */
boolean FIL_FileExists(char const *name)
{
	return access(name,0)+1;
}


/** Check if the filename OK to write
  *
  * \param name   Filename to check.
  * \return true if file write-able, false if it doesn't.
  */
boolean FIL_WriteFileOK(char const *name)
{
	return access(name,2)+1;
}


/** Check if the filename OK to read
  *
  * \param name   Filename to check.
  * \return true if file read-able, false if it doesn't.
  */
boolean FIL_ReadFileOK(char const *name)
{
	return access(name,4)+1;
}

/** Check if the filename OK to read/write
  *
  * \param name   Filename to check.
  * \return true if file (read/write)-able, false if it doesn't.
  */
boolean FIL_FileOK(char const *name)
{
	return access(name,6)+1;
}


/** Checks if a pathname has a file extension and adds the extension provided
  * if not.
  *
  * \param path      Pathname to check.
  * \param extension Extension to add if no extension is there.
  */
void FIL_DefaultExtension(char *path, const char *extension)
{
	char *src;

	// search for '.' from end to begin, add .EXT only when not found
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return; // it has an extension
		src--;
	}

	strcat(path, extension);
}

void FIL_ForceExtension(char *path, const char *extension)
{
	char *src;

	// search for '.' from end to begin, add .EXT only when not found
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
		{
			*src = '\0';
			break; // it has an extension
		}
		src--;
	}

	strcat(path, extension);
}

/** Checks if a filename extension is found.
  * Lump names do not contain dots.
  *
  * \param in String to check.
  * \return True if an extension is found, otherwise false.
  */
boolean FIL_CheckExtension(const char *in)
{
	while (*in++)
		if (*in == '.')
			return true;

	return false;
}

// ==========================================================================
//                        CONFIGURATION FILE
// ==========================================================================

//
// DEFAULTS
//

char configfile[MAX_WADPATH];

// ==========================================================================
//                          CONFIGURATION
// ==========================================================================
static boolean gameconfig_loaded = false; // true once config.cfg loaded AND executed

/** Saves a player's config, possibly to a particular file.
  *
  * \sa Command_LoadConfig_f
  */
void Command_SaveConfig_f(void)
{
	char tmpstr[MAX_WADPATH];

	if (COM_Argc() < 2)
	{
		CONS_Printf("saveconfig <filename[.cfg]> [-silent] : save config to a file\n");
		return;
	}
	strcpy(tmpstr, COM_Argv(1));
	FIL_ForceExtension(tmpstr, ".cfg");

	M_SaveConfig(tmpstr);
	if (stricmp(COM_Argv(2), "-silent"))
		CONS_Printf("config saved as %s\n", configfile);
}

/** Loads a game config, possibly from a particular file.
  *
  * \sa Command_SaveConfig_f, Command_ChangeConfig_f
  */
void Command_LoadConfig_f(void)
{
	if (COM_Argc() != 2)
	{
		CONS_Printf("loadconfig <filename[.cfg]> : load config from a file\n");
		return;
	}

	strcpy(configfile, COM_Argv(1));
	FIL_ForceExtension(configfile, ".cfg");
	COM_BufInsertText(va("exec \"%s\"\n", configfile));
}

/** Saves the current configuration and loads another.
  *
  * \sa Command_LoadConfig_f, Command_SaveConfig_f
  */
void Command_ChangeConfig_f(void)
{
	if (COM_Argc() != 2)
	{
		CONS_Printf("changeconfig <filename[.cfg]> : save current config and load another\n");
		return;
	}

	COM_BufAddText(va("saveconfig \"%s\"\n", configfile));
	COM_BufAddText(va("loadconfig \"%s\"\n", COM_Argv(1)));
}

/** Loads the default config file.
  *
  * \sa Command_LoadConfig_f
  */
void M_FirstLoadConfig(void)
{
	// configfile is initialised by d_main when searching for the wad?

	// check for a custom config file
	if (M_CheckParm("-config") && M_IsNextParm())
	{
		strcpy(configfile, M_GetNextParm());
		CONS_Printf("config file: %s\n",configfile);
	}

	// load default control
	G_Controldefault();

	// load config, make sure those commands doesnt require the screen..
	CONS_Printf("\n");
	COM_BufInsertText(va("exec \"%s\"\n", configfile));
	// no COM_BufExecute() needed; that does it right away

	// make sure I_Quit() will write back the correct config
	// (do not write back the config if it crash before)
	gameconfig_loaded = true;
}

/** Saves the game configuration.
  *
  * \sa Command_SaveConfig_f
  */
void M_SaveConfig(char *filename)
{
	FILE *f;

	// make sure not to write back the config until it's been correctly loaded
	if (!gameconfig_loaded)
		return;

	// can change the file name
	if (filename)
	{
		f = fopen(filename, "w");
		// change it only if valid
		if (f)
			strcpy(configfile, filename);
		else
		{
			CONS_Printf("Couldn't save game config file %s\n", filename);
			return;
		}
	}
	else
	{
		f = fopen(configfile, "w");
		if (!f)
		{
			CONS_Printf("Couldn't save game config file %s\n", configfile);
			return;
		}
	}

	// header message
	fprintf(f, "// SRB2 configuration file.\n");

	// FIXME: save key aliases if ever implemented..

	CV_SaveVariables(f);
	if (!dedicated) G_SaveKeySetting(f);

	fclose(f);
}


#if !defined (DC) && !defined (_WIN32_WCE) && !defined (PSP)
static const char *Newsnapshotfile(const char *pathname, const char *ext)
{
	static char freename[13] = "srb2XXXX.ext";
	int i = 5000; // start in the middle: num screenshots divided by 2
	int add = i; // how much to add or subtract if wrong; gets divided by 2 each time
	int result; // -1 = guess too high, 0 = correct, 1 = guess too low

	// find a file name to save it to
	strcpy(freename+9,ext);

	for (;;)
	{
		freename[4] = (char)('0' + (char)(i/1000));
		freename[5] = (char)('0' + (char)((i/100)%10));
		freename[6] = (char)('0' + (char)((i/10)%10));
		freename[7] = (char)('0' + (char)(i%10));

		if (FIL_WriteFileOK(va(pandf,pathname,freename))) // access succeeds
			result = 1; // too low
		else // access fails: equal or too high
		{
			if (!i)
				break; // not too high, so it must be equal! YAY!

			freename[4] = (char)('0' + (char)((i-1)/1000));
			freename[5] = (char)('0' + (char)(((i-1)/100)%10));
			freename[6] = (char)('0' + (char)(((i-1)/10)%10));
			freename[7] = (char)('0' + (char)((i-1)%10));
			if (!FIL_WriteFileOK(va(pandf,pathname,freename))) // access fails
				result = -1; // too high
			else
				break; // not too high, so equal, YAY!
		}

		add /= 2;

		if (!add) // don't get stuck at 5 due to truncation!
			add = 1;

		i += add * result;

		if (add < 0 || add > 9999)
			return NULL;
	}

	freename[4] = (char)('0' + (char)(i/1000));
	freename[5] = (char)('0' + (char)((i/100)%10));
	freename[6] = (char)('0' + (char)((i/10)%10));
	freename[7] = (char)('0' + (char)(i%10));

	return freename;
}
#endif

#ifdef HAVE_MNG // SRB2 MNG Moives

static mng_handle MNG_ptr;

typedef struct MNG_user_struct
{
		FILE *fh;                 /* file handle */
		mng_palette8ep pal;        /* 256RGB pal*/
} MNG_userdata;
typedef MNG_userdata *MNG_userdatap;

static mng_ptr MNG_DECL MNG_memalloc(mng_size_t iLen)
{return (mng_ptr)calloc (1,iLen);}

static void MNG_DECL MNG_free(mng_ptr iPtr, mng_size_t iLen)
{ iLen = 0; free (iPtr);}

static mng_bool MNG_DECL MNG_open (mng_handle hHandle)
{ hHandle = NULL; return MNG_TRUE;}

static mng_bool MNG_DECL MNG_close (mng_handle hHandle)
{
	MNG_userdatap privdata = (MNG_userdatap)mng_get_userdata(hHandle);
	if (privdata->fh)
		fclose(privdata->fh);
	privdata->fh = 0;
	return MNG_TRUE;
}

static mng_bool MNG_DECL MNG_write (mng_handle  hHandle,
                                    mng_ptr     pBuf,
                                    mng_uint32  iBuflen,
                                    mng_uint32p pWritten)
{
  MNG_userdatap privdata = (MNG_userdatap)mng_get_userdata(hHandle);
  *pWritten = fwrite(pBuf, 1, iBuflen, privdata->fh);
  return MNG_TRUE;
}

static boolean M_SetupMNG(const mng_pchar filename, byte *pal)
{
	MNG_userdatap MNG_shared;
	mng_retcode ret;

	MNG_shared = malloc(sizeof (MNG_userdata));
	if (!MNG_shared)
	{
		return false;
	}

	MNG_shared->fh = fopen(filename,"wb");
	if (!MNG_shared->fh)
	{
		free(MNG_shared);
		return false;
	}
	MNG_shared->pal = (mng_palette8ep)pal;

	MNG_ptr = mng_initialize ((mng_ptr)MNG_shared, MNG_memalloc, MNG_free, MNG_NULL);
	if (!MNG_ptr)
	{
		fclose(MNG_shared->fh);
		free(MNG_shared);
		return false;
	}

	ret = mng_setcb_openstream  (MNG_ptr, MNG_open);
	ret = mng_setcb_closestream (MNG_ptr, MNG_close);
	ret = mng_setcb_writedata   (MNG_ptr, MNG_write);
	ret = mng_create(MNG_ptr);
	ret = mng_putchunk_mhdr(MNG_ptr, vid.width, vid.height, TICRATE, 0, 0, 0,
	      MNG_SIMPLICITY_VALID | MNG_SIMPLICITY_SIMPLEFEATURES);
	ret = mng_putchunk_term(MNG_ptr, MNG_TERMACTION_REPEAT,
	      MNG_ITERACTION_FIRSTFRAME, TICRATE, 0x7FFFFFFF);

	// iTXT

	if (pal)
	{
		ret = mng_putchunk_plte(MNG_ptr, 256, MNG_shared->pal);
	}

	ret = mng_putchunk_back(MNG_ptr, 0,0,0, 0, 0, MNG_BACKGROUNDIMAGE_NOTILE);
	ret = mng_putchunk_fram(MNG_ptr, MNG_FALSE, MNG_FRAMINGMODE_1, 0,MNG_NULL,
			MNG_CHANGEDELAY_DEFAULT, MNG_CHANGETIMOUT_NO, MNG_CHANGECLIPPING_NO, MNG_CHANGESYNCID_NO,
			100/TICRATE, 0,0,0,0,0,0, MNG_NULL,0);

	ret = mng_putchunk_defi(MNG_ptr, 0, MNG_DONOTSHOW_NOTVISIBLE, MNG_CONCRETE, MNG_FALSE, 0,0, MNG_FALSE, 0,0,0,0);

	return true;
}

boolean M_OpenMNG(void)
{
	const char *freename = NULL, *pathname = ".";
	boolean ret = false;

	if (cv_screenshot_option.value == 0)
		pathname = usehome ? srb2home : srb2path;
	else if (cv_screenshot_option.value == 1)
		pathname = srb2home;
	else if (cv_screenshot_option.value == 2)
		pathname = srb2path;
	else if (cv_screenshot_option.value == 3 && *cv_screenshot_folder.string != '\0')
		pathname = cv_screenshot_folder.string;

	if (rendermode == render_none)
		I_Error("Can't take a MNG movie without a render system");
	else
		freename = Newsnapshotfile(pathname,"mng");

	if (!freename)
		goto failure;

	if (rendermode == render_soft)
		ret = M_SetupMNG(va(pandf,pathname,freename), W_CacheLumpName("PLAYPAL", PU_CACHE));
	else
		ret = M_SetupMNG(va(pandf,pathname,freename), NULL);

failure:
	if (!ret)
	{
		if (freename)
			CONS_Printf("Couldn't create MNG movie file %s in %s\n", freename, pathname);
		else
			CONS_Printf("Couldn't create MNG movie file (all 10000 slots used!) in %s\n", pathname);
	}
	return ret;
}

void M_SaveMNG(void)
{
	MNG_userdatap privdata;
	mng_retcode ret;
	mng_uint8 mColortype;
	if (!MNG_ptr) return;

	privdata = (MNG_userdatap)mng_get_userdata(MNG_ptr);

	if (privdata->pal)
	{
		mColortype = MNG_COLORTYPE_INDEXED;
	}
	else
	{
		mColortype = MNG_COLORTYPE_RGB;
	}

	ret = mng_putchunk_ihdr(MNG_ptr, vid.width, vid.height,
	      MNG_BITDEPTH_8, mColortype, MNG_COMPRESSION_DEFLATE,
		   MNG_FILTER_ADAPTIVE, MNG_INTERLACE_NONE);

	if (privdata->pal)
		ret = mng_putchunk_plte (MNG_ptr, 0, privdata->pal);

	ret = mng_putchunk_idat(MNG_ptr,0,NULL); //RAW?
	ret = mng_putchunk_iend(MNG_ptr);
}

boolean M_CloseMNG(void)
{
	mng_ptr privdata;
	if (!MNG_ptr) return false;

	privdata = mng_get_userdata(MNG_ptr);
	if (privdata)
		free(privdata);

	mng_putchunk_mend(MNG_ptr);
	mng_write(MNG_ptr);
	mng_cleanup(&MNG_ptr);

	CONS_Printf("MNG movie saved\n");
	return false;
}

#endif

// ==========================================================================
//                            SCREEN SHOTS
// ==========================================================================
#ifdef USE_PNG
static void PNG_error(png_structp PNG, png_const_charp text)
{
	CONS_Printf("libpng error at %p: %s", PNG,text);
	//I_Error("libpng error at %p: %s", PNG,text);
}

static void PNG_warn(png_structp PNG, png_const_charp text)
{
	CONS_Printf("libpng warning at %p: %s", PNG, text);
}

/** Writes a PNG file to disk.
  *
  * \param filename Filename to write to.
  * \param data     The image data.
  * \param width    Width of the picture.
  * \param height   Height of the picture.
  * \param palette  Palette of image data
  *  \note if palette is NULL, BGR888 format
  */
boolean M_SavePNG(const char *filename, void *data, int width, int height, const byte *palette)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	const int png_interlace = PNG_INTERLACE_NONE; //PNG_INTERLACE_ADAM7
	size_t i; //png_size_t
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif
	png_FILE_p png_FILE;

	png_FILE = fopen(filename,"wb");
	if (!png_FILE)
	{
		CONS_Printf("M_SavePNG: Error on opening %s for write\n", filename);
		return false;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
	 PNG_error, PNG_warn);
	if (!png_ptr)
	{
		CONS_Printf("M_SavePNG: Error on initialize libpng\n");
		fclose(png_FILE);
		remove(filename);
		return false;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		CONS_Printf("M_SavePNG: Error on allocate for libpng\n");
		png_destroy_write_struct(&png_ptr,  NULL);
		fclose(png_FILE);
		remove(filename);
		return false;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		//CONS_Printf("libpng write error on %s\n", filename);
		png_destroy_write_struct(&png_ptr, &png_info_ptr);
		fclose(png_FILE);
		remove(filename);
		return false;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr),jmpbuf, sizeof (jmp_buf));
#endif
	png_init_io(png_ptr, png_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, MAXVIDWIDTH, MAXVIDHEIGHT);
#endif

	//png_set_filter(png_ptr, 0, PNG_ALL_FILTERS);

	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
	png_set_compression_window_bits(png_ptr, 15);
	png_set_compression_mem_level(png_ptr, 7);
	png_set_compression_method(png_ptr, 8);
#ifdef Z_RLE
	//png_set_compression_strategy(png_ptr, Z_RLE);
#endif

	if (palette)
	{
		png_colorp png_PLTE = png_malloc(png_ptr, sizeof(png_color)*256); //palette
		const png_byte *pal = palette;
		for (i = 0; i < 256; i++)
		{
			png_PLTE[i].red   = *pal; pal++;
			png_PLTE[i].green = *pal; pal++;
			png_PLTE[i].blue  = *pal; pal++;
		}
		png_set_IHDR(png_ptr, png_info_ptr, width, height, 8, PNG_COLOR_TYPE_PALETTE,
		 png_interlace, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_set_PLTE(png_ptr, png_info_ptr, png_PLTE, 256);
		png_free(png_ptr, (png_voidp)png_PLTE); // safe in libpng-1.2.1+
		png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_NONE);
		png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
	}
	else
	{
		png_set_IHDR(png_ptr, png_info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
		 png_interlace, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
#ifdef PNG_WRITE_BGR_SUPPORTED
		if (rendermode == render_glide) png_set_bgr(png_ptr);
#endif
		png_set_compression_strategy(png_ptr, Z_FILTERED);
	}

	{
#ifdef PNG_TEXT_SUPPORTED
#define SRB2PNGTXT 5
		png_text png_infotext[SRB2PNGTXT];
		char keytxt[SRB2PNGTXT][12] = {"Title", "Author", "Description", "Playername", "Interface"}; //PNG_KEYWORD_MAX_LENGTH(79) is the max
		char titletxt[] = "Sonic Robo Blast 2"VERSIONSTRING;
		png_charp authortxt = I_GetUserName();
		png_charp playertxt =  cv_playername.zstring;
		char desctxt[] = "SRB2 Screenshot";
		char interfacetxt[] =
#ifdef SDL
		"SDL";
#elif defined (_WINDOWS)
		"DirectX";
#elif defined (PC_DOS)
		"Allegro";
#else
		"Unknown";
#endif
		png_memset(png_infotext,0x00,sizeof (png_infotext));

		for (i = 0; i < SRB2PNGTXT; i++)
			png_infotext[i].key  = keytxt[i];

		png_infotext[0].text = titletxt;
		png_infotext[1].text = authortxt;
		png_infotext[2].text = desctxt;
		png_infotext[3].text = playertxt;
		png_infotext[4].text = interfacetxt;

		png_set_text(png_ptr, png_info_ptr, png_infotext, SRB2PNGTXT);
#undef SRB2PNGTXT
#endif
		png_write_info(png_ptr, png_info_ptr);
	}

	{
		png_uint_32 pitch = png_get_rowbytes(png_ptr, png_info_ptr);
		png_bytep png_buf = (png_bytep)data;
		png_bytepp row_pointers = png_malloc(png_ptr, height* sizeof (png_bytep));
		png_int_32 y;
		for (y = 0; y < height; y++)
		{
			row_pointers[y] = png_buf;
			png_buf += pitch;
		}
		png_write_image(png_ptr, row_pointers);
		png_free(png_ptr, (png_voidp)row_pointers);
	}

	png_write_end(png_ptr, png_info_ptr);
	png_destroy_write_struct(&png_ptr, &png_info_ptr);

	fclose(png_FILE);
	return true;
}
#else
/** PCX file structure.
  */
typedef struct
{
	char manufacturer;
	char version;
	char encoding;
	char bits_per_pixel;

	unsigned short xmin, ymin;
	unsigned short xmax, ymax;
	unsigned short hres, vres;
	unsigned char palette[48];

	char reserved;
	char color_planes;
	unsigned short bytes_per_line;
	unsigned short palette_type;

	char filler[58];
	unsigned char data; ///< Unbounded; used for all picture data.
} pcx_t;

/** Writes a PCX file to disk.
  *
  * \param filename Filename to write to.
  * \param data     The image data.
  * \param width    Width of the picture.
  * \param height   Height of the picture.
  * \param palette  Palette of image data
  */
#if !defined (DC) && !defined (_WIN32_WCE) && !defined (PSP)
static boolean WritePCXfile(const char *filename, const byte *data, int width, int height, const byte *palette)
{
	int i;
	size_t length;
	pcx_t *pcx;
	byte *pack;

	pcx = Z_Malloc(width*height*2 + 1000, PU_STATIC, NULL);

	pcx->manufacturer = 0x0a; // PCX id
	pcx->version = 5; // 256 color
	pcx->encoding = 1; // uncompressed
	pcx->bits_per_pixel = 8; // 256 color
	pcx->xmin = pcx->ymin = 0;
	pcx->xmax = SHORT(width - 1);
	pcx->ymax = SHORT(height - 1);
	pcx->hres = SHORT(width);
	pcx->vres = SHORT(height);
	memset(pcx->palette, 0, sizeof (pcx->palette));
	pcx->reserved = 0;
	pcx->color_planes = 1; // chunky image
	pcx->bytes_per_line = SHORT(width);
	pcx->palette_type = SHORT(1); // not a grey scale
	memset(pcx->filler, 0, sizeof (pcx->filler));

	// pack the image
	pack = &pcx->data;

	for (i = 0; i < width*height; i++)
	{
		if ((*data & 0xc0) != 0xc0)
			*pack++ = *data++;
		else
		{
			*pack++ = 0xc1;
			*pack++ = *data++;
		}
	}

	// write the palette
	*pack++ = 0x0c; // palette ID byte
	for (i = 0; i < 768; i++)
		*pack++ = *palette++;

	// write output file
	length = pack - (byte *)pcx;
	i = FIL_WriteFile(filename, pcx, length);

	Z_Free(pcx);
	return i;
}
#endif
#endif

/** Takes a screenshot.
  * The screenshot is saved as "srb2xxxx.pcx" (or "srb2xxxx.tga" in hardware
  * rendermode) where xxxx is the lowest four-digit number for which a file
  * does not already exist.
  *
  * \sa HWR_ScreenShot
  */
void M_ScreenShot(void)
{
#if !defined (DC) && !defined (_WIN32_WCE) && !defined (PSP)
	const char *freename = NULL, *pathname = ".";
	boolean ret = false;
	byte *linear = NULL;

	if (cv_screenshot_option.value == 0)
		pathname = usehome ? srb2home : srb2path;
	else if (cv_screenshot_option.value == 1)
		pathname = srb2home;
	else if (cv_screenshot_option.value == 2)
		pathname = srb2path;
	else if (cv_screenshot_option.value == 3 && *cv_screenshot_folder.string != '\0')
		pathname = cv_screenshot_folder.string;

#ifdef USE_PNG
	if (rendermode != render_none)
		freename = Newsnapshotfile(pathname,"png");
#else
	if (rendermode == render_soft)
		freename = Newsnapshotfile(pathname,"pcx");
	else if (rendermode != render_none)
		freename = Newsnapshotfile(pathname,"tga");
#endif
	else
		I_Error("Can't take a screenshot without a render system");

	if (rendermode == render_soft)
	{
		// munge planar buffer to linear
		linear = screens[2];
		I_ReadScreen(linear);
	}

	if (!freename)
		goto failure;

		// save the pcx file
#ifdef HWRENDER
	if (rendermode != render_soft)
		ret = HWR_Screenshot(va(pandf,pathname,freename));
	else
#endif
	if (rendermode != render_none)
	{
#ifdef USE_PNG
		ret = M_SavePNG(va(pandf,pathname,freename), linear, vid.width, vid.height,
			W_CacheLumpName("PLAYPAL", PU_CACHE));
#else
		ret = WritePCXfile(va(pandf,pathname,freename), linear, vid.width, vid.height,
			W_CacheLumpName("PLAYPAL", PU_CACHE));
#endif
	}

failure:
	if (ret)
	{
		if (!moviemode)
			CONS_Printf("screen shot %s saved in %s\n", freename, pathname);
	}
	else
	{
		if (freename)
			CONS_Printf("Couldn't create screen shot %s in %s\n", freename, pathname);
		else
			CONS_Printf("Couldn't create screen shot (all 10000 slots used!) in %s\n", pathname);
	}
#endif
}

// ==========================================================================
//                        MISC STRING FUNCTIONS
// ==========================================================================

/** Returns a temporary string made out of varargs.
  * For use with CONS_Printf().
  *
  * \param format Format string.
  * \return Pointer to a static buffer of 1024 characters, containing the
  *         resulting string.
  */
char *va(const char *format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

/** Creates a string in the first argument that is the second argument followed
  * by the third argument followed by the first argument.
  * Useful for making filenames with full path. s1 = s2+s3+s1
  *
  * \param s1 First string, suffix, and destination.
  * \param s2 Second string. Ends up first in the result.
  * \param s3 Third string. Ends up second in the result.
  */
void strcatbf(char *s1, const char *s2, const char *s3)
{
	char tmp[1024];

	strcpy(tmp, s1);
	strcpy(s1, s2);
	strcat(s1, s3);
	strcat(s1, tmp);
}

#if defined (__GNUC__) && defined (__i386__) // from libkwave, under GPL
// Alam: note libkwave memcpy code comes from mplayer's libvo/aclib_template.c, r699

/* for small memory blocks (<256 bytes) this version is faster */
#define small_memcpy(dest,src,n)\
{\
register unsigned long int dummy;\
__asm__ __volatile__(\
	"rep; movsb"\
	:"=&D"(dest), "=&S"(src), "=&c"(dummy)\
	:"0" (dest), "1" (src),"2" (n)\
	: "memory");\
}
/* linux kernel __memcpy (from: /include/asm/string.h) */
ATTRINLINE static FUNCINLINE void *__memcpy (void *dest, const void * src, size_t n)
{
	int d0, d1, d2;

	if ( n < 4 )
	{
		small_memcpy(dest, src, n);
	}
	else
	{
		__asm__ __volatile__ (
			"rep ; movsl;"
			"testb $2,%b4;"
			"je 1f;"
			"movsw;"
			"1:\ttestb $1,%b4;"
			"je 2f;"
			"movsb;"
			"2:"
		: "=&c" (d0), "=&D" (d1), "=&S" (d2)
		:"0" (n/4), "q" (n),"1" ((long) dest),"2" ((long) src)
		: "memory");
	}

	return dest;
}

#define SSE_MMREG_SIZE 16
#define MMX_MMREG_SIZE 8

#define MMX1_MIN_LEN 0x800  /* 2K blocks */
#define MIN_LEN 0x40  /* 64-byte blocks */

/* SSE note: i tried to move 128 bytes a time instead of 64 but it
didn't make any measureable difference. i'm using 64 for the sake of
simplicity. [MF] */
static void *sse_cpy(void * dest, const void * src, size_t n)
{
	void *retval = dest;
	size_t i;

	/* PREFETCH has effect even for MOVSB instruction ;) */
	__asm__ __volatile__ (
		"prefetchnta (%0);"
		"prefetchnta 32(%0);"
		"prefetchnta 64(%0);"
		"prefetchnta 96(%0);"
		"prefetchnta 128(%0);"
		"prefetchnta 160(%0);"
		"prefetchnta 192(%0);"
		"prefetchnta 224(%0);"
		"prefetchnta 256(%0);"
		"prefetchnta 288(%0);"
		: : "r" (src) );

	if (n >= MIN_LEN)
	{
		register unsigned long int delta;
		/* Align destinition to MMREG_SIZE -boundary */
		delta = ((unsigned long int)dest)&(SSE_MMREG_SIZE-1);
		if (delta)
		{
			delta=SSE_MMREG_SIZE-delta;
			n -= delta;
			small_memcpy(dest, src, delta);
		}
		i = n >> 6; /* n/64 */
		n&=63;
		if (((unsigned long)src) & 15)
		/* if SRC is misaligned */
		 for (; i>0; i--)
		 {
			__asm__ __volatile__ (
				"prefetchnta 320(%0);"
				"prefetchnta 352(%0);"
				"movups (%0), %%xmm0;"
				"movups 16(%0), %%xmm1;"
				"movups 32(%0), %%xmm2;"
				"movups 48(%0), %%xmm3;"
				"movntps %%xmm0, (%1);"
				"movntps %%xmm1, 16(%1);"
				"movntps %%xmm2, 32(%1);"
				"movntps %%xmm3, 48(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = (const unsigned char *)src + 64;
			dest = (unsigned char *)dest + 64;
		}
		else
			/*
			   Only if SRC is aligned on 16-byte boundary.
			   It allows to use movaps instead of movups, which required data
			   to be aligned or a general-protection exception (#GP) is generated.
			*/
		 for (; i>0; i--)
		 {
			__asm__ __volatile__ (
				"prefetchnta 320(%0);"
				"prefetchnta 352(%0);"
				"movaps (%0), %%xmm0;"
				"movaps 16(%0), %%xmm1;"
				"movaps 32(%0), %%xmm2;"
				"movaps 48(%0), %%xmm3;"
				"movntps %%xmm0, (%1);"
				"movntps %%xmm1, 16(%1);"
				"movntps %%xmm2, 32(%1);"
				"movntps %%xmm3, 48(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = ((const unsigned char *)src) + 64;
			dest = ((unsigned char *)dest) + 64;
		}
		/* since movntq is weakly-ordered, a "sfence"
		 * is needed to become ordered again. */
		__asm__ __volatile__ ("sfence":::"memory");
		/* enables to use FPU */
		__asm__ __volatile__ ("emms":::"memory");
	}
	/*
	 *	Now do the tail of the block
	 */
	if (n) __memcpy(dest, src, n);
	return retval;
}

static void *mmx2_cpy(void *dest, const void *src, size_t n)
{
	void *retval = dest;
	size_t i;

	/* PREFETCH has effect even for MOVSB instruction ;) */
	__asm__ __volatile__ (
		"prefetchnta (%0);"
		"prefetchnta 32(%0);"
		"prefetchnta 64(%0);"
		"prefetchnta 96(%0);"
		"prefetchnta 128(%0);"
		"prefetchnta 160(%0);"
		"prefetchnta 192(%0);"
		"prefetchnta 224(%0);"
		"prefetchnta 256(%0);"
		"prefetchnta 288(%0);"
	: : "r" (src));

	if (n >= MIN_LEN)
	{
		register unsigned long int delta;
		/* Align destinition to MMREG_SIZE -boundary */
		delta = ((unsigned long int)dest)&(MMX_MMREG_SIZE-1);
		if (delta)
		{
			delta=MMX_MMREG_SIZE-delta;
			n -= delta;
			small_memcpy(dest, src, delta);
		}
		i = n >> 6; /* n/64 */
		n&=63;
		for (; i>0; i--)
		{
			__asm__ __volatile__ (
				"prefetchnta 320(%0);"
				"prefetchnta 352(%0);"
				"movq (%0), %%mm0;"
				"movq 8(%0), %%mm1;"
				"movq 16(%0), %%mm2;"
				"movq 24(%0), %%mm3;"
				"movq 32(%0), %%mm4;"
				"movq 40(%0), %%mm5;"
				"movq 48(%0), %%mm6;"
				"movq 56(%0), %%mm7;"
				"movntq %%mm0, (%1);"
				"movntq %%mm1, 8(%1);"
				"movntq %%mm2, 16(%1);"
				"movntq %%mm3, 24(%1);"
				"movntq %%mm4, 32(%1);"
				"movntq %%mm5, 40(%1);"
				"movntq %%mm6, 48(%1);"
				"movntq %%mm7, 56(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = ((const unsigned char *)src) + 64;
			dest = ((unsigned char *)dest) + 64;
		}
		/* since movntq is weakly-ordered, a "sfence"
		* is needed to become ordered again. */
		__asm__ __volatile__ ("sfence":::"memory");
		__asm__ __volatile__ ("emms":::"memory");
	}
	/*
	 *	Now do the tail of the block
	 */
	if (n) __memcpy(dest, src, n);
	return retval;
}

static void *mmx1_cpy(void *dest, const void *src, size_t n) //3DNOW
{
	void *retval = dest;
	size_t i;

	/* PREFETCH has effect even for MOVSB instruction ;) */
	__asm__ __volatile__ (
		"prefetch (%0);"
		"prefetch 32(%0);"
		"prefetch 64(%0);"
		"prefetch 96(%0);"
		"prefetch 128(%0);"
		"prefetch 160(%0);"
		"prefetch 192(%0);"
		"prefetch 224(%0);"
		"prefetch 256(%0);"
		"prefetch 288(%0);"
	: : "r" (src));

	if (n >= MMX1_MIN_LEN)
	{
		register unsigned long int delta;
		/* Align destinition to MMREG_SIZE -boundary */
		delta = ((unsigned long int)dest)&(MMX_MMREG_SIZE-1);
		if (delta)
		{
			delta=MMX_MMREG_SIZE-delta;
			n -= delta;
			small_memcpy(dest, src, delta);
		}
		i = n >> 6; /* n/64 */
		n&=63;
		for (; i>0; i--)
		{
			__asm__ __volatile__ (
				"prefetch 320(%0);"
				"prefetch 352(%0);"
				"movq (%0), %%mm0;"
				"movq 8(%0), %%mm1;"
				"movq 16(%0), %%mm2;"
				"movq 24(%0), %%mm3;"
				"movq 32(%0), %%mm4;"
				"movq 40(%0), %%mm5;"
				"movq 48(%0), %%mm6;"
				"movq 56(%0), %%mm7;"
				"movq %%mm0, (%1);"
				"movq %%mm1, 8(%1);"
				"movq %%mm2, 16(%1);"
				"movq %%mm3, 24(%1);"
				"movq %%mm4, 32(%1);"
				"movq %%mm5, 40(%1);"
				"movq %%mm6, 48(%1);"
				"movq %%mm7, 56(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = ((const unsigned char *)src) + 64;
			dest = ((unsigned char *)dest) + 64;
		}
		__asm__ __volatile__ ("femms":::"memory"); // same as mmx_cpy() but with a femms
	}
	/*
	 *	Now do the tail of the block
	 */
	if (n) __memcpy(dest, src, n);
	return retval;
}
#endif

static void *mmx_cpy(void *dest, const void *src, size_t n)
{
#if defined (_MSC_VER) && !defined (_WIN32_WCE)
	_asm
	{
		mov ecx, [n]
		mov esi, [src]
		mov edi, [dest]
		shr ecx, 6 // mit mmx: 64bytes per iteration
		jz lower_64 // if lower than 64 bytes
		loop_64: // MMX transfers multiples of 64bytes
		movq mm0,  0[ESI] // read sources
		movq mm1,  8[ESI]
		movq mm2, 16[ESI]
		movq mm3, 24[ESI]
		movq mm4, 32[ESI]
		movq mm5, 40[ESI]
		movq mm6, 48[ESI]
		movq mm7, 56[ESI]

		movq  0[EDI], mm0 // write destination
		movq  8[EDI], mm1
		movq 16[EDI], mm2
		movq 24[EDI], mm3
		movq 32[EDI], mm4
		movq 40[EDI], mm5
		movq 48[EDI], mm6
		movq 56[EDI], mm7

		add esi, 64
		add edi, 64
		dec ecx
		jnz loop_64
		emms // close mmx operation
		lower_64:// transfer rest of buffer
		mov ebx,esi
		sub ebx,src
		mov ecx,[n]
		sub ecx,ebx
		shr ecx, 3 // multiples of 8 bytes
		jz lower_8
		loop_8:
		movq  mm0, [esi] // read source
		movq [edi], mm0 // write destination
		add esi, 8
		add edi, 8
		dec ecx
		jnz loop_8
		emms // close mmx operation
		lower_8:
		mov ebx,esi
		sub ebx,src
		mov ecx,[n]
		sub ecx,ebx
		rep movsb
		mov eax, [dest] // return dest
	}
#elif defined (__GNUC__) && defined (__i386__)
	void *retval = dest;
	size_t i;

	if (n >= MMX1_MIN_LEN)
	{
		register unsigned long int delta;
		/* Align destinition to MMREG_SIZE -boundary */
		delta = ((unsigned long int)dest)&(MMX_MMREG_SIZE-1);
		if (delta)
		{
			delta=MMX_MMREG_SIZE-delta;
			n -= delta;
			small_memcpy(dest, src, delta);
		}
		i = n >> 6; /* n/64 */
		n&=63;
		for (; i>0; i--)
		{
			__asm__ __volatile__ (
				"movq (%0), %%mm0;"
				"movq 8(%0), %%mm1;"
				"movq 16(%0), %%mm2;"
				"movq 24(%0), %%mm3;"
				"movq 32(%0), %%mm4;"
				"movq 40(%0), %%mm5;"
				"movq 48(%0), %%mm6;"
				"movq 56(%0), %%mm7;"
				"movq %%mm0, (%1);"
				"movq %%mm1, 8(%1);"
				"movq %%mm2, 16(%1);"
				"movq %%mm3, 24(%1);"
				"movq %%mm4, 32(%1);"
				"movq %%mm5, 40(%1);"
				"movq %%mm6, 48(%1);"
				"movq %%mm7, 56(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = ((const unsigned char *)src) + 64;
			dest = ((unsigned char *)dest) + 64;
		}
		__asm__ __volatile__ ("emms":::"memory");
	}
	/*
	 *	Now do the tail of the block
	 */
	if (n) __memcpy(dest, src, n);
	return retval;
#else
	return memcpy(dest, src, n);
#endif
}

// Alam: why? memcpy may be __cdecl/_System and our code may be not the same type
static void *cpu_cpy(void *dest, const void *src, size_t n)
{
	return memcpy(dest, src, n);
}

void *(*M_Memcpy)(void* dest, const void* src, size_t n) = cpu_cpy;

/** Memcpy that uses MMX, 3DNow, MMXExt or even SSE
  * Do not use on overlapped memory, use memmove for that
  */
void M_SetupMemcpy(void)
{
#if defined (__GNUC__) && defined (__i386__)
	if (R_SSE2)
		M_Memcpy = sse_cpy;
	else if (R_MMXExt)
		M_Memcpy = mmx2_cpy;
	else if (R_3DNow)
		M_Memcpy = mmx1_cpy;
	else
#endif
	if (R_MMX)
		M_Memcpy = mmx_cpy;
}
