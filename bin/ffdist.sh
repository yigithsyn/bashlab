#!/bin/bash

# ==============================================================================
# Definition
# ==============================================================================
definition=$(cat << EOF
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
# Common variables
# ==============================================================================
ERRSTR_TRY=$(echo "Try '$(echo $definition | jq -r '.name') --help' for more information")
ERRSTR_MIS="Missing option/argument"
ERRSTR_UNR="Unrecognized option/argument"
ERRSTR_TOO="Too many arguments"
NPARG=$(echo $definition | jq '.pargs | length')

# ==============================================================================
# Help
# ==============================================================================
info="
$(echo $(echo $definition | jq '.name'): $(echo $definition | jq '.desc') | tr -d \")
Usage: ffdist freq D [--help] [--verbose]

  freq                      frequency in Hertz [Hz]
  D                         aperture cross-sectional size in meters [m]
  --help                    display this help and exit
  --verbose                 print processing details \n
"
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
      echo $definition | jq '.'
      exit 0
      ;;
    --help)
      printf "$info"
      exit 0
      ;;
    --verbose)
      verbose=1
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
if [ $# -eq $NPARG ]; then
  while [[ $# -gt 0 ]]; do
    case $1 in
      *)
        echo $1
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
for i in ${POSITIONAL_ARGS[@]}; do echo $i; done
for i in ${POSITIONAL_ARGS[@]}; do 
  if [[ $i =~ ^[+-]?[0-9]+\.?[0-9]*$ ]]; then
    echo "Input is a number."
  else
    echo "Input is a string."
  fi 
done



echo ${POSITIONAL_ARGS[0]}
echo ${POSITIONAL_ARGS[1]}

echo ${POSITIONAL_ARGS[0]} | jq empty >/dev/null 2>&1
echo $?
echo ${POSITIONAL_ARGS[1]} | jq empty >/dev/null 2>&1
echo $?

# if jq . >/dev/null 2>&1 <<<"12"; then
#     echo "Parsed JSON successfully and got something other than false/null"
# else
#     echo "Failed to parse JSON, or got false/null"
# fi

# echo $string | jq -c '.pargs | length'

# # NPARG=$(echo $string | jq '.pargs | length')
# # echo $NPARG

# echo $(( $(echo $string | jq '.pargs | length') - 1))
# echo $(( $# - 1 ))

# if [ $# -eq $(echo $string | jq '.pargs | length') ]
#   then
#     echo "All positional arguments supplied"
#     exit 1
# fi

# if [ $# -eq 0 ]
#   then
#     echo "No arguments supplied"
#     exit 1
# fi

# if [ $# -eq 1 ]
#   then
#     echo $1
#     if [[ $1 -eq "--help" ]]
#       then
#         echo "Usage:"
#         exit 1
#     fi
# fi

# if [ -z "$1" ]
#   then
#     echo "No argument supplied"
# fi