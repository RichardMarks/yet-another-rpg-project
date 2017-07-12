#!/bin/sh

if [ ! -d 'bin' ]
then
  mkdir bin
fi

g++ -o bin/game src/main.cpp `sdl2-config --cflags --libs` `pkg-config SDL2_ttf --cflags --libs`
