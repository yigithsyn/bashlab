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
      "desc": "frequency in Hertz [Hz]"
    },
    {
      "name": "D",
      "desc": "aperture cross-sectional size in meters [m]"
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
ERRSTR_TOO="Too many arguments"

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
while [[ $# -eq 1 ]]; do
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
      shift
      ;;
      *)
      echo "$ERRSTR_UNR '$1'" | tr -d \"
      echo $ERRSTR_TRY | tr -d \"
      exit 1
  esac
done

# ------------------------------------------------------------------------------
# Arguments: Positional
# -----------------------------------------------------------------------------
POSITIONAL_ARGS=()
if [ $# -eq $(echo $DEFINITION | jq '.pargs | length') ]; then
  while [[ $# -gt 0 ]]; do
    case $1 in
      *)
        # echo $1
        POSITIONAL_ARGS+=("$1") # save positional arg
        shift # past argument
        ;;
    esac
  done
else
  echo $ERRSTR_TOO | tr -d \"
  echo $ERRSTR_TRY | tr -d \"
  exit 1
fi

# set -- "${POSITIONAL_ARGS[@]}" # restore positional parameters
# for i in ${POSITIONAL_ARGS[@]}; do echo $i; done
for i in ${POSITIONAL_ARGS[@]}; do 
  if [[ $i =~ ^[+-]?[0-9]+\.?[0-9]*$ ]]; then
    # echo "Input is a number."
    continue
  else
    # echo "Input is a string."
    if [ -z "$BASHLAB_DB" ]; then
      echo $ERRSTR_MIS | tr -d \"
      echo $ERRSTR_TRY | tr -d \"
      exit 1
    else
      echo $BASHLAB_DB
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
