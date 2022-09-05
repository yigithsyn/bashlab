#!/bin/bash


if [ $# -eq 0 ]
  then
    echo "No arguments supplied"
    exit 1
fi

if [ $# -eq 1 ]
  then
    echo $1
    if [[ $1 -eq "--help" ]]
      then
        echo "Usage:"
        exit 1
    fi
fi

# if [ -z "$1" ]
#   then
#     echo "No argument supplied"
# fi