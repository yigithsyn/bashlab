#!/bin/bash

# ==============================================================================
# Definition
# ==============================================================================
DEFINITION=$(cat << EOF
{
  "name": "ffdist",
  "desc": "Calculates far-field (Fraunhofer) distance of an aperture",
  "pargs": [
    {
      "name": "freq",
      "desc": "frequency in Hertz [Hz]",
      "minc": 1,
      "maxc": 1
    },
    {
      "name": "D",
      "desc": "aperture cross-sectional size in meters [m]",
      "minc": 1,
      "maxc": 1
    }
  ],
  "oargs": [],
  "opts": []
}
EOF
)

# ==============================================================================
# Help
# ==============================================================================
INFO="
$(echo $(echo $DEFINITION | jq '.name'): $(echo $DEFINITION | jq '.desc') | tr -d \")
Usage: ffdist freq D [--help] [--verbose]

  freq                      frequency in Hertz [Hz]
  D                         aperture cross-sectional size in meters [m]
  --help                    display this help and exit
  --verbose                 print processing details \n
"

# ==============================================================================
# Common variables
# ==============================================================================
ERRSTR_TRY=$(echo "Try '$(echo $DEFINITION | jq -r '.name') --help' for more information")
ERRSTR_MIS="Missing option/argument"
ERRSTR_UNR="Unrecognized option/argument"
ERRSTR_NDB="BashLab database not defined."
ERRSTR_VAR="BashLab database variable not found."
ERRSTR_TOO="Too many arguments"

NPOSARGS=$#

# ==============================================================================
# Arguments
# ==============================================================================
# ------------------------------------------------------------------------------
# Arguments: Missing
# ------------------------------------------------------------------------------
if [ -z "$1" ]
  then
    echo $ERRSTR_MIS | tr -d \"
    echo $ERRSTR_TRY | tr -d \"
    exit 1
fi

# ------------------------------------------------------------------------------
# Arguments: Optional
# ------------------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
  case $1 in
    --json)
      echo $DEFINITION | jq '.'
      exit 0
      ;;
    --help)
      printf "$INFO"
      exit 0
      ;;
    --verbose)
      VERBOSE=1
      let NPOSARGS--
      shift
      ;;
    -*|--*)
      echo $ERRSTR_UNR "'$1'" | tr -d \"
      echo $ERRSTR_TRY | tr -d \"
      exit 1
      ;;
    *)
      POSITIONAL_ARGS+=("$1") # save positional arg
      shift # past argument
      ;;
  esac
done

# echo $NPOSARGS
# echo $DEFINITION | jq '[.pargs[].maxc] | add'
# exit 0

# ------------------------------------------------------------------------------
# Arguments: Positional
# ------------------------------------------------------------------------------
if [ $NPOSARGS -gt $(echo $DEFINITION | jq '[.pargs[].maxc] | add') ]; then
  echo $ERRSTR_TOO | tr -d \"
  echo $ERRSTR_TRY | tr -d \"
  exit 1
fi

for (( i=1; i<=$(echo $DEFINITION | jq '.pargs | length'); i++ )); do
   echo "Welcome $i times"
done
exit 0
# set -- "${POSITIONAL_ARGS[@]}" # restore positional parameters
# for i in ${POSITIONAL_ARGS[@]}; do echo $i; done
for i in ${POSITIONAL_ARGS[@]}; do 
  if [[ $i =~ ^[+-]?[0-9]+\.?[0-9]*$ ]]; then
    echo "${i}: Input is a number."
    continue
  else
    echo "${i}: Input is a string."
    if [ -z "$BASHLAB_DB" ]; then
      echo $ERRSTR_NDB | tr -d \"
      echo $ERRSTR_TRY | tr -d \"
      exit 1
    else
      mongo $BASHLAB_DB --eval "db.workspace.count({'name':'${i}'})" --quiet
      if [ -z "$BASHLAB_VAR" ]; then
        echo $ERRSTR_MIS | tr -d \"
        echo $ERRSTR_TRY | tr -d \"
        exit 1
      else
        echo $BASHLAB_VAR
      fi
    fi
  fi
done



# echo ${POSITIONAL_ARGS[0]}
# echo ${POSITIONAL_ARGS[1]}

# echo ${POSITIONAL_ARGS[0]} | jq empty >/dev/null 2>&1
# echo $?
# echo ${POSITIONAL_ARGS[1]} | jq empty >/dev/null 2>&1
# echo $?
