# Microsoft Developer Studio Project File - Name="opengl32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=opengl32 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "opengl32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "opengl32.mak" CFG="opengl32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "opengl32 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "opengl32 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "opengl32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\bin\VC\Release\r_d3d\opengl"
# PROP Intermediate_Dir "..\..\..\..\objs\VC\Release\r_d3d\opengl"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OPENGL32_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "__WIN32__" /D "__MSC__" /D "MESAD3D" /D "NOCRYPT" /D "FAST_MATH" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"..\..\..\..\bin\VC\Release\opengl32.dll"

!ELSEIF  "$(CFG)" == "opengl32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\bin\VC\Debug\r_d3d\opengl"
# PROP Intermediate_Dir "..\..\..\..\objs\VC\Debug\r_d3d\opengl"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OPENGL32_EXPORTS" /YX /FD /GZ  /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "D3D_DEBUG" /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "__WIN32__" /D "__MSC__" /D "MESAD3D" /D "NOCRYPT" /D "FAST_MATH" /YX /FD /GZ  /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\..\..\..\bin\VC\Debug/opengl32.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "opengl32 - Win32 Release"
# Name "opengl32 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\accum.c
# End Source File
# Begin Source File

SOURCE=.\alpha.c
# End Source File
# Begin Source File

SOURCE=.\alphabuf.c
# End Source File
# Begin Source File

SOURCE=.\api1.c
# End Source File
# Begin Source File

SOURCE=.\api2.c
# End Source File
# Begin Source File

SOURCE=.\apiext.c
# End Source File
# Begin Source File

SOURCE=.\asm_mmx.c
# End Source File
# Begin Source File

SOURCE=.\attrib.c
# End Source File
# Begin Source File

SOURCE=.\bitmap.c
# End Source File
# Begin Source File

SOURCE=.\blend.c
# End Source File
# Begin Source File

SOURCE=.\clip.c
# End Source File
# Begin Source File

SOURCE=.\colortab.c
# End Source File
# Begin Source File

SOURCE=.\context.c
# End Source File
# Begin Source File

SOURCE=.\copypix.c
# End Source File
# Begin Source File

SOURCE=.\depth.c
# End Source File
# Begin Source File

SOURCE=.\dlist.c
# End Source File
# Begin Source File

SOURCE=.\drawpix.c
# End Source File
# Begin Source File

SOURCE=.\enable.c
# End Source File
# Begin Source File

SOURCE=.\eval.c
# End Source File
# Begin Source File

SOURCE=.\feedback.c
# End Source File
# Begin Source File

SOURCE=.\fog.c
# End Source File
# Begin Source File

SOURCE=.\get.c
# End Source File
# Begin Source File

SOURCE=.\hash.c
# End Source File
# Begin Source File

SOURCE=.\image.c
# End Source File
# Begin Source File

SOURCE=.\light.c
# End Source File
# Begin Source File

SOURCE=.\lines.c
# End Source File
# Begin Source File

SOURCE=.\logic.c
# End Source File
# Begin Source File

SOURCE=.\masking.c
# End Source File
# Begin Source File

SOURCE=.\matrix.c
# End Source File
# Begin Source File

SOURCE=.\misc.c
# End Source File
# Begin Source File

SOURCE=.\mmath.c
# End Source File
# Begin Source File

SOURCE=.\pb.c
# End Source File
# Begin Source File

SOURCE=.\pixel.c
# End Source File
# Begin Source File

SOURCE=.\pointers.c
# End Source File
# Begin Source File

SOURCE=.\points.c
# End Source File
# Begin Source File

SOURCE=.\polygon.c
# End Source File
# Begin Source File

SOURCE=.\quads.c
# End Source File
# Begin Source File

SOURCE=.\rastpos.c
# End Source File
# Begin Source File

SOURCE=.\readpix.c
# End Source File
# Begin Source File

SOURCE=.\rect.c
# End Source File
# Begin Source File

SOURCE=.\scissor.c
# End Source File
# Begin Source File

SOURCE=.\shade.c
# End Source File
# Begin Source File

SOURCE=.\span.c
# End Source File
# Begin Source File

SOURCE=.\stencil.c
# End Source File
# Begin Source File

SOURCE=.\teximage.c
# End Source File
# Begin Source File

SOURCE=.\texobj.c
# End Source File
# Begin Source File

SOURCE=.\texstate.c
# End Source File
# Begin Source File

SOURCE=.\texture.c
# End Source File
# Begin Source File

SOURCE=.\triangle.c
# End Source File
# Begin Source File

SOURCE=.\varray.c
# End Source File
# Begin Source File

SOURCE=.\vb.c
# End Source File
# Begin Source File

SOURCE=.\vbfill.c
# End Source File
# Begin Source File

SOURCE=.\vbrender.c
# End Source File
# Begin Source File

SOURCE=.\vbxform.c
# End Source File
# Begin Source File

SOURCE=.\winpos.c
# End Source File
# Begin Source File

SOURCE=.\xform.c
# End Source File
# Begin Source File

SOURCE=.\zoom.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\accum.h
# End Source File
# Begin Source File

SOURCE=.\all.h
# End Source File
# Begin Source File

SOURCE=.\alpha.h
# End Source File
# Begin Source File

SOURCE=.\alphabuf.h
# End Source File
# Begin Source File

SOURCE=.\api.h
# End Source File
# Begin Source File

SOURCE=.\asm_386.h
# End Source File
# Begin Source File

SOURCE=.\asm_mmx.h
# End Source File
# Begin Source File

SOURCE=.\attrib.h
# End Source File
# Begin Source File

SOURCE=.\bitmap.h
# End Source File
# Begin Source File

SOURCE=.\blend.h
# End Source File
# Begin Source File

SOURCE=.\clip.h
# End Source File
# Begin Source File

SOURCE=.\colortab.h
# End Source File
# Begin Source File

SOURCE=.\CONFIG.H
# End Source File
# Begin Source File

SOURCE=.\context.h
# End Source File
# Begin Source File

SOURCE=.\copypix.h
# End Source File
# Begin Source File

SOURCE=.\dd.h
# End Source File
# Begin Source File

SOURCE=.\depth.h
# End Source File
# Begin Source File

SOURCE=.\dlist.h
# End Source File
# Begin Source File

SOURCE=.\drawpix.h
# End Source File
# Begin Source File

SOURCE=.\enable.h
# End Source File
# Begin Source File

SOURCE=.\eval.h
# End Source File
# Begin Source File

SOURCE=.\feedback.h
# End Source File
# Begin Source File

SOURCE=.\fixed.h
# End Source File
# Begin Source File

SOURCE=.\fog.h
# End Source File
# Begin Source File

SOURCE=.\get.h
# End Source File
# Begin Source File

SOURCE=.\hash.h
# End Source File
# Begin Source File

SOURCE=.\image.h
# End Source File
# Begin Source File

SOURCE=.\light.h
# End Source File
# Begin Source File

SOURCE=.\lines.h
# End Source File
# Begin Source File

SOURCE=.\linetemp.h
# End Source File
# Begin Source File

SOURCE=.\logic.h
# End Source File
# Begin Source File

SOURCE=.\macros.h
# End Source File
# Begin Source File

SOURCE=.\masking.h
# End Source File
# Begin Source File

SOURCE=.\matrix.h
# End Source File
# Begin Source File

SOURCE=.\misc.h
# End Source File
# Begin Source File

SOURCE=.\mmath.h
# End Source File
# Begin Source File

SOURCE=.\pb.h
# End Source File
# Begin Source File

SOURCE=.\pixel.h
# End Source File
# Begin Source File

SOURCE=.\pointers.h
# End Source File
# Begin Source File

SOURCE=.\points.h
# End Source File
# Begin Source File

SOURCE=.\polygon.h
# End Source File
# Begin Source File

SOURCE=.\quads.h
# End Source File
# Begin Source File

SOURCE=.\rastpos.h
# End Source File
# Begin Source File

SOURCE=.\readpix.h
# End Source File
# Begin Source File

SOURCE=.\rect.h
# End Source File
# Begin Source File

SOURCE=.\scissor.h
# End Source File
# Begin Source File

SOURCE=.\shade.h
# End Source File
# Begin Source File

SOURCE=.\span.h
# End Source File
# Begin Source File

SOURCE=.\stencil.h
# End Source File
# Begin Source File

SOURCE=.\teximage.h
# End Source File
# Begin Source File

SOURCE=.\texobj.h
# End Source File
# Begin Source File

SOURCE=.\texstate.h
# End Source File
# Begin Source File

SOURCE=.\texture.h
# End Source File
# Begin Source File

SOURCE=.\triangle.h
# End Source File
# Begin Source File

SOURCE=.\tritemp.h
# End Source File
# Begin Source File

SOURCE=.\TYPES.H
# End Source File
# Begin Source File

SOURCE=.\varray.h
# End Source File
# Begin Source File

SOURCE=.\vb.h
# End Source File
# Begin Source File

SOURCE=.\vbfill.h
# End Source File
# Begin Source File

SOURCE=.\vbrender.h
# End Source File
# Begin Source File

SOURCE=.\vbxform.h
# End Source File
# Begin Source File

SOURCE=.\winpos.h
# End Source File
# Begin Source File

SOURCE=.\xform.h
# End Source File
# Begin Source File

SOURCE=.\zoom.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
