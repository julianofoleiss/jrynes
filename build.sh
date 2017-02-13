#!/bin/bash
#CC=/opt/ekopath-4.0.12.1/bin/pathcc
#CC=gcc-4.6
CC=gcc
CXX=g++
CLANG=clang
OPT=-O0
COREOPT=-O0
LDFLAGS="-rdynamic"
#CFLAGS="-g -DPAPI_TIMING"
CFLAGS="-g"
INSTRUCTION_TRACING="-DINSTRUCTION_TRACING -DINSTRUCTION_TRACING_DBT"
INSTRUCTIONIR="-DINSTRUCTIONIR"
DEBUG="-DDEBUG -DDEBUGCONSOLE"
if [ "`uname -s`" == "Darwin" ]
then
    ASMUTILITIES=""
else
    ASMUTILITIES="-DASMUTILITIES"
fi
#ASMUTILITIES=""
ASMOBJ=""

echo "Compiling core emulator files..."
$CC $OPT -c 6502.c $DEBUG $INSTRUCTION_TRACING $CFLAGS $LDFLAGS
$CC $COREOPT -c 6502_new.c $DEBUG $INSTRUCTION_TRACING $CFLAGS $LDFLAGS
$CC $OPT -c romLoader.c $DEBUG  $CFLAGS $LDFLAGS

$CC $OPT -c NESMemory.c $DEBUG $CFLAGS $LDFLAGS

$CC $OPT -c PPU.c $DEBUG $CFLAGS $LDFLAGS
$CC $OPT -c PPUMemory.c $ASMUTILITIES $DEBUG $CFLAGS $LDFLAGS
$CC $OPT -c main.c $ASMUTILITIES $DEBUG $CFLAGS $LDFLAGS
$CC $OPT -c NES.c $DEBUG $CFLAGS $LDFLAGS
$CC $OPT -c KeyboardJoypad.c $CFLAGS $LDFLAGS
$CC $OPT -c Input.c $CFLAGS $LDFLAGS
$CC $OPT -c Movie.c $CFLAGS $LDFLAGS
$CC $OPT -c 6502_Debug.c $CFLAGS $LDFLAGS
$CC $OPT -c 6502_Dis.c $CFLAGS $LDFLAGS
$CC $OPT -c List.c $CFLAGS $LDFLAGS
$CC $OPT -c Statistics.c $CFLAGS $LDFLAGS
$CC $OPT -c DynArray.c $CFLAGS $LDFLAGS

echo "Compiling Mappers..."
$CC $OPT -c mappers/mapper.c $CFLAGS $LDFLAGS
$CC $OPT -c mappers/mapper000.c $CFLAGS $LDFLAGS
$CC $OPT -c mappers/mapper002.c $CFLAGS $LDFLAGS
$CC $OPT -c mappers/mapper007.c $CFLAGS $LDFLAGS

echo "Compiling SDL frontend..."
$CC $OPT -c SDL/SDL.c $DEBUG $CFLAGS $LDFLAGS
$CC $OPT -c SDL/Video.c $DEBUG $CFLAGS $LDFLAGS
$CC $OPT -c SDL/Keyboard.c $CFLAGS $LDFLAGS

if [ "$ASMUTILITIES" != "" ]; then
    echo "Assembling x86_64 utilities for Linux..."
    as ASMUtilities.asm -o ASMUtilities.o --gstabs
    ASMOBJ="ASMUtilities.o"
fi


if [ "`uname -s`" == "Darwin" ]
then
    if [ ! -d "/tmp/sdl12" ]
    then
        (
            echo "Downloading and building libSDL 1.2.15 without Cocoa support..."
            curl http://www.libsdl.org/release/SDL-1.2.15.tar.gz -o /tmp/SDL-1.2.15.tar.gz
            cd /tmp
            tar xzf SDL-1.2.15.tar.gz
            cd SDL-1.2.15
            ./configure --with-x --enable-video-carbon=no --enable-video-cocoa=no --prefix=/tmp/sdl12 --enable-video-x11=yes --enable-audio=no --enable-cdrom=no --enable-joystick=no
            make
            make install
            echo "Done."
        )
    fi
    echo "Linking..."
    $CC $OPT -o jrynes main.o 6502.o $ASMOBJ mapper.o mapper000.o mapper002.o mapper007.o DynArray.o 6502_new.o Statistics.o  List.o romLoader.o NESMemory.o PPU.o PPUMemory.o NES.o SDL.o Video.o Keyboard.o KeyboardJoypad.o Input.o 6502_Debug.o Movie.o 6502_Dis.o $(sdl-config --cflags) $(/tmp/sdl12/bin/sdl-config --cflags) $(/tmp/sdl12/bin/sdl-config --libs) $(pkg-config --cflags --libs x11) -rdynamic -ldl $CFLAGS
else
    echo "Linking..."
    $CC $OPT -o jrynes main.o 6502.o $ASMOBJ mapper.o mapper000.o mapper002.o mapper007.o DynArray.o 6502_new.o Statistics.o  List.o romLoader.o NESMemory.o PPU.o PPUMemory.o NES.o SDL.o Video.o Keyboard.o KeyboardJoypad.o Input.o 6502_Debug.o Movie.o 6502_Dis.o $(sdl-config --cflags) $(sdl-config --libs) -rdynamic -ldl -lpthread $CFLAGS
fi

echo "Done."
