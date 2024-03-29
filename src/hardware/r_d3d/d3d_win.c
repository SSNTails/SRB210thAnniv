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
/// \brief Windows specific part of the Direct3D API for Doom Legacy
///
///	TODO:
///	- check if windowed mode works
///	- support different pixel formats


#ifdef __WIN32__

//#define WIN32_LEAN_AND_MEAN
#include "r_d3d.h"
#include <windows.h>
#include <time.h>

BOOL InitOpenGL( HINSTANCE hInst );
BOOL TermOpenGL( HINSTANCE hInst );
HGLRC wd3CreateContext( HDC hdc );
BOOL wd3DeleteContext( HGLRC hglrc );
BOOL wd3MakeCurrent( HDC hdc, HGLRC hglrc );


// **************************************************************************
//                                                                    GLOBALS
// **************************************************************************

#ifdef DEBUG_TO_FILE
static unsigned long nb_frames=0;
static clock_t my_clock;
HANDLE logstream;
#endif

static  HDC     hDC   = NULL;       // the window's device context
static  HGLRC   hGLRC = NULL;       // the OpenGL rendering context
static  HWND    hWnd  = NULL;
static  BOOL    WasFullScreen = FALSE;
static void UnSetRes(void);

#ifdef USE_WGL_SWAP
PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT = NULL;
PFNWGLEXTGETSWAPINTERVALPROC wglGetSwapIntervalEXT = NULL;
#endif

#define MAX_VIDEO_MODES   32
static  vmode_t     video_modes[MAX_VIDEO_MODES];
int     oglflags = 0;

// **************************************************************************
//                                                                  FUNCTIONS
// **************************************************************************

// -----------------+
// APIENTRY DllMain : DLL Entry Point,
//                  : open/close debug log
// Returns          :
// -----------------+
BOOL WINAPI DllMain(HINSTANCE hinstDLL, // handle to DLL module
                    DWORD fdwReason,    // reason for calling function
                    LPVOID lpvReserved) // reserved
{
    // Perform actions based on the reason for calling.
    UNREFERENCED_PARAMETER(lpvReserved);
    switch ( fdwReason )
    {
        case DLL_PROCESS_ATTACH:
         // Initialize once for each new process.
         // Return FALSE to fail DLL load.
#ifdef DEBUG_TO_FILE
            logstream = INVALID_HANDLE_VALUE;
            logstream = CreateFileA("d3dlog.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL/*|FILE_FLAG_WRITE_THROUGH*/, NULL);
            if (logstream == INVALID_HANDLE_VALUE)
                return FALSE;
#endif
				return InitOpenGL( hinstDLL );
            break;

        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
         // Perform any necessary cleanup.
#ifdef DEBUG_TO_FILE
            if ( logstream != INVALID_HANDLE_VALUE ) {
                CloseHandle ( logstream );
                logstream  = INVALID_HANDLE_VALUE;
            }
#endif
				return TermOpenGL( hModule );
            break;
    }

    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}


// -----------------+
// SetupPixelFormat : Set the device context's pixel format
// Note             : Because we currently use only the desktop's BPP format, all the
//                  : video modes in Doom Legacy OpenGL are of the same BPP, thus the
//                  : PixelFormat is set only once.
//                  : Setting the pixel format more than once on the same window
//                  : doesn't work. (ultimately for different pixel formats, we
//                  : should close the window, and re-create it)
// -----------------+
int SetupPixelFormat( int WantColorBits, int WantStencilBits, int WantDepthBits )
{
    static DWORD iLastPFD = 0;
    int nPixelFormat;
    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof (PIXELFORMATDESCRIPTOR),  // size
        1,                              // version
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,                  // color type
        32 /*WantColorBits*/,           // cColorBits : prefered color depth
        0, 0,                           // cRedBits, cRedShift
        0, 0,                           // cGreenBits, cGreenShift
        0, 0,                           // cBlueBits, cBlueShift
        0, 0,                           // cAlphaBits, cAlphaShift
        0,                              // cAccumBits
        0, 0, 0, 0,                     // cAccum Red/Green/Blue/Alpha Bits
        0,                              // cDepthBits (0,16,24,32)
        0,                              // cStencilBits
        0,                              // cAuxBuffers
        PFD_MAIN_PLANE,                 // iLayerType
        0,                              // reserved, must be zero
        0, 0, 0,                        // dwLayerMask, dwVisibleMask, dwDamageMask
    };

    DWORD iPFD = (WantColorBits<<16) | (WantStencilBits<<8) | WantDepthBits;

	 pfd.cDepthBits = (BYTE)WantDepthBits;
	 pfd.cStencilBits = (BYTE)WantStencilBits;
    if ( iLastPFD )
    {
        DBG_Printf( "WARNING : SetPixelFormat() called twise not supported by all drivers !\n" );
    }

    // set the pixel format only if different than the current
    if ( iPFD == iLastPFD )
        return 2;
    else
        iLastPFD = iPFD;

    DBG_Printf( "SetupPixelFormat() - %d ColorBits - %d StencilBits - %d DepthBits\n",
                WantColorBits, WantStencilBits, WantDepthBits );


    nPixelFormat = ChoosePixelFormat( hDC, &pfd );

    if ( nPixelFormat==0 )
        DBG_Printf( "ChoosePixelFormat() FAILED\n" );

    if ( SetPixelFormat( hDC, nPixelFormat, &pfd ) == 0 )
    {
        DBG_Printf( "SetPixelFormat() FAILED\n" );
        return 0;
    }

    return 1;
}


// -----------------+
// SetRes           : Set a display mode
// Notes            : pcurrentmode is actually not used
// -----------------+
static int SetRes( viddef_t *lvid, vmode_t *pcurrentmode )
{
    LPCVOID *renderer;
    BOOL WantFullScreen = !(lvid->u.windowed);  //(lvid->u.windowed ? 0 : CDS_FULLSCREEN );

    pcurrentmode = NULL;
    DBG_Printf ("SetMode(): %dx%d %d bits (%s)\n",
                lvid->width, lvid->height, lvid->bpp*8,
                WantFullScreen ? "fullscreen" : "windowed");

	if (lvid->WndParent)
		hWnd = lvid->WndParent;
	else DBG_Printf("hWnd NULL\n");


    // BP : why flush texture ?
    //      if important flush also the first one (white texture) and restore it !
    Flush();    // Flush textures.

// TODO: if not fullscreen, skip display stuff and just resize viewport stuff ...

    // Exit previous mode
    //if ( hGLRC ) //Hurdler: TODO: check if this is valid
    //    UnSetRes();
	if (WasFullScreen)
		ChangeDisplaySettings( NULL, CDS_FULLSCREEN ); //switch in and out of fullscreen

        // Change display settings.
    if ( WantFullScreen )
    {
        DEVMODE dm;
        ZeroMemory( &dm, sizeof (dm) );
        dm.dmSize       = sizeof (dm);
        dm.dmPelsWidth  = lvid->width;
        dm.dmPelsHeight = lvid->height;
        dm.dmBitsPerPel = lvid->bpp*8;
        dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
        if ( ChangeDisplaySettings( &dm, CDS_TEST ) != DISP_CHANGE_SUCCESSFUL )
            return -2;
        if ( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) !=DISP_CHANGE_SUCCESSFUL )
            return -3;

        SetWindowLong( hWnd, GWL_STYLE, WS_POPUP|WS_VISIBLE );
        // faB : book says to put these, surely for windowed mode
        //WS_CLIPCHILDREN|WS_CLIPSIBLINGS );
        SetWindowPos( hWnd, HWND_TOPMOST, 0, 0, lvid->width, lvid->height,
                      SWP_NOACTIVATE | SWP_NOZORDER );
    }
    else
    {
        RECT bounds;
        int x = 0, y = 0;
        int w = lvid->width, h = lvid->height;
        GetClientRect(hWnd, &bounds);
        AdjustWindowRectEx(&bounds, GetWindowLong(hWnd, GWL_STYLE), 0, 0);
        x = (GetSystemMetrics(SM_CXSCREEN)-(bounds.right-bounds.left))/2;
        y = (GetSystemMetrics(SM_CYSCREEN)-(bounds.bottom-bounds.top))/2;
        w += GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
        h += GetSystemMetrics(SM_CYCAPTION);
        h += GetSystemMetrics(SM_CYFIXEDFRAME) * 2;
        SetWindowPos(hWnd, HWND_NOTOPMOST, x, y, w, h, Wflags);
    }

    if ( !hDC )
        hDC = GetDC( hWnd );
    if ( !hDC )
    {
        DBG_Printf("GetDC() FAILED\n");
        return 0;
    }

    {
        int res;

        // Set res.
        res = SetupPixelFormat( lvid->bpp*8, 0, 16 );
        if ( res==0 )
           return 0;
        else if ( res==1 )
        {
            // Exit previous mode
            if ( hGLRC )
                UnSetRes();
            hGLRC = wd3CreateContext( hDC );
            if ( !hGLRC )
            {
                DBG_Printf("d3dCreateContext() FAILED\n");
                return 0;
            }
            if ( !wd3MakeCurrent( hDC, hGLRC ) )
            {
                DBG_Printf("d3dMakeCurrent() FAILED\n");
                return 0;
            }
        }
    }

    gl_extensions = glGetString(GL_EXTENSIONS);
    // Get info and extensions.
    //BP: why don't we make it earlier ?
    //Hurdler: we cannot do that before intialising gl context
    renderer = glGetString(GL_RENDERER);
    DBG_Printf("Vendor     : %s\n", glGetString(GL_VENDOR) );
    DBG_Printf("Renderer   : %s\n", renderer );
    DBG_Printf("Version    : %s\n", glGetString(GL_VERSION) );
    DBG_Printf("Extensions : %s\n", gl_extensions );

    // BP: disable advenced feature that don't work on somes hardware
    // Hurdler: Now works on G400 with bios 1.6 and certified drivers 6.04
    if ( strstr(renderer, "810" ) )   oglflags |= GLF_NOZBUFREAD;
    DBG_Printf("d3dflags   : 0x%X\n", oglflags );

#ifdef USE_PALETTED_TEXTURE
    if ( isExtAvailable("GL_EXT_paletted_texture") )
    {
        glColorTableEXT=(PFNGLCOLORTABLEEXTPROC)wd3GetProcAddress("glColorTableEXT");
    }
#endif

    screen_depth = lvid->bpp*8;
    if ( screen_depth > 16)
        textureformatGL = GL_RGBA;
    else
        textureformatGL = GL_RGB5_A1;

    SetModelView( lvid->width, lvid->height );
    SetStates();
    // we need to clear the depth buffer. Very important!!!
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    lvid->buffer = NULL;    // unless we use the software view
    lvid->direct = NULL;    // direct access to video memory, old DOS crap

    WasFullScreen = WantFullScreen;

    return 1;               // on renvoie une valeur pour dire que cela s'est bien pass�
}


// -----------------+
// UnSetRes         : Restore the original display mode
// -----------------+
static void UnSetRes( void )
{
    DBG_Printf( "UnSetRes()\n" );

    wd3MakeCurrent( hDC, NULL );
    wd3DeleteContext( hGLRC );
    hGLRC = NULL;
    if ( WasFullScreen )
        ChangeDisplaySettings( NULL, CDS_FULLSCREEN );
}


// -----------------
// GetModeList      : Return the list of available display modes.
// Returns          : pvidmodes   - points to list of detected OpenGL video modes
//                  : numvidmodes - number of detected OpenGL video modes
// -----------------+
EXPORT void HWRAPI( GetModeList ) ( vmode_t** pvidmodes, int *numvidmodes )
{
	int  i;

#if 1
	int iMode;
/*
	faB test code

	Commented out because there might be a problem with combo (Voodoo2 + other card),
	we need to get the 3D card's display modes only.
*/
	(*pvidmodes) = &video_modes[0];

	// Get list of device modes
	for ( i=0,iMode=0; iMode<MAX_VIDEO_MODES; i++ )
	{
		DEVMODE Tmp;
		ZeroMemory(&Tmp, sizeof (Tmp));
		Tmp.dmSize = sizeof ( Tmp );
		if ( !EnumDisplaySettings( NULL, i, &Tmp ) )
			break;

		// add video mode
		if (Tmp.dmBitsPerPel==16 &&
			 (iMode==0 || !
			  (Tmp.dmPelsWidth == video_modes[iMode-1].width &&
			   Tmp.dmPelsHeight == video_modes[iMode-1].height)
			 )
			)
		{
			video_modes[iMode].pnext = &video_modes[iMode+1];
			video_modes[iMode].windowed = 0;                    // fullscreen is the default
			video_modes[iMode].misc = 0;
			video_modes[iMode].name = (char *)malloc(12);
			sprintf(video_modes[iMode].name, "%dx%d", (int)Tmp.dmPelsWidth, (int)Tmp.dmPelsHeight);
			DBG_Printf ("Mode: %s\n", video_modes[iMode].name);
			video_modes[iMode].width = Tmp.dmPelsWidth;
			video_modes[iMode].height = Tmp.dmPelsHeight;
			video_modes[iMode].bytesperpixel = Tmp.dmBitsPerPel/8;
			video_modes[iMode].rowbytes = Tmp.dmPelsWidth * video_modes[iMode].bytesperpixel;
			video_modes[iMode].pextradata = NULL;
			video_modes[iMode].setmode = SetRes;
			iMode++;
		}
	}
	(*numvidmodes) = iMode;
#else

	// classic video modes (fullscreen/windowed)
	// Added some. Tails
	int res[][2] = {
					{ 320, 200},
					{ 320, 240},
					{ 400, 300},
					{ 512, 384},
					{ 640, 400},
					{ 640, 480},
					{ 800, 600},
					{ 960, 600},
					{1024, 768},
					{1152, 864},
					{1280, 800},
					{1280, 960},
					{1280,1024},
					{1600,1000},
					{1920,1200},
};

	HDC bpphdc;
	int iBitsPerPel;

	DBG_Printf ("HWRAPI GetModeList()\n");

	bpphdc = GetDC(NULL); // on obtient le bpp actuel
	iBitsPerPel = GetDeviceCaps( bpphdc, BITSPIXEL );

	ReleaseDC( NULL, bpphdc );

	(*pvidmodes) = &video_modes[0];
	(*numvidmodes) = sizeof (res) / sizeof (res[0]);
	for ( i=0; i<(*numvidmodes); i++ )
	{
		video_modes[i].pnext = &video_modes[i+1];
		video_modes[i].windowed = 0; // fullscreen is the default
		video_modes[i].misc = 0;
		video_modes[i].name = (char *)malloc(12);
		sprintf(video_modes[i].name, "%dx%d", res[i][0], res[i][1]);
		DBG_Printf ("Mode: %s\n", video_modes[i].name);
		video_modes[i].width = res[i][0];
		video_modes[i].height = res[i][1];
		video_modes[i].bytesperpixel = iBitsPerPel/8;
		video_modes[i].rowbytes = res[i][0] * video_modes[i].bytesperpixel;
		video_modes[i].pextradata = NULL;
		video_modes[i].setmode = SetRes;
	}
#endif
	video_modes[(*numvidmodes)-1].pnext = NULL;
}


// -----------------+
// Shutdown         : Shutdown OpenGL, restore the display mode
// -----------------+
EXPORT void HWRAPI( Shutdown ) ( void )
{
#ifdef DEBUG_TO_FILE
    long nb_centiemes;

    DBG_Printf ("HWRAPI Shutdown()\n");
    nb_centiemes = ((clock()-my_clock)*100)/CLOCKS_PER_SEC;
    DBG_Printf("Nb frames: %li ;  Nb sec: %2.2f  ->  %2.1f fps\n",
                    nb_frames, nb_centiemes/100.0f, (100*nb_frames)/(double)nb_centiemes);
#endif

    Flush();

    // Exit previous mode
    if ( hGLRC )
        UnSetRes();

    if ( hDC )
    {
        ReleaseDC( hWnd, hDC );
        hDC = NULL;
    }

    DBG_Printf ("HWRAPI Shutdown(DONE)\n");
}

// -----------------+
// FinishUpdate     : Swap front and back buffers
// -----------------+
EXPORT void HWRAPI( FinishUpdate ) ( int waitvbl )
{
#ifdef USE_WGL_SWAP
	int oldwaitvbl = 0;
#else
	waitvbl = 0;
#endif
	// DBG_Printf ("FinishUpdate()\n");
#ifdef DEBUG_TO_FILE
	if ( (++nb_frames)==2 )  // on ne commence pas � la premi�re frame
		my_clock = clock();
#endif

#ifdef USE_WGL_SWAP
	if (wglGetSwapIntervalEXT)
		oldwaitvbl = wglGetSwapIntervalEXT();
	if (oldwaitvbl != waitvbl && wglSwapIntervalEXT)
		wglSwapIntervalEXT(waitvbl);
#endif

	SwapBuffers( hDC );

#ifdef USE_WGL_SWAP
	if (oldwaitvbl != waitvbl && wglSwapIntervalEXT)
		wglSwapIntervalEXT(oldwaitvbl);
#endif
}


// -----------------+
// SetPalette       : Set the color lookup table for paletted textures
//                  : in OpenGL, we store values for conversion of paletted graphics when
//                  : they are downloaded to the 3D card.
// -----------------+
EXPORT void HWRAPI( SetPalette ) ( RGBA_t *pal, RGBA_t *gamma )
{
    int i;

    for (i=0; i<256; i++) {
        myPaletteData[i].s.red   = MIN((pal[i].s.red*gamma->s.red)/127,     255);
        myPaletteData[i].s.green = MIN((pal[i].s.green*gamma->s.green)/127, 255);
        myPaletteData[i].s.blue  = MIN((pal[i].s.blue*gamma->s.blue)/127,   255);
        myPaletteData[i].s.alpha = pal[i].s.alpha;
    }
#ifdef USE_PALETTED_TEXTURE
    if (glColorTableEXT)
    {
        for (i=0; i<256; i++)
        {
            palette_tex[3*i+0] = pal[i].s.red;
            palette_tex[3*i+1] = pal[i].s.green;
            palette_tex[3*i+2] = pal[i].s.blue;
        }
        glColorTableEXT(GL_TEXTURE_2D, GL_RGB8, 256, GL_RGB, GL_UNSIGNED_BYTE, palette_tex);
    }
#endif
    // on a chang� de palette, il faut recharger toutes les textures
    Flush();
}

#endif
