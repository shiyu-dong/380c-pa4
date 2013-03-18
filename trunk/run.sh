#!/usr/bin/env bash

# A script that invokes your compiler.
# check number of parameters
if [ $# -eq 0 ]
then
  echo "$0: need at least one parameter"
  exit 1
fi

if [ $1 == "-backend=c" ]
then
  python ./convert.py
  exit 0
fi

if [ $1 == "-backend=cfg" ]
then
  ./dce -opt=none -backend=cfg
  exit 0
fi

if [ $1 == "-backend=3addr" ]
then
  ./dce -opt=none -backend=3addr
  exit 0
fi

if [ $1 == "-opt=dce" ]
then
  if [ $# -eq 1 ]
  then
    echo "$0: need to specify backend"
    exit 1
  fi

  if [ $2 == "-backend=3addr" ]
  then
   ./dce -opt=dce -backend=3addr
    exit 0
  fi

  if [ $2 == "-backend=c" ]
  then
    ./dce -opt=dce -backend=3addr | python ./convert.py
    exit 0
  fi

  if [ $2 == "-backend=cfg" ]
  then
    ./dce -opt=dce -backend=cfg
    exit 0
  fi
fi

if [ $1 == "-opt=pre" ]
then
  if [ $# -eq 1 ]
  then
    echo "$0: need to specify backend"
    exit 1
  fi

  if [ $2 == "-backend=3addr" ]
  then
   ./dce -opt=pre -backend=3addr
    exit 0
  fi

  if [ $2 == "-backend=c" ]
  then
    ./dce -opt=pre -backend=3addr | python ./convert.py
    exit 0
  fi

  if [ $2 == "-backend=cfg" ]
  then
    ./dce -opt=pre -backend=cfg
    exit 0
  fi
fi
