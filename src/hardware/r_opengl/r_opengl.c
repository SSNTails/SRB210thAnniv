// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2006 by Sonic Team Junior.
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
/// \brief OpenGL API for Doom Legacy

#if defined (_WIN32) || defined (_WIN64)
//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif

#ifdef __GNUC__
#include <unistd.h>
#endif

#include <stdarg.h>
#include <math.h>
#include "r_opengl.h"

// for KOS: GL_TEXTURE_ENV, glAlphaFunc, glColorMask, glPolygonOffset, glReadPixels, GL_ALPHA_TEST, GL_POLYGON_OFFSET_FILL

// ==========================================================================
//                                                                  CONSTANTS
// ==========================================================================

// With OpenGL 1.1+, the first texture should be 1
#define NOTEXTURE_NUM     1     // small white texture
#define FIRST_TEX_AVAIL   (NOTEXTURE_NUM + 1)

#define      N_PI_DEMI               (M_PI/2) //(1.5707963268f)

#define      ASPECT_RATIO            (1.0f)  //(320.0f/200.0f)
#define      FAR_CLIPPING_PLANE      15360.0f // Draw further! Tails 01-21-2001
static float NEAR_CLIPPING_PLANE =   NZCLIP_PLANE;

#define      MIPMAP_MASK             0x0100

// **************************************************************************
//                                                                    GLOBALS
// **************************************************************************


static  GLuint      NextTexAvail    = FIRST_TEX_AVAIL;
static  GLuint      tex_downloaded  = 0;
static  GLfloat     fov             = 90.0f;
static  GLuint      pal_col         = 0;
static  FRGBAFloat  const_pal_col;
static  FBITFIELD   CurrentPolyFlags;

static  FTextureInfo*  gr_cachetail = NULL;
static  FTextureInfo*  gr_cachehead = NULL;

RGBA_t  myPaletteData[256];
GLint   screen_width    = 0;               // used by Draw2DLine()
GLint   screen_height   = 0;
GLbyte  screen_depth    = 0;
GLint   textureformatGL = 0;
GLint maximumAnisotropy = 0;
static GLint min_filter = GL_LINEAR;
static GLint mag_filter = GL_LINEAR;
static GLint anisotropic_filter = 0;
static FTransform  md2_transform;

const   GLubyte     *gl_extensions = NULL;

//Hurdler: 04/10/2000: added for the kick ass coronas as Boris wanted;-)
#ifndef MINI_GL_COMPATIBILITY
static GLdouble    modelMatrix[16];
static GLdouble    projMatrix[16];
static GLint       viewport[4];
#endif


#ifdef USE_PALETTED_TEXTURE
	PFNGLCOLORTABLEEXTPROC  glColorTableEXT = NULL;
	GLubyte                 palette_tex[256*3];
#endif

// shortcut for ((float)1/i)
static const GLfloat byte2float[256] = {
	0.000000f, 0.003922f, 0.007843f, 0.011765f, 0.015686f, 0.019608f, 0.023529f, 0.027451f,
	0.031373f, 0.035294f, 0.039216f, 0.043137f, 0.047059f, 0.050980f, 0.054902f, 0.058824f,
	0.062745f, 0.066667f, 0.070588f, 0.074510f, 0.078431f, 0.082353f, 0.086275f, 0.090196f,
	0.094118f, 0.098039f, 0.101961f, 0.105882f, 0.109804f, 0.113725f, 0.117647f, 0.121569f,
	0.125490f, 0.129412f, 0.133333f, 0.137255f, 0.141176f, 0.145098f, 0.149020f, 0.152941f,
	0.156863f, 0.160784f, 0.164706f, 0.168627f, 0.172549f, 0.176471f, 0.180392f, 0.184314f,
	0.188235f, 0.192157f, 0.196078f, 0.200000f, 0.203922f, 0.207843f, 0.211765f, 0.215686f,
	0.219608f, 0.223529f, 0.227451f, 0.231373f, 0.235294f, 0.239216f, 0.243137f, 0.247059f,
	0.250980f, 0.254902f, 0.258824f, 0.262745f, 0.266667f, 0.270588f, 0.274510f, 0.278431f,
	0.282353f, 0.286275f, 0.290196f, 0.294118f, 0.298039f, 0.301961f, 0.305882f, 0.309804f,
	0.313726f, 0.317647f, 0.321569f, 0.325490f, 0.329412f, 0.333333f, 0.337255f, 0.341176f,
	0.345098f, 0.349020f, 0.352941f, 0.356863f, 0.360784f, 0.364706f, 0.368627f, 0.372549f,
	0.376471f, 0.380392f, 0.384314f, 0.388235f, 0.392157f, 0.396078f, 0.400000f, 0.403922f,
	0.407843f, 0.411765f, 0.415686f, 0.419608f, 0.423529f, 0.427451f, 0.431373f, 0.435294f,
	0.439216f, 0.443137f, 0.447059f, 0.450980f, 0.454902f, 0.458824f, 0.462745f, 0.466667f,
	0.470588f, 0.474510f, 0.478431f, 0.482353f, 0.486275f, 0.490196f, 0.494118f, 0.498039f,
	0.501961f, 0.505882f, 0.509804f, 0.513726f, 0.517647f, 0.521569f, 0.525490f, 0.529412f,
	0.533333f, 0.537255f, 0.541177f, 0.545098f, 0.549020f, 0.552941f, 0.556863f, 0.560784f,
	0.564706f, 0.568627f, 0.572549f, 0.576471f, 0.580392f, 0.584314f, 0.588235f, 0.592157f,
	0.596078f, 0.600000f, 0.603922f, 0.607843f, 0.611765f, 0.615686f, 0.619608f, 0.623529f,
	0.627451f, 0.631373f, 0.635294f, 0.639216f, 0.643137f, 0.647059f, 0.650980f, 0.654902f,
	0.658824f, 0.662745f, 0.666667f, 0.670588f, 0.674510f, 0.678431f, 0.682353f, 0.686275f,
	0.690196f, 0.694118f, 0.698039f, 0.701961f, 0.705882f, 0.709804f, 0.713726f, 0.717647f,
	0.721569f, 0.725490f, 0.729412f, 0.733333f, 0.737255f, 0.741177f, 0.745098f, 0.749020f,
	0.752941f, 0.756863f, 0.760784f, 0.764706f, 0.768627f, 0.772549f, 0.776471f, 0.780392f,
	0.784314f, 0.788235f, 0.792157f, 0.796078f, 0.800000f, 0.803922f, 0.807843f, 0.811765f,
	0.815686f, 0.819608f, 0.823529f, 0.827451f, 0.831373f, 0.835294f, 0.839216f, 0.843137f,
	0.847059f, 0.850980f, 0.854902f, 0.858824f, 0.862745f, 0.866667f, 0.870588f, 0.874510f,
	0.878431f, 0.882353f, 0.886275f, 0.890196f, 0.894118f, 0.898039f, 0.901961f, 0.905882f,
	0.909804f, 0.913726f, 0.917647f, 0.921569f, 0.925490f, 0.929412f, 0.933333f, 0.937255f,
	0.941177f, 0.945098f, 0.949020f, 0.952941f, 0.956863f, 0.960784f, 0.964706f, 0.968628f,
	0.972549f, 0.976471f, 0.980392f, 0.984314f, 0.988235f, 0.992157f, 0.996078f, 1.000000
};

float byteasfloat(byte fbyte)
{
	return (float)(byte2float[fbyte]*2.0f);
}

static I_Error_t I_Error_GL = NULL;


// -----------------+
// DBG_Printf       : Output error messages to debug log if DEBUG_TO_FILE is defined,
//                  : else do nothing
// Returns          :
// -----------------+
#if !(defined (SDL) && defined (STATIC3DS))
FUNCPRINTF void DBG_Printf(const char *lpFmt, ...)
{
#ifdef DEBUG_TO_FILE
	char    str[4096] = "";
	va_list arglist;

	va_start (arglist, lpFmt);
	vsnprintf (str, 4096, lpFmt, arglist);
	va_end   (arglist);
#ifdef _WINDOWS
	{
		DWORD bytesWritten;
		if (logstream != INVALID_HANDLE_VALUE)
			WriteFile(logstream, str, (DWORD)strlen(str), &bytesWritten, NULL);
	}
#else
	if (logstream)
		fwrite(str, strlen(str), 1, logstream);
#endif

#else
	UNREFERENCED_PARAMETER(lpFmt);
#endif
}
#endif

// -----------------+
// SetNoTexture     : Disable texture
// -----------------+
static void SetNoTexture(void)
{
	// Set small white texture.
	if (tex_downloaded != NOTEXTURE_NUM)
	{
		glBindTexture(GL_TEXTURE_2D, NOTEXTURE_NUM);
		tex_downloaded = NOTEXTURE_NUM;
	}
}


// -----------------+
// SetModelView     :
// -----------------+
void SetModelView(GLint w, GLint h)
{
	DBG_Printf("SetModelView(): %dx%d\n", w, h);

	screen_width = w;
	screen_height = h;

	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, ASPECT_RATIO, NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//glScalef(1.0f, 320.0f/200.0f, 1.0f);  // gr_scalefrustum (ORIGINAL_ASPECT)

	// added for new coronas' code (without depth buffer)
#ifndef MINI_GL_COMPATIBILITY
	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
#endif
}


// -----------------+
// SetStates        : Set permanent states
// -----------------+
void SetStates(void)
{
	// Bind little white RGBA texture to ID NOTEXTURE_NUM.
	FUINT Data[8*8];
	int i;

	DBG_Printf("SetStates()\n");

	// Hurdler: not necessary, is it?
	//glShadeModel(GL_SMOOTH);      // iterate vertice colors
	glShadeModel(GL_FLAT);

	glEnable(GL_TEXTURE_2D);      // two-dimensional texturing
#ifndef KOS_GL_COMPATIBILITY
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glAlphaFunc(GL_NOTEQUAL, 0.0f);
#endif
	//glDisable(GL_ALPHA_TEST);     // enable alpha testing
	//glBlendFunc(GL_ONE, GL_ZERO); // copy pixel to frame buffer (opaque)
	glEnable(GL_BLEND);           // enable color blending

#ifndef KOS_GL_COMPATIBILITY
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#endif

	//glDisable(GL_DITHER);         // faB: ??? (undocumented in OpenGL 1.1)
								  // Hurdler: yes, it is!
	glEnable(GL_DEPTH_TEST);    // check the depth buffer
	//glDepthMask(1);             // enable writing to depth buffer
	glClearDepth(1.0f);
	glDepthRange(0.0f, 1.0f);
	glDepthFunc(GL_LEQUAL);

	// this set CurrentPolyFlags to the acctual configuration
	CurrentPolyFlags = 0xffffffff;
	SetBlend(0);

	for (i = 0; i < 64; i++)
		Data[i] = 0xffFFffFF;       // white pixel

	tex_downloaded = (GLuint)-1;
	SetNoTexture();
	//glBindTexture(GL_TEXTURE_2D, NOTEXTURE_NUM);
	//tex_downloaded = NOTEXTURE_NUM;
	//glTexImage2D(GL_TEXTURE_2D, 0, 4, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data);

#ifndef KOS_GL_COMPATIBILITY
	glPolygonOffset(-1.0f, -1.0f);
#endif

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glPolygonMode(GL_FRONT, GL_LINE);

	//glFogi(GL_FOG_MODE, GL_EXP);
	//glHint(GL_FOG_HINT, GL_NICEST);
	//glFogfv(GL_FOG_COLOR, fogcolor);
	//glFogf(GL_FOG_DENSITY, 0.0005f);

	// bp : when no t&l :)
	glLoadIdentity();
	glScalef(1.0f, 1.0f, -1.0f);
#ifndef MINI_GL_COMPATIBILITY
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix); // added for new coronas' code (without depth buffer)
#endif
}


// -----------------+
// Flush            : flush OpenGL textures
//                  : Clear list of downloaded mipmaps
// -----------------+
void Flush(void)
{
	//DBG_Printf ("HWR_Flush()\n");

	while (gr_cachehead)
	{
		// ceci n'est pas du tout necessaire vu que tu les a charger normalement et
		// donc il sont dans ta liste !
#if 0
		//Hurdler: 25/04/2000: now support colormap in hardware mode
		FTextureInfo    *tmp = gr_cachehead->nextskin;

		// The memory should be freed in the main code
		while (tmp)
		{
			glDeleteTextures(1, &tmp->downloaded);
			tmp->downloaded = 0;
			tmp = tmp->nextcolormap;
		}
#endif
		glDeleteTextures(1, (GLuint *)&gr_cachehead->downloaded);
		gr_cachehead->downloaded = 0;
		gr_cachehead = gr_cachehead->nextmipmap;
	}
	gr_cachetail = gr_cachehead = NULL; //Hurdler: well, gr_cachehead is already NULL
	NextTexAvail = FIRST_TEX_AVAIL;
	tex_downloaded = 0;
}


// -----------------+
// isExtAvailable   : Look if an OpenGL extension is available
// Returns          : true if extension available
// -----------------+
int isExtAvailable(const char *extension, const GLubyte *start)
{
	GLubyte         *where, *terminator;

	if (!extension || !start) return 0;
	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return 0;

	for (;;)
	{
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where)
			break;
		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return 1;
		start = terminator;
	}
	return 0;
}


// -----------------+
// Init             : Initialise the OpenGL interface API
// Returns          :
// -----------------+
EXPORT boolean HWRAPI(Init) (I_Error_t FatalErrorFunction)
{
	I_Error_GL = FatalErrorFunction;
	DBG_Printf ("%s%s\n", DRIVER_STRING, VERSIONSTRING);
	return 1;
}


// -----------------+
// ClearMipMapCache : Flush OpenGL textures from memory
// -----------------+
EXPORT void HWRAPI(ClearMipMapCache) (void)
{
	// DBG_Printf ("HWR_Flush(exe)\n");
	Flush();
}


// -----------------+
// ReadRect         : Read a rectangle region of the truecolor framebuffer
//                  : store pixels as 16bit 565 RGB
// Returns          : 16bit 565 RGB pixel array stored in dst_data
// -----------------+
EXPORT void HWRAPI(ReadRect) (int x, int y, int width, int height,
                                int dst_stride, unsigned short * dst_data)
{
	int i;
	// DBG_Printf ("ReadRect()\n");
#ifdef KOS_GL_COMPATIBILITY
	(void)x;
	(void)y;
	(void)width;
	(void)height;
	(void)dst_stride;
	(void)dst_data;
#else
	if (dst_stride == width*3)
	{
		GLubyte*top = (GLvoid*)dst_data, *bottom = top + dst_stride * (height - 1);
		GLubyte *row = malloc(dst_stride);
		if (!row) return;
		glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, dst_data);
		for(i = 0; i < height/2; i++)
		{
			memcpy(row, top, dst_stride);
			memcpy(top, bottom, dst_stride);
			memcpy(bottom, row, dst_stride);
			top += dst_stride;
			bottom -= dst_stride;
		}
		free(row);
	}
	else
	{
		int j;
		GLubyte *image = malloc(width*height*3*sizeof (*image));
		if (!image) return;
		glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);
		for (i = height-1; i >= 0; i--)
		{
			for (j = 0; j < width; j++)
			{
				dst_data[(height-1-i)*width+j] = (unsigned short)(
				                ((image[(i*width+j)*3]>>3)<<11) |
				                ((image[(i*width+j)*3+1]>>2)<<5) |
				                ((image[(i*width+j)*3+2]>>3)));
			}
		}
		free(image);
	}
#endif
}


// -----------------+
// GClipRect        : Defines the 2D hardware clipping window
// -----------------+
EXPORT void HWRAPI(GClipRect) (int minx, int miny, int maxx, int maxy, float nearclip)
{
	// DBG_Printf ("GClipRect(%d, %d, %d, %d)\n", minx, miny, maxx, maxy);

	glViewport(minx, screen_height-maxy, maxx-minx, maxy-miny);
	NEAR_CLIPPING_PLANE = nearclip;

	//glScissor(minx, screen_height-maxy, maxx-minx, maxy-miny);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, ASPECT_RATIO, NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);
	glMatrixMode(GL_MODELVIEW);

	// added for new coronas' code (without depth buffer)
#ifndef MINI_GL_COMPATIBILITY
	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
#endif
}


// -----------------+
// ClearBuffer      : Clear the color/alpha/depth buffer(s)
// -----------------+
EXPORT void HWRAPI(ClearBuffer) (FBOOLEAN ColorMask,
                                    FBOOLEAN DepthMask,
                                    FRGBAFloat * ClearColor)
{
	// DBG_Printf ("ClearBuffer(%d)\n", alpha);
	FUINT ClearMask = 0;

	if (ColorMask)
	{
		if (ClearColor)
			glClearColor(ClearColor->red,
			              ClearColor->green,
			              ClearColor->blue,
			              ClearColor->alpha);
		ClearMask |= GL_COLOR_BUFFER_BIT;
	}
	if (DepthMask)
	{
		//glClearDepth(1.0f);     //Hurdler: all that are permanen states
		//glDepthRange(0.0f, 1.0f);
		//glDepthFunc(GL_LEQUAL);
		ClearMask |= GL_DEPTH_BUFFER_BIT;
	}

	SetBlend(DepthMask ? PF_Occlude | CurrentPolyFlags : CurrentPolyFlags&~PF_Occlude);

	glClear(ClearMask);
}


// -----------------+
// HWRAPI Draw2DLine: Render a 2D line
// -----------------+
EXPORT void HWRAPI(Draw2DLine) (F2DCoord * v1,
                                   F2DCoord * v2,
                                   RGBA_t Color)
{
	FRGBAFloat c;

	// DBG_Printf ("DrawLine() (%f %f %f) %d\n", v1->x, -v1->y, -v1->z, v1->argb);
#ifdef MINI_GL_COMPATIBILITY
	GLfloat x1, x2, x3, x4;
	GLfloat y1, y2, y3, y4;
	GLfloat dx, dy;
	GLfloat angle;
#endif

	// BP: we should reflect the new state in our variable
	//SetBlend(PF_Modulated|PF_NoTexture);

	glDisable(GL_TEXTURE_2D);

	c.red   = byte2float[Color.s.red];
	c.green = byte2float[Color.s.green];
	c.blue  = byte2float[Color.s.blue];
	c.alpha = byte2float[Color.s.alpha];

#ifndef MINI_GL_COMPATIBILITY
	glColor4fv((float *)&c);    // is in RGBA float format
	glBegin(GL_LINES);
		glVertex3f(v1->x, -v1->y, 1.0f);
		glVertex3f(v2->x, -v2->y, 1.0f);
	glEnd();
#else
	if (v2->x != v1->x)
		angle = (float)atan((v2->y-v1->y)/(v2->x-v1->x));
	else
		angle = N_PI_DEMI;
	dx = (float)sin(angle) / (float)screen_width;
	dy = (float)cos(angle) / (float)screen_height;

	x1 = v1->x - dx;  y1 = v1->y + dy;
	x2 = v2->x - dx;  y2 = v2->y + dy;
	x3 = v2->x + dx;  y3 = v2->y - dy;
	x4 = v1->x + dx;  y4 = v1->y - dy;

	glColor4f(c.red, c.green, c.blue, c.alpha);
	glBegin(GL_TRIANGLE_FAN);
		glVertex3f(x1, -y1, 1);
		glVertex3f(x2, -y2, 1);
		glVertex3f(x3, -y3, 1);
		glVertex3f(x4, -y4, 1);
	glEnd();
#endif

	glEnable(GL_TEXTURE_2D);
}


// -----------------+
// SetBlend         : Set render mode
// -----------------+
// PF_Masked - we could use an ALPHA_TEST of GL_EQUAL, and alpha ref of 0,
//             is it faster when pixels are discarded ?
EXPORT void HWRAPI(SetBlend) (FBITFIELD PolyFlags)
{
	FBITFIELD Xor;
	Xor = CurrentPolyFlags^PolyFlags;
	if (Xor & (PF_Blending|PF_RemoveYWrap|PF_ForceWrapX|PF_ForceWrapY|PF_Occlude|PF_NoTexture|PF_Modulated|PF_NoDepthTest|PF_Decal|PF_Invisible|PF_NoAlphaTest))
	{
		if (Xor&(PF_Blending)) // if blending mode must be changed
		{
			switch (PolyFlags & PF_Blending) {
				case PF_Translucent & PF_Blending:
					 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // alpha = level of transparency
					 break;
				case PF_Masked & PF_Blending:
					 // Hurdler: does that mean lighting is only made by alpha src?
					 // it sounds ok, but not for polygonsmooth
					 glBlendFunc(GL_SRC_ALPHA, GL_ZERO);                // 0 alpha = holes in texture
					 break;
				case PF_Additive & PF_Blending:
#ifdef ATI_RAGE_PRO_COMPATIBILITY
					 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // alpha = level of transparency
#else
					 glBlendFunc(GL_SRC_ALPHA, GL_ONE);                 // src * alpha + dest
#endif
					 break;
				case PF_Environment & PF_Blending:
					 glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					 break;
				case PF_Substractive & PF_Blending:
					 // good for shadow
					 // not realy but what else ?
					 glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
					 break;
				default : // must be 0, otherwise it's an error
					 // No blending
					 glBlendFunc(GL_ONE, GL_ZERO);   // the same as no blending
					 break;
			}
		}
#ifndef KOS_GL_COMPATIBILITY
		if (Xor & PF_NoAlphaTest)
		{
			if (PolyFlags & PF_NoAlphaTest)
				glDisable(GL_ALPHA_TEST);
			else
				glEnable(GL_ALPHA_TEST);      // discard 0 alpha pixels (holes in texture)
		}

		if (Xor & PF_Decal)
		{
			if (PolyFlags & PF_Decal)
				glEnable(GL_POLYGON_OFFSET_FILL);
			else
				glDisable(GL_POLYGON_OFFSET_FILL);
		}
#endif
		if (Xor&PF_NoDepthTest)
		{
			if (PolyFlags & PF_NoDepthTest)
				glDepthFunc(GL_ALWAYS); //glDisable(GL_DEPTH_TEST);
			else
				glDepthFunc(GL_LEQUAL); //glEnable(GL_DEPTH_TEST);
		}

		if (Xor&PF_RemoveYWrap)
		{
			if (PolyFlags & PF_RemoveYWrap)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}

		if (Xor&PF_ForceWrapX)
		{
			if (PolyFlags & PF_ForceWrapX)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		}

		if (Xor&PF_ForceWrapY)
		{
			if (PolyFlags & PF_ForceWrapY)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}

#ifdef KOS_GL_COMPATIBILITY
		if (Xor&PF_Modulated && !(PolyFlags & PF_Modulated))
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
#else
		if (Xor&PF_Modulated)
		{
#ifdef LINUX
			if (oglflags & GLF_NOTEXENV)
			{
				if (!(PolyFlags & PF_Modulated))
					glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			}
			else
#endif
			if (PolyFlags & PF_Modulated)
			{   // mix texture colour with Surface->FlatColor
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			}
			else
			{   // colour from texture is unchanged before blending
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
		}
#endif

		if (Xor & PF_Occlude) // depth test but (no) depth write
		{
			if (PolyFlags&PF_Occlude)
			{
				glDepthMask(1);
			}
			else
				glDepthMask(0);
		}
		////Hurdler: not used if we don't define POLYSKY
		if (Xor & PF_Invisible)
		{
//            glColorMask((PolyFlags&PF_Invisible)==0, (PolyFlags&PF_Invisible)==0,
//                         (PolyFlags&PF_Invisible)==0, (PolyFlags&PF_Invisible)==0);

			if (PolyFlags&PF_Invisible)
				glBlendFunc(GL_ZERO, GL_ONE);         // transparent blending
			else
			{   // big hack: (TODO: manage that better)
				// we test only for PF_Masked because PF_Invisible is only used
				// (for now) with it (yeah, that's crappy, sorry)
				if ((PolyFlags&PF_Blending)==PF_Masked)
					glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
			}
		}
		if (PolyFlags & PF_NoTexture)
		{
			SetNoTexture();
		}
	}
	CurrentPolyFlags = PolyFlags;
}


// -----------------+
// SetTexture       : The mipmap becomes the current texture source
// -----------------+
EXPORT void HWRAPI(SetTexture) (FTextureInfo *pTexInfo)
{
	if (!pTexInfo)
	{
		SetNoTexture();
		return;
	}
	else if (pTexInfo->downloaded)
	{
		if (pTexInfo->downloaded != tex_downloaded)
		{
			glBindTexture(GL_TEXTURE_2D, pTexInfo->downloaded);
			tex_downloaded = pTexInfo->downloaded;
		}
	}
	else
	{
		// Download a mipmap
#ifdef KOS_GL_COMPATIBILITY
		static GLushort tex[2048*2048];
#else
		static RGBA_t   tex[2048*2048];
#endif
		const GLvoid   *ptex = tex;
		int             w, h;

		//DBG_Printf ("DownloadMipmap %d %x\n",NextTexAvail,pTexInfo->grInfo.data);

		w = pTexInfo->width;
		h = pTexInfo->height;

#ifdef USE_PALETTED_TEXTURE
		if (glColorTableEXT &&
			(pTexInfo->grInfo.format == GR_TEXFMT_P_8) &&
			!(pTexInfo->flags & TF_CHROMAKEYED))
		{
			// do nothing here.
			// Not a problem with MiniGL since we don't use paletted texture
		}
		else
#endif
#ifdef KOS_GL_COMPATIBILITY
		if ((pTexInfo->grInfo.format == GR_TEXFMT_P_8) ||
			(pTexInfo->grInfo.format == GR_TEXFMT_AP_88))
		{
			const GLubyte *pImgData = (const GLubyte *)pTexInfo->grInfo.data;
			int i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					if ((*pImgData == HWR_PATCHES_CHROMAKEY_COLORINDEX) &&
					     (pTexInfo->flags & TF_CHROMAKEYED))
					{
						tex[w*j+i] = 0;
					}
					else
					{
						if (pTexInfo->grInfo.format == GR_TEXFMT_AP_88 && !(pTexInfo->flags & TF_CHROMAKEYED))
							tex[w*j+i] = 0;
						else
							tex[w*j+i] = (myPaletteData[*pImgData].s.alpha>>4)<<12;

						tex[w*j+i] |= (myPaletteData[*pImgData].s.red  >>4)<<8;
						tex[w*j+i] |= (myPaletteData[*pImgData].s.green>>4)<<4;
						tex[w*j+i] |= (myPaletteData[*pImgData].s.blue >>4);
					}

					pImgData++;

					if (pTexInfo->grInfo.format == GR_TEXFMT_AP_88)
					{
						if (!(pTexInfo->flags & TF_CHROMAKEYED))
							tex[w*j+i] |= ((*pImgData)>>4)<<12;
						pImgData++;
					}

				}
			}
		}
		else if (pTexInfo->grInfo.format == GR_RGBA)
		{
			// corona test : passed as ARGB 8888, which is not in glide formats
			// Hurdler: not used for coronas anymore, just for dynamic lighting
			const RGBA_t *pImgData = (const RGBA_t *)pTexInfo->grInfo.data;
			int i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					tex[w*j+i]  = (pImgData->s.alpha>>4)<<12;
					tex[w*j+i] |= (pImgData->s.red  >>4)<<8;
					tex[w*j+i] |= (pImgData->s.green>>4)<<4;
					tex[w*j+i] |= (pImgData->s.bule >>4);
					pImgData++;
				}
			}
		}
		else if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_INTENSITY_88)
		{
			const GLubyte *pImgData = (const GLubyte *)pTexInfo->grInfo.data;
			int i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					const GLubyte sID = (*pImgData)>>4;
					tex[w*j+i] = sID<<8 | sID<<4 |= sID;
					pImgData++;
					tex[w*j+i] |= ((*pImgData)>>4)<<12;
					pImgData++;
				}
			}
		}
		else
			DBG_Printf ("SetTexture(bad format) %d\n", pTexInfo->grInfo.format);
#else
		if ((pTexInfo->grInfo.format == GR_TEXFMT_P_8) ||
			(pTexInfo->grInfo.format == GR_TEXFMT_AP_88))
		{
			const GLubyte *pImgData = (const GLubyte *)pTexInfo->grInfo.data;
			int i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					if ((*pImgData == HWR_PATCHES_CHROMAKEY_COLORINDEX) &&
					     (pTexInfo->flags & TF_CHROMAKEYED))
					{
						tex[w*j+i].s.red   = 0;
						tex[w*j+i].s.green = 0;
						tex[w*j+i].s.blue  = 0;
						tex[w*j+i].s.alpha = 0;
					}
					else
					{
						tex[w*j+i].s.red   = myPaletteData[*pImgData].s.red;
						tex[w*j+i].s.green = myPaletteData[*pImgData].s.green;
						tex[w*j+i].s.blue  = myPaletteData[*pImgData].s.blue;
						tex[w*j+i].s.alpha = myPaletteData[*pImgData].s.alpha;
					}

					pImgData++;

					if (pTexInfo->grInfo.format == GR_TEXFMT_AP_88)
					{
						if (!(pTexInfo->flags & TF_CHROMAKEYED))
							tex[w*j+i].s.alpha = *pImgData;
						pImgData++;
					}

				}
			}
		}
		else if (pTexInfo->grInfo.format == GR_RGBA)
		{
			// corona test : passed as ARGB 8888, which is not in glide formats
			// Hurdler: not used for coronas anymore, just for dynamic lighting
			ptex = pTexInfo->grInfo.data;
		}
		else if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_INTENSITY_88)
		{
			const GLubyte *pImgData = (const GLubyte *)pTexInfo->grInfo.data;
			int i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					tex[w*j+i].s.red   = *pImgData;
					tex[w*j+i].s.green = *pImgData;
					tex[w*j+i].s.blue  = *pImgData;
					pImgData++;
					tex[w*j+i].s.alpha = *pImgData;
					pImgData++;
				}
			}
		}
		else
			DBG_Printf ("SetTexture(bad format) %ld\n", pTexInfo->grInfo.format);
#endif

		pTexInfo->downloaded = NextTexAvail++;
		tex_downloaded = pTexInfo->downloaded;
		glBindTexture(GL_TEXTURE_2D, pTexInfo->downloaded);

#ifdef KOS_GL_COMPATIBILITY
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ARGB4444, w, h, 0, GL_ARGB4444, GL_UNSIGNED_BYTE, ptex);
#else
#ifdef MINI_GL_COMPATIBILITY
		//if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_INTENSITY_88)
		//    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
		//else
			if (min_filter & MIPMAP_MASK)
				gluBuild2DMipmaps(GL_TEXTURE_2D, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
			else
				glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
#else
#ifdef USE_PALETTED_TEXTURE
			//Hurdler: not really supported and not tested recently
		if (glColorTableEXT &&
			(pTexInfo->grInfo.format == GR_TEXFMT_P_8) &&
			!(pTexInfo->flags & TF_CHROMAKEYED))
		{
			glColorTableEXT(GL_TEXTURE_2D, GL_RGB8, 256, GL_RGB, GL_UNSIGNED_BYTE, palette_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, w, h, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, pTexInfo->grInfo.data);
		}
		else
#endif
		if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_INTENSITY_88)
		{
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
			if (min_filter & MIPMAP_MASK)
				gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE_ALPHA, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
			else
				glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
		}
		else
		{
			if (min_filter & MIPMAP_MASK)
				gluBuild2DMipmaps(GL_TEXTURE_2D, textureformatGL, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
			else
				glTexImage2D(GL_TEXTURE_2D, 0, textureformatGL, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
		}
#endif
#endif

		if (pTexInfo->flags & TF_WRAPX)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);

		if (pTexInfo->flags & TF_WRAPY)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
		if (maximumAnisotropy)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropic_filter);

		pTexInfo->nextmipmap = NULL;
		if (gr_cachetail)
		{ // insertion en fin de liste
			gr_cachetail->nextmipmap = pTexInfo;
			gr_cachetail = pTexInfo;
		}
		else // initialisation de la liste
			gr_cachetail = gr_cachehead =  pTexInfo;
	}
#ifdef MINI_GL_COMPATIBILITY
	switch (pTexInfo->flags)
	{
		case 0 :
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			break;
		default:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			break;
	}
#endif
}


// -----------------+
// DrawPolygon      : Render a polygon, set the texture, set render mode
// -----------------+
EXPORT void HWRAPI(DrawPolygon) (FSurfaceInfo  *pSurf,
                                    //FTextureInfo  *pTexInfo,
                                    FOutVector    *pOutVerts,
                                    FUINT         iNumPts,
                                    FBITFIELD     PolyFlags)
{
	FUINT i;
#ifndef MINI_GL_COMPATIBILITY
	FUINT j;
#endif
	FRGBAFloat c = {0,0,0,0};

#ifdef MINI_GL_COMPATIBILITY
	if (PolyFlags & PF_Corona)
		PolyFlags &= ~PF_NoDepthTest;
#else
	if ((PolyFlags & PF_Corona) && (oglflags & GLF_NOZBUFREAD))
		PolyFlags &= ~(PF_NoDepthTest|PF_Corona);
#endif

	SetBlend(PolyFlags);    //TODO: inline (#pragma..)

	// If Modulated, mix the surface colour to the texture
	if ((CurrentPolyFlags & PF_Modulated) && pSurf)
	{
		if (pal_col)
		{ // hack for non-palettized mode
			c.red   = (const_pal_col.red  +byte2float[pSurf->FlatColor.s.red])  /2.0f;
			c.green = (const_pal_col.green+byte2float[pSurf->FlatColor.s.green])/2.0f;
			c.blue  = (const_pal_col.blue +byte2float[pSurf->FlatColor.s.blue]) /2.0f;
			c.alpha = byte2float[pSurf->FlatColor.s.alpha];
		}
		else
		{
			c.red   = byte2float[pSurf->FlatColor.s.red];
			c.green = byte2float[pSurf->FlatColor.s.green];
			c.blue  = byte2float[pSurf->FlatColor.s.blue];
			c.alpha = byte2float[pSurf->FlatColor.s.alpha];
		}

#ifdef MINI_GL_COMPATIBILITY
		glColor4f(c.red, c.green, c.blue, c.alpha);
#else
		glColor4fv((float *)&c);    // is in RGBA float format
#endif
	}

	// this test is added for new coronas' code (without depth buffer)
	// I think I should do a separate function for drawing coronas, so it will be a little faster
#ifndef MINI_GL_COMPATIBILITY
	if (PolyFlags & PF_Corona) // check to see if we need to draw the corona
	{
		//rem: all 8 (or 8.0f) values are hard coded: it can be changed to a higher value
		GLfloat     buf[8][8];
		GLdouble    cx, cy, cz;
		GLdouble    px, py, pz;
		GLfloat     scalef = 0;

		cx = (pOutVerts[0].x + pOutVerts[2].x) / 2.0f; // we should change the coronas' ...
		cy = (pOutVerts[0].y + pOutVerts[2].y) / 2.0f; // ... code so its only done once.
		cz = pOutVerts[0].z;

		// I dont know if this is slow or not
		gluProject(cx, cy, cz, modelMatrix, projMatrix, viewport, &px, &py, &pz);
		//DBG_Printf("Projection: (%f, %f, %f)\n", px, py, pz);

		if ((pz <  0.0) ||
		     (px < -8.0) ||
		     (py < viewport[1]-8.0) ||
		     (px > viewport[2]+8.0) ||
		     (py > viewport[1]+viewport[3]+8.0))
			return;

		// the damned slow glReadPixels functions :(
		glReadPixels((int)px-4, (int)py, 8, 8, GL_DEPTH_COMPONENT, GL_FLOAT, buf);
		//DBG_Printf("DepthBuffer: %f %f\n", buf[0][0], buf[3][3]);

		for (i = 0; i < 8; i++)
			for (j = 0; j < 8; j++)
				scalef += (pz > buf[i][j]+0.00005f) ? 0 : 1;

		// quick test for screen border (not 100% correct, but looks ok)
		if (px < 4) scalef -= (GLfloat)(8*(4-px));
		if (py < viewport[1]+4) scalef -= (GLfloat)(8*(viewport[1]+4-py));
		if (px > viewport[2]-4) scalef -= (GLfloat)(8*(4-(viewport[2]-px)));
		if (py > viewport[1]+viewport[3]-4) scalef -= (GLfloat)(8*(4-(viewport[1]+viewport[3]-py)));

		scalef /= 64;
		//DBG_Printf("Scale factor: %f\n", scalef);

		if (scalef < 0.05f)
			return;

		c.alpha *= scalef; // change the alpha value (it seems better than changing the size of the corona)
		glColor4fv((float *)&c);
	}
#endif
	if (PolyFlags & PF_MD2)
		return;

	glBegin(GL_TRIANGLE_FAN);
	for (i = 0; i < iNumPts; i++)
	{
		glTexCoord2f(pOutVerts[i].sow, pOutVerts[i].tow);
		//Hurdler: test code: -pOutVerts[i].z => pOutVerts[i].z
		glVertex3f(pOutVerts[i].x, pOutVerts[i].y, pOutVerts[i].z);
		//glVertex3f(pOutVerts[i].x, pOutVerts[i].y, -pOutVerts[i].z);
	}
	glEnd();

	if (PolyFlags & PF_RemoveYWrap)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if (PolyFlags & PF_ForceWrapX)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);

	if (PolyFlags & PF_ForceWrapY)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}


// ==========================================================================
//
// ==========================================================================
EXPORT void HWRAPI(SetSpecialState) (hwdspecialstate_t IdState, int Value)
{
	switch (IdState)
	{
		case 77:
		{
			//08/01/00: Hurdler this is a test for mirror
			if (!Value)
				ClearBuffer(false, true, 0); // clear depth buffer
			break;
		}

		case HWD_SET_PALETTECOLOR:
		{
			pal_col = Value;
			const_pal_col.blue  = byte2float[((Value>>16)&0xff)];
			const_pal_col.green = byte2float[((Value>>8)&0xff)];
			const_pal_col.red   = byte2float[((Value)&0xff)];
			break;
		}

		case HWD_SET_FOG_COLOR:
		{
			GLfloat fogcolor[4];

			fogcolor[0] = byte2float[((Value>>16)&0xff)];
			fogcolor[1] = byte2float[((Value>>8)&0xff)];
			fogcolor[2] = byte2float[((Value)&0xff)];
			fogcolor[3] = 0x0;
			glFogfv(GL_FOG_COLOR, fogcolor);
			break;
		}
		case HWD_SET_FOG_DENSITY:
			glFogf(GL_FOG_DENSITY, Value*1200/(500*1000000.0f));
			break;

		case HWD_SET_FOG_MODE:
			if (Value)
			{
				glEnable(GL_FOG);
				// experimental code
				/*
				switch (Value)
				{
					case 1:
						glFogi(GL_FOG_MODE, GL_LINEAR);
						glFogf(GL_FOG_START, -1000.0f);
						glFogf(GL_FOG_END, 2000.0f);
						break;
					case 2:
						glFogi(GL_FOG_MODE, GL_EXP);
						break;
					case 3:
						glFogi(GL_FOG_MODE, GL_EXP2);
						break;
				}
				*/
			}
			else
				glDisable(GL_FOG);
			break;

		case HWD_SET_POLYGON_SMOOTH:
#ifdef KOS_GL_COMPATIBILITY // GL_POLYGON_SMOOTH_HINT
			if (Value)
				glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);
			else
				glHint(GL_POLYGON_SMOOTH_HINT,GL_FASTEST);
#else
			if (Value)
				glEnable(GL_POLYGON_SMOOTH);
			else
				glDisable(GL_POLYGON_SMOOTH);
#endif
			break;

		case HWD_SET_TEXTUREFILTERMODE:
			switch (Value)
			{
#ifdef KOS_GL_COMPATIBILITY
				case HWD_SET_TEXTUREFILTER_TRILINEAR:
				case HWD_SET_TEXTUREFILTER_BILINEAR :
					min_filter = mag_filter = GL_FILTER_BILINEAR;
					break;
				case HWD_SET_TEXTUREFILTER_POINTSAMPLED :
					min_filter = mag_filter = GL_NEAREST;
					break;
				case HWD_SET_TEXTUREFILTER_MIXED1 :
				case HWD_SET_TEXTUREFILTER_MIXED2 :
					min_filter = mag_filter = GL_FILTER_NONE;
					break;
#else
				case HWD_SET_TEXTUREFILTER_TRILINEAR:
					min_filter = GL_LINEAR_MIPMAP_LINEAR;
					mag_filter = GL_LINEAR;
					break;
				case HWD_SET_TEXTUREFILTER_BILINEAR :
					min_filter = mag_filter = GL_LINEAR;
					break;
				case HWD_SET_TEXTUREFILTER_POINTSAMPLED :
					min_filter = mag_filter = GL_NEAREST;
					break;
				case HWD_SET_TEXTUREFILTER_MIXED1 :
					mag_filter = GL_LINEAR;
					min_filter = GL_NEAREST;
					break;
				case HWD_SET_TEXTUREFILTER_MIXED2 :
					mag_filter = GL_NEAREST;
					min_filter = GL_LINEAR;
					break;
#endif
			}
			Flush(); //??? if we want to change filter mode by texture, remove this
			break;

		case HWD_SET_TEXTUREANISOTROPICMODE:
			anisotropic_filter = min(Value,maximumAnisotropy);
			if (maximumAnisotropy)
				Flush(); //??? if we want to change filter mode by texture, remove this
			break;

		default:
			break;
	}
}

// -----------------+
// HWRAPI DrawMD2   : Draw an MD2 model with glcommands
// -----------------+
//EXPORT void HWRAPI(DrawMD2) (md2_model_t *model, int frame)
EXPORT void HWRAPI(DrawMD2) (int *gl_cmd_buffer, md2_frame_t *frame, FTransform *pos, float scale)
{
	int     val, count, pindex;
	GLfloat s, t;

	//TODO: Maybe we can put all this in a display list the first time it's
	//      called and after, use this display list: faster (how much?) but
	//      require more memory (how much?)

	DrawPolygon(NULL, NULL, 0, PF_Masked|PF_Modulated|PF_Occlude|PF_Clip);

	glPushMatrix(); // should be the same as glLoadIdentity
	//Hurdler: now it seems to work
	glTranslatef(pos->x, pos->z, pos->y);
	glRotatef(pos->angley, 0.0f, -1.0f, 0.0f);
	glRotatef(pos->anglex, -1.0f, 0.0f, 0.0f);
	glScalef(scale, scale, scale);

	val = *gl_cmd_buffer++;

	while (val != 0)
	{
		if (val < 0)
		{
			glBegin (GL_TRIANGLE_FAN);
			count = -val;
		}
		else
		{
			glBegin (GL_TRIANGLE_STRIP);
			count = val;
		}

		while (count--)
		{
			s = *(float *) gl_cmd_buffer++;
			t = *(float *) gl_cmd_buffer++;
			pindex = *gl_cmd_buffer++;

			glTexCoord2f (s, t);
			glVertex3f (frame->vertices[pindex].vertex[0]/2.0f,
			            frame->vertices[pindex].vertex[1]/2.0f,
			            frame->vertices[pindex].vertex[2]/2.0f);
		}

		glEnd ();

		val = *gl_cmd_buffer++;
	}
	glPopMatrix(); // should be the same as glLoadIdentity
}

// -----------------+
// SetTransform     :
// -----------------+
EXPORT void HWRAPI(SetTransform) (FTransform *stransform)
{
	static int special_splitscreen;
	glLoadIdentity();
	if (stransform)
	{
		// keep a trace of the transformation for md2
		memcpy(&md2_transform, stransform, sizeof (md2_transform));
		glScalef(stransform->scalex, stransform->scaley, -stransform->scalez);
		glRotatef(stransform->anglex      , 1.0f, 0.0f, 0.0f);
		glRotatef(stransform->angley+270.0f, 0.0f, 1.0f, 0.0f);
		glTranslatef(-stransform->x, -stransform->z, -stransform->y);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		special_splitscreen = (stransform->splitscreen && stransform->fovxangle == 90.0f);
		if (special_splitscreen)
			gluPerspective(53.13, 2*ASPECT_RATIO,  // 53.13 = 2*atan(0.5)
			                NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);
		else
			gluPerspective(stransform->fovxangle, ASPECT_RATIO, NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);
#ifndef MINI_GL_COMPATIBILITY
		glGetDoublev(GL_PROJECTION_MATRIX, projMatrix); // added for new coronas' code (without depth buffer)
#endif
		glMatrixMode(GL_MODELVIEW);
	}
	else
	{
		glScalef(1.0f, 1.0f, -1.0f);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		if (special_splitscreen)
			gluPerspective(53.13, 2*ASPECT_RATIO,  // 53.13 = 2*atan(0.5)
						   NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);
		else
			//Hurdler: is "fov" correct?
			gluPerspective(fov, ASPECT_RATIO, NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);
#ifndef MINI_GL_COMPATIBILITY
		glGetDoublev(GL_PROJECTION_MATRIX, projMatrix); // added for new coronas' code (without depth buffer)
#endif
		glMatrixMode(GL_MODELVIEW);
	}

#ifndef MINI_GL_COMPATIBILITY
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix); // added for new coronas' code (without depth buffer)
#endif
}

EXPORT int  HWRAPI(GetTextureUsed) (void)
{
	FTextureInfo*   tmp = gr_cachehead;
	int             res = 0;

	while (tmp)
	{
		res += tmp->height*tmp->width*(screen_depth/8);
		tmp = tmp->nextmipmap;
	}
	return res;
}

EXPORT int  HWRAPI(GetRenderVersion) (void)
{
	return VERSION;
}
