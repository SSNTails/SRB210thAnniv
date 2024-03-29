// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
/// \brief Graphical Alerts for MacOSX
///
///	Shows alerts, since we can't just print these to the screen when
///	launched graphically on a mac.

#ifdef __APPLE_CC__

#include "mac_alert.h"
#include <CoreFoundation/CoreFoundation.h>

int MacShowAlert(const char *title, const char *message, const char *button1, const char *button2, const char *button3)
{
	CFOptionFlags results;

	CFUserNotificationDisplayAlert(0,
	 kCFUserNotificationStopAlertLevel | kCFUserNotificationNoDefaultButtonFlag,
	 NULL, NULL, NULL,
	 CFStringCreateWithCString(NULL, title, kCFStringEncodingASCII),
	 CFStringCreateWithCString(NULL, message, kCFStringEncodingASCII),
	 button1 != NULL ? CFStringCreateWithCString(NULL, button1, kCFStringEncodingASCII) : NULL,
	 button2 != NULL ? CFStringCreateWithCString(NULL, button2, kCFStringEncodingASCII) : NULL,
	 button3 != NULL ? CFStringCreateWithCString(NULL, button3, kCFStringEncodingASCII) : NULL,
	 &results);

	return results;
}

#endif
