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
/// \brief SRB2 main program
///
///	SRB2 main program (D_SRB2Main) and game loop (D_SRB2Loop),
///	plus functions to parse command line parameters, configure game
///	parameters, and call the startup functions.

#ifdef UNIXLIKE
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __GNUC__
#include <unistd.h> // for getcwd
#endif

#ifdef PC_DOS
#include <stdio.h> // for snprintf
int	snprintf(char *str, size_t n, const char *fmt, ...);
//int	vsnprintf(char *str, size_t n, const char *fmt, va_list ap);
#endif

#if (defined (_WIN32) && !defined (_WIN32_WCE)) && !defined (_XBOX)
#include <direct.h>
#include <malloc.h>
#endif

#if !defined (UNDER_CE)
#include <time.h>
#elif defined (_XBOX)
#define NO_TIME
#endif

#include "doomdef.h"
#include "am_map.h"
#include "console.h"
#include "d_net.h"
#include "dstrings.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_sound.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "r_main.h"
#include "r_local.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "d_main.h"
#include "d_netfil.h"
#include "m_cheat.h"
#include "y_inter.h"
#include "p_local.h" // chasecam
#include "mserv.h" // cv_internetserver

#ifdef _XBOX
#include "sdl/SRB2XBOX/xboxhelp.h"
#endif

#ifdef HWRENDER
#include "hardware/hw_main.h" // 3D View Rendering
#endif

#ifdef _WINDOWS
#include "win32/win_main.h" // I_DoStartupMouse
#endif

#include "hardware/hw3sound.h"

//
// DEMO LOOP
//
//static int demosequence;
static int pagetic = 0;
static const char *pagename = "MAP1PIC";
static char *startupwadfiles[MAX_WADFILES];

boolean devparm = false; // started game with -devparm
byte xmasmode = false; // Xmas Mode
byte xmasoverride = false;
byte eastermode = false; // Easter mode
boolean mariomode = false; // Mario Mode

boolean singletics = false; // timedemo
boolean lastdraw = false;

postimg_t postimgtype = postimg_none;
int postimgparam;

#ifdef _XBOX
boolean nomusic = true, nosound = true;
boolean nofmod = true;
#else
boolean nomusic = false, nosound = false;
boolean nofmod = false; // No fmod-based music
#endif

// These variables are only true if
// the respective sound system is initialized
// and active, but no sounds/music should play.
boolean music_disabled = false;
boolean sound_disabled = false;
boolean digital_disabled = false;

boolean advancedemo;
#ifdef DEBUGFILE
int debugload = 0;
#endif

#ifdef _arch_dreamcast
char srb2home[256] = "/cd";
char srb2path[256] = "/cd";
#else
char srb2home[256] = ".";
char srb2path[256] = ".";
#endif
boolean usehome = true;
const char *pandf = "%s" PATHSEP "%s";

//
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
// referenced from i_system.c for I_GetKey()

event_t events[MAXEVENTS];
int eventhead, eventtail;

boolean dedicated = false;

//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void D_PostEvent(const event_t *ev)
{
	events[eventhead] = *ev;
	eventhead = (eventhead+1) & (MAXEVENTS-1);
}
// just for lock this function
#ifndef DOXYGEN
void D_PostEvent_end(void) {};
#endif

//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
void D_ProcessEvents(void)
{
	event_t *ev;

	for (; eventtail != eventhead; eventtail = (eventtail+1) & (MAXEVENTS-1))
	{
		ev = &events[eventtail];

		if (gameaction == ga_nothing
			&& gamestate == GS_TITLESCREEN)
		{
			if (cht_Responder(ev))
				continue;
		}

		// Menu input
		if (M_Responder(ev))
			continue; // menu ate the event

		// console input
		if (CON_Responder(ev))
			continue; // ate the event

		G_Responder(ev);
	}
}

//
// D_Display
// draw current display, possibly wiping it from the previous
//

// wipegamestate can be set to -1 to force a wipe on the next draw
// added comment : there is a wipe eatch change of the gamestate
gamestate_t wipegamestate = GS_DEMOSCREEN;

static void D_Display(void)
{
	static boolean menuactivestate = false;
	static gamestate_t oldgamestate = -1;
	tic_t nowtime, tics, wipestart, y;
	boolean done, redrawsbar = false, viewactivestate = false;
	static boolean wipe = false;

	if (dedicated)
		return;

	if (nodrawers)
		return; // for comparative timing/profiling

	// check for change of screen size (video mode)
	if (setmodeneeded && !wipe)
		SCR_SetMode(); // change video mode

	if (vid.recalc)
		SCR_Recalc(); // NOTE! setsizeneeded is set by SCR_Recalc()

	// change the view size if needed
	if (setsizeneeded)
	{
		R_ExecuteSetViewSize();
		oldgamestate = -1; // force background redraw
		redrawsbar = true;
	}

	// save the current screen if about to wipe
	if (gamestate != wipegamestate)
	{
		wipe = true;

		if (rendermode == render_soft)
			F_WipeStartScreen();
	}
	else
		wipe = false;

	// Hardware mode does not fade wipe.
	// Thus, don't delay it unless needed for synchronisity.
	if (rendermode != render_soft /*&& !netgame*/)
	{
		if (gamestate != GS_INTRO
		&& gamestate != GS_INTRO2
		&& gamestate != GS_CUTSCENE)
			wipe = false;
	}

	// draw buffered stuff to screen
	// Used only by linux GGI version
	I_UpdateNoBlit();

	// Fade to black first
	if (rendermode == render_soft)
	{
		if (wipe)
		{
			if (!(mapheaderinfo[gamemap-1].interscreen[0] == '#'
				&& gamestate == GS_INTERMISSION))
				V_DrawFill(0, 0, vid.width, vid.height, 31);
			F_WipeEndScreen(0, 0, vid.width, vid.height);

			wipestart = I_GetTime() - 1;
			y = wipestart + 2*TICRATE; // init a timeout
			do
			{
				do
				{
					nowtime = I_GetTime();
					tics = nowtime - wipestart;
					if (!tics) I_Sleep();
				} while (!tics);
				wipestart = nowtime;
				done = F_ScreenWipe(0, 0, vid.width, vid.height, tics);
				I_OsPolling();
				I_UpdateNoBlit();
				if (gamestate != GS_TIMEATTACK)
					M_Drawer(); // menu is drawn even on top of wipes
				I_FinishUpdate(); // page flip or blit buffer
			} while (!done && I_GetTime() < y);
			WipeInAction = false;
		}

		F_WipeStartScreen();
	}
	else if (wipe && rendermode != render_none) // Delay the hardware modes as well
	{

		wipestart = I_GetTime() - 1;
		y = wipestart + 32; // init a timeout
		do
		{
			do
			{
				nowtime = I_GetTime();
				tics = nowtime - wipestart;
				if (!tics) I_Sleep();
			} while (!tics);

			I_OsPolling();
			I_UpdateNoBlit();
			if (gamestate != GS_TIMEATTACK)
				M_Drawer(); // menu is drawn even on top of wipes
//			I_FinishUpdate(); // page flip or blit buffer
		} while (I_GetTime() < y);
	}

	// do buffered drawing
	switch (gamestate)
	{
		case GS_LEVEL:
			if (!gametic)
				break;
			HU_Erase();
			if (automapactive)
				AM_Drawer();
			if (wipe || menuactivestate || (rendermode != render_soft && rendermode != render_none) || vid.recalc)
				redrawsbar = true;
			break;

		case GS_INTERMISSION:
			Y_IntermissionDrawer();
			HU_Erase();
			HU_Drawer();
			break;

		case GS_TIMEATTACK:
			break;

		case GS_INTRO:
		case GS_INTRO2:
			F_IntroDrawer();
			break;

		case GS_CUTSCENE:
			F_CutsceneDrawer();
			HU_Erase();
			HU_Drawer();
			break;

		case GS_DEMOEND:
			F_DemoEndDrawer();
			break;

		case GS_EVALUATION:
			F_GameEvaluationDrawer();
			break;

		case GS_CREDITS:
			F_CreditDrawer();
			HU_Erase();
			HU_Drawer();
			break;

		case GS_TITLESCREEN:
			F_TitleScreenDrawer();
			break;

		case GS_DEMOSCREEN:
			D_PageDrawer(pagename);
		case GS_DEDICATEDSERVER:
		case GS_WAITINGPLAYERS:
		case GS_NULL:
			break;
	}

	// Transitions for Introduction
	if (gamestate == GS_INTRO && oldgamestate == GS_INTRO2)
		wipe = true;
	else if (gamestate == GS_INTRO2 && oldgamestate == GS_INTRO)
		wipe = true;

	// clean up border stuff
	// see if the border needs to be initially drawn
	if (gamestate == GS_LEVEL)
	{
		if (oldgamestate != GS_LEVEL)
		{
			viewactivestate = false; // view was not active
#if 0
			R_FillBackScreen(); // draw the pattern into the back screen
#endif
		}

		// draw the view directly
		if (!automapactive && !dedicated && cv_renderview.value)
		{
			if (players[displayplayer].mo)
			{
				topleft = screens[0] + viewwindowy*vid.width + viewwindowx;
#ifdef HWRENDER
				if (rendermode != render_soft)
					HWR_RenderPlayerView(0, &players[displayplayer]);
				else
#endif
				if (rendermode != render_none)
					R_RenderPlayerView(&players[displayplayer]);
			}

			// render the second screen
			if (secondarydisplayplayer != consoleplayer && players[secondarydisplayplayer].mo)
			{
#ifdef HWRENDER
				if (rendermode != render_soft)
					HWR_RenderPlayerView(1, &players[secondarydisplayplayer]);
				else
#endif
				if (rendermode != render_none)
				{
					viewwindowy = vid.height / 2;
					M_Memcpy(ylookup, ylookup2, viewheight*sizeof (ylookup[0]));

					topleft = screens[0] + viewwindowy*vid.width + viewwindowx;

					R_RenderPlayerView(&players[secondarydisplayplayer]);

					viewwindowy = 0;
					M_Memcpy(ylookup, ylookup1, viewheight*sizeof (ylookup[0]));
				}
			}
		}

		// Image postprocessing effect
		if (postimgtype)
			V_DoPostProcessor(postimgtype);

		if (lastdraw)
		{
			if (rendermode == render_soft)
				VID_BlitLinearScreen(screens[0], screens[1], vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.rowbytes);
			lastdraw = false;
		}

		ST_Drawer(redrawsbar);

		HU_Drawer();
	}

	// change gamma if needed
	if (gamestate != oldgamestate && gamestate != GS_LEVEL)
		V_SetPalette(0);

	menuactivestate = menuactive;
	oldgamestate = wipegamestate = gamestate;

	// draw pause pic
	if (paused && (!menuactive || netgame))
	{
		int py;
		patch_t *patch;
		if (automapactive)
			py = 4;
		else
			py = viewwindowy + 4;
		patch = W_CachePatchName("M_PAUSE", PU_CACHE);
		V_DrawScaledPatch(viewwindowx + (BASEVIDWIDTH - patch->width)/2, py, 0, patch);
	}

	// vid size change is now finished if it was on...
	vid.recalc = 0;

	// FIXME: draw either console or menu, not the two
	if (gamestate != GS_TIMEATTACK)
		CON_Drawer();

	M_Drawer(); // menu is drawn even on top of everything
	NetUpdate(); // send out any new accumulation

	// It's safe to end the game now.
	if (G_GetExitGameFlag())
	{
		Command_ExitGame_f();
		G_ClearExitGameFlag();
	}

	//
	// normal update
	//
	if (!wipe)
	{
		if (cv_netstat.value)
		{
			char s[50];
			Net_GetNetStat();
			snprintf(s, sizeof s, "get %d b/s", getbps);
			V_DrawString(BASEVIDWIDTH - V_StringWidth(s), BASEVIDHEIGHT-ST_HEIGHT-40, V_YELLOWMAP, s);
			snprintf(s, sizeof s, "send %d b/s", sendbps);
			V_DrawString(BASEVIDWIDTH - V_StringWidth(s), BASEVIDHEIGHT-ST_HEIGHT-30, V_YELLOWMAP, s);
			snprintf(s, sizeof s, "GameMiss %.2f%%", gamelostpercent);
			V_DrawString(BASEVIDWIDTH - V_StringWidth(s), BASEVIDHEIGHT-ST_HEIGHT-20, V_YELLOWMAP, s);
			snprintf(s, sizeof s, "SysMiss %.2f%%", lostpercent);
			V_DrawString(BASEVIDWIDTH - V_StringWidth(s), BASEVIDHEIGHT-ST_HEIGHT-10, V_YELLOWMAP, s);
		}
		I_FinishUpdate(); // page flip or blit buffer
		return;
	}

	//
	// wipe update
	//

	if (rendermode == render_soft)
	{
		F_WipeEndScreen(0, 0, vid.width, vid.height);

		wipestart = I_GetTime () - 1;
		y = wipestart + 2*TICRATE; // init a timeout
		do
		{
			do
			{
				nowtime = I_GetTime();
				tics = nowtime - wipestart;
				if (!tics) I_Sleep();
			} while (!tics);
			wipestart = nowtime;
			done = F_ScreenWipe(0, 0, vid.width, vid.height, tics);
			I_OsPolling();
			I_UpdateNoBlit();
			if (gamestate != GS_TIMEATTACK)
				M_Drawer(); // menu is drawn even on top of wipes
			I_FinishUpdate(); // page flip or blit buffer
		} while (!done && I_GetTime() < y);
		WipeInAction = false;
	}
#ifdef HWRENDER
	else if (rendermode != render_none) // Delay the hardware modes as well
	{

		wipestart = I_GetTime() - 1;
		y = wipestart + 32; // init a timeout
		do
		{
			do
			{
				nowtime = I_GetTime();
				tics = nowtime - wipestart;
				if (!tics) I_Sleep();
			} while (!tics);

			I_OsPolling();
			I_UpdateNoBlit();
			if (gamestate != GS_TIMEATTACK)
				M_Drawer(); // menu is drawn even on top of wipes
//			I_FinishUpdate(); // page flip or blit buffer
		} while (I_GetTime() < y);
	}
#endif
}

// =========================================================================
// D_SRB2Loop
// =========================================================================

tic_t rendergametic;
boolean supdate;

void D_SRB2Loop(void)
{
	tic_t oldentertics = 0, entertic = 0, realtics = 0, rendertimeout = (tic_t)-1;

	if (demorecording)
		G_BeginRecording();

	// user settings
	if (dedicated)
		COM_BufAddText(va("exec \"%s"PATHSEP"adedserv.cfg\"\n", srb2home));
	else
		COM_BufAddText(va("exec \"%s"PATHSEP"autoexec.cfg\" -noerror\n", srb2home));

	if (dedicated)
		server = true;

	if (M_CheckParm("-voodoo")) // 256x256 Texture Limiter
		COM_BufAddText("gr_voodoocompatibility on\n");

	// Pushing of + parameters is now done back in D_SRB2Main, not here.

	// end of loading screen: CONS_Printf() will no more call FinishUpdate()
	con_startup = false;

	CONS_Printf("I_StartupKeyboard...\n");
	I_StartupKeyboard();

#ifdef _WINDOWS
	CONS_Printf("I_StartupMouse...\n");
	I_DoStartupMouse();
#endif

	oldentertics = I_GetTime();

	// make sure to do a d_display to init mode _before_ load a level
	SCR_SetMode(); // change video mode
	SCR_Recalc();

	for (;;)
	{
		// get real tics
		entertic = I_GetTime();
		realtics = entertic - oldentertics;
		oldentertics = entertic;

#ifdef DEBUGFILE
		if (!realtics)
			if (debugload)
				debugload--;
#endif

		if (!realtics && !singletics)
		{
			I_Sleep();
			continue;
		}

#ifdef HW3SOUND
		HW3S_BeginFrameUpdate();
#endif

		// process tics (but maybe not if realtic == 0)
		TryRunTics(realtics);

		if (lastdraw || singletics || gametic > rendergametic)
		{
			rendergametic = gametic;
			rendertimeout = entertic+TICRATE/17;

			// Update display, next frame, with current state.
			D_Display();
			supdate = false;

			if (moviemode)
			{
#ifdef HAVE_MNG
				M_SaveMNG();
#else
				COM_BufAddText("screenshot");
#endif
			}
		}
		else if (rendertimeout < entertic) // in case the server hang or netsplit
		{
			// Lagless camera! Yay!
			if (gamestate == GS_LEVEL && netgame)
			{
				if (splitscreen && camera2.chase)
					P_MoveChaseCamera(&players[secondarydisplayplayer], &camera2, true);
				if (camera.chase)
					P_MoveChaseCamera(&players[displayplayer], &camera, true);
			}
			D_Display();
		}

		// consoleplayer -> displayplayer (hear sounds from viewpoint)
		S_UpdateSounds(); // move positional sounds

		// check for media change, loop music..
		I_UpdateCD();

#ifdef HW3SOUND
		HW3S_EndFrameUpdate();
#endif
	}
}

//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker(void)
{
	if (--pagetic < 0)
		D_AdvanceDemo();
}

//
// D_PageDrawer: draw a patch supposed to fill the screen,
//               fill the borders with a background pattern (a flat)
//               if the patch doesn't fit all the screen.
//
void D_PageDrawer(const char *lumpname)
{
	byte *src;
	byte *dest;
	int x, y;

	// software mode which uses generally lower resolutions doesn't look
	// good when the pic is scaled, so it fills space aorund with a pattern,
	// and the pic is only scaled to integer multiples (x2, x3...)
	lumpname = NULL;
	if (rendermode == render_soft)
	{
		if ((vid.width > BASEVIDWIDTH) || (vid.height > BASEVIDHEIGHT))
		{
			src = scr_borderpatch;
			dest = screens[0];

			for (y = 0; y < vid.height; y++)
			{
				for (x = 0; x < vid.width/64; x++)
				{
					M_Memcpy(dest, src+((y&63)<<6), 64);
					dest += 64;
				}
				if (vid.width & 63)
				{
					M_Memcpy(dest, src + ((y&63)<<6), vid.width&63);
					dest += (vid.width&63);
				}
			}
		}
	}

	// if you wanna centre the pages it's here.
	// I think it's not so beautiful to have the pic centered,
	// so I leave it in the upper-left corner for now...
	//V_DrawPatch (0,0, 0, W_CachePatchName(pagename, PU_CACHE));
}

//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void D_AdvanceDemo(void)
{
	advancedemo = true;
}

// =========================================================================
// D_SRB2Main
// =========================================================================

//
// D_StartTitle
//
void D_StartTitle(void)
{
	if (netgame)
		return;

	gameaction = ga_nothing;
	playerdeadview = false;
	displayplayer = consoleplayer = 0;
	//demosequence = -1;
	gametype = GT_COOP;
	paused = false;
	advancedemo = false;
	F_StartTitleScreen();
	CON_ToggleOff();

	// Reset the palette
#ifdef HWRENDER
	if ((rendermode == render_opengl) || (rendermode == render_d3d))
		HWR_SetPaletteColor(0);
	else
#endif
	if (rendermode != render_none)
		V_SetPaletteLump("PLAYPAL");
}

//
// D_AddFile
//
static void D_AddFile(const char *file)
{
	size_t pnumwadfiles;
	char *newfile;

	for (pnumwadfiles = 0; startupwadfiles[pnumwadfiles]; pnumwadfiles++)
		;

	newfile = malloc(strlen(file) + 1);
	if (!newfile)
	{
		I_Error("No more free memory to AddFile %s",file);
	}
	strcpy(newfile, file);

	startupwadfiles[pnumwadfiles] = newfile;
}

static inline void D_CleanFile(void)
{
	size_t pnumwadfiles;
	for (pnumwadfiles = 0; startupwadfiles[pnumwadfiles]; pnumwadfiles++)
	{
		free(startupwadfiles[pnumwadfiles]);
		startupwadfiles[pnumwadfiles] = NULL;
	}
}

#ifndef _MAX_PATH
#define _MAX_PATH MAX_WADPATH
#endif

// ==========================================================================
// Identify the SRB2 version, and IWAD file to use.
// ==========================================================================

static void IdentifyVersion(void)
{
	char *srb2wad1, *srb2wad2;
	const char *srb2waddir = NULL;

#if defined (UNIXLIKE) || defined (SDL)
	// change to the directory where 'srb2.srb' is found
	srb2waddir = I_LocateWad();
#endif

	// get the current directory (possible problem on NT with "." as current dir)
	if (srb2waddir)
	{
		strncpy(srb2path,srb2waddir,256);
	}
	else
	{
#ifndef _WIN32_WCE
		if (getcwd(srb2path, 256) != NULL)
			srb2waddir = srb2path;
		else
#endif
		{
#ifdef _arch_dreamcast
			srb2waddir = "/cd";
#else
			srb2waddir = ".";
#endif
		}
	}

#if defined (macintosh) && !defined (SDL)
	// cwd is always "/" when app is dbl-clicked
	if (!stricmp(srb2waddir, "/"))
		srb2waddir = I_GetWadDir();
#endif
	// Commercial.
	srb2wad1 = malloc(strlen(srb2waddir)+1+8+1);
	srb2wad2 = malloc(strlen(srb2waddir)+1+8+1);
	if (srb2wad1 == NULL && srb2wad2 == NULL)
		I_Error("No more free memory to look in %s", srb2waddir);
	if (srb2wad1 != NULL)
		sprintf(srb2wad1, pandf, srb2waddir, text[SRB2SRB_NUM]);
	if (srb2wad2 != NULL)
		sprintf(srb2wad2, pandf, srb2waddir, text[SRB2WAD_NUM]);

	// will be overwritten in case of -cdrom or unix/win home
	snprintf(configfile, sizeof configfile, "%s" PATHSEP CONFIGFILENAME, srb2waddir);

	// Load the IWAD
	if (srb2wad2 != NULL && FIL_ReadFileOK(srb2wad2))
		D_AddFile(srb2wad2);
	else if (srb2wad1 != NULL && FIL_ReadFileOK(srb2wad1))
		D_AddFile(srb2wad1);
	else
		I_Error("SRB2.SRB/SRB2.WAD not found! Expected in %s, ss files: %s and %s\n", srb2waddir, srb2wad1, srb2wad2);

	if (srb2wad1)
		free(srb2wad1);
	if (srb2wad2)
		free(srb2wad2);

	// if you change the ordering of this or add/remove a file, be sure to update the md5
	// checking in D_SRB2Main

	// Add the players
	D_AddFile(va(pandf,srb2waddir,text[SONICPLR_NUM])); //sonic.plr
	D_AddFile(va(pandf,srb2waddir,text[TAILSPLR_NUM])); //tails.plr
	D_AddFile(va(pandf,srb2waddir,text[KNUXPLR_NUM])); //knux.plr

#ifndef _PSP_ // PSPDEV's CRT haves a limit of 16 files opened at a time
	// Add the weapons
	D_AddFile(va(pandf,srb2waddir,"rings.wpn")); //rings.wpn
#endif

	// Add... nights?
	D_AddFile(va(pandf,srb2waddir,"drill.dta")); //drill.dta
	D_AddFile(va(pandf,srb2waddir,"soar.dta")); //soar.dta

	D_AddFile(va(pandf,srb2waddir,"11match.wad")); //11match.wad

#if !defined (SDL) || defined (HAVE_MIXER)
	{
#ifdef DC
		const char *musicfile = "music_dc.dta";
#else
		const char *musicfile = text[MUSICWAD_NUM];
#endif
		const char *musicpath = va(pandf,srb2waddir,musicfile);
		int ms = W_VerifyNMUSlumps(musicpath); // Don't forget the music!
		if (ms == 1)
			D_AddFile(musicpath);
		else if (ms == 0)
			I_Error("File %s has been modified with non-music lumps",musicfile);
	}
#endif

	// Temporary patchfile loading
	//D_AddFile(va(pandf,srb2waddir,"srb2pat.wad"));//srb2pat.wad

}

/* ======================================================================== */
// Just print the nice red titlebar like the original SRB2 for DOS.
/* ======================================================================== */
#ifdef PC_DOS
static inline void D_Titlebar(char *title1, char *title2)
{
	// SRB2 banner
	clrscr();
	textattr((BLUE<<4)+WHITE);
	clreol();
	cputs(title1);

	// standard srb2 banner
	textattr((RED<<4)+WHITE);
	clreol();
	gotoxy((80-strlen(title2))/2, 2);
	cputs(title2);
	normvideo();
	gotoxy(1,3);
}
#endif

//
// Center the title string, then add the date and time of compilation.
//
static inline void D_MakeTitleString(char *s)
{
	char temp[82];
	char *t;
	const char *u;
	int i;

	for (i = 0, t = temp; i < 82; i++)
		*t++=' ';

	for (t = temp + (80-strlen(s))/2, u = s; *u != '\0' ;)
		*t++ = *u++;

	u = compdate;
	for (t = temp + 1, i = 11; i-- ;)
		*t++ = *u++;
	u = comptime;
	for (t = temp + 71, i = 8; i-- ;)
		*t++ = *u++;

	temp[80] = '\0';
	strcpy(s, temp);
}

static inline void D_InitCutsceneInfo(void)
{
	int i,j;

	for (i = 0; i < 128; i++)
		for (j = 0; j < 128; j++)
			cutscenes[i].scene[j].text = NULL;
}

static void D_TimeWad(void)
{
#ifndef NO_TIME
	time_t t1; // Date-checker
	struct tm* tptr; // Date-checker
	// Do special stuff around Christmas time Tails 11-15-2001
	t1 = time(NULL);
	if (t1 != (time_t)-1)
	{
		tptr = localtime(&t1);

		if (tptr)
		{
			if ((tptr->tm_mon == 11 && tptr->tm_mday > 24))
			{
				if (!M_CheckParm("-noxmas")) // Christmas to New Years (by popular demand)
				{
					//D_AddFile("3drend.dll");
					xmasmode = true;
					xmasoverride = true;
					modifiedgame = false;
				}
			}
			else if (tptr->tm_mon == 3) // Easter changes every year, so just have it for all of April
			{
				if (tptr->tm_mday < 2)
					introtoplay = 127; // ???

				eastermode = true;
				modifiedgame = false;
			}
		}
	}
#endif
	if (!eastermode && M_CheckParm("-xmas"))
	{
		//D_AddFile("3drend.dll");
		xmasmode = true;
		xmasoverride = true;
		modifiedgame = false;
	}
#ifdef NO_TIME
	else if (M_CheckParm("-easter"))
	{
		eastermode = true; // Hunt for the eggs!
		modifiedgame = false;
	}
#endif
}

//
// D_SRB2Main
//
void D_SRB2Main(void)
{
	int p;
	char srb2[82]; // srb2 title banner
	char title[82];

	int pstartmap = 1;
	boolean autostart = false;

	// keep error messages until the final flush(stderr)
#if !defined (PC_DOS) && !defined (_WIN32_WCE)
	if (setvbuf(stderr, NULL, _IOFBF, 1000))
		CONS_Printf("setvbuf didnt work\n");
#endif

	// get parameters from a response file (eg: srb2 @parms.txt)
	M_FindResponseFile();

	// MAINCFG is now taken care of where "OBJCTCFG" is handled
	G_LoadGameSettings();

	// identify the main IWAD file to use
	D_TimeWad();
	IdentifyVersion();

#ifndef _WIN32_WCE
	setbuf(stdout, NULL); // non-buffered output
#endif

#if defined (_WIN32_WCE) //|| defined (_DEBUG) || defined (GP2X)
	devparm = !M_CheckParm("-nodebug");
#else
	devparm = M_CheckParm("-debug");
#endif

	// for dedicated server
#if !defined (_WINDOWS) //already check in win_main.c
	dedicated = M_CheckParm("-dedicated") != 0;
#endif

	strcpy(title, "Sonic Robo Blast 2");
	strcpy(srb2, "Sonic Robo Blast 2");
	D_MakeTitleString(srb2);

#ifdef PC_DOS
	D_Titlebar(srb2, title);
#else
	CONS_Printf("%s\n%s\n", srb2, title);
#endif

#if defined (__OS2__) && !defined (SDL)
	// set PM window title
	snprintf(pmData->title, sizeof (pmData->title),
		"Sonic Robo Blast 2" VERSIONSTRING ": %s",
		title);
#endif

	if (devparm)
		CONS_Printf(D_DEVSTR);

	P_ClearMapHeaderInfo();
	D_InitCutsceneInfo();

	// default savegame
	strcpy(savegamename,text[NORM_SAVEI_NUM]);

	{
		const char *userhome = D_Home(); //Alam: path to home

		if (!userhome)
		{
#if defined (UNIXLIKE) && !defined (__CYGWIN__) && !defined (DC) && !defined (PSP) && !defined(GP2X)
			I_Error("Please set $HOME to your home directory\n");
//#elif defined (_WIN32_WCE)
//			if (dedicated)
//				snprintf(configfile, sizeof configfile, "/Storage Card/SRB2DEMO/d"CONFIGFILENAME);
//			else
//				snprintf(configfile, sizeof configfile, "/Storage Card/SRB2DEMO/"CONFIGFILENAME);
#else
			if (dedicated)
				snprintf(configfile, sizeof configfile, "d"CONFIGFILENAME);
			else
				snprintf(configfile, sizeof configfile, CONFIGFILENAME);
#endif
		}
		else
		{
			// use user specific config file
#ifdef DEFAULTDIR
			snprintf(srb2home, sizeof srb2home, "%s" PATHSEP DEFAULTDIR, userhome);
			snprintf(downloaddir, sizeof downloaddir, "%s" PATHSEP "DOWNLOAD", srb2home);
			if (dedicated)
				snprintf(configfile, sizeof configfile, "%s" PATHSEP "d"CONFIGFILENAME, srb2home);
			else
				snprintf(configfile, sizeof configfile, "%s" PATHSEP CONFIGFILENAME, srb2home);

			// can't use sprintf since there is %u in savegamename
			strcatbf(savegamename, srb2home, PATHSEP);

			I_mkdir(srb2home, 0700);
#else
			snprintf(srb2home, sizeof srb2home, "%s", userhome);
			snprintf(downloaddir, sizeof downloaddir, "%s", userhome);
			if (dedicated)
				snprintf(configfile, sizeof configfile, "%s" PATHSEP "d"CONFIGFILENAME, userhome);
			else
				snprintf(configfile, sizeof configfile, "%s" PATHSEP CONFIGFILENAME, userhome);

			// can't use sprintf since there is %u in savegamename
			strcatbf(savegamename, userhome, PATHSEP);
#endif
		}
#ifdef _arch_dreamcast
	strcpy(downloaddir, "/ram"); // the dreamcast's TMP
#endif
	}

	if (M_CheckParm("-password") && M_IsNextParm())
	{
		char *pw = M_GetNextParm();
		strncpy(adminpassword, pw, 8);
		if (strlen(pw) < 8)
		{
			size_t z;
			for (z = strlen(pw); z < 8; z++)
				adminpassword[z] = 'a';
		}
	}
	else
	{
		int z;
		srand((unsigned int)time(NULL));
		for (z = 0; z < 8; z++)
			adminpassword[z] = (char)(rand() & 127);
	}
	adminpassword[8] = '\0';

	// get skill / map from parms

	if (M_CheckParm("-server") || dedicated)
		netgame = server = true;

	if (M_CheckParm("-skill") && M_IsNextParm())
	{
		if (modifiedgame || netgame)
		{
			gameskill = M_GetNextParm()[0]-'0';
			autostart = true;
		}
	}

	if (M_CheckParm("-warp") && M_IsNextParm())
	{
		if ((modifiedgame && !savemoddata) || netgame)
		{
			pstartmap = atoi(M_GetNextParm());
			autostart = true;
			savemoddata = false;
		}
	}

	CONS_Printf(text[Z_INIT_NUM]);
	Z_Init();

	// adapt tables to SRB2's needs, including extra slots for dehacked file support
	P_PatchInfoTables();

	CONS_Printf(text[W_INIT_NUM]);

	//---------------------------------------------------- READY TIME
	// we need to check for dedicated before initialization of some subsystems

	CONS_Printf("I_StartupTimer...\n");
	I_StartupTimer();

	// Make backups of some SOCcable tables.
	P_BackupTables();

	// load wad, including the main wad file
	if (!W_InitMultipleFiles(startupwadfiles))
		CONS_Error("A WAD file was not found or not valid\n");
	D_CleanFile();

	// Check MD5s of autoloaded files
	W_VerifyFileMD5(0, "64c46c0b83eb049848a21a6de700a123"); // srb2.srb
	W_VerifyFileMD5(1, "9f5e4ffe1b9599742c9fd97a18bd2c07"); // sonic.plr
	W_VerifyFileMD5(2, "2e4581829338e31e20577af0b82804b6"); // tails.plr
	W_VerifyFileMD5(3, "822f064fde3b3a5fed834ea615d697ad"); // knux.plr
	W_VerifyFileMD5(4, "dea5b132a46bbdea7be7fb5eeb613cc8"); // rings.wpn
	W_VerifyFileMD5(5, "6b1cf9b41e41a46ac58606dc6e7c9e05"); // drill.dta
	W_VerifyFileMD5(6, "8d080c050ecf03691562aa7b60156fec"); // soar.dta
	W_VerifyFileMD5(7, "52c40e1911c5d8e58ba6cc25b28c25bf"); // 11match.wad

	// don't check music.dta because people like to modify it, and it doesn't matter if they do
	// ...except it does if they slip maps in there, and that's what W_VerifyNMUSlumps is for.


	mainwads = 8; // there 12 wads not to unloads

	// check 3drend.dll if needed
	if (xmasmode)
	{
#if 0
		W_VerifyFileMD5(7, "d2dc36f4442d22780f6ee32e7a61c800");
#endif
		mainwads++; // no wait, there one more wad that should not be unloaded?
	}

	// Check and print which version is executed.
	CONS_Printf(text[COMERCIAL_NUM]);

	cht_Init();

	//---------------------------------------------------- READY SCREEN
	// we need to check for dedicated before initialization of some subsystems

	CONS_Printf("I_StartupGraphics...\n");
	I_StartupGraphics();

	//--------------------------------------------------------- CONSOLE
	// setup loading screen
	SCR_Startup();

	// we need the font of the console
	CONS_Printf(text[HU_INIT_NUM]);
	HU_Init();

	COM_Init();
	CON_Init();

	D_RegisterServerCommands();
	D_RegisterClientCommands(); // be sure that this is called before D_CheckNetGame
	R_RegisterEngineStuff();
	S_RegisterSoundStuff();

	//--------------------------------------------------------- CONFIG.CFG
	M_FirstLoadConfig(); // WARNING : this do a "COM_BufExecute()"

	if (!M_CheckParm("-resetdata"))
		G_LoadGameData();

	if (gameskill >= sk_insane && !(grade & 128))
		gameskill = sk_insane-1;

#if defined (UNIXLIKE) || defined (SDL)
	VID_PrepareModeList(); // Regenerate Modelist according to cv_fullscreen
#endif

	// set user default mode or mode set at cmdline
	SCR_CheckDefaultMode();

	wipegamestate = gamestate;

	P_InitMapHeaders();
	savedata.lives = 0; // flag this as not-used

	//------------------------------------------------ COMMAND LINE PARAMS

	// Initialize CD-Audio
	if (M_CheckParm("-usecd") && !dedicated)
		I_InitCD();

	if (M_CheckParm("-nodownloading"))
		COM_BufAddText("downloading 0\n");

	CONS_Printf(text[M_INIT_NUM]);
	M_Init();

	CONS_Printf(text[R_INIT_NUM]);
	R_Init();

	// setting up sound
	CONS_Printf(text[S_SETSOUND_NUM]);
	if (M_CheckParm("-nosound"))
		nosound = true;
	if (M_CheckParm("-nomusic")) // combines -nomidimusic and -nodigmusic
		nomusic = nofmod = true;
	else
	{
		if (M_CheckParm("-nomidimusic"))
			nomusic = true; ; // WARNING: DOS version initmusic in I_StartupSound
		if (M_CheckParm("-nodigmusic"))
			nofmod = true; // WARNING: DOS version initmusic in I_StartupSound
	}
	I_StartupSound();
	I_InitMusic();
	S_Init(cv_soundvolume.value, cv_digmusicvolume.value, cv_midimusicvolume.value);

	CONS_Printf(text[ST_INIT_NUM]);
	ST_Init();

	if (M_CheckParm("-internetserver"))
		CV_SetValue(&cv_internetserver, 1);

	// init all NETWORK
	CONS_Printf(text[D_CHECKNET_NUM]);
	if (D_CheckNetGame())
		autostart = true;

	// check for a driver that wants intermission stats
	// start the apropriate game based on parms
	if (M_CheckParm("-record") && M_IsNextParm())
	{
		G_RecordDemo(M_GetNextParm());
		autostart = true;
	}

	p = M_CheckParm("-timetic");
	if (p)
		CV_Set(&cv_timetic, "On");

	if (!autostart)
		M_PushSpecialParameters(); // push all "+" parameters at the command buffer

	// demo doesn't need anymore to be added with D_AddFile()
	p = M_CheckParm("-playdemo");
	if (!p)
		p = M_CheckParm("-timedemo");
	if (p && M_IsNextParm())
	{
		char tmp[MAX_WADPATH];
		// add .lmp to identify the EXTERNAL demo file
		// it is NOT possible to play an internal demo using -playdemo,
		// rather push a playdemo command.. to do.

		strcpy(tmp, M_GetNextParm());
		// get spaced filename or directory
		while (M_IsNextParm())
		{
			strcat(tmp, " ");
			strcat(tmp, M_GetNextParm());
		}

		FIL_DefaultExtension(tmp, ".lmp");

		CONS_Printf("Playing demo %s.\n", tmp);

		if (M_CheckParm("-playdemo"))
		{
			singledemo = true; // quit after one demo
			G_DeferedPlayDemo(tmp);
		}
		else
			G_TimeDemo(tmp);

		G_SetGamestate(GS_NULL);
		wipegamestate = GS_NULL;
		return;
	}

	if (M_CheckParm("-loadgame") && M_IsNextParm())
	{
		G_LoadGame(atoi(M_GetNextParm()));
		return;
	}

	if (autostart || netgame || M_CheckParm("+connect") || M_CheckParm("-connect"))
	{
		gameaction = ga_nothing;

		CV_ClearChangedFlags();

		// Do this here so if you run SRB2 with eg +timelimit 5, the time limit counts
		// as having been modified for the first game.
		M_PushSpecialParameters(); // push all "+" parameter at the command buffer

		if (M_CheckParm("-gametype") && M_IsNextParm())
		{
			// from Command_Map_f
			int j, newgametype = -1;
			char *sgametype = M_GetNextParm();

			for (j = 0; gametype_cons_t[j].strvalue; j++)
				if (!strcasecmp(gametype_cons_t[j].strvalue, sgametype))
				{
					if (gametype_cons_t[j].value == GTF_TEAMMATCH)
					{
						newgametype = GT_MATCH;
						CV_SetValue(&cv_teamplay, 1);
					}
					else if (gametype_cons_t[j].value == GTF_TIMEONLYRACE)
					{
						newgametype = GT_RACE;
						CV_SetValue(&cv_racetype, 1);
					}
					else
						newgametype = gametype_cons_t[j].value;

					break;
				}
			if (!gametype_cons_t[j].strvalue) // reached end of the list with no match
			{
				j = atoi(sgametype); // assume they gave us a gametype number, which is okay too
				if (j >= 0 && j < NUMGAMETYPES)
					newgametype = j;
			}

			if (newgametype != -1)
			{
				j = gametype;
				gametype = newgametype;
				D_GameTypeChanged(j);
			}
		}

		if (server && !M_CheckParm("+map") && !M_CheckParm("+connect")
			&& !M_CheckParm("-connect"))
		{
			D_MapChange(pstartmap, gametype, gameskill, 1, 0, false, false);
		}
	}
	else
		F_StartIntro(); // Tails 03-03-2002

	if (dedicated && server)
	{
		pagename = "TITLESKY";
		levelstarttic = gametic;
		G_SetGamestate(GS_LEVEL);
		if (!P_SetupLevel(gamemap, gameskill, false))
			I_Quit(); // fail so reset game stuff
	}
}

const char *D_Home(void)
{
	char *userhome = NULL;

#ifdef _arch_dreamcast
	putenv("HOME=/vmu/a1"); //don't use I_PutEnv
#endif

	if (M_CheckParm("-home") && M_IsNextParm())
		userhome = M_GetNextParm();
	else
	{
#ifdef GP2X
		usehome = false; //let use the CWD
		return NULL;
#elif !defined (UNIXLIKE) && !defined (__APPLE__) && !defined(_WIN32_WCE)
		if (FIL_FileOK(CONFIGFILENAME))
			usehome = false; // Let's NOT use home
		else
#endif
			userhome = I_GetEnv("HOME"); //Alam: my new HOME for srb2
	}
#if defined (_WIN32) && !defined(_WIN32_WCE) //Alam: only Win32 have APPDATA and USERPROFILE
	if (!userhome && usehome) //Alam: Still not?
	{
		char *testhome = NULL;
		testhome = I_GetEnv("APPDATA");
		if (testhome != NULL
			&& (FIL_FileOK(va("%s" PATHSEP "%s" PATHSEP CONFIGFILENAME, testhome, DEFAULTDIR))))
		{
			userhome = testhome;
		}
	}
#ifndef __CYGWIN__
	if (!userhome && usehome) //Alam: All else fails?
	{
		char *testhome = NULL;
		testhome = I_GetEnv("USERPROFILE");
		if (testhome != NULL
			&& (FIL_FileOK(va("%s" PATHSEP "%s" PATHSEP CONFIGFILENAME, testhome, DEFAULTDIR))))
		{
			userhome = testhome;
		}
	}
#endif// !__CYGWIN__
#endif// _WIN32
	if (usehome) return userhome;
	else return NULL;
}
