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
/// \brief Fixed point implementation

#include "doomdef.h"
#include "m_fixed.h"

#ifdef __USE_C_FIXED__

/**	\brief	The FixedMul function

	\param	a	fixed_t number
	\param	b	fixed_t number

	\return	a*b>>FRACBITS


*/
fixed_t FixedMul(fixed_t a, fixed_t b)
{
#if defined (_WIN32) && !defined (_XBOX) && !defined (_WIN32_WCE)
	return MulDiv(a,b,FRACUNIT);
#else
	return (fixed_t)(((INT64) a * (INT64) b)>>FRACBITS);
#endif
}

/**	\brief	The FixedDiv2 function

	\param	a	fixed_t number
	\param	b	fixed_t number

	\return	a/b * FRACUNIT


*/
fixed_t FixedDiv2(fixed_t a, fixed_t b)
{
#if defined (_WIN32) && !defined (_XBOX) && !defined (_WIN32_WCE)
	int c = MulDiv(a,FRACUNIT,b);
	if (c == -1)
		I_Error("FixedDiv: divide by zero");
#else
	double c = ((double)a) / ((double)b) * FRACUNIT;

	if (c >= 2147483648.0 || c < -2147483648.0)
		I_Error("FixedDiv: divide by zero");
#endif
	return (fixed_t)c;
}

#endif // useasm
