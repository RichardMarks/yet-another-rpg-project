#!/bin/sh

if [ ! -d 'bin' ]
then
  mkdir bin
fi

if [[ ! -z "${IS_DEBUG}" ]]
then
  echo "building debug version"
  g++ -D IS_DEBUG -o bin/game src/main.cpp `sdl2-config --cflags --libs` `pkg-config SDL2_ttf --cflags --libs`
else
  echo "building release version"
  g++ -o bin/game src/main.cpp `sdl2-config --cflags --libs` `pkg-config SDL2_ttf --cflags --libs`
fi
