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
/// \brief Globally defined strings.

#include "doomdef.h"
#include "dstrings.h"

/*
Character reference for future "Spanish Mode"

á, é, í, ó, ú, ñ, ¿, ¡
*/

const char *text[NUMTEXT] =
{
	"Development mode ON.\n",
	"CD-ROM Version: default.cfg from c:\\srb2data\n",
	"press a key.",
	"press y or n.",
	"only the server can do a load net game!\n\npress a key.",
	"you can't quickload during a netgame!\n\npress a key.",
	"you haven't picked a quicksave slot yet!\n\npress a key.",
	"you can't save if you aren't playing!\n\npress a key.",
	"quicksave over your game named\n\n'%s'?\n\npress y or n.",
	"do you want to quickload the game named\n\n'%s'?\n\npress y or n.",
	"You are in a network game.\n""End it?\n(Y/N).\n",
	"are you sure? this skill level\nisn't even remotely fair.\n\npress y or n.",
	"Messages are OFF",
	"Messages are ON",
	"you can't end a netgame!\n\npress a key.",
	"are you sure you want to end the game?\n\npress y or n.",

	"%s\n\n(press 'y' to quit)",

	"Gamma correction OFF\n",
	"Gamma correction level 1\n",
	"Gamma correction level 2\n",
	"Gamma correction level 3\n",
	"Gamma correction level 4\n",
	"empty slot\n",

	"game saved.\n",
	"[Message unsent]\n",

	"Speeding off to %s...\n",

	"Player is dead, etc.\n",
	"Unable to teleport to that spot!\n",
	"You can't teleport while in a netgame!\n",
	"Not a valid location\n",
	"\nYou can't play a demo while in a netgame\n",
	"This command is only for capture the flag games.\n",
	"You're already on that team!\n",
	"Server does not allow team change.\n",
	"You are not the server. You cannot do this.\n",
	"WARNING: Game must be restarted to record statistics.\n",
	"File doesn't exist.\n",
	"The server tried to add %s,\nbut you don't have this file.\nYou need to find it in order\nto play on this server.",
	"Devmode must be enabled.\n",
	"You haven't earned this yet.\n",
	"You must be in a level to use this.\n",
	"You can't use this in single player!\n",
	"Can't use this in a multiplayer game, sorry!\n",
	"This only works in a netgame.\n",
	"This only works in single player.\n",
	"Only the server can pause the game.\n",
	"You can only pause while in a level or intermission.\n",
	"Couldn't read file %s\n",
	"Objectplace Controls:\\"\
	"\\"\
	"Camera L: Cycle backwards      \\"\
	"Camera R: Cycle forwards       \\"\
	"    Jump: Float up             \\"\
	"    Spin: Float down           \\"\
	"   Throw: Place object         \\"\
	"   Taunt: Remove object (buggy)\\",

	"ERROR: Powerup has no target!\n",

	"%s returned the red flag to base.\n",
	"%s picked up the red flag!\n",
	"%s returned the blue flag to base.\n",
	"%s picked up the blue flag!\n",
	"%s tossed the %s flag.\n",
	"%s dropped the %s flag.\n",
	"%s%s%s crushed %s%s%s with a heavy object!\n",
	"%s%s%s suicides\n",
	"%s%s%s died.\n",
	"hit",
	"killed",
	"%s%s%s got nuked by %s%s%s!\n",
	"%s%s%s was burnt by %s%s%s's fire trail.\n",
	"%s%s%s was fried by %s%s%s's fire trail.\n",
	"%s%s%s was %s by %s%s%s's bounce ring.\n",
	"%s%s%s was %s by %s%s%s's automatic ring.\n",
	"%s%s%s was %s by %s%s%s's explosion ring.\n",
	"%s%s%s was %s by %s%s%s's rail ring.\n",
	"%s%s%s was %s by %s%s%s's scatter ring.\n",
	"%s%s%s was %s by %s%s%s's grenade ring.\n",
	"%s%s%s was %s by %s%s%s's ring.\n",
	"%s%s%s was %s by %s%s%s\n",
	"%s%s%s drowned.\n",
	"%s%s%s was crushed.\n",
	"%s%s%s was %s by a blue crawla!\n",
	"%s%s%s was %s by a red crawla!\n",
	"%s%s%s was %s by a jetty-syn gunner!\n",
	"%s%s%s was %s by a jetty-syn bomber!\n",
	"%s%s%s was %s by a crawla commander!\n",
	"%s%s%s was %s by the Egg Mobile!\n",
	"%s%s%s was %s by the Egg Slimer!\n",
	"%s%s%s fell into a bottomless pit.\n",
	"%s%s%s fell in some nasty goop!\n",
	"%s%s%s was %s by %s%s%s\n",
	"%s caused a world of pain.\n",
	"%s got a game over.\n",
	"%s has %d lives remaining.\n",
	"%s is it!\n",

	"Warning: State Cycle Detected\n",

	"%s finished the final lap\n",
	"%s started lap %d\n",

	"%s has completed the level.\n",
	"Sorry, you're too high to place this object (max: 4095 above bottom floor).\n",
	"Sorry, you're too high to place this object (max: 2047 above bottom floor).\n",
	"%s ran out of time.\n",
	"ERROR: GAME SKILL UNDETERMINED!\n",

	" #",

	"Two months had passed since Dr. Eggman\n"\
	"tried to take over the world using his\n"\
	"Ring Satellite.\n#",

	"As it was about to drain the rings\n"\
	"away from the planet, Sonic burst into\n"\
	"the Satellite and for what he thought\n"\
	"would be the last time, defeated\n"\
	"Dr. Eggman.\n#",

	"\nWhat Sonic, Tails, and Knuckles had\n"\
	"not anticipated was that Eggman would\n"
	"return, bringing an all new threat.\n#",

	"About once every year, a strange asteroid\n"\
	"hovers around the planet. it suddenly\n"\
	"appears from nowhere, circles around, and\n"\
	"- just as mysteriously as it arrives, it\n"\
	"vanishes after about two months.\n"\
	"No one knows why it appears, or how.\n#",

	"\"Curses!\" Eggman yelled. \"That hedgehog\n"\
	"and his ridiculous friends will pay\n"\
	"dearly for this!\" Just then his scanner\n"\
	"blipped as the Black Rock made its\n"\
	"appearance from nowhere. Eggman looked at\n"\
	"the screen, and just shrugged it off.\n#",

	"It was only later\n"\
	"that he had an\n"\
	"idea. \"The Black\n"\
	"Rock usually has a\n"\
	"lot of energy\n"\
	"within it... if I\n"\
	"can somehow\n"\
	"harness this, I\n"\
	"can turn it into\n"\
	"the ultimate\n"\
	"battle station,\n"\
	"and every last\n"\
	"person will be\n"\
	"begging for mercy,\n"\
	"including Sonic!\"\n#",

	"\n\nBefore beginning his scheme,\n"\
	"Eggman decided to give Sonic\n"\
	"a reunion party...\n#",

	"\"We're ready to fire in 15 seconds!\"\n"\
	"The robot said, his voice crackling a\n"
	"little down the com-link. \"Good!\"\n"\
	"Eggman sat back in his Egg-Mobile and\n"\
	"began to count down as he saw the\n"\
	"GreenFlower city on the main monitor.\n#",

	"\"10...9...8...\"\n"\
	"Meanwhile, Sonic was tearing across the\n"\
	"zones, and everything became nothing but\n"\
	"a blur as he ran around loops, skimmed\n"\
	"over water, and catapulted himself off\n"\
	"rocks with his phenomenal speed.\n#",

	"\"5...4...3...\"\n"\
	"Sonic knew he was getting closer to the\n"\
	"City, and pushed himself harder. Finally,\n"\
	"the city appeared in the horizon.\n"\
	"\"2...1...Zero.\"\n#",

	"GreenFlower City was gone.\n"\
	"Sonic arrived just in time to see what\n"\
	"little of the 'ruins' were left. Everyone\n"\
	"and everything in the city had been\n"\
	"obliterated.\n#",

	"\"You're not quite as dead as we thought,\n"\
	"huh? Are you going to tell us your plan as\n"\
	"usual or will I 'have to work it out' or\n"\
	"something?\"                         \n"\
	"\"We'll see... let's give you a quick warm\n"\
	"up, Sonic! JETTYSYNS! Open fire!\"\n#",

	"Eggman took this\n"\
	"as his cue and\n"\
	"blasted off,\n"\
	"leaving Sonic\n"\
	"and Tails behind.\n"\
	"Tails looked at\n"\
	"the ruins of the\n"\
	"Greenflower City\n"\
	"with a grim face\n"\
	"and sighed.           \n"\
	"\"Now what do we\n"\
	"do?\", he asked.\n#",

	"\"Easy! We go\n"\
	"find Eggman\n"\
	"and stop his\n"\
	"latest\n"\
	"insane plan.\n"\
	"Just like\n"\
	"we've always\n"\
	"done, right?                 \n\n"\
	"...                    \n\n"\
	"\"Tails,what\n"\
	"*ARE* you\n"\
	"doing?\"\n#",

	"\"I'm just finding what mission obje...\n"\
	"a-ha! Here it is! This will only give\n"\
	"the robot's primary objective. It says,\n"\
	"* LOCATE AND RETRIEVE CHAOS EMERALD.\n"\
	"ESTIMATED LOCATION: GREENFLOWER ZONE *\"\n"\
	"\"All right, then let's go!\"\n#",

/*
"What are we waiting for? The first emerald is ours!" Sonic was about to
run, when he saw a shadow pass over him, he recognized the silhouette
instantly.
	"Knuckles!" Sonic said. The echidna stopped his glide and landed
facing Sonic. "What are you doing here?"
	He replied, "This crisis affects the Floating Island,
if that explosion I saw is anything to go by."
	"If you're willing to help then... let's go!"
	*/

	"Press 'N' and Shadow\nwill cut himself!\n...\nPress 'N'! Press 'N'!",
	"lol dont go\npaly sum moar, n00b!!1!!!!11!1!!",
	"Press 'N' to obtain HMS123311",
	"Press 'Y' to be banned.",
	"You're going to\ngive me up, aren't you?",
	"Want some grape soda?",
	"What do you mean\nmy erotic sonic\nRP server was against\nthe rules!?",
	"Press 'Y' to\n**** Kroze",

	"No, we still don't\nhave slopes",
	"Stop jumping on\nmy house already!",
	"Are you done?\nKTHXBYE",
	"Are you one\nof the jerks from\nInternetgamebox\nstealing our pictures\nfor advertising?",
	"Do you has life?",
	"Don't go!\nYou might find dsz!",
	"lolwut?",

	"===========================================================================\n"
	"                       Sonic Robo Blast II!\n"
	"                       by Sonic Team Junior\n"
	"                      http://www.srb2.org\n"
	"      This is a modified version. Go to our site for the original.\n"
	"===========================================================================\n",

	"===========================================================================\n"
	"                   We hope you enjoy this game as\n"
	"                     much as we did making it!\n"
	"                            ...wait. =P\n"
	"===========================================================================\n",

	"M_LoadDefaults: Load system defaults.\n",
	"Z_Init: Init zone memory allocation daemon. \n",
	"W_Init: Init WADfiles.\n",
	"M_Init: Init miscellaneous info.\n",
	"R_Init: Init SRB2 refresh daemon - ",
	"\nP_Init: Init Playloop state.\n",
	"I_Init: Setting up machine state.\n",
	"D_CheckNetGame: Checking network game status.\n",
	"S_Init: Setting up sound.\n",
	"HU_Init: Setting up heads up display.\n",
	"ST_Init: Init status bar.\n",

	"srb2.srb",
	"srb2.wad",
	"sonic.plr",
	"tails.plr",
	"knux.plr",
	"music.dta",

	"c:\\srb2data\\"SAVEGAMENAME"%u.ssg",
	SAVEGAMENAME"%u.ssg",

	//BP: here is special dehacked handling, include centering and version
	"Sonic Robo Blast 2:" VERSIONSTRING,
};

char savegamename[256];
