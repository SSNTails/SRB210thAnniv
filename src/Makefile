#############################################################################
#
#     GNU Make makefile for SRB2
#
# Copyright (C) 1998-2000 by DooM Legacy Team.
# Copyright (C) 2003-2004 by Sonic Team Jr.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
#     -DPC_DOS    -> use DOS specific code (eg:textmode stuff)...
#     -DLINUX     -> use for the GNU/Linux specific
#     -D_WINDOWS  -> use for the Win32/DirectX specific
#     -DSDL       -> use for the SDL interface
#
# Sets:
#     Compile the DGJPP/DOS version with 'make WATTCP=1'
#     Compile the DirectX/Mingw version with 'make MINGW=1'
#     Compile the SDL/Mingw version with 'make MINGW=1 SDL=1'
#     Compile the SDL/Linux version with 'make LINUX=1'
#     Compile the SDL/Solaris version with 'make SOLARIS=1'
#     Compile the SDL/FreeBSD version with 'gmake FREEBSD=1'
#     Compile the SDL/Cygwin version with 'make CYGWIN32=1'
#     Compile the SDL/other version try with 'make SDL=1'
#
# 'Targets':
#     clean
#       Remove all object files
#     cleandep
#       Remove depend.dep
#     dll
#       compile primary HW render DLL/SO
#     all_dll
#       compile all HW render and 3D sound DLLs for the set
#     opengl_dll
#       Pure Mingw only, compile OpenGL HW render DLL
#     minigl_dll
#       Pure Mingw only, compile MiniGL HW render DLL
#     ds3d_dll
#       Pure Mingw only, compile DirectX DirectSound HW sound DLL
#     fmod_dll
#       Pure Mingw only, compile FMOD HW sound DLL
#     openal_dll
#       Pure Mingw only, compile OpenAL HW sound DLL
#     fmod_so
#       Non-Mingw, compile FMOD HW sound SO
#     openal_so
#       Non-Mingw, compile OpenAL HW sound SO
#
#
# Addon:
#     To Cross-Compile, CC=gcc-version make * CCBS=1 PREFIX=<dir>
#     Compile with GCC 2.97 version, add 'GCC29=1'
#     Compile with GCC 4.0x version, add 'GCC40=1'
#     Compile with GCC 4.1x version, add 'GCC41=1'
#     Compile a profile version, add 'PROFILEMODE=1'
#     Compile a debug version, add 'DEBUGMODE=1'
#     Compile with extra warnings, add 'WARNINGMODE=1'
#     Compile without NASM's tmap.nas, add 'NOASM=1'
#     Compile without 3D hardware support, add 'NOHW=1'
#     Compile without 3D sound support, add 'NOHS=1'
#     Compile with GDBstubs, add 'RDB=1'
#     Compile without PNG, add 'NOPNG=1'
#
# Addon for SDL:
#     To Cross-Compile, add 'SDL_CONFIG=/usr/*/bin/sdl-config'
#     Compile without SDL_Mixer, add 'NOMIXER=1'
#     Compile without BSD API, add 'NONET=1'
#     Compile without IPX/SPX, add 'NOIPX=1'
#     Compile Mingw/SDL with S_DS3S, add 'DS3D=1'
#     Comple with S_FMOD3D, add 'FMOD=1' (WIP)
#     Comple with S_OPENAL, add 'OPENAL=1' (WIP)
#     To link with the whole SDL_Image lib to load Icons, add 'SDL_IMAGE=1' but it isn't not realy needed
#     To link with SDLMain to hide console or make on a console-less binary, add 'SDLMAIN=1'
#
#
#############################################################################

include Makefile.cfg

ifdef DUMMY
NOPNG=1
NOMNG=1
NONET=1
NOHW=1
NOHS=1
NOASM=1
NOIPX=1
EXENAME?=srb2dummy
OPTS+=-DUNIXLIKE
OBJS=$(OBJDIR)/i_video.o
LIBS=-lm
endif

ifdef DJGPPDOS
include djgppdos/Makefile.cfg
endif

ifdef MINGW
include win32/Makefile.cfg
endif #ifdef MINGW

ifdef LINUX
UNIXLIKE=1
endif

ifdef SOLARIS
UNIXLIKE=1
endif

ifdef FREEBSD
UNIXLIKE=1
endif

ifdef SDL
include sdl/Makefile.cfg
endif #ifdef SDL

ifdef DISTCC
        CC:=distcc $(CC)
endif

ifdef CCACHE
        CC:=ccache $(CC)
endif

ifndef ECHO
	NASM:=@$(NASM)
	REMOVE:=@$(REMOVE)
	CC:=@$(CC)
	CXX:=@$(CXX)
	WINDRES:=@$(WINDRES)
	DLLWRAP:=@$(DLLWRAP)
endif

ifdef NONET
	OPTS+=-DNONET
else
ifdef NOIPX
	OPTS+=-DNOIPX
endif
endif

ifdef NOHW
	OPTS+=-DNOHW
else
ifndef DC
	#Hurdler: not really supported and not tested recently
	#OPTS+=-DUSE_PALETTED_TEXTURE
endif
	OPTS+=-DHWRENDER
	OBJS+=$(OBJDIR)/hw_bsp.o $(OBJDIR)/hw_draw.o $(OBJDIR)/hw_light.o \
		 $(OBJDIR)/hw_main.o $(OBJDIR)/hw_md2.o $(OBJDIR)/hw_cache.o $(OBJDIR)/hw_trick.o
endif

ifdef NOHS
	OPTS+=-DNOHS
else
	OPTS+=-DHW3SOUND
	OBJS+=$(OBJDIR)/hw3sound.o
endif

ifndef NONX86
ifndef GCC29
	M5=-march=pentium
	M4=-march=i486
else
	M5=-mpentium
	M4=-m486
endif
endif

ifndef NOASM
ifndef NONX86
	OBJS+=$(OBJDIR)/tmap.o $(OBJDIR)/tmap_mmx.o $(OBJDIR)/vid_copy.o
	OPTS+=-DUSEASM
endif
endif

ifndef NOPNG
OPTS+=-DHAVE_PNG

ifdef PREFIX
SDL_CONFIG?=$(PREFIX)-sdl-config
else
PNG_CONFIG?=libpng-config
endif

ifdef STATIC
PNG_CFLAGS?=$(shell $(PNG_CONFIG) --static --cflags)
PNG_LDFLAGS?=$(shell $(PNG_CONFIG) --static --libs)
else
PNG_CFLAGS?=$(shell $(PNG_CONFIG) --cflags)
PNG_LDFLAGS?=$(shell $(PNG_CONFIG) --libs)
endif

LIBS+=$(PNG_LDFLAGS)
CFLAGS+=$(PNG_CFLAGS)

ifndef NOMNG
#OPTS+=-DHAVE_MNG
#LIBS:=-lmng $(LIBS)
endif
endif

ifdef STATIC
LIBS:=-static $(LIBS)
endif

ifdef NOMD5
	OPTS+=-DNOMD5
else
	OBJS:=$(OBJDIR)/md5.o $(OBJS)
endif

ifdef FAKEDC
	OPTS+=-DDC
endif

	OPTS:=-fno-exceptions -ffloat-store $(OPTS)

ifdef DEBUGMODE

	# build with debugging information
	WINDRESFLAGS = -D_DEBUG
	CFLAGS+=-g -O0 $(OPTS) $(M5) -DPARANOIA -DRANGECHECK $(WINDRESFLAGS)
else


	# build a normal optimised version
	WINDRESFLAGS = -DNDEBUG
	CFLAGS+=$(OPTS) $(M5) -Os $(WINDRESFLAGS)
ifndef DC
	CFLAGS+=-fomit-frame-pointer
endif
		# -fgcse -fcse-follow-jumps -fcse-skip-blocks -frerun-cse-after-loop -ffunction-cse \
		# -fexpensive-optimizations -fforce-mem -fstrength-reduce -fthread-jumps  \
		# -fregmove -fschedule-insns -fschedule-insns2 -fmove-all-movables \
		# -fcaller-saves -finline-functions -mieee-fp -mfp-ret-in-387 -m80387 -mhard-float -finline
	LIBS:=-s $(LIBS)
endif

ifdef PROFILEMODE
	# build with profiling information
	CFLAGS:=-pg $(CFLAGS)
endif

# not too sophisticated dependency
OBJS:=$(i_main_o) \
		$(OBJDIR)/comptime.o \
		$(OBJDIR)/string.o   \
		$(OBJDIR)/d_main.o   \
		$(OBJDIR)/d_clisrv.o \
		$(OBJDIR)/d_net.o    \
		$(OBJDIR)/d_netfil.o \
		$(OBJDIR)/dehacked.o \
		$(OBJDIR)/dstrings.o \
		$(OBJDIR)/z_zone.o   \
		$(OBJDIR)/f_finale.o \
		$(OBJDIR)/f_wipe.o   \
		$(OBJDIR)/g_game.o   \
		$(OBJDIR)/g_input.o  \
		$(OBJDIR)/am_map.o   \
		$(OBJDIR)/command.o  \
		$(OBJDIR)/console.o  \
		$(OBJDIR)/hu_stuff.o \
		$(OBJDIR)/y_inter.o  \
		$(OBJDIR)/st_stuff.o \
		$(OBJDIR)/m_argv.o   \
		$(OBJDIR)/m_bbox.o   \
		$(OBJDIR)/m_cheat.o  \
		$(OBJDIR)/m_fixed.o  \
		$(OBJDIR)/m_menu.o   \
		$(OBJDIR)/m_misc.o   \
		$(OBJDIR)/m_random.o \
		$(OBJDIR)/m_queue.o  \
		$(OBJDIR)/info.o     \
		$(OBJDIR)/p_ceilng.o \
		$(OBJDIR)/p_enemy.o  \
		$(OBJDIR)/p_fab.o    \
		$(OBJDIR)/p_floor.o  \
		$(OBJDIR)/p_inter.o  \
		$(OBJDIR)/p_lights.o \
		$(OBJDIR)/p_map.o    \
		$(OBJDIR)/p_maputl.o \
		$(OBJDIR)/p_mobj.o   \
		$(OBJDIR)/p_polyobj.o\
		$(OBJDIR)/p_saveg.o  \
		$(OBJDIR)/p_setup.o  \
		$(OBJDIR)/p_sight.o  \
		$(OBJDIR)/p_spec.o   \
		$(OBJDIR)/p_telept.o \
		$(OBJDIR)/p_tick.o   \
		$(OBJDIR)/p_user.o   \
		$(OBJDIR)/tables.o   \
		$(OBJDIR)/r_bsp.o    \
		$(OBJDIR)/r_data.o   \
		$(OBJDIR)/r_draw.o   \
		$(OBJDIR)/r_main.o   \
		$(OBJDIR)/r_plane.o  \
		$(OBJDIR)/r_segs.o   \
		$(OBJDIR)/r_sky.o    \
		$(OBJDIR)/r_splats.o \
		$(OBJDIR)/r_things.o \
		$(OBJDIR)/screen.o   \
		$(OBJDIR)/v_video.o  \
		$(OBJDIR)/s_sound.o  \
		$(OBJDIR)/sounds.o   \
		$(OBJDIR)/w_wad.o    \
		$(OBJDIR)/filesrch.o \
		$(OBJDIR)/mserv.o    \
		$(OBJDIR)/i_tcp.o    \
		$(OBJDIR)/lzf.o	     \
		$(i_cdmus_o)    \
		$(i_net_o)      \
		$(i_system_o)   \
		$(i_sound_o)    \
		$(OBJS)

ifdef DJGPPDOS
all:	 pre-build $(BIN)/$(EXENAME)
endif

ifdef XBOX
all:	 pre-build $(BIN)/$(BINNAME)
endif

ifdef PSP
all:	 pre-build $(BIN)/$(BINNAME) post-build
endif

ifndef PSP
ifndef XBOX
ifdef MINGW
ifndef SDL
all:	 pre-build $(BIN)/$(EXENAME) dll
endif
endif
endif

ifdef SDL
all:	 pre-build $(BIN)/$(EXENAME)
endif
endif

ifdef DUMMY
all:	$(BIN)/$(EXENAME)
endif

cleandep:
	$(REMOVE) $(OBJDIR)/depend.dep

pre-build:
	-@touch comptime.c

clean:
	$(REMOVE) *~ *.flc
	$(REMOVE) $(OBJDIR)/*.o

ifdef MINGW
	$(REMOVE) $(OBJDIR)/*.res
endif

ifdef CYGWIN32
	$(REMOVE) $(OBJDIR)/*.res
endif

#make a big srb2.s that is the disasm of the exe (dos only ?)
asm:
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -g $(OBJDIR)/i_main.o \
	-o $(OBJDIR)/tmp.exe $(LIBS)
	objdump  -d $(OBJDIR)/tmp.exe --no-show-raw-insn > srb2.s
	$(REMOVE) $(OBJDIR)/tmp.exe

# executable

$(BIN)/$(EXENAME): $(OBJS) $(OBJDIR)/d_netcmd.o
	-@mkdir -p $(BIN)
	@echo Linking $(EXENAME)...
	$(LD) $(LDFLAGS) $(OBJS) $(OBJDIR)/d_netcmd.o \
	-o $(BIN)/$(EXENAME) $(LIBS)

$(OBJDIR):
	-@mkdir -p $(OBJDIR)

ifndef SDL
#dll
dll : opengl_dll
ifdef MINGW
all_dll: opengl_dll minigl_dll ds3d_dll fmod_dll openal_dll
opengl_dll : $(OBJDIR)/r_opengl.o $(OBJDIR)/ogl_win.o hardware/r_mingw.def
	@echo Linking R_OpenGL.dll...
	$(DLLWRAP) --def hardware/r_mingw.def $(OBJDIR)/r_opengl.o $(OBJDIR)/ogl_win.o -lopengl32 -lglu32 -lgdi32 -mwindows $(M5) -s -o $(BIN)/r_opengl.dll

minigl_dll: $(OBJDIR)/r_minigl.o hardware/r_mingw.def
	@echo Linking R_MiniGL.dll...
	$(DLLWRAP) --def hardware/r_mingw.def $(OBJDIR)/r_minigl.o -lopengl32 -lglu32 -lgdi32 -mwindows $(M5) -s -o $(BIN)/r_minigl.dll

ds3d_dll: $(OBJDIR)/s_ds3d.o hardware/s_mingw.def
	@echo Linking S_DS3d.dll...
	$(DLLWRAP) --def hardware/s_mingw.def $(OBJDIR)/s_ds3d.o -ldsound -luuid -mwindows $(M5) -s -o $(BIN)/s_ds3d.dll

fmod_dll: $(OBJDIR)/s_fmod.o hardware/s_mingw.def
	@echo Linking S_FMOD.dll...
	$(DLLWRAP) --def hardware/s_mingw.def $(OBJDIR)/s_fmod.o -lfmod -mwindows $(M5) -s -o $(BIN)/s_fmod.dll

openal_dll: $(OBJDIR)/s_openal.o hardware/s_mingw.def
	@echo Linking S_OpenAL.dll...
	$(DLLWRAP) --def hardware/s_mingw.def $(OBJDIR)/s_openal.o -lopenal32 -mwindows $(M5) -s -o $(BIN)/s_openal.dll
else
all_dll: fmod_so openal_so

fmod_so: $(OBJDIR)/s_fmod.o
	@echo Linking S_FMOD.so...
	$(CC) -s -Os -o $(BIN)/s_fmod.so -shared -nostartfiles $(OBJDIR)/s_fmod.o -L. -lfmod -lm

openal_so: $(OBJDIR)/s_openal.o
	@echo Linking S_OpenAL.so...
	$(CC) -s -Os -o $(BIN)/s_openal.so -shared -nostartfiles $(OBJDIR)/s_openal.o -lopenal -lm

endif

else
ifdef SDL
ifdef MINGW
$(OBJDIR)/r_opengl.o: hardware/r_opengl/r_opengl.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@
else
$(OBJDIR)/r_opengl.o: hardware/r_opengl/r_opengl.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -I/usr/X11R6/include -c $< -o $@
endif
endif

endif

#dependecy made by gcc itself !
$(OBJS):
ifndef DUMMY
-include $(OBJDIR)/depend.dep
endif

$(OBJDIR)/depend.dep:
	@echo "Creating dependency file, depend.dep"
	$(CC) $(CFLAGS) -MM *.c > $(OBJDIR)/depend.ped
	$(CC) $(CFLAGS) -MM $(INTERFACE)/*.c >> $(OBJDIR)/depend.ped
ifndef NOHW
	$(CC) $(CFLAGS) -MM hardware/*.c >> $(OBJDIR)/depend.ped
endif
	@sed -e 's,\(.*\)\.o: ,$(subst /,\/,$(OBJDIR))\/&,g' < $(OBJDIR)/depend.ped > $(OBJDIR)/depend.dep
	$(REMOVE) $(OBJDIR)/depend.ped
	@echo "Created dependency file, depend.dep"

ifdef DC
$(OBJDIR)/v_video.o: v_video.c doomdef.h doomtype.h g_state.h m_swap.h r_local.h \
  tables.h m_fixed.h screen.h command.h m_bbox.h r_main.h d_player.h \
  p_pspr.h info.h d_think.h sounds.h p_mobj.h doomdata.h d_ticcmd.h \
  r_data.h r_defs.h r_state.h r_bsp.h r_segs.h r_plane.h r_sky.h \
  r_things.h r_draw.h v_video.h hu_stuff.h d_event.h w_wad.h console.h \
  i_video.h z_zone.h doomstat.h d_clisrv.h d_netcmd.h
	$(CC) $(CFLAGS) -fno-omit-frame-pointer $(WFLAGS) -c $< -o $@
endif

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(INTERFACE)/%.c
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/%.o: hardware/%.c
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.nas
	$(NASM) -o $@ -f $(NASMFORMAT) $<

$(OBJDIR)/vid_copy.o: vid_copy.s asm_defs.inc
	$(CC) $(OPTS) -x assembler-with-cpp -c $< -o $@

$(OBJDIR)/%.o: %.s
	$(CC) $(OPTS) -x assembler-with-cpp -c $< -o $@

$(OBJDIR)/SRB2.res: win32/Srb2win.rc win32/afxres.h win32/resource.h
	$(WINDRES) -i $< -O rc $(WINDRESFLAGS) --include-dir=win32 -o $@ -O coff

ifdef DC
$(OBJDIR)/romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d ../data -v

$(OBJDIR)/romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

$(OBJDIR)/dchelp.o: $(INTERFACE)/SRB2DC/dchelp.c
		$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/i_udp.o: $(INTERFACE)/SRB2DC/i_udp.c
		$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@
endif

ifdef XBOX
$(OBJDIR)/xboxhelp.o: $(INTERFACE)/SRB2XBOX/xboxhelp.c
		$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(BIN)/$(BINNAME): $(BIN)/$(EXENAME)
	$(CXBE) -OUT:"$@" -DUMPINFO:"$(BIN)/SRB2XBOX.cxbe"  -TITLE:"Sonic Robo Blast 2" $<
endif

ifdef MINGW
ifndef SDL
ifndef NOHW
$(OBJDIR)/r_opengl.o: hardware/r_opengl/r_opengl.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -D_WINDOWS -mwindows -c $< -o $@

$(OBJDIR)/ogl_win.o: hardware/r_opengl/ogl_win.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -D_WINDOWS -mwindows -c $< -o $@

$(OBJDIR)/r_minigl.o: hardware/r_minigl/r_minigl.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -D_WINDOWS -mwindows -c $< -o $@
endif

ifndef NOHS
$(OBJDIR)/s_ds3d.o: hardware/s_ds3d/s_ds3d.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_ds3d.o $(WFLAGS) -D_WINDOWS -mwindows -c hardware/s_ds3d/s_ds3d.c

$(OBJDIR)/s_fmod.o: hardware/s_openal/s_openal.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_fmod.o $(WFLAGS) -D_WINDOWS -mwindows -c hardware/s_fmod/s_fmod.c

$(OBJDIR)/s_openal.o: hardware/s_openal/s_openal.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_openal.o $(WFLAGS) -D_WINDOWS -mwindows -c hardware/s_openal/s_openal.c
endif
endif
endif

ifdef SDL
ifdef STATICHS
$(OBJDIR)/s_openal.o: hardware/s_openal/s_openal.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/s_fmod.o: hardware/s_fmod/s_fmod.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

ifdef MINGW
$(OBJDIR)/s_ds3d.o: hardware/s_ds3d/s_ds3d.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@
endif
else

$(OBJDIR)/s_fmod.o: hardware/s_fmod/s_fmod.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_fmod.o -DHW3SOUND -DUNIXLIKE -shared -nostartfiles -c hardware/s_fmod/s_fmod.c

$(OBJDIR)/s_openal.o: hardware/s_openal/s_openal.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_openal.o -DHW3SOUND -DUNIXLIKE -shared -nostartfiles -c hardware/s_openal/s_openal.c
endif

ifdef FILTERS
$(OBJDIR)/%.o: $(INTERFACE)/filter/%.c
	@echo $< needs deps
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/filters.o: $(INTERFACE)/filter/filters.c $(INTERFACE)/filter/filters.c \
 $(INTERFACE)/filter/filters.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/hq2x.o: $(INTERFACE)/filter/hq2x.c $(INTERFACE)/filter/hq2x.c \
 $(INTERFACE)/filter/filters.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/lq2x.o: $(INTERFACE)/filter/lq2x.c $(INTERFACE)/filter/lq2x.c \
 $(INTERFACE)/filter/filters.h $(INTERFACE)/filter/interp.h \
 $(INTERFACE)/filter/hq2x.h $(INTERFACE)/filter/lq2x.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@
endif
endif

#############################################################
#
#############################################################
