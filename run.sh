#!/bin/sh

if [ -f "./bin/game" ]
then
  time ./bin/game
else
  ./build.sh && ./run.sh
fi
