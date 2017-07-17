#!/bin/sh

if [ ! -d 'bin' ]
then
  mkdir bin
fi

if [[ ! -z "${IS_DEBUG}" ]]
then
  echo "building debug version"
  g++ --std=c++11 -D IS_DEBUG -o bin/game src/main.cpp `sdl2-config --cflags --libs` `pkg-config SDL2_ttf --cflags --libs` `pkg-config SDL2_image --cflags --libs`
else
  echo "building release version"
  g++ --std=c++11 -o bin/game src/main.cpp `sdl2-config --cflags --libs` `pkg-config SDL2_ttf --cflags --libs` `pkg-config SDL2_image --cflags --libs`
fi
