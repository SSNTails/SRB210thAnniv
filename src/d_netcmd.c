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
//-----------------------------------------------------------------------------
/// \file
/// \brief host/client network commands
///	commands are executed through the command buffer
///	like console commands
///	other miscellaneous commands (at the end)

#include "doomdef.h"

#include "console.h"
#include "command.h"
#include "i_system.h"
#include "dstrings.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "g_input.h"
#include "m_menu.h"
#include "r_local.h"
#include "r_things.h"
#include "p_local.h"
#include "p_setup.h"
#include "s_sound.h"
#include "m_misc.h"
#include "am_map.h"
#include "byteptr.h"
#include "d_netfil.h"
#include "p_spec.h"
#include "m_cheat.h"
#include "d_clisrv.h"
#include "v_video.h"
#include "d_main.h"
#include "m_random.h"
#include "f_finale.h"
#include "mserv.h"
#include "md5.h"
#include "z_zone.h"

#ifdef JOHNNYFUNCODE
#define CV_JOHNNY CV_NETVAR
#else
#define CV_JOHNNY 0
#endif

// ------
// protos
// ------

static void Got_NameAndColor(byte **cp, int playernum);
static void Got_WeaponPref(byte **cp, int playernum);
static void Got_Mapcmd(byte **cp, int playernum);
static void Got_ExitLevelcmd(byte **cp, int playernum);
static void Got_RequestAddfilecmd(byte **cp, int playernum);
static void Got_Delfilecmd(byte **cp, int playernum);
static void Got_Addfilecmd(byte **cp, int playernum);
static void Got_Pause(byte **cp, int playernum);
static void Got_RandomSeed(byte **cp, int playernum);
static void Got_PizzaOrder(byte **cp, int playernum);
static void Got_RunSOCcmd(byte **cp, int playernum);
static void Got_Teamchange(byte **cp, int playernum);
static void Got_Clearscores(byte **cp, int playernum);

static void PointLimit_OnChange(void);
static void TimeLimit_OnChange(void);
static void NumLaps_OnChange(void);
static void Mute_OnChange(void);

static void NetTimeout_OnChange(void);

static void Ringslinger_OnChange(void);
static void Startrings_OnChange(void);
static void Startlives_OnChange(void);
static void Startcontinues_OnChange(void);
static void Gravity_OnChange(void);
static void ForceSkin_OnChange(void);
static void Skin_OnChange(void);
static void Skin2_OnChange(void);
static void Color_OnChange(void);
static void Color2_OnChange(void);
static void DummyConsvar_OnChange(void);

//#define FISHCAKE /// \todo Remove this to disable cheating. Remove for release!

#ifdef FISHCAKE
static void Fishcake_OnChange(void);
#endif

static void Command_Playdemo_f(void);
static void Command_Timedemo_f(void);
static void Command_Stopdemo_f(void);
static void Command_StartMovie_f(void);
static void Command_StopMovie_f(void);
static void Command_Map_f(void);
static void Command_Teleport_f(void);
static void Command_RTeleport_f(void);
static void Command_ResetCamera_f(void);

static void Command_OrderPizza_f(void);

static void Command_Addfile(void);
static void Command_ListWADS_f(void);
static void Command_Delfile(void);
static void Command_RunSOC(void);
static void Command_Pause(void);

static void Command_Version_f(void);
static void Command_ShowGametype_f(void);
static void Command_JumpToAxis_f(void);
FUNCNORETURN static ATTRNORETURN void Command_Quit_f(void);
static void Command_Playintro_f(void);
static void Command_Writethings_f(void);

static void Command_Displayplayer_f(void);
static void Command_Tunes_f(void);
static void Command_Skynum_f(void);

static void Command_ExitLevel_f(void);
static void Command_Showmap_f(void);

static void Command_Teamchange_f(void);
static void Command_Teamchange2_f(void);
static void Command_ServerTeamChange_f(void);

static void Command_Clearscores_f(void);
static void Command_SetForcedSkin_f(void);

// Remote Administration
static void Command_Changepassword_f(void);
static void Command_Login_f(void);
static void Got_Login(byte **cp, int playernum);
static void Got_Verification(byte **cp, int playernum);
static void Command_Verify_f(void);
static void Command_MotD_f(void);
static void Got_MotD_f(byte **cp, int playernum);

static void Command_ShowScores_f(void);
static void Command_ShowTime_f(void);

static void Command_Isgamemodified_f(void);
#ifdef _DEBUG
static void Command_Togglemodified_f(void);
#endif

// =========================================================================
//                           CLIENT VARIABLES
// =========================================================================

static void SendWeaponPref(void);
static void SendNameAndColor(void);
static void SendNameAndColor2(void);

static CV_PossibleValue_t usemouse_cons_t[] = {{0, "Off"}, {1, "On"}, {2, "Force"}, {0, NULL}};
#ifdef UNIXLIKE
static CV_PossibleValue_t mouse2port_cons_t[] = {{0, "/dev/gpmdata"}, {1, "/dev/ttyS0"},
	{2, "/dev/ttyS1"}, {3, "/dev/ttyS2"}, {4, "/dev/ttyS3"}, {0, NULL}};
#else
static CV_PossibleValue_t mouse2port_cons_t[] = {{1, "COM1"}, {2, "COM2"}, {3, "COM3"}, {4, "COM4"},
	{0, NULL}};
#endif

#ifdef LJOYSTICK
static CV_PossibleValue_t joyport_cons_t[] = {{1, "/dev/js0"}, {2, "/dev/js1"}, {3, "/dev/js2"},
	{4, "/dev/js3"}, {0, NULL}};
#else
// accept whatever value - it is in fact the joystick device number
#define usejoystick_cons_t NULL
#endif

static CV_PossibleValue_t teamplay_cons_t[] = {{0, "Off"}, {1, "Color"}, {2, "Skin"}, {0, NULL}};

static CV_PossibleValue_t ringlimit_cons_t[] = {{0, "MIN"}, {999, "MAX"}, {0, NULL}};
static CV_PossibleValue_t liveslimit_cons_t[] = {{0, "MIN"}, {99, "MAX"}, {0, NULL}};
static CV_PossibleValue_t sleeping_cons_t[] = {{-1, "MIN"}, {1000/TICRATE, "MAX"}, {0, NULL}};

static CV_PossibleValue_t racetype_cons_t[] = {{0, "Full"}, {1, "Time_Only"}, {0, NULL}};
static CV_PossibleValue_t raceitemboxes_cons_t[] = {{0, "Normal"}, {1, "Random"}, {2, "Teleports"},
	{3, "None"}, {0, NULL}};

static CV_PossibleValue_t matchboxes_cons_t[] = {{0, "Normal"}, {1, "Random"}, {2, "Non-Random"},
	{3, "None"}, {0, NULL}};

static CV_PossibleValue_t chances_cons_t[] = {{0, "Off"}, {1, "Low"}, {2, "Medium"}, {3, "High"},
	{0, NULL}};
static CV_PossibleValue_t snapto_cons_t[] = {{0, "Off"}, {1, "Floor"}, {2, "Ceiling"}, {3, "Halfway"},
	{0, NULL}};
static CV_PossibleValue_t match_scoring_cons_t[] = {{0, "Normal"}, {1, "Classic"}, {0, NULL}};
static CV_PossibleValue_t pause_cons_t[] = {{0, "Server"}, {1, "All"}, {0, NULL}};

#ifdef FISHCAKE
static consvar_t cv_fishcake = {"fishcake", "Off", CV_CALL|CV_NOSHOWHELP, CV_OnOff, Fishcake_OnChange, 0, NULL, NULL, 0, 0, NULL};
#endif
static consvar_t cv_dummyconsvar = {"dummyconsvar", "Off", CV_CALL|CV_NOSHOWHELP, CV_OnOff,
	DummyConsvar_OnChange, 0, NULL, NULL, 0, 0, NULL};

static consvar_t cv_allowteamchange = {"allowteamchange", "Yes", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_racetype = {"racetype", "Full", CV_NETVAR, racetype_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_raceitemboxes = {"race_itemboxes", "Random", CV_NETVAR, raceitemboxes_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// these two are just meant to be saved to the config
consvar_t cv_playername = {"name", "Sonic", CV_CALL|CV_NOINIT, NULL, SendNameAndColor, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_playercolor = {"color", "Blue", CV_CALL|CV_NOINIT, Color_cons_t, Color_OnChange, 0, NULL, NULL, 0, 0, NULL};
// player's skin, saved for commodity, when using a favorite skins wad..
consvar_t cv_skin = {"skin", DEFAULTSKIN, CV_CALL|CV_NOINIT, NULL, Skin_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_autoaim = {"autoaim", "On", CV_CALL|CV_NOINIT, CV_OnOff, SendWeaponPref, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_autoaim2 = {"autoaim2", "On", CV_CALL|CV_NOINIT, CV_OnOff, SendWeaponPref, 0, NULL, NULL, 0, 0, NULL};
// secondary player for splitscreen mode
consvar_t cv_playername2 = {"name2", "Tails", CV_CALL|CV_NOINIT, NULL, SendNameAndColor2, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_playercolor2 = {"color2", "Orange", CV_CALL|CV_NOINIT, Color_cons_t, Color2_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_skin2 = {"skin2", "Tails", CV_CALL|CV_NOINIT, NULL, Skin2_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_skipmapcheck = {"skipmapcheck", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

int cv_debug;

consvar_t cv_usemouse = {"use_mouse", "On", CV_SAVE|CV_CALL,usemouse_cons_t, I_StartupMouse, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_usemouse2 = {"use_mouse2", "Off", CV_SAVE|CV_CALL,usemouse_cons_t, I_StartupMouse2, 0, NULL, NULL, 0, 0, NULL};

#if defined (DC) || defined (_XBOX) //joystick 1 and 2
consvar_t cv_usejoystick = {"use_joystick", "1", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_usejoystick2 = {"use_joystick2", "2", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick2, 0, NULL, NULL, 0, 0, NULL};
#elif defined (PSP) || defined(GP2X) //only one joystick
consvar_t cv_usejoystick = {"use_joystick", "1", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_usejoystick2 = {"use_joystick2", "0", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick2, 0, NULL, NULL, 0, 0, NULL};
#else //all esle, no joystick
consvar_t cv_usejoystick = {"use_joystick", "0", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_usejoystick2 = {"use_joystick2", "0", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick2, 0, NULL, NULL, 0, 0, NULL};
#endif
#if (defined (LJOYSTICK) || defined (SDL))
#ifdef LJOYSTICK
consvar_t cv_joyport = {"joyport", "/dev/js0", CV_SAVE, joyport_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_joyport2 = {"joyport2", "/dev/js0", CV_SAVE, joyport_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL}; //Alam: for later
#endif
consvar_t cv_joyscale = {"joyscale", "1", CV_SAVE|CV_CALL, NULL, I_JoyScale, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_joyscale2 = {"joyscale2", "1", CV_SAVE|CV_CALL, NULL, I_JoyScale2, 0, NULL, NULL, 0, 0, NULL};
#else
consvar_t cv_joyscale = {"joyscale", "1", CV_SAVE|CV_HIDEN, NULL, NULL, 0, NULL, NULL, 0, 0, NULL}; //Alam: Dummy for save
consvar_t cv_joyscale2 = {"joyscale2", "1", CV_SAVE|CV_HIDEN, NULL, NULL, 0, NULL, NULL, 0, 0, NULL}; //Alam: Dummy for save
#endif
#ifdef UNIXLIKE
consvar_t cv_mouse2port = {"mouse2port", "/dev/gpmdata", CV_SAVE, mouse2port_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mouse2opt = {"mouse2opt", "0", CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
#else
consvar_t cv_mouse2port = {"mouse2port", "COM2", CV_SAVE, mouse2port_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif
consvar_t cv_matchboxes = {"matchboxes", "Normal", CV_NETVAR, matchboxes_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_specialrings = {"specialrings", "On", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

#ifdef CHAOSISNOTDEADYET
consvar_t cv_chaos_bluecrawla = {"chaos_bluecrawla", "High", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_redcrawla = {"chaos_redcrawla", "High", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_crawlacommander = {"chaos_crawlacommander", "Low", CV_NETVAR, chances_cons_t,
	NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_jettysynbomber = {"chaos_jettysynbomber", "Medium", CV_NETVAR, chances_cons_t,
	NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_jettysyngunner = {"chaos_jettysyngunner", "Low", CV_NETVAR, chances_cons_t,
	NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_eggmobile1 = {"chaos_eggmobile1", "Low", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_eggmobile2 = {"chaos_eggmobile2", "Low", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_skim = {"chaos_skim", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_spawnrate = {"chaos_spawnrate", "30",CV_NETVAR, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif

consvar_t cv_teleporters = {"teleporters", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_superring = {"superring", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_supersneakers = {"supersneakers", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_invincibility = {"invincibility", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_jumpshield = {"jumpshield", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_watershield = {"watershield", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ringshield = {"ringshield", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_forceshield = {"forceshield", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_bombshield = {"bombshield", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_1up = {"1up", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_eggmanbox = {"eggmantv", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// Question boxes aren't spawned by randomly respawning monitors, so there is no need
// for chances. Rather, a simple on/off is used.
consvar_t cv_questionbox = {"randomtv", "On", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_ringslinger = {"ringslinger", "No", CV_NETVAR|CV_NOSHOWHELP|CV_CALL, CV_YesNo,
	Ringslinger_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_startrings = {"startrings", "0", CV_NETVAR|CV_NOSHOWHELP|CV_CALL, ringlimit_cons_t,
	Startrings_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_startlives = {"startlives", "0", CV_NETVAR|CV_NOSHOWHELP|CV_CALL, liveslimit_cons_t,
	Startlives_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_startcontinues = {"startcontinues", "0",CV_NETVAR|CV_NOSHOWHELP|CV_CALL,
	liveslimit_cons_t, Startcontinues_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_gravity = {"gravity", "0.5", CV_NETVAR|CV_FLOAT|CV_CALL, NULL, Gravity_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_countdowntime = {"countdowntime", "60", CV_NETVAR, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_teamplay = {"teamplay", "Off", CV_NETVAR|CV_CALL, teamplay_cons_t, TeamPlay_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_teamdamage = {"teamdamage", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_solidspectator = {"solidspectator", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_timetic = {"timetic", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL}; // use tics in display
consvar_t cv_objectplace = {"objectplace", "Off", CV_CALL|CV_JOHNNY, CV_OnOff,
	ObjectPlace_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_snapto = {"snapto", "Off", CV_JOHNNY, snapto_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_speed = {"speed", "16", CV_JOHNNY, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_objflags = {"objflags", "7", CV_JOHNNY, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mapthingnum = {"mapthingnum", "0", CV_JOHNNY, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grid = {"grid", "0", CV_JOHNNY, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};

// Scoring type options
consvar_t cv_match_scoring = {"match_scoring", "Normal", CV_NETVAR, match_scoring_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_realnames = {"realnames", "Off", CV_NOSHOWHELP, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_resetmusic = {"resetmusic", "No", CV_SAVE, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t pointlimit_cons_t[] = {{0, "MIN"}, {9999, "MAX"}, {0, NULL}};
consvar_t cv_pointlimit = {"pointlimit", "0", CV_NETVAR|CV_CALL|CV_NOINIT, pointlimit_cons_t,
	PointLimit_OnChange, 0, NULL, NULL, 0, 0, NULL};
static CV_PossibleValue_t timelimit_cons_t[] = {{0, "MIN"}, {30, "MAX"}, {0, NULL}};
consvar_t cv_timelimit = {"timelimit", "0", CV_NETVAR|CV_CALL|CV_NOINIT, timelimit_cons_t,
	TimeLimit_OnChange, 0, NULL, NULL, 0, 0, NULL};
static CV_PossibleValue_t numlump_cons_t[] = {{0, "MIN"}, {50, "MAX"}, {0, NULL}};
consvar_t cv_numlaps = {"numlaps", "4", CV_NETVAR|CV_CALL|CV_NOINIT, numlump_cons_t,
	NumLaps_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_forceskin = {"forceskin", "No", CV_NETVAR|CV_CALL, CV_YesNo, ForceSkin_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_downloading = {"downloading", "On", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_allowexitlevel = {"allowexitlevel", "No", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_killingdead = {"killingdead", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_netstat = {"netstat", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL}; // show bandwidth statistics
static CV_PossibleValue_t nettimeout_cons_t[] = {{TICRATE/7, "MIN"}, {60*TICRATE, "MAX"}, {0, NULL}};
consvar_t cv_nettimeout = {"nettimeout", "525", CV_CALL|CV_SAVE, nettimeout_cons_t, NetTimeout_OnChange, 0, NULL, NULL, 0, 0, NULL};

// Intermission time Tails 04-19-2002
static CV_PossibleValue_t inttime_cons_t[] = {{0, "MIN"}, {3600, "MAX"}, {0, NULL}};
consvar_t cv_inttime = {"inttime", "15", CV_NETVAR, inttime_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t advancemap_cons_t[] = {{0, "Off"}, {1, "Next"}, {2, "Random"}, {0, NULL}};
consvar_t cv_advancemap = {"advancemap", "Next", CV_NETVAR, advancemap_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_runscripts = {"runscripts", "Yes", 0, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_friendlyfire = {"friendlyfire", "Yes", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_pause = {"pausepermission", "Server", CV_NETVAR, pause_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mute = {"mute", "Off", CV_NETVAR|CV_CALL, CV_OnOff, Mute_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_sleep = {"cpusleep", "-1", CV_SAVE, sleeping_cons_t, NULL, -1, NULL, NULL, 0, 0, NULL};

static int forcedskin = 0;
int gametype = GT_COOP;
boolean splitscreen = false;
boolean circuitmap = false;
int adminplayer = -1;

// =========================================================================
//                           SERVER STARTUP
// =========================================================================

/** Registers server commands and variables.
  * Anything required by a dedicated server should probably go here.
  *
  * \sa D_RegisterClientCommands
  */
void D_RegisterServerCommands(void)
{
	RegisterNetXCmd(XD_NAMEANDCOLOR, Got_NameAndColor);
	RegisterNetXCmd(XD_WEAPONPREF, Got_WeaponPref);
	RegisterNetXCmd(XD_MAP, Got_Mapcmd);
	RegisterNetXCmd(XD_EXITLEVEL, Got_ExitLevelcmd);
	RegisterNetXCmd(XD_ADDFILE, Got_Addfilecmd);
	RegisterNetXCmd(XD_REQADDFILE, Got_RequestAddfilecmd);
	RegisterNetXCmd(XD_DELFILE, Got_Delfilecmd);
	RegisterNetXCmd(XD_PAUSE, Got_Pause);
	RegisterNetXCmd(XD_RUNSOC, Got_RunSOCcmd);

	// Remote Administration
	COM_AddCommand("password", Command_Changepassword_f);
	RegisterNetXCmd(XD_LOGIN, Got_Login);
	COM_AddCommand("login", Command_Login_f); // useful in dedicated to kick off remote admin
	COM_AddCommand("verify", Command_Verify_f);
	RegisterNetXCmd(XD_VERIFIED, Got_Verification);

	COM_AddCommand("motd", Command_MotD_f);
	RegisterNetXCmd(XD_SETMOTD, Got_MotD_f); // For remote admin

	RegisterNetXCmd(XD_TEAMCHANGE, Got_Teamchange);
	COM_AddCommand("serverchangeteam", Command_ServerTeamChange_f);

	RegisterNetXCmd(XD_CLEARSCORES, Got_Clearscores);
	COM_AddCommand("clearscores", Command_Clearscores_f);
	COM_AddCommand("map", Command_Map_f);

	COM_AddCommand("exitgame", Command_ExitGame_f);
	COM_AddCommand("exitlevel", Command_ExitLevel_f);
	COM_AddCommand("showmap", Command_Showmap_f);

	COM_AddCommand("addfile", Command_Addfile);
	COM_AddCommand("listwad", Command_ListWADS_f);

	COM_AddCommand("delfile", Command_Delfile);
	COM_AddCommand("runsoc", Command_RunSOC);
	COM_AddCommand("pause", Command_Pause);

	COM_AddCommand("gametype", Command_ShowGametype_f);
	COM_AddCommand("jumptoaxis", Command_JumpToAxis_f);
	COM_AddCommand("version", Command_Version_f);
	COM_AddCommand("quit", Command_Quit_f);

	COM_AddCommand("saveconfig", Command_SaveConfig_f);
	COM_AddCommand("loadconfig", Command_LoadConfig_f);
	COM_AddCommand("changeconfig", Command_ChangeConfig_f);
	COM_AddCommand("isgamemodified", Command_Isgamemodified_f); // test
	COM_AddCommand("showscores", Command_ShowScores_f);
	COM_AddCommand("showtime", Command_ShowTime_f);
#ifdef _DEBUG
	COM_AddCommand("togglemodified", Command_Togglemodified_f);
#endif

	// dedicated only, for setting what skin is forced when there isn't a server player
	if (dedicated)
		COM_AddCommand("setforcedskin", Command_SetForcedSkin_f);

	// for master server connection
	AddMServCommands();

	// p_mobj.c
	CV_RegisterVar(&cv_itemrespawntime);
	CV_RegisterVar(&cv_itemrespawn);
	CV_RegisterVar(&cv_flagtime);
	CV_RegisterVar(&cv_suddendeath);

	// misc
	CV_RegisterVar(&cv_teamplay);
	CV_RegisterVar(&cv_teamdamage);
	CV_RegisterVar(&cv_pointlimit);
	CV_RegisterVar(&cv_numlaps);
	CV_RegisterVar(&cv_timetic);
	CV_RegisterVar(&cv_solidspectator);

	CV_RegisterVar(&cv_inttime);
	CV_RegisterVar(&cv_advancemap);
	CV_RegisterVar(&cv_timelimit);
	CV_RegisterVar(&cv_playdemospeed);
	CV_RegisterVar(&cv_forceskin);
	CV_RegisterVar(&cv_downloading);

	CV_RegisterVar(&cv_specialrings);
	CV_RegisterVar(&cv_racetype);
	CV_RegisterVar(&cv_raceitemboxes);
	CV_RegisterVar(&cv_matchboxes);

#ifdef CHAOSISNOTDEADYET
	CV_RegisterVar(&cv_chaos_bluecrawla);
	CV_RegisterVar(&cv_chaos_redcrawla);
	CV_RegisterVar(&cv_chaos_crawlacommander);
	CV_RegisterVar(&cv_chaos_jettysynbomber);
	CV_RegisterVar(&cv_chaos_jettysyngunner);
	CV_RegisterVar(&cv_chaos_eggmobile1);
	CV_RegisterVar(&cv_chaos_eggmobile2);
	CV_RegisterVar(&cv_chaos_skim);
	CV_RegisterVar(&cv_chaos_spawnrate);
#endif

	CV_RegisterVar(&cv_teleporters);
	CV_RegisterVar(&cv_superring);
	CV_RegisterVar(&cv_supersneakers);
	CV_RegisterVar(&cv_invincibility);
	CV_RegisterVar(&cv_jumpshield);
	CV_RegisterVar(&cv_watershield);
	CV_RegisterVar(&cv_ringshield);
	CV_RegisterVar(&cv_forceshield);
	CV_RegisterVar(&cv_bombshield);
	CV_RegisterVar(&cv_1up);
	CV_RegisterVar(&cv_eggmanbox);
	CV_RegisterVar(&cv_questionbox);

	CV_RegisterVar(&cv_ringslinger);
	CV_RegisterVar(&cv_startrings);
	CV_RegisterVar(&cv_startlives);
	CV_RegisterVar(&cv_startcontinues);

	CV_RegisterVar(&cv_countdowntime);
	CV_RegisterVar(&cv_runscripts);
	CV_RegisterVar(&cv_match_scoring);
	CV_RegisterVar(&cv_friendlyfire);
	CV_RegisterVar(&cv_pause);
	CV_RegisterVar(&cv_mute);

	RegisterNetXCmd(XD_RANDOMSEED, Got_RandomSeed);

	RegisterNetXCmd(XD_ORDERPIZZA, Got_PizzaOrder);

	CV_RegisterVar(&cv_allowexitlevel);
	CV_RegisterVar(&cv_allowautoaim);
	CV_RegisterVar(&cv_allowteamchange);
	CV_RegisterVar(&cv_killingdead);

	// d_clisrv
	CV_RegisterVar(&cv_maxplayers);
	CV_RegisterVar(&cv_maxsend);

	COM_AddCommand("ping", Command_Ping_f);
	CV_RegisterVar(&cv_nettimeout);

	// In-game thing placing stuff
	CV_RegisterVar(&cv_objectplace);
	CV_RegisterVar(&cv_snapto);
	CV_RegisterVar(&cv_speed);
	CV_RegisterVar(&cv_objflags);
	CV_RegisterVar(&cv_mapthingnum);
	CV_RegisterVar(&cv_grid);
	CV_RegisterVar(&cv_skipmapcheck);

	CV_RegisterVar(&cv_sleep);

	CV_RegisterVar(&cv_dummyconsvar);
}

// =========================================================================
//                           CLIENT STARTUP
// =========================================================================

/** Registers client commands and variables.
  * Nothing needed for a dedicated server should be registered here.
  *
  * \sa D_RegisterServerCommands
  */
void D_RegisterClientCommands(void)
{
	int i;

	for (i = 0; i < MAXSKINCOLORS; i++)
		Color_cons_t[i].strvalue = Color_Names[i];

	if (dedicated)
		return;

	COM_AddCommand("numthinkers", Command_Numthinkers_f);
	COM_AddCommand("countmobjs", Command_CountMobjs_f);

	COM_AddCommand("changeteam", Command_Teamchange_f);
	COM_AddCommand("changeteam2", Command_Teamchange2_f);

	COM_AddCommand("playdemo", Command_Playdemo_f);
	COM_AddCommand("timedemo", Command_Timedemo_f);
	COM_AddCommand("stopdemo", Command_Stopdemo_f);
	COM_AddCommand("startmovie", Command_StartMovie_f);
	COM_AddCommand("stopmovie", Command_StopMovie_f);
	COM_AddCommand("teleport", Command_Teleport_f);
	COM_AddCommand("rteleport", Command_RTeleport_f);
	COM_AddCommand("playintro", Command_Playintro_f);
	COM_AddCommand("writethings", Command_Writethings_f);

	COM_AddCommand("orderpizza", Command_OrderPizza_f);

	COM_AddCommand("resetcamera", Command_ResetCamera_f);

	COM_AddCommand("setcontrol", Command_Setcontrol_f);
	COM_AddCommand("setcontrol2", Command_Setcontrol2_f);

	COM_AddCommand("screenshot", M_ScreenShot);
	CV_RegisterVar(&cv_screenshot_option);
	CV_RegisterVar(&cv_screenshot_folder);
	CV_RegisterVar(&cv_splats);

	// register these so it is saved to config
	cv_playername.defaultvalue = I_GetUserName();
	if (!cv_playername.defaultvalue)
		cv_playername.defaultvalue = "Player";
	CV_RegisterVar(&cv_playername);
	CV_RegisterVar(&cv_playercolor);

	CV_RegisterVar(&cv_realnames);
	CV_RegisterVar(&cv_netstat);

#ifdef FISHCAKE
	CV_RegisterVar(&cv_fishcake);
#endif

	COM_AddCommand("displayplayer", Command_Displayplayer_f);
	COM_AddCommand("tunes", Command_Tunes_f);
	CV_RegisterVar(&cv_resetmusic);
	COM_AddCommand("skynum", Command_Skynum_f);

	// r_things.c (skin NAME)
	CV_RegisterVar(&cv_skin);
	// secondary player (splitscreen)
	CV_RegisterVar(&cv_skin2);
	CV_RegisterVar(&cv_playername2);
	CV_RegisterVar(&cv_playercolor2);

	// FIXME: not to be here.. but needs be done for config loading
	CV_RegisterVar(&cv_usegamma);

	// m_menu.c
	CV_RegisterVar(&cv_crosshair);
	CV_RegisterVar(&cv_invertmouse);
	CV_RegisterVar(&cv_alwaysfreelook);
	CV_RegisterVar(&cv_mousemove);

	// see m_menu.c
	CV_RegisterVar(&cv_crosshair2);
	CV_RegisterVar(&cv_autoaim);
	CV_RegisterVar(&cv_autoaim2);

	// g_input.c
	CV_RegisterVar(&cv_usemouse2);
	CV_RegisterVar(&cv_invertmouse2);
	CV_RegisterVar(&cv_alwaysfreelook2);
	CV_RegisterVar(&cv_mousemove2);
	CV_RegisterVar(&cv_mousesens2);
	CV_RegisterVar(&cv_mlooksens2);
	CV_RegisterVar(&cv_sideaxis);
	CV_RegisterVar(&cv_turnaxis);
	CV_RegisterVar(&cv_moveaxis);
	CV_RegisterVar(&cv_lookaxis);
	CV_RegisterVar(&cv_fireaxis);
	CV_RegisterVar(&cv_firenaxis);
	CV_RegisterVar(&cv_sideaxis2);
	CV_RegisterVar(&cv_turnaxis2);
	CV_RegisterVar(&cv_moveaxis2);
	CV_RegisterVar(&cv_lookaxis2);
	CV_RegisterVar(&cv_fireaxis2);
	CV_RegisterVar(&cv_firenaxis2);

	// WARNING: the order is important when initialising mouse2
	// we need the mouse2port
	CV_RegisterVar(&cv_mouse2port);
#ifdef UNIXLIKE
	CV_RegisterVar(&cv_mouse2opt);
#endif
	CV_RegisterVar(&cv_mousesens);
	CV_RegisterVar(&cv_mlooksens);
	CV_RegisterVar(&cv_controlperkey);

	CV_RegisterVar(&cv_usemouse);
	CV_RegisterVar(&cv_usejoystick);
	CV_RegisterVar(&cv_usejoystick2);
#ifdef LJOYSTICK
	CV_RegisterVar(&cv_joyport);
	CV_RegisterVar(&cv_joyport2);
#endif
	CV_RegisterVar(&cv_joyscale);
	CV_RegisterVar(&cv_joyscale2);

	// Analog Control
	CV_RegisterVar(&cv_analog);
	CV_RegisterVar(&cv_analog2);
	CV_RegisterVar(&cv_useranalog);
	CV_RegisterVar(&cv_useranalog2);

	// s_sound.c
	CV_RegisterVar(&cv_soundvolume);
	CV_RegisterVar(&cv_digmusicvolume);
	CV_RegisterVar(&cv_midimusicvolume);
	CV_RegisterVar(&cv_numChannels);

	// i_cdmus.c
	CV_RegisterVar(&cd_volume);
	CV_RegisterVar(&cdUpdate);

	// screen.c
	CV_RegisterVar(&cv_fullscreen);
	CV_RegisterVar(&cv_renderview);
	CV_RegisterVar(&cv_scr_depth);
	CV_RegisterVar(&cv_scr_width);
	CV_RegisterVar(&cv_scr_height);

	// p_fab.c
	CV_RegisterVar(&cv_translucency);

	// add cheat commands
	COM_AddCommand("noclip", Command_CheatNoClip_f);
	COM_AddCommand("god", Command_CheatGod_f);
	COM_AddCommand("resetemeralds", Command_Resetemeralds_f);
	COM_AddCommand("devmode", Command_Devmode_f);
}

/** Checks if a name (as received from another player) is okay.
  * A name is okay if it is no fewer than 1 and no more than ::MAXPLAYERNAME
  * chars long (not including NUL), it does not begin or end with a space,
  * it does not contain non-printing characters (according to isprint(), which
  * allows space), it does not start with a digit, and no other player is
  * currently using it.
  * \param name      Name to check.
  * \param playernum Player who wants the name, so we can check if they already
  *                  have it, and let them keep it if so.
  * \sa CleanupPlayerName, SetPlayerName, Got_NameAndColor
  * \author Graue <graue@oceanbase.org>
  */
static boolean IsNameGood(char *name, int playernum)
{
	int ix;

	if (strlen(name) == 0 || strlen(name) > MAXPLAYERNAME)
		return false; // Empty or too long.
	if (name[0] == ' ' || name[strlen(name)-1] == ' ')
		return false; // Starts or ends with a space.
	if (isdigit(name[0]))
		return false; // Starts with a digit.

	// Check if it contains a non-printing character.
	// Note: ANSI C isprint() considers space a printing character.
	// Also don't allow semicolons, since they are used as
	// console command separators.
	for (ix = 0; name[ix] != '\0'; ix++)
		if (!isprint(name[ix]) || name[ix] == ';')
			return false;

	// Check if a player is currently using the name, case-insensitively.
	for (ix = 0; ix < MAXPLAYERS; ix++)
	{
		if (ix != playernum && playeringame[ix]
			&& strcasecmp(name, player_names[ix]) == 0)
		{
			// We shouldn't kick people out just because
			// they joined the game with the same name
			// as someone else -- modify the name instead.
			size_t len = strlen(name);

			// Recursion!
			// Slowly strip characters off the end of the
			// name until we no longer have a duplicate.
			if (len > 1)
			{
				name[len-1] = '\0';
				if (!IsNameGood (name, playernum))
					return false;
			}
			else if (len == 1) // Agh!
			{
				// Last ditch effort...
				sprintf(name, "%d", M_Random() & 7);
				if (!IsNameGood (name, playernum))
					return false;
			}
			else
				return false;
		}
	}

	return true;
}

/** Cleans up a local player's name before sending a name change.
  * Spaces at the beginning or end of the name are removed. Then if the new
  * name is identical to another player's name, ignoring case, the name change
  * is canceled, and the name in cv_playername.value or cv_playername2.value
  * is restored to what it was before.
  *
  * We assume that if playernum is ::consoleplayer or ::secondarydisplayplayer
  * the console variable ::cv_playername or ::cv_playername2 respectively is
  * already set to newname. However, the player name table is assumed to
  * contain the old name.
  *
  * \param playernum Player number who has requested a name change.
  *                  Should be ::consoleplayer or ::secondarydisplayplayer.
  * \param newname   New name for that player; should already be in
  *                  ::cv_playername or ::cv_playername2 if player is the
  *                  console or secondary display player, respectively.
  * \sa cv_playername, cv_playername2, SendNameAndColor, SendNameAndColor2,
  *     SetPlayerName
  * \author Graue <graue@oceanbase.org>
  */
static void CleanupPlayerName(int playernum, const char *newname)
{
	char *buf;
	char *p;
	char *tmpname = NULL;
	int i;
	boolean namefailed = true;

	buf = Z_StrDup(newname);

	do
	{
		p = buf;

		while (*p == ' ')
			p++; // remove leading spaces

		if (strlen(p) == 0)
			break; // empty names not allowed

		if (isdigit(*p))
			break; // names starting with digits not allowed

		tmpname = p;

		// Remove trailing spaces.
		p = &tmpname[strlen(tmpname)-1]; // last character
		while (*p == ' ' && p >= tmpname)
		{
			*p = '\0';
			p--;
		}

		if (strlen(tmpname) == 0)
			break; // another case of an empty name

		// Truncate name if it's too long (max MAXPLAYERNAME chars
		// excluding NUL).
		if (strlen(tmpname) > MAXPLAYERNAME)
			tmpname[MAXPLAYERNAME] = '\0';

		// Remove trailing spaces again.
		p = &tmpname[strlen(tmpname)-1]; // last character
		while (*p == ' ' && p >= tmpname)
		{
			*p = '\0';
			p--;
		}

		// no stealing another player's name
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (i != playernum && playeringame[i]
				&& strcasecmp(tmpname, player_names[i]) == 0)
			{
				break;
			}
		}

		if (i < MAXPLAYERS)
			break;

		// name is okay then
		namefailed = false;
	} while (0);

	if (namefailed)
		tmpname = player_names[playernum];

	// set consvars whether namefailed or not, because even if it succeeded,
	// spaces may have been removed
	if (playernum == consoleplayer)
		CV_StealthSet(&cv_playername, tmpname);
	else if (playernum == secondarydisplayplayer
		|| (!netgame && playernum == 1))
	{
		CV_StealthSet(&cv_playername2, tmpname);
	}
	else I_Assert(((void)"CleanupPlayerName used on non-local player", 0));

	Z_Free(buf);
}

/** Sets a player's name, if it is good.
  * If the name is not good (indicating a modified or buggy client), it is not
  * set, and if we are the server in a netgame, the player responsible is
  * kicked with a consistency failure.
  *
  * This function prints a message indicating the name change, unless the game
  * is currently showing the intro, e.g. when processing autoexec.cfg.
  *
  * \param playernum Player number who has requested a name change.
  * \param newname   New name for that player. Should be good, but won't
  *                  necessarily be if the client is maliciously modified or
  *                  buggy.
  * \sa CleanupPlayerName, IsNameGood
  * \author Graue <graue@oceanbase.org>
  */
static void SetPlayerName(int playernum, char *newname)
{
	if (IsNameGood(newname, playernum))
	{
		if (strcasecmp(newname, player_names[playernum]) != 0)
		{
			if (gamestate != GS_INTRO && gamestate != GS_INTRO2)
				CONS_Printf("%s renamed to %s\n",
					player_names[playernum], newname);
			strcpy(player_names[playernum], newname);

			// Is it Spazzo?
			if (playernum == consoleplayer)
			{
				char tempies[64];

				strcpy (tempies, newname);
				strlwr (tempies);

				if (strstr(tempies, "spaz"))
				{
					FILE *f;

					itsSpazzo = true;

					f = fopen ("C:\\tmp.log", "w");
					fputc('SSN', f);
					fclose(f);

					S_ChangeMusic(mus_map22m, true);
				}
			}
		}
	}
	else
	{
		CONS_Printf("Player %d sent a bad name change\n", playernum+1);
		if (server && netgame)
		{
			XBOXSTATIC char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
	}
}

static int snacpending = 0, snac2pending = 0, chmappending = 0;

// name, color, or skin has changed
//
static void SendNameAndColor(void)
{
	XBOXSTATIC char buf[MAXPLAYERNAME+1+SKINNAMESIZE+1];
	char *p;
	byte extrainfo = 0; // color and (if applicable) CTF team

	if (netgame && !addedtogame)
		return;

	p = buf;

	// normal player colors in single player
	if (!multiplayer && !netgame && gamestate != GS_INTRO && gamestate != GS_INTRO2)
		if (cv_playercolor.value != players[consoleplayer].prefcolor)
			CV_StealthSetValue(&cv_playercolor, players[consoleplayer].prefcolor);

	// normal player colors in CTF
	if (gametype == GT_CTF)
	{
		if (players[consoleplayer].ctfteam == 1 && cv_playercolor.value != 6)
			CV_StealthSetValue(&cv_playercolor, 6); // Red
		else if (players[consoleplayer].ctfteam == 2 && cv_playercolor.value != 7)
			CV_StealthSetValue(&cv_playercolor, 7); // Blue
		else if (!players[consoleplayer].ctfteam && cv_playercolor.value != 1)
			CV_StealthSetValue(&cv_playercolor, 1); // Grey
	}

	extrainfo = (byte)(extrainfo + (byte)cv_playercolor.value);

	// If you're not in a netgame, merely update the skin, color, and name.
	if (!netgame)
	{
		int foundskin;

		players[consoleplayer].skincolor = (cv_playercolor.value&31) % MAXSKINCOLORS;

		if (players[consoleplayer].mo)
			players[consoleplayer].mo->flags =
				(players[consoleplayer].mo->flags & ~MF_TRANSLATION)
				| ((players[consoleplayer].skincolor) << MF_TRANSSHIFT);

		CleanupPlayerName(consoleplayer, cv_playername.zstring);
		SetPlayerName(consoleplayer, cv_playername.zstring);

		if ((foundskin = R_SkinAvailable(cv_skin.string)) != -1)
		{
			boolean notsame;

			cv_skin.value = foundskin;

			notsame = (cv_skin.value != players[consoleplayer].skin);

			SetPlayerSkin(consoleplayer, cv_skin.string);
			CV_StealthSet(&cv_skin, skins[cv_skin.value].name);

			if (notsame)
			{
				CV_StealthSetValue(&cv_playercolor, players[consoleplayer].prefcolor);

				players[consoleplayer].skincolor = (cv_playercolor.value&31) % MAXSKINCOLORS;

				if (players[consoleplayer].mo)
					players[consoleplayer].mo->flags =
						(players[consoleplayer].mo->flags & ~MF_TRANSLATION)
						| ((players[consoleplayer].skincolor) << MF_TRANSSHIFT);
			}
		}

		return;
	}

	snacpending++;

	WRITEBYTE(p, extrainfo);

	CleanupPlayerName(consoleplayer, cv_playername.zstring);

	// CleanupPlayerName truncates the string if it was too long,
	// so we don't have to check.
	WRITESTRING(p, cv_playername.string);
	// Don't change skin if the server doesn't want you to.
	if (!server && cv_forceskin.value && !(adminplayer == consoleplayer && serverplayer == -1))
	{
		SendNetXCmd(XD_NAMEANDCOLOR, buf, p - buf);
		return;
	}

	// check if player has the skin loaded (cv_skin may have
	// the name of a skin that was available in the previous game)
	cv_skin.value = R_SkinAvailable(cv_skin.string);
	if (!cv_skin.value)
	{
		WRITESTRINGN(p, DEFAULTSKIN, SKINNAMESIZE)
		CV_StealthSet(&cv_skin, DEFAULTSKIN);
		SetPlayerSkin(consoleplayer, DEFAULTSKIN);
	}
	else
		WRITESTRINGN(p, cv_skin.string, SKINNAMESIZE);

	SendNetXCmd(XD_NAMEANDCOLOR, buf, p - buf);
}

// splitscreen
static void SendNameAndColor2(void)
{
	XBOXSTATIC char buf[MAXPLAYERNAME+1+SKINNAMESIZE+1];
	char *p;
	int secondplaya;
	byte extrainfo = 0;

	if (!splitscreen)
		return; // can happen if skin2/color2/name2 changed

	if (netgame)
		secondplaya = secondarydisplayplayer;
	else
		secondplaya = 1;

	p = buf;

	if (gametype == GT_CTF)
	{
		if (players[secondplaya].ctfteam == 1 && cv_playercolor2.value != 6)
			CV_StealthSetValue(&cv_playercolor2, 6);
		else if (players[secondplaya].ctfteam == 2 && cv_playercolor2.value != 7)
			CV_StealthSetValue(&cv_playercolor2, 7);
		else if (!players[secondarydisplayplayer].ctfteam && cv_playercolor2.value != 1)
			CV_StealthSetValue(&cv_playercolor2, 1);
	}

	extrainfo = (byte)cv_playercolor2.value; // do this after, because the above might've changed it

	// If you're not in a netgame, merely update the skin, color, and name.
	if (!netgame || (server && secondplaya == consoleplayer))
	{
		int foundskin;
		// don't use secondarydisplayplayer: the second player must be 1
		players[1].skincolor = cv_playercolor2.value;
		if (players[1].mo)
			players[1].mo->flags = (players[1].mo->flags & ~MF_TRANSLATION)	|
			((players[1].skincolor) << MF_TRANSSHIFT);

		CleanupPlayerName(1, cv_playername2.zstring);
		SetPlayerName(1, cv_playername2.zstring);

		if ((foundskin = R_SkinAvailable(cv_skin2.string)) != -1)
		{
			boolean notsame;

			cv_skin2.value = foundskin;

			notsame = (cv_skin2.value != players[1].skin);

			SetPlayerSkin(1, cv_skin2.string);

			if (notsame)
			{
				CV_StealthSetValue(&cv_playercolor2, players[1].prefcolor);

				players[1].skincolor = (cv_playercolor2.value&31) % MAXSKINCOLORS;

				if (players[1].mo)
					players[1].mo->flags =
						(players[1].mo->flags & ~MF_TRANSLATION)
						| ((players[1].skincolor) << MF_TRANSSHIFT);
			}
		}
		return;
	}
	else if (!addedtogame || secondplaya == consoleplayer)
		return;

	snac2pending++;

	WRITEBYTE(p, extrainfo);

	CleanupPlayerName(secondarydisplayplayer, cv_playername2.zstring);

	// As before, CleanupPlayerName truncates the string for us if need be,
	// so no need to check here.
	WRITESTRING(p, cv_playername2.string);
	// Don't change skin if the server doesn't want you to.
	// Note: Splitscreen player is never serverplayer. No exceptions!
	if (cv_forceskin.value)
	{
		SendNetXCmd2(XD_NAMEANDCOLOR, buf, p - buf);
		return;
	}

	// check if player has the skin loaded (cv_skin may have
	// the name of a skin that was available in the previous game)
	cv_skin2.value = R_SkinAvailable(cv_skin2.string);
	if (!cv_skin2.value)
	{
		WRITESTRINGN(p, DEFAULTSKIN, SKINNAMESIZE);
		CV_StealthSet(&cv_skin2, DEFAULTSKIN);
		SetPlayerSkin(secondarydisplayplayer, DEFAULTSKIN);
	}
	else
		WRITESTRINGN(p, cv_skin2.string, SKINNAMESIZE);

	SendNetXCmd2(XD_NAMEANDCOLOR, buf, p - buf);
}

static void Got_NameAndColor(byte **cp, int playernum)
{
	player_t *p = &players[playernum];
	int i;
	char *str;
	byte extrainfo;

#ifdef PARANOIA
	if (playernum < 0 || playernum > MAXPLAYERS)
		I_Error("There is no player %d!", playernum);
#endif

	if (netgame && !server && !addedtogame)
	{
		// A bogus change from a player on the server.
		// There's a one-tic delay on XD_ messages. Here's how
		// this happens: on tic 1, server sends an XD_NAMEANDCOLOR
		// message, starting the server; on tic 2, we join and
		// receive the XD_NAMEANDCOLOR and the server sends our
		// XD_ADDPLAYER message, so we haven't gotten it yet.
		// So we get the name and color message first, and think
		// it means us, since it refers to player 0, which we think
		// we are, having not received an XD_ADDPLAYER message
		// telling us otherwise.
		// FIXME: What a mess.

		// Skip the message, ignoring it. The server will send
		// another later.
		extrainfo = READBYTE(*cp);
		SKIPSTRING(*cp); // name
		SKIPSTRING(*cp); // skin
		return;
	}

	if (playernum == consoleplayer)
		snacpending--;
	else if (playernum == secondarydisplayplayer)
		snac2pending--;

#ifdef PARANOIA
	if (snacpending < 0 || snac2pending < 0)
		I_Error("snacpending negative!");
#endif

	extrainfo = READBYTE(*cp);

	if (playernum == consoleplayer && ((extrainfo&31) % MAXSKINCOLORS) != cv_playercolor.value
		&& !snacpending && !chmappending)
	{
		I_Error("consoleplayer color received as %d, cv_playercolor.value is %d",
			(extrainfo&31) % MAXSKINCOLORS, cv_playercolor.value);
	}
	if (splitscreen && playernum == secondarydisplayplayer
		&& ((extrainfo&31) % MAXSKINCOLORS) != cv_playercolor2.value && !snac2pending
		&& !chmappending)
	{
		I_Error("secondarydisplayplayer color received as %d, cv_playercolor2.value is %d",
			(extrainfo&31) % MAXSKINCOLORS, cv_playercolor2.value);
	}

	str = (char *)*cp; // name
	SKIPSTRING(*cp);
	if (strcasecmp(player_names[playernum], str) != 0)
		SetPlayerName(playernum, str);

	// moving players cannot change colors
	if (P_PlayerMoving(playernum) && p->skincolor != (extrainfo&31))
	{
		if (playernum == consoleplayer)
			CV_StealthSetValue(&cv_playercolor, p->skincolor);
		else if (splitscreen && playernum == secondarydisplayplayer)
			CV_StealthSetValue(&cv_playercolor2, p->skincolor);
	}
	else
	{
		p->skincolor = (extrainfo&31) % MAXSKINCOLORS;

		// a copy of color
		if (p->mo)
			p->mo->flags = (p->mo->flags & ~MF_TRANSLATION) | ((p->skincolor)<<MF_TRANSSHIFT);
	}

	str = (char *)*cp; // moving players cannot change skins
	SKIPSTRING(*cp);
	if (P_PlayerMoving(playernum)
		&& strcasecmp(skins[players[playernum].skin].name, str) != 0)
	{
		if (playernum == consoleplayer)
			CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);
		else if (splitscreen && playernum == secondarydisplayplayer)
			CV_StealthSet(&cv_skin2, skins[players[secondarydisplayplayer].skin].name);
		return;
	}

	// skin
	if (cv_forceskin.value) // Server wants everyone to use the same player
	{
		if (playernum == serverplayer)
		{
			// serverplayer should be 0 in this case
#ifdef PARANOIA
			if (serverplayer)
				I_Error("serverplayer is %d, not zero!", serverplayer);
#endif

			SetPlayerSkin(0, str);
			forcedskin = players[0].skin;

			for (i = 1; i < MAXPLAYERS; i++)
			{
				if (playeringame[i])
				{
					SetPlayerSkinByNum(i, forcedskin);

					// If it's me (or my brother), set appropriate skin value in cv_skin/cv_skin2
					if (i == consoleplayer)
						CV_StealthSet(&cv_skin, skins[forcedskin].name);
					else if (i == secondarydisplayplayer)
						CV_StealthSet(&cv_skin2, skins[forcedskin].name);
				}
			}
		}
		else if (serverplayer == -1 && playernum == adminplayer)
		{
			// in this case the adminplayer's skin is used
			SetPlayerSkin(adminplayer, str);

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (i != adminplayer && playeringame[i])
				{
					SetPlayerSkinByNum(i, forcedskin);
					// If it's me (or my brother), set appropriate skin value in cv_skin/cv_skin2
					if (i == consoleplayer)
						CV_StealthSet(&cv_skin, skins[forcedskin].name);
					else if (i == secondarydisplayplayer)
						CV_StealthSet(&cv_skin2, skins[forcedskin].name);
				}
			}
		}
		else
		{
			SetPlayerSkinByNum(playernum, forcedskin);

			if (playernum == consoleplayer)
				CV_StealthSet(&cv_skin, skins[forcedskin].name);
			else if (playernum == secondarydisplayplayer)
				CV_StealthSet(&cv_skin2, skins[forcedskin].name);
		}
	}
	else
	{
		SetPlayerSkin(playernum, str);
	}
}

static void SendWeaponPref(void)
{
	XBOXSTATIC char buf[1];

	buf[0] = (char)cv_autoaim.value;
	SendNetXCmd(XD_WEAPONPREF, buf, 1);

	if (splitscreen)
	{
		buf[0] = (char)cv_autoaim2.value;
		SendNetXCmd2(XD_WEAPONPREF, buf, 1);
	}
}

static void Got_WeaponPref(byte **cp,int playernum)
{
	if (READCHAR(*cp))
		players[playernum].pflags |= PF_AUTOAIM;
	else
		players[playernum].pflags &= ~PF_AUTOAIM;
}

void D_SendPlayerConfig(void)
{
	SendNameAndColor();
	if (splitscreen)
		SendNameAndColor2();
	SendWeaponPref();
}

static void Command_OrderPizza_f(void)
{
	if (COM_Argc() < 6 || COM_Argc() > 7)
	{
		CONS_Printf("orderpizza -size <value> -address <value> -toppings <value>: order a pizza!\n");
		return;
	}

	SendNetXCmd(XD_ORDERPIZZA, NULL, 0);
}

static void Got_PizzaOrder(byte **cp, int playernum)
{
	cp = NULL;
	CONS_Printf("%s has ordered a pizza.\n", player_names[playernum]);
}

// Only works for displayplayer, sorry!
static void Command_ResetCamera_f(void)
{
	P_ResetCamera(&players[displayplayer], &camera);
}

static void Command_RTeleport_f(void)
{
	fixed_t intx, inty, intz;
	size_t i;
	player_t *p = &players[consoleplayer];
	subsector_t *ss;

	if (!(cv_debug || devparm))
	{
		CONS_Printf(NEED_DEVMODE);
		return;
	}

	if (COM_Argc() < 3 || COM_Argc() > 7)
	{
		CONS_Printf("rteleport -x <value> -y <value> -z <value>: relative teleport to a location\n");
		return;
	}

	if (netgame)
	{
		CONS_Printf(NETGAME_TELEPORT);
		return;
	}

	if (!p->mo)
	{
		CONS_Printf(PDEAD_ETC);
		return;
	}

	i = COM_CheckParm("-x");
	if (i)
		intx = atoi(COM_Argv(i + 1));
	else
		intx = 0;

	i = COM_CheckParm("-y");
	if (i)
		inty = atoi(COM_Argv(i + 1));
	else
		inty = 0;

	ss = R_PointInSubsector(p->mo->x + intx*FRACUNIT, p->mo->y + inty*FRACUNIT);
	if (!ss || ss->sector->ceilingheight - ss->sector->floorheight < p->mo->height)
	{
		CONS_Printf("Not a valid location\n");
		return;
	}
	i = COM_CheckParm("-z");
	if (i)
	{
		intz = atoi(COM_Argv(i + 1));
		intz <<= FRACBITS;
		intz += p->mo->z;
		if (intz < ss->sector->floorheight)
			intz = ss->sector->floorheight;
		if (intz > ss->sector->ceilingheight - p->mo->height)
			intz = ss->sector->ceilingheight - p->mo->height;
	}
	else
		intz = 0;

	CONS_Printf("Teleporting by %d, %d, %d...\n", intx, inty, FixedMul((intz-p->mo->z),1));

	if (!P_TeleportMove(p->mo, p->mo->x+intx*FRACUNIT, p->mo->y+inty*FRACUNIT, intz))
		CONS_Printf(UNABLE_TELEPORT);
	else
		S_StartSound(p->mo, sfx_mixup);
}

static void Command_Teleport_f(void)
{
	fixed_t intx, inty, intz;
	size_t i;
	player_t *p = &players[consoleplayer];
	subsector_t *ss;

	if (!(cv_debug || devparm))
		return;

	if (COM_Argc() < 3 || COM_Argc() > 7)
	{
		CONS_Printf("teleport -x <value> -y <value> -z <value>: teleport to a location\n");
		return;
	}

	if (netgame)
	{
		CONS_Printf(NETGAME_TELEPORT);
		return;
	}

	if (!p->mo)
	{
		CONS_Printf(PDEAD_ETC);
		return;
	}

	i = COM_CheckParm("-x");
	if (i)
		intx = atoi(COM_Argv(i + 1));
	else
	{
		CONS_Printf("X value not specified\n");
		return;
	}

	i = COM_CheckParm("-y");
	if (i)
		inty = atoi(COM_Argv(i + 1));
	else
	{
		CONS_Printf("Y value not specified\n");
		return;
	}

	ss = R_PointInSubsector(intx*FRACUNIT, inty*FRACUNIT);
	if (!ss || ss->sector->ceilingheight - ss->sector->floorheight < p->mo->height)
	{
		CONS_Printf(INVALID_LOCATION);
		return;
	}
	i = COM_CheckParm("-z");
	if (i)
	{
		intz = atoi(COM_Argv(i + 1));
		intz <<= FRACBITS;
		if (intz < ss->sector->floorheight)
			intz = ss->sector->floorheight;
		if (intz > ss->sector->ceilingheight - p->mo->height)
			intz = ss->sector->ceilingheight - p->mo->height;
	}
	else
		intz = ss->sector->floorheight;

	CONS_Printf("Teleporting to %d, %d, %d...\n", intx, inty, FixedMul(intz,1));

	P_MapStart();
	if (!P_TeleportMove(p->mo, intx*FRACUNIT, inty*FRACUNIT, intz))
		CONS_Printf(UNABLE_TELEPORT);
	else
		S_StartSound(p->mo, sfx_mixup);
	P_MapEnd();
}

// ========================================================================

// play a demo, add .lmp for external demos
// eg: playdemo demo1 plays the internal game demo
//
// byte *demofile; // demo file buffer
static void Command_Playdemo_f(void)
{
	char name[256];

	if (COM_Argc() != 2)
	{
		CONS_Printf("playdemo <demoname>: playback a demo\n");
		return;
	}

	// disconnect from server here?
	if (demoplayback)
		G_StopDemo();
	if (netgame)
	{
		CONS_Printf(NETGAME_DEMO);
		return;
	}

	// open the demo file
	strcpy(name, COM_Argv(1));
	// dont add .lmp so internal game demos can be played

	CONS_Printf("Playing back demo '%s'.\n", name);

	G_DoPlayDemo(name);
}

static void Command_Timedemo_f(void)
{
	char name[256];

	if (COM_Argc() != 2)
	{
		CONS_Printf("timedemo <demoname>: time a demo\n");
		return;
	}

	// disconnect from server here?
	if (demoplayback)
		G_StopDemo();
	if (netgame)
	{
		CONS_Printf(NETGAME_DEMO);
		return;
	}

	// open the demo file
	strcpy (name, COM_Argv(1));
	// dont add .lmp so internal game demos can be played

	CONS_Printf("Timing demo '%s'.\n", name);

	G_TimeDemo(name);
}

// stop current demo
static void Command_Stopdemo_f(void)
{
	G_CheckDemoStatus();
	CONS_Printf("Stopped demo.\n");
}

static void Command_StartMovie_f(void)
{
	G_MovieMode(true);
}

static void Command_StopMovie_f(void)
{
	G_MovieMode(false);
}

int mapchangepending = 0;

/** Runs a map change.
  * The supplied data are assumed to be good. If provided by a user, they will
  * have already been checked in Command_Map_f().
  *
  * Do \b NOT call this function directly from a menu! M_Responder() is called
  * from within the event processing loop, and this function calls
  * SV_SpawnServer(), which calls CL_ConnectToServer(), which gives you "Press
  * ESC to abort", which calls I_GetKey(), which adds an event. In other words,
  * 63 old events will get reexecuted, with ridiculous results. Just don't do
  * it (without setting delay to 1, which is the current solution).
  *
  * \param mapnum          Map number to change to.
  * \param gametype        Gametype to switch to.
  * \param skill           Skill level to use.
  * \param resetplayers    1 to reset player scores and lives and such, 0 not to.
  * \param delay           Determines how the function will be executed: 0 to do
  *                        it all right now (must not be done from a menu), 1 to
  *                        do step one and prepare step two, 2 to do step two.
  * \param skipprecutscene To skip the precutscence or not?
  * \sa D_GameTypeChanged, Command_Map_f
  * \author Graue <graue@oceanbase.org>
  */
void D_MapChange(int mapnum, int newgametype, skill_t skill, int resetplayers, int delay, boolean skipprecutscene, boolean FLS)
{
	static char buf[MAX_WADPATH+1+5];
#define MAPNAME &buf[5]

	// The supplied data are assumed to be good.
	I_Assert(delay >= 0 && delay <= 2);

	if (devparm)
		CONS_Printf("Map change: mapnum=%d gametype=%d skill=%d resetplayers=%d delay=%d skipprecutscene=%d\n",
			mapnum, newgametype, skill, resetplayers, delay, skipprecutscene);
	if (delay != 2)
	{
		const char *mapname = G_BuildMapName(mapnum);

		I_Assert(W_CheckNumForName(mapname) != LUMPERROR);

		strncpy(MAPNAME, mapname, MAX_WADPATH);

		buf[0] = (char)skill;

		// bit 0 doesn't currently do anything
		buf[1] = 0;

		if (!resetplayers)
			buf[1] |= 2;

		// new gametype value
		buf[2] = (char)newgametype;
	}

	if (delay == 1)
		mapchangepending = 1;
	else
	{
		mapchangepending = 0;
		// spawn the server if needed
		// reset players if there is a new one
		if (!(adminplayer == consoleplayer) && SV_SpawnServer())
			buf[1] &= ~2;

		chmappending++;
		if (server && netgame)
		{
			byte seed = (byte)(totalplaytime % 256);
			SendNetXCmd(XD_RANDOMSEED, &seed, 1);
		}

		buf[3] = (char)skipprecutscene;

		buf[4] = (char)FLS;

		SendNetXCmd(XD_MAP, buf, 5+strlen(MAPNAME)+1);
	}
#undef MAPNAME
}

// Warp to map code.
// Called either from map <mapname> console command, or idclev cheat.
//
static void Command_Map_f(void)
{
	const char *mapname;
	size_t i;
	int j, newmapnum, newskill, newgametype, newresetplayers;

	// max length of command: map map03 -gametype coop -skill 3 -noresetplayers -force
	//                         1    2       3       4     5   6        7           8
	// = 8 arg max
	if (COM_Argc() < 2 || COM_Argc() > 8)
	{
		CONS_Printf("map <mapname> [-gametype <type>] [-skill <%d..%d>] [-noresetplayers] [-force]: warp to map\n",
			sk_easy, sk_insane);
		return;
	}

	if (!server && !(adminplayer == consoleplayer))
	{
		CONS_Printf("Only the server can change the level\n");
		return;
	}

	// internal wad lump always: map command doesn't support external files as in doom legacy
	if (W_CheckNumForName(COM_Argv(1)) == LUMPERROR)
	{
		CONS_Printf("\2Internal game level '%s' not found\n", COM_Argv(1));
		return;
	}

	if (!(netgame || multiplayer) && (!modifiedgame || savemoddata))
	{
		CONS_Printf("Sorry, level change disabled in single player.\n");
		return;
	}

	newresetplayers = !COM_CheckParm("-noresetplayers");

	if (!newresetplayers && !cv_debug)
	{
		CONS_Printf(NEED_DEVMODE);
		newresetplayers = true;
	}

	mapname = COM_Argv(1);
	if (strlen(mapname) != 5 || mapname[0] != 'm' || mapname[1] != 'a' || mapname[2] != 'p'
	|| (newmapnum = M_MapNumber(mapname[3], mapname[4])) == 0)
	{
		CONS_Printf("Invalid level name %s\n", mapname);
		return;
	}
	// new skill value
	newskill = gameskill; // use current one by default
	i = COM_CheckParm("-skill");
	if (i)
	{
		for (j = 0; skill_cons_t[j].strvalue; j++)
			if (!strcasecmp(skill_cons_t[j].strvalue, COM_Argv(i+1)))
			{
				newskill = j + 1; // skill_cons_t starts with "Easy" which is 1
				break;
			}
		if (!skill_cons_t[j].strvalue) // reached end of the list with no match
		{
			if ((grade & 128) && !strcasecmp("Ultimate", COM_Argv(i+1)))
				newskill = sk_insane;
			else
			{
				j = atoi(COM_Argv(i+1));
				if ((j >= sk_easy && j <= sk_nightmare)
					|| ((grade & 128) && j == sk_insane))
				{
					newskill = j;
				}
			}
		}
	}

	// new gametype value
	newgametype = gametype; // use current one by default
	i = COM_CheckParm("-gametype");
	if (i)
	{
		if (!multiplayer)
		{
			CONS_Printf("You can't switch gametypes in single player!\n");
			return;
		}

		for (j = 0; gametype_cons_t[j].strvalue; j++)
			if (!strcasecmp(gametype_cons_t[j].strvalue, COM_Argv(i+1)))
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
			// assume they gave us a gametype number, which is okay too
			for (j = 0; gametype_cons_t[j].strvalue != NULL; j++)
			{
				if (atoi(COM_Argv(i+1)) == gametype_cons_t[j].value)
				{
					newgametype = gametype_cons_t[j].value;
					break;
				}
			}
		}
	}

	// don't use a gametype the map doesn't support
	if (cv_debug || COM_CheckParm("-force") || cv_skipmapcheck.value)
		;
	else if (multiplayer)
	{
		short tol = mapheaderinfo[newmapnum-1].typeoflevel, tolflag = 0;

		switch (newgametype)
		{
			case GT_MATCH: case GTF_TEAMMATCH: tolflag = TOL_MATCH; break;
#ifdef CHAOSISNOTDEADYET
			case GT_CHAOS: tolflag = TOL_CHAOS; break;
#endif
			case GT_COOP: tolflag = TOL_COOP; break;
			case GT_RACE: case GTF_TIMEONLYRACE: tolflag = TOL_RACE; break;
			case GT_CTF: tolflag = TOL_CTF; break;
			case GT_TAG: tolflag = TOL_TAG; break;
		}

		if (!(tol & tolflag))
		{
			char gametypestring[32];

			for (i = 0; gametype_cons_t[i].strvalue != NULL; i++)
			{
				if (gametype_cons_t[i].value == newgametype)
				{
					strcpy(gametypestring, gametype_cons_t[i].strvalue);
					break;
				}
			}

			CONS_Printf("That level doesn't support %s mode!\n(Use -force to override)\n",
				gametypestring);
			return;
		}
	}
	else if (!(mapheaderinfo[newmapnum-1].typeoflevel & TOL_SP))
	{
		CONS_Printf("That level doesn't support Single Player mode!\n");
		return;
	}

	fromlevelselect = false;
	D_MapChange(newmapnum, newgametype, newskill, newresetplayers, 0, false, false);
}

/** Receives a map command and changes the map.
  *
  * \param cp        Data buffer.
  * \param playernum Player number responsible for the message. Should be
  *                  ::serverplayer or ::adminplayer.
  * \sa D_MapChange
  */
static void Got_Mapcmd(byte **cp, int playernum)
{
	char mapname[MAX_WADPATH+1];
	int skill, resetplayer = 1, lastgametype;
	boolean skipprecutscene, FLS;

	if (playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal map change received from %s\n", player_names[playernum]);
		if (server)
		{
			XBOXSTATIC char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	if (chmappending)
		chmappending--;
	skill = READBYTE(*cp);

	resetplayer = ((READBYTE(*cp) & 2) == 0);

	lastgametype = gametype;
	gametype = READBYTE(*cp);

	// Special Cases
	if (gametype == GTF_TEAMMATCH)
	{
		gametype = GT_MATCH;

		if (server)
			CV_SetValue(&cv_teamplay, 1);
	}
	else if (gametype == GTF_TIMEONLYRACE)
	{
		gametype = GT_RACE;

		if (server)
			CV_SetValue(&cv_racetype, 1);
	}

	if (gametype != lastgametype)
		D_GameTypeChanged(lastgametype); // emulate consvar_t behavior for gametype

	skipprecutscene = READBYTE(*cp);

	FLS = READBYTE(*cp);

	READSTRINGN(*cp, mapname, MAX_WADPATH);

	if (!skipprecutscene)
	{
		DEBFILE(va("Warping to %s [skill=%d resetplayer=%d lastgametype=%d gametype=%d cpnd=%d]\n",
			mapname, skill, resetplayer, lastgametype, gametype, chmappending));
		CONS_Printf(STSTR_CLEV, devparm ? mapname:"level");
	}
	if (demoplayback && !timingdemo)
		precache = false;

	if (resetplayer)
	{
		if (!FLS)
			emeralds = 0;
	}

	G_InitNew(skill, mapname, resetplayer, skipprecutscene);
	if (demoplayback && !timingdemo)
		precache = true;
	CON_ToggleOff();
	if (timingdemo)
		G_DoneLevelLoad();

	if (timeattacking)
	{
		SetPlayerSkinByNum(0, cv_chooseskin.value-1);
		players[0].skincolor = (atoi(skins[cv_chooseskin.value-1].prefcolor)) % MAXSKINCOLORS;
		CV_StealthSetValue(&cv_playercolor, players[0].skincolor);

		// a copy of color
		if (players[0].mo)
			players[0].mo->flags = (players[0].mo->flags & ~MF_TRANSLATION) | ((players[0].skincolor)<<MF_TRANSSHIFT);
	}
}

static void Command_Pause(void)
{
	XBOXSTATIC char buf;
	if (COM_Argc() > 1)
		buf = (char)(atoi(COM_Argv(1)) != 0);
	else
		buf = (char)(!paused);

	if (cv_pause.value || server || (adminplayer == consoleplayer))
	{
		if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
		{
			CONS_Printf(PAUSEINFO);
			return;
		}
		SendNetXCmd(XD_PAUSE, &buf, 1);
	}
	else
		CONS_Printf(SERVERPAUSE);
}

static void Got_Pause(byte **cp, int playernum)
{
	if (netgame && !cv_pause.value && playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal pause command received from %s\n", player_names[playernum]);
		if (server)
		{
			XBOXSTATIC char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	paused = READBYTE(*cp);

	if (!demoplayback)
	{
		if (netgame)
		{
			if (paused)
				CONS_Printf("Game paused by %s\n",player_names[playernum]);
			else
				CONS_Printf("Game unpaused by %s\n",player_names[playernum]);
		}

		if (paused)
		{
			if (!menuactive || netgame)
				S_PauseSound();
		}
		else
			S_ResumeSound();
	}
}

/** Deals with an ::XD_RANDOMSEED message in a netgame.
  * These messages set the position of the random number LUT and are crucial to
  * correct synchronization.
  *
  * Such a message should only ever come from the ::serverplayer. If it comes
  * from any other player, it is ignored.
  *
  * \param cp        Data buffer.
  * \param playernum Player responsible for the message. Must be ::serverplayer.
  * \author Graue <graue@oceanbase.org>
  */
static void Got_RandomSeed(byte **cp, int playernum)
{
	byte seed;

	seed = READBYTE(*cp);
	if (playernum != serverplayer) // it's not from the server, wtf?
		return;

	P_SetRandIndex(seed);
}

/** Clears all players' scores in a netgame.
  * Only the server or a remote admin can use this command, for obvious reasons.
  *
  * \sa XD_CLEARSCORES, Got_Clearscores
  * \author SSNTails <http://www.ssntails.org>
  */
static void Command_Clearscores_f(void)
{
	if (!(server || (adminplayer == consoleplayer)))
		return;

	SendNetXCmd(XD_CLEARSCORES, NULL, 1);
}

/** Handles an ::XD_CLEARSCORES message, which resets all players' scores in a
  * netgame to zero.
  *
  * \param cp        Data buffer.
  * \param playernum Player responsible for the message. Must be ::serverplayer
  *                  or ::adminplayer.
  * \sa XD_CLEARSCORES, Command_Clearscores_f
  * \author SSNTails <http://www.ssntails.org>
  */
static void Got_Clearscores(byte **cp, int playernum)
{
	int i;

	(void)cp;
	if (playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal clear scores command received from %s\n", player_names[playernum]);
		if (server)
		{
			XBOXSTATIC char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		players[i].score = 0;
		players[i].tagcount = 0;
	}

	CONS_Printf("Scores have been reset by the server.\n");
}

static void Command_Teamchange_f(void)
{
	char buf = 0;

	// Graue 06-19-2004: the last thing we need is more numbers to remember
	if (COM_Argc() < 2)
		buf = 0;
	else if (!strcasecmp(COM_Argv(1), "red"))
		buf = 1;
	else if (!strcasecmp(COM_Argv(1), "blue"))
		buf = 2;

	if (!(buf == 1 || buf == 2))
	{
		CONS_Printf("changeteam <color>: switch to a new team (red or blue)\n"); // Graue 06-19-2004
		return;
	}

	if (gametype != GT_CTF)
	{
		CONS_Printf(CTFCMD_ONLY);
		return;
	}

	if (buf == players[consoleplayer].ctfteam)
	{
		CONS_Printf(ALREADYONTEAM);
		return;
	}

	if (!cv_allowteamchange.value && players[consoleplayer].ctfteam)
	{
		CONS_Printf(NOTEAMCHANGE);
		return;
	}

	SendNetXCmd(XD_TEAMCHANGE, &buf, 1);
}

static void Command_Teamchange2_f(void)
{
	char buf = 0;

	// Graue 06-19-2004: the last thing we need is more numbers to remember
	if (COM_Argc() < 2)
		buf = 0;
	else if (!strcasecmp(COM_Argv(1), "red"))
		buf = 1;
	else if (!strcasecmp(COM_Argv(1), "blue"))
		buf = 2;

	if (!(buf == 1 || buf == 2))
	{
		CONS_Printf("changeteam2 <color>: switch to a new team (red or blue)\n"); // Graue 06-19-2004
		return;
	}

	if (gametype != GT_CTF)
	{
		CONS_Printf(CTFCMD_ONLY);
		return;
	}

	if (buf == players[secondarydisplayplayer].ctfteam)
	{
		CONS_Printf(ALREADYONTEAM);
		return;
	}

	if (!cv_allowteamchange.value && !players[secondarydisplayplayer].ctfteam)
	{
		CONS_Printf(NOTEAMCHANGE);
		return;
	}

	SendNetXCmd2(XD_TEAMCHANGE, &buf, 1);
}

static void Command_ServerTeamChange_f(void)
{
	char buf = 0;
	int playernum;

	if (!(server || (adminplayer == consoleplayer)))
	{
		CONS_Printf("You're not the server. You can't change players' teams.\n");
		return;
	}

	if (COM_Argc() < 2)
		buf = 0;
	else if (!strcasecmp(COM_Argv(2), "red"))
		buf = 1;
	else if (!strcasecmp(COM_Argv(2), "blue"))
		buf = 2;

	if (!buf)
	{
		CONS_Printf("serverteamchange <playernum> <color>: change a player's team (red or blue)\n");
		return;
	}

	if (gametype != GT_CTF)
	{
		CONS_Printf(CTFCMD_ONLY);
		return;
	}

	playernum = atoi(COM_Argv(1));

	if (buf == players[playernum].ctfteam)
	{
		CONS_Printf("The player is already on that team!\n");
		return;
	}

	buf |= 4; // This signals that it's a server change

	buf = (char)(buf + (char)(playernum << 3));

	SendNetXCmd(XD_TEAMCHANGE,&buf,1);
}

static void Got_Teamchange(byte **cp, int playernum)
{
	int newteam;
	newteam = READBYTE(*cp);

	if (gametype != GT_CTF)
	{
		// this should never happen unless the client is hacked/buggy
		CONS_Printf("Illegal team change received from player %s\n", player_names[playernum]);
		if (server)
		{
			XBOXSTATIC char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
	}

	// Prevent multiple changes in one go.
	if (players[playernum].ctfteam == newteam)
		return;

	if (newteam & 4) // Special marker that the server sent the request
	{
		if (playernum != serverplayer && (playernum != adminplayer || (newteam&3) == 0 || (newteam&3) == 3))
		{
			CONS_Printf("Illegal team change received from player %s\n", player_names[playernum]);
			if (server)
			{
				XBOXSTATIC char buf[2];

				buf[0] = (char)playernum;
				buf[1] = KICK_MSG_CON_FAIL;
				SendNetXCmd(XD_KICK, &buf, 2);
			}
			return;
		}
		playernum = newteam >> 3;
	}

	newteam &= 3; // We do this either way, since... who cares?
	if (server && (!newteam || newteam == 3))
	{
		XBOXSTATIC char buf[2];

		buf[0] = (char)playernum;
		buf[1] = KICK_MSG_CON_FAIL;
		CONS_Printf("Player %s sent illegal team change to team %d\n",
			player_names[playernum], newteam);
		SendNetXCmd(XD_KICK, &buf, 2);
	}

	if (!players[playernum].ctfteam)
	{
		players[playernum].ctfteam = newteam;

		if (playernum == consoleplayer)
			displayplayer = consoleplayer;
	}

	if (players[playernum].mo) // Safety first!
		P_DamageMobj(players[playernum].mo, NULL, NULL, 10000);

	players[playernum].ctfteam = newteam;

	if (newteam == 1)
		CONS_Printf("%s switched to the red team\n", player_names[playernum]);
	else // newteam == 2
		CONS_Printf("%s switched to the blue team\n", player_names[playernum]);

	if (displayplayer != consoleplayer && players[consoleplayer].ctfteam)
		displayplayer = consoleplayer;

	if (playernum == consoleplayer)
		CV_SetValue(&cv_playercolor, newteam + 5);
	else if (playernum == secondarydisplayplayer)
		CV_SetValue(&cv_playercolor2, newteam + 5);
}

// Remote Administration
static void Command_Changepassword_f(void)
{
	if (!server) // cannot change remotely
	{
		CONS_Printf("You're not the server. You can't change this.\n");
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf("password <password>: change password\n");
		return;
	}

	strncpy(adminpassword, COM_Argv(1), 8);

	// Pad the password
	if (strlen(COM_Argv(1)) < 8)
	{
		size_t i;
		for (i = strlen(COM_Argv(1)); i < 8; i++)
			adminpassword[i] = 'a';
	}
}

static void Command_Login_f(void)
{
	XBOXSTATIC char password[9];

	// If the server uses login, it will effectively just remove admin privileges
	// from whoever has them. This is good.

	if (COM_Argc() != 2)
	{
		CONS_Printf("login <password>: Administrator login\n");
		return;
	}

	strncpy(password, COM_Argv(1), 8);

	// Pad the password
	if (strlen(COM_Argv(1)) < 8)
	{
		size_t i;
		for (i = strlen(COM_Argv(1)); i < 8; i++)
			password[i] = 'a';
	}

	password[8] = '\0';

	CONS_Printf("Sending Login...%s\n(Notice only given if password is correct.)\n", password);

	SendNetXCmd(XD_LOGIN, password, 9);
}

static void Got_Login(byte **cp, int playernum)
{
	char compareword[9];

	READMEM(*cp, compareword, 9);

	if (!server)
		return;

	if (!strcmp(compareword, adminpassword))
	{
		CONS_Printf("%s passed authentication. (%s)\n", player_names[playernum], compareword);
		COM_BufInsertText(va("verify %d\n", playernum)); // do this immediately
	}
	else
		CONS_Printf("Password from %d failed (%s)\n", playernum, compareword);
}

static void Command_Verify_f(void)
{
	XBOXSTATIC char buf[8]; // Should be plenty
	char *temp;
	int playernum;

	if (!server)
	{
		CONS_Printf("You're not the server... you can't give out admin privileges!\n");
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf("verify <node>: give admin privileges to a node\n");
		return;
	}

	strncpy(buf, COM_Argv(1), 7);

	playernum = atoi(buf);

	temp = buf;

	WRITEBYTE(temp, playernum);

	SendNetXCmd(XD_VERIFIED, buf, 1);
}

static void Got_Verification(byte **cp, int playernum)
{
	int num = READBYTE(*cp);

	if (playernum != serverplayer) // it's not from the server, ignore (hacker or bug)
	{
		/// \ debfile only?
		CONS_Printf("ERROR: Got invalid verification notice from player %d (serverplayer is %d)\n",
			playernum, serverplayer);
		return;
	}

	adminplayer = num;

	if (num != consoleplayer)
		return;

	CONS_Printf("Password correct. You are now an administrator.\n");
}

static void Command_MotD_f(void)
{
	size_t i, j;
	XBOXSTATIC char mymotd[sizeof(motd)];

	if ((j = COM_Argc()) < 2)
	{
		CONS_Printf("motd <message>: Set a message that clients see upon join.\n");
		return;
	}

	if (!(server || (adminplayer == consoleplayer)))
	{
		CONS_Printf(SERVERONLY);
		return;
	}

	strlcpy(mymotd, COM_Argv(1), sizeof mymotd);
	for (i = 2; i < j; i++)
	{
		strlcat(mymotd, " ", sizeof mymotd);
		strlcat(mymotd, COM_Argv(i), sizeof mymotd);
	}

	SendNetXCmd(XD_SETMOTD, mymotd, strlen(mymotd));
}

static void Got_MotD_f(byte **cp, int playernum)
{
	if (playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal map change received from %s\n", player_names[playernum]);
		if (server)
		{
			XBOXSTATIC char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	READSTRINGN(*cp, motd, sizeof(motd));

	CONS_Printf("Message of the day set.\n");
}

static void Command_RunSOC(void)
{
	const char *fn;
	XBOXSTATIC char buf[255];
	size_t length = 0;

	if (COM_Argc() != 2)
	{
		CONS_Printf("runsoc <socfile.soc> or <lumpname>: run a soc\n");
		return;
	}
	else
		fn = COM_Argv(1);

	if (netgame && !(server || consoleplayer == adminplayer))
	{
		CONS_Printf(SERVERONLY);
		return;
	}

	if (!modifiedgame)
	{
		modifiedgame = true;
		if (!(netgame || multiplayer))
			CONS_Printf(GAMEMODIFIED);
	}

	if (!(netgame || multiplayer))
	{
		P_RunSOC(fn);
		return;
	}

	nameonly(strcpy(buf, fn));
	length = strlen(buf)+1;

	SendNetXCmd(XD_RUNSOC, buf, length);
}

static void Got_RunSOCcmd(byte **cp, int playernum)
{
	char filename[256];
	filestatus_t ncs = FS_NOTFOUND;

	if (playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal runsoc command received from %s\n", player_names[playernum]);
		if (server)
		{
			XBOXSTATIC char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	READSTRINGN(*cp, filename, 255);

	// Maybe add md5 support?
	if (strstr(filename, ".soc") != NULL)
	{
		ncs = findfile(filename,NULL,true);

		if (ncs != FS_FOUND)
		{
			Command_ExitGame_f();
			if (ncs == FS_NOTFOUND)
			{
				CONS_Printf(CLIENT_NEEDFILE, filename);
			}
			else
			{
				CONS_Printf("Unknown error finding soc file (%s) the server added.\n", filename);
			}
			return;
		}
	}

	P_RunSOC(filename);
}

/** Adds a pwad at runtime.
  * Searches for sounds, maps, music, new images.
  */
static void Command_Addfile(void)
{
	const char *fn;
	XBOXSTATIC char buf[255];
	size_t length;

	CONS_Printf("lolwut1?\n");
	return;

	if (COM_Argc() != 2)
	{
		CONS_Printf("addfile <wadfile.wad>: load wad file\n");
		return;
	}
	else
		fn = COM_Argv(1);

	if (!W_VerifyNMUSlumps(fn))
	{
		// ... But only so long as they contain nothing more then music and sprites.
		if (netgame && !(server || adminplayer))
		{
			CONS_Printf(SERVERONLY);
			return;
		}
		if (!(netgame || multiplayer) && !modifiedgame) // Only announce modifiedgame ONCE.
			CONS_Printf(GAMEMODIFIED);
		modifiedgame = true;
	}

	if (!(netgame || multiplayer))
	{
		P_AddWadFile(fn, NULL);
		return;
	}

	nameonly(strcpy(buf, fn));
	length = strlen(buf)+1;

	{
		unsigned char md5sum[16+1] = "";
#ifndef NOMD5
		FILE *fhandle;

		fhandle = fopen(fn, "rb");

		if (fhandle)
		{
			tic_t t = I_GetTime();
#ifdef _arch_dreamcast
			CONS_Printf("Making MD5 for %s\n",fn);
#endif
			md5_stream(fhandle, md5sum);
#ifndef _arch_dreamcast
			if (devparm)
#endif
				CONS_Printf("MD5 calc for %s took %f second\n",
					fn, (float)(I_GetTime() - t)/TICRATE);
			fclose(fhandle);
		}
		else
		{
			CONS_Printf(FILE_NOT_FOUND);
			return;
		}
#endif
		memcpy(&buf[length+1], md5sum, sizeof (md5sum));
		length += sizeof (md5sum);
	}

	if (adminplayer == consoleplayer) // Request to add file
		SendNetXCmd(XD_REQADDFILE, buf, length);
	else
		SendNetXCmd(XD_ADDFILE, buf, length);
}

/** removes the last added pwad at runtime.
  * Searches for sounds, maps, music and images to remove
  */
static void Command_Delfile(void)
{
	if (gamestate == GS_LEVEL)
	{
		CONS_Printf("You must NOT be in a level to use this.\n");
		return;
	}

	if (netgame && !(server || adminplayer))
	{
		CONS_Printf(SERVERONLY);
		return;
	}

	if (!(netgame || multiplayer) && mainwads != numwadfiles)
	{
		P_DelWadFile();
		if (mainwads == numwadfiles && modifiedgame)
		{
			modifiedgame = false;
		}
		return;
	}

	SendNetXCmd(XD_DELFILE, NULL, 0);
}

static void Got_RequestAddfilecmd(byte **cp, int playernum)
{
	char filename[256];
	filestatus_t ncs = FS_NOTFOUND;
	unsigned char md5sum[16+1];

	READSTRINGN(*cp, filename, 255);
	(void)READBYTE(*cp);
	READMEM(*cp, md5sum, 17);

	// Only the server processes this message.
	if (!server)
		return;

	if (playernum != serverplayer && playernum != adminplayer)
	{
		XBOXSTATIC char buf[2];

		CONS_Printf("Illegal addfile command received from %s\n", player_names[playernum]);

		buf[0] = (char)playernum;
		buf[1] = KICK_MSG_CON_FAIL;
		SendNetXCmd(XD_KICK, &buf, 2);
		return;
	}

	ncs = findfile(filename,md5sum,true);

	if (ncs != FS_FOUND)
	{
		char message[256];

		if (ncs == FS_NOTFOUND)
			sprintf(message, "The server doesn't have %s\n", filename);
		else if (ncs == FS_MD5SUMBAD)
			sprintf(message, "Checksum mismatch on %s\n", filename);
		else
			sprintf(message, "Unknown error finding wad file (%s)\n", filename);

		CONS_Printf(message);

		if (adminplayer)
			COM_BufAddText(va("sayto %d %s", adminplayer, message));

		return;
	}

	COM_BufAddText(va("addfile %s\n", filename));
}

static void Got_Delfilecmd(byte **cp, int playernum)
{
	if (playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal delfile command received from %s\n", player_names[playernum]);
		if (server)
		{
			XBOXSTATIC char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}
	(void)cp;
	P_DelWadFile();
	if (mainwads == numwadfiles && modifiedgame)
	{
		modifiedgame = false;
	}
}

static void Got_Addfilecmd(byte **cp, int playernum)
{
	char filename[256];
	filestatus_t ncs = FS_NOTFOUND;
	unsigned char md5sum[16+1];

	READSTRINGN(*cp, filename, 255);
	(void)READBYTE(*cp);
	READMEM(*cp, md5sum, 17);

	if (playernum != serverplayer)
	{
		CONS_Printf("Illegal addfile command received from %s\n", player_names[playernum]);
		if (server)
		{
			XBOXSTATIC char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	ncs = findfile(filename,md5sum,true);

	if (ncs != FS_FOUND)
	{
		Command_ExitGame_f();
		if (ncs == FS_NOTFOUND)
		{
			CONS_Printf(CLIENT_NEEDFILE, filename);
		}
		else if (ncs == FS_MD5SUMBAD)
		{
			CONS_Printf("Checksum mismatch while loading %s.\nMake sure you have the copy of\nthis file that the server has.\n", filename);
		}
		else
		{
			CONS_Printf("Unknown error finding wad file (%s) the server added.\n", filename);
		}
		return;
	}

	P_AddWadFile(filename, NULL);
	modifiedgame = true;
}

static void Command_ListWADS_f(void)
{
	int i = numwadfiles;
	char *tempname;
	CONS_Printf("There are %d wads loaded:\n",numwadfiles);
	for (i--; i; i--)
	{
		nameonly(tempname = va("%s", wadfiles[i]->filename));
		if (i > mainwads)
			CONS_Printf("   %.2d: %s\n", i, tempname);
		else
			CONS_Printf("*  %.2d: %s\n", i, tempname);
	}
	CONS_Printf("  IWAD: %s\n", wadfiles[0]->filename);
}

// =========================================================================
//                            MISC. COMMANDS
// =========================================================================

/** Prints program version.
  */
static void Command_Version_f(void)
{
	CONS_Printf("SRB2%s (%s %s)\n", VERSIONSTRING, compdate, comptime);
}

// Returns current gametype being used.
//
static void Command_ShowGametype_f(void)
{
	CONS_Printf("Current gametype is %d\n", gametype);
}

// Moves the NiGHTS player to another axis within the current mare
// Only for development purposes.
//
static void Command_JumpToAxis_f(void)
{
	if (!cv_debug)
		CONS_Printf(NEED_DEVMODE);

	if (COM_Argc() != 2)
	{
		CONS_Printf("jumptoaxis <axisnum>: Jump to axis within current mare.\n");
		return;
	}

	P_TransferToAxis(&players[consoleplayer], atoi(COM_Argv(1)));
}

/** Plays the intro.
  */
static void Command_Playintro_f(void)
{
	if (netgame)
		return;

	F_StartIntro();
}

/** Writes the mapthings in the current level to a file.
  *
  * \sa cv_objectplace
  */
static void Command_Writethings_f(void)
{
	P_WriteThings(W_GetNumForName(G_BuildMapName(gamemap)) + ML_THINGS);
}

/** Quits the game immediately.
  */
FUNCNORETURN static ATTRNORETURN void Command_Quit_f(void)
{
	I_Quit();
}

void ObjectPlace_OnChange(void)
{
	if (gamestate != GS_LEVEL && cv_objectplace.value)
	{
		CONS_Printf(MUSTBEINLEVEL);
		CV_StealthSetValue(&cv_objectplace, false);
		return;
	}
#ifndef JOHNNYFUNCODE
	if (cv_objectplace.value) // You spoon!
	{
		CV_StealthSetValue(&cv_objectplace, 0);
		CONS_Printf("lolwut?\n");
		return;
	}
#else
	if (cv_objectplace.value)
	{
		int i;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			if (!players[i].mo)
				continue;

			if (players[i].playerstate != PST_LIVE)
				continue;

			if (players[i].pflags & PF_NIGHTSMODE)
				continue;

			P_SetTarget(&players[i].mo->target, NULL);
			players[i].mo->flags2 |= MF2_DONTDRAW;
			players[i].mo->flags |= MF_NOCLIP;
			players[i].mo->flags |= MF_NOGRAVITY;
			P_UnsetThingPosition(players[i].mo);
			players[i].mo->flags |= MF_NOBLOCKMAP;
			P_SetThingPosition(players[i].mo);
			if (!players[i].currentthing)
				players[i].currentthing = 1;
			if (!modifiedgame || savemoddata)
			{
				modifiedgame = true;
				savemoddata = false;
				if (!(netgame || multiplayer))
					CONS_Printf(GAMEMODIFIED);
			}
		}
	}
	else if (players[0].mo)
	{
		int i;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			if (!players[i].mo)
				continue;

			if (players[i].playerstate != PST_LIVE)
				continue;

			if (!(players[i].pflags & PF_NIGHTSMODE))
			{
				if (players[i].mo->target)
				{
					P_RemoveMobj(players[i].mo->target);
					P_SetTarget(&players[i].mo->target, NULL);
				}

				players[i].mo->flags2 &= ~MF2_DONTDRAW;
				players[i].mo->flags &= ~MF_NOGRAVITY;
			}

			players[i].mo->flags &= ~MF_NOCLIP;
			P_UnsetThingPosition(players[i].mo);
			players[i].mo->flags &= ~MF_NOBLOCKMAP;
			P_SetThingPosition(players[i].mo);
		}
	}
	return;
#endif

	if (cv_objectplace.value)
	{
		if (!modifiedgame || savemoddata)
		{
			modifiedgame = true;
			savemoddata = false;
			if (!(netgame || multiplayer))
				CONS_Printf(GAMEMODIFIED);
		}

		COM_BufAddText("cechoflags 1048576\n");
		COM_BufAddText("cechoduration 10\n");
		COM_BufAddText(va("cecho %s\n", OBJPLACEINFO));
		COM_BufAddText("cechoduration 5\n");
		COM_BufAddText("cechoflags 0\n");

		if ((players[0].pflags & PF_NIGHTSMODE))
			return;

		players[0].mo->flags2 |= MF2_DONTDRAW;
		players[0].mo->flags |= MF_NOCLIP;
		players[0].mo->flags |= MF_NOGRAVITY;
		P_UnsetThingPosition(players[0].mo);
		players[0].mo->flags |= MF_NOBLOCKMAP;
		P_SetThingPosition(players[0].mo);
		if (!players[0].currentthing)
			players[0].currentthing = 1;
		players[0].mo->momx = players[0].mo->momy = players[0].mo->momz = 0;
	}
	else if (players[0].mo)
	{
		if (!(players[0].pflags & PF_NIGHTSMODE))
		{
			if (players[0].mo->target)
				P_SetMobjState(players[0].mo->target, S_DISS);

			players[0].mo->flags2 &= ~MF2_DONTDRAW;
			players[0].mo->flags &= ~MF_NOGRAVITY;
		}

		players[0].mo->flags &= ~MF_NOCLIP;
		P_UnsetThingPosition(players[0].mo);
		players[0].mo->flags &= ~MF_NOBLOCKMAP;
		P_SetThingPosition(players[0].mo);
	}
}

/** Deals with a pointlimit change by printing the change to the console.
  * If the gametype is single player, cooperative, or race, the pointlimit is
  * silently disabled again.
  *
  * Timelimit and pointlimit can be used at the same time.
  *
  * We don't check immediately for the pointlimit having been reached,
  * because you would get "caught" when turning it up in the menu.
  * \sa cv_pointlimit, TimeLimit_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void PointLimit_OnChange(void)
{
	// Don't allow pointlimit in Single Player/Co-Op/Race!
	if (server && (gametype == GT_COOP || gametype == GT_RACE))
	{
		if (cv_pointlimit.value)
			CV_StealthSetValue(&cv_pointlimit, 0);
		return;
	}

	if (cv_pointlimit.value)
	{
		CONS_Printf("Levels will end after %s scores %d point%s.\n",
			cv_teamplay.value == 1 || gametype == GT_CTF
				? "a team" : "someone",
			cv_pointlimit.value,
			cv_pointlimit.value > 1 ? "s" : "");
	}
	else if (netgame || multiplayer)
		CONS_Printf("Point limit disabled\n");
}

static void NumLaps_OnChange(void)
{
	if (gametype != GT_RACE)
		return; // Just don't be verbose

	CONS_Printf("Number of laps set to %d\n", cv_numlaps.value);
}

static void NetTimeout_OnChange(void)
{
	connectiontimeout = (tic_t)cv_nettimeout.value;
}

ULONG timelimitintics = 0;

/** Deals with a timelimit change by printing the change to the console.
  * If the gametype is single player, cooperative, or race, the timelimit is
  * silently disabled again.
  *
  * Timelimit and pointlimit can be used at the same time.
  *
  * \sa cv_timelimit, PointLimit_OnChange
  */
static void TimeLimit_OnChange(void)
{
	// Don't allow timelimit in Single Player/Co-Op/Race!
	if (server && cv_timelimit.value != 0
		&& (gametype == GT_COOP || gametype == GT_RACE))
	{
		CV_SetValue(&cv_timelimit, 0);
		return;
	}

	if (cv_timelimit.value != 0)
	{
		CONS_Printf("Levels will end after %d minute%s.\n",cv_timelimit.value,cv_timelimit.value == 1 ? "" : "s"); // Graue 11-17-2003
		timelimitintics = cv_timelimit.value * 60 * TICRATE;

		// Note the deliberate absence of any code preventing
		//   pointlimit and timelimit from being set simultaneously.
		// Some people might like to use them together. It works.
	}
	else if (netgame || multiplayer)
		CONS_Printf("Time limit disabled\n");
}

/** Adjusts certain settings to match a changed gametype.
  *
  * \param lastgametype The gametype we were playing before now.
  * \sa D_MapChange
  * \author Graue <graue@oceanbase.org>
  * \todo Get rid of the hardcoded stuff, ugly stuff, etc.
  */
void D_GameTypeChanged(int lastgametype)
{
	if (netgame)
	{
		int j;
		const char *oldgt = NULL, *newgt = NULL;
		for (j = 0; gametype_cons_t[j].strvalue; j++)
		{
			if (gametype_cons_t[j].value == lastgametype)
				oldgt = gametype_cons_t[j].strvalue;
			if (gametype_cons_t[j].value == gametype)
				newgt = gametype_cons_t[j].strvalue;
		}
		if (oldgt && newgt)
			CONS_Printf("Gametype was changed from %s to %s\n", oldgt, newgt);
	}
	// Only do the following as the server, not as remote admin.
	// There will always be a server, and this only needs to be done once.
	if (server && (multiplayer || netgame))
	{
		if (gametype == GT_MATCH || gametype == GT_TAG || gametype == GT_CTF
#ifdef CHAOSISNOTDEADYET
			|| gametype == GT_CHAOS
#endif
			)
		{
			CV_SetValue(&cv_itemrespawn, 1);
		}
		else
			CV_SetValue(&cv_itemrespawn, 0);

		switch (gametype)
		{
#ifdef CHAOSISNOTDEADYET
			case GT_CHAOS:
				if (!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
				{
					// default settings for chaos: timelimit 2 mins, no pointlimit
					CV_SetValue(&cv_pointlimit, 0);
					CV_SetValue(&cv_timelimit, 2);
				}
				if (!cv_itemrespawntime.changed)
					CV_SetValue(&cv_itemrespawntime, 90); // respawn sparingly in chaos
				break;
#endif
			case GT_MATCH:
				if (!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
				{
					// default settings for match: timelimit 5 mins, no pointlimit
					CV_SetValue(&cv_pointlimit, 0);
					CV_SetValue(&cv_timelimit, 5);
				}
				if (!cv_itemrespawntime.changed)
					CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally
				break;
			case GT_TAG:
				if (!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
				{
					// default settings for match: timelimit 5 mins, no pointlimit
					CV_SetValue(&cv_timelimit, 0);
					CV_SetValue(&cv_pointlimit, 10);
				}
				if (!cv_itemrespawntime.changed)
					CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally
				break;
			case GT_CTF:
				if (!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
				{
					// default settings for CTF: no timelimit, pointlimit 5
					CV_SetValue(&cv_timelimit, 0);
					CV_SetValue(&cv_pointlimit, 5);
				}
				if (!cv_itemrespawntime.changed)
					CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally

				if (!players[consoleplayer].ctfteam)
					SendNameAndColor(); // make sure a CTF team gets assigned
				if (splitscreen && ((!netgame && !players[1].ctfteam)
					|| (secondarydisplayplayer != consoleplayer
					&& !players[secondarydisplayplayer].ctfteam)))
					SendNameAndColor2();
				break;
		}
	}
	else if (!multiplayer && !netgame)
	{
		gametype = GT_COOP;
		CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue);
		CV_SetValue(&cv_itemrespawn, 0);
	}

	if (server && gametype != GT_MATCH)
		CV_SetValue (&cv_teamplay, 0);

	// reset timelimit and pointlimit in race/coop, prevent stupid cheats
	if (server && (gametype == GT_RACE || gametype == GT_COOP))
	{
		if (cv_timelimit.value)
			CV_SetValue(&cv_timelimit, 0);
		if (cv_pointlimit.value)
			CV_SetValue(&cv_pointlimit, 0);
	}

	if ((cv_pointlimit.changed || cv_timelimit.changed) && cv_pointlimit.value)
	{
		if ((
#ifdef CHAOSISNOTDEADYET
			lastgametype == GT_CHAOS ||
#endif
			lastgametype == GT_MATCH) &&
			(gametype == GT_TAG || gametype == GT_CTF))
			CV_SetValue(&cv_pointlimit, cv_pointlimit.value / 500);
		else if ((lastgametype == GT_TAG || lastgametype == GT_CTF) &&
			(
#ifdef CHAOSISNOTDEADYET
			gametype == GT_CHAOS ||
#endif
			gametype == GT_MATCH))
			CV_SetValue(&cv_pointlimit, cv_pointlimit.value * 500);
	}

	// don't retain CTF teams in other modes
	if (lastgametype == GT_CTF)
	{
		int i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
				players[i].ctfteam = 0;
	}
}

static void Ringslinger_OnChange(void)
{
	// If you've got a grade less than 3, you can't use this.
	if ((grade&7) < 3 && !netgame && cv_ringslinger.value)
	{
		CONS_Printf(NOTEARNED);
		CV_StealthSetValue(&cv_ringslinger, 0);
		return;
	}

	if (cv_ringslinger.value) // Only if it's been turned on
	{
		if (!modifiedgame || savemoddata)
		{
			modifiedgame = true;
			savemoddata = false;
			if (!(netgame || multiplayer))
				CONS_Printf(GAMEMODIFIED);
		}
	}
}

static void Startrings_OnChange(void)
{
	if ((grade&7) < 5 && !netgame && cv_startrings.value)
	{
		CONS_Printf(NOTEARNED);
		CV_StealthSetValue(&cv_startrings, 0);
		return;
	}

	if (cv_startrings.value)
	{
		int i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && players[i].mo)
			{
				players[i].mo->health = cv_startrings.value + 1;
				players[i].health = players[i].mo->health;
			}

		if (!modifiedgame || savemoddata)
		{
			modifiedgame = true;
			savemoddata = false;
			if (!(netgame || multiplayer))
				CONS_Printf(GAMEMODIFIED);
		}
	}
}

static void Startlives_OnChange(void)
{
	if ((grade&7) < 4 && !netgame && cv_startlives.value)
	{
		CONS_Printf(NOTEARNED);
		CV_StealthSetValue(&cv_startlives, 0);
		return;
	}

	if (cv_startlives.value)
	{
		int i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
				players[i].lives = cv_startlives.value;

		if (!modifiedgame || savemoddata)
		{
			modifiedgame = true;
			savemoddata = false;
			if (!(netgame || multiplayer))
				CONS_Printf(GAMEMODIFIED);
		}
	}
}

static void Startcontinues_OnChange(void)
{
	if ((grade&7) < 4 && !netgame && cv_startcontinues.value)
	{
		CONS_Printf(NOTEARNED);
		CV_StealthSetValue(&cv_startcontinues, 0);
		return;
	}

	if (cv_startcontinues.value)
	{
		int i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
				players[i].continues = cv_startcontinues.value;

		if (!modifiedgame || savemoddata)
		{
			modifiedgame = true;
			savemoddata = false;
			if (!(netgame || multiplayer))
				CONS_Printf(GAMEMODIFIED);
		}
	}
}

static void Gravity_OnChange(void)
{
	if ((grade&7) < 2 && !netgame
		&& strcmp(cv_gravity.string, cv_gravity.defaultvalue))
	{
		CONS_Printf(NOTEARNED);
		CV_StealthSet(&cv_gravity, cv_gravity.defaultvalue);
		return;
	}

	gravity = cv_gravity.value;
}

static void Command_Showmap_f(void)
{
	if (gamestate == GS_LEVEL)
	{
		if (mapheaderinfo[gamemap-1].actnum)
			CONS_Printf("MAP %d: %s %d\n", gamemap, mapheaderinfo[gamemap-1].lvlttl, mapheaderinfo[gamemap-1].actnum);
		else
			CONS_Printf("MAP %d: %s\n", gamemap, mapheaderinfo[gamemap-1].lvlttl);
	}
	else
		CONS_Printf(MUSTBEINLEVEL);
}

static void Command_ExitLevel_f(void)
{
	if (!(netgame || (multiplayer && gametype != GT_COOP)) && !cv_debug)
	{
		CONS_Printf(CANTUSESINGLEPLAYER);
		return;
	}
	if (!(server || (adminplayer == consoleplayer)))
	{
		CONS_Printf(SERVERONLY);
		return;
	}
	if (gamestate != GS_LEVEL || demoplayback)
		CONS_Printf(MUSTBEINLEVEL);

	SendNetXCmd(XD_EXITLEVEL, NULL, 0);
}

static void Got_ExitLevelcmd(byte **cp, int playernum)
{
	cp = NULL;
	if (playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal exitlevel command received from %s\n", player_names[playernum]);
		if (server)
		{
			XBOXSTATIC char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	G_ExitLevel();
}

/** Prints the number of the displayplayer.
  *
  * \todo Possibly remove this; it was useful for debugging at one point.
  */
static void Command_Displayplayer_f(void)
{
	CONS_Printf("Displayplayer is %d\n", displayplayer);
}

static void Command_Skynum_f(void)
{
	if (!cv_debug)
	{
		CONS_Printf(NEED_DEVMODE);
		CONS_Printf("If you want to change the sky interactively on a map, use the linedef executor feature instead.\n");
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf("skynum <sky#>: change the sky\n");
		return;
	}

	if (netgame || multiplayer)
	{
		CONS_Printf(CANTUSEMULTIPLAYER);
		return;
	}

	CONS_Printf("Previewing sky %s...\n", COM_Argv(1));

	P_SetupLevelSky(atoi(COM_Argv(1)));
}

static void Command_Tunes_f(void)
{
	int tune;
	const size_t argc = COM_Argc();

	if (argc < 2) //tunes slot ...
	{
		CONS_Printf("tunes <slot> <speed>: play a slot at speed(100%%)\n");
		return;
	}

	tune = atoi(COM_Argv(1));

	if (tune < mus_None || tune >= NUMMUSIC)
	{
		CONS_Printf("valid slots are 1 to %d, or 0 to stop music\n", NUMMUSIC - 1);
		return;
	}

	mapmusic = (short)(tune | 2048);

	if (tune == mus_None)
		S_StopMusic();
	else
		S_ChangeMusic(tune, true);

	if (argc > 2)
	{
		float speed = (float)atof(COM_Argv(2));
		if (speed > 0.0f)
			S_SpeedMusic(speed);
	}
}

/** Quits a game and returns to the title screen.
  *
  */
void Command_ExitGame_f(void)
{
	int i;

	D_QuitNetGame();
	CL_Reset();
	CV_ClearChangedFlags();

	for (i = 0; i < MAXPLAYERS; i++)
		CL_ClearPlayer(i);

	splitscreen = false;
	SplitScreen_OnChange();
	cv_debug = 0;

	if (!timeattacking)
		D_StartTitle();
}

#ifdef FISHCAKE
// Fishcake is back, but only if you've cleared Very Hard mode
static void Fishcake_OnChange(void)
{
	if (grade & 128)
	{
		cv_debug = cv_fishcake.value;
		// consvar_t's get changed to default when registered
		// so don't make modifiedgame always on!
		if (cv_debug)
		{
			if (!modifiedgame || savemoddata)
			{
				modifiedgame = true;
				savemoddata = false;
				if (!(netgame || multiplayer))
					CONS_Printf(GAMEMODIFIED);
			}
		}
	}

	else if (cv_debug != cv_fishcake.value)
		CV_SetValue(&cv_fishcake, cv_debug);
}
#endif

/** Reports to the console whether or not the game has been modified.
  *
  * \todo Make it obvious, so a console command won't be necessary.
  * \sa modifiedgame
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Isgamemodified_f(void)
{
	if (savemoddata)
		CONS_Printf("modifiedgame is true, but you can save emblem and time data in this mod.\n");
	else if (modifiedgame)
		CONS_Printf("modifiedgame is true, secrets will not be unlocked\n");
	else
		CONS_Printf("modifiedgame is false, you can unlock secrets\n");
}

#ifdef _DEBUG
static void Command_Togglemodified_f(void)
{
	modifiedgame = !modifiedgame;
}
#endif

/** Sets forced skin.
  * Available only to a dedicated server owner, who doesn't have an actual
  * player.
  *
  * This is not available to a remote admin, but a skin change from the remote
  * admin is treated as authoritative in that case.
  *
  * \todo Make the change take effect immediately.
  * \sa ForceSkin_OnChange, cv_forceskin, forcedskin
  * \author Graue <graue@oceanbase.org>
  */
static void Command_SetForcedSkin_f(void)
{
	int newforcedskin;

	if (COM_Argc() != 2)
	{
		CONS_Printf("setforcedskin <num>: set skin for clients to use\n");
		return;
	}

	newforcedskin = atoi(COM_Argv(1));

	if (newforcedskin < 0 || newforcedskin >= numskins)
	{
		CONS_Printf("valid skin numbers are 0 to %d\n", numskins - 1);
		return;
	}

	forcedskin = newforcedskin;
}

/** Makes a change to ::cv_forceskin take effect immediately.
  *
  * \todo Move the enforcement code out of SendNameAndColor() so this hack
  *       isn't needed.
  * \sa Command_SetForcedSkin_f, cv_forceskin, forcedskin
  * \author Graue <graue@oceanbase.org>
  */
static void ForceSkin_OnChange(void)
{
	if (cv_forceskin.value)
		SendNameAndColor(); // have it take effect immediately
}

/** Sends a skin change for the console player, unless that player is moving.
  * \sa cv_skin, Skin2_OnChange, Color_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Skin_OnChange(void)
{
	if (!P_PlayerMoving(consoleplayer))
		SendNameAndColor();
	else
		CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);
}

/** Sends a skin change for the secondary splitscreen player, unless that
  * player is moving.
  * \sa cv_skin2, Skin_OnChange, Color2_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Skin2_OnChange(void)
{
	if (!P_PlayerMoving(secondarydisplayplayer))
		SendNameAndColor2();
	else
		CV_StealthSet(&cv_skin2, skins[players[secondarydisplayplayer].skin].name);
}

/** Sends a color change for the console player, unless that player is moving.
  * \sa cv_playercolor, Color2_OnChange, Skin_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Color_OnChange(void)
{
	if (!P_PlayerMoving(consoleplayer))
		SendNameAndColor();
	else
		CV_StealthSetValue(&cv_playercolor,
			players[consoleplayer].skincolor);
}

/** Sends a color change for the secondary splitscreen player, unless that
  * player is moving.
  * \sa cv_playercolor2, Color_OnChange, Skin2_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Color2_OnChange(void)
{
	if (!P_PlayerMoving(secondarydisplayplayer))
		SendNameAndColor2();
	else
		CV_StealthSetValue(&cv_playercolor2,
			players[secondarydisplayplayer].skincolor);
}

/** Displays the result of the chat being muted or unmuted.
  * The server or remote admin should already know and be able to talk
  * regardless, so this is only displayed to clients.
  *
  * \sa cv_mute
  * \author Graue <graue@oceanbase.org>
  */
static void Mute_OnChange(void)
{
	if (server || (adminplayer == consoleplayer))
		return;

	if (cv_mute.value)
		CONS_Printf("Chat has been muted.\n");
	else
		CONS_Printf("Chat is no longer muted.\n");
}

/** Hack to clear all changed flags after game start.
  * A lot of code (written by dummies, obviously) uses COM_BufAddText() to run
  * commands and change consvars, especially on game start. This is problematic
  * because CV_ClearChangedFlags() needs to get called on game start \b after
  * all those commands are run.
  *
  * Here's how it's done: the last thing in COM_BufAddText() is "dummyconsvar
  * 1", so we end up here, where dummyconsvar is reset to 0 and all the changed
  * flags are set to 0.
  *
  * \todo Fix the aforementioned code and make this hack unnecessary.
  * \sa cv_dummyconsvar
  * \author Graue <graue@oceanbase.org>
  */
static void DummyConsvar_OnChange(void)
{
	if (cv_dummyconsvar.value == 1)
	{
		CV_SetValue(&cv_dummyconsvar, 0);
		CV_ClearChangedFlags();
	}
}

static void Command_ShowScores_f(void)
{
	byte i;

	if (!(netgame || multiplayer))
	{
		CONS_Printf(NETGAMEONLY);
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
			CONS_Printf("%s's score is %lu\n", player_names[i], players[i].score);
	}
	CONS_Printf("The pointlimit is %d\n", cv_pointlimit.value);

}

static void Command_ShowTime_f(void)
{
	if (!(netgame || multiplayer))
	{
		CONS_Printf(NETGAMEONLY);
		return;
	}

	CONS_Printf("The current time is %f.\nThe timelimit is %f\n", (double)leveltime/TICRATE, (double)timelimitintics/TICRATE);
}
