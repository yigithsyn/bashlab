import json
import os
import sys
import argparse
from pymongo import MongoClient
from pymongo import uri_parser
from pymongo.errors import ConnectionFailure as MongoDBConnectionFailure
import numpy as np

PROGNAME = "ams2nsi"
program_json = """
{
    \"name\": \"ams2nsi\",
    \"desc\": \"Convert ASYSOL AMS CSV data file to NSI importable formats\",
    \"pargs\": [
      {\"name\":\"file\", \"minc\":1, \"maxc\":1, \"desc\":\"AMS csv data file\"}
    ],
    \"oargs\": [
    ],
    \"opts\": [
    ]
}"""

mdb_cli = mdb_dtb = mdb_col = None
mdb_dtb_str = "bashlab"
mdb_col_str = "workspace"
mdb_var_str = "ans"

# ==============================================================================
# fetch program definitions
# ==============================================================================
program = json.loads(program_json)
# print(program)

# ==============================================================================
# register program
# ==============================================================================
if os.getenv("BASHLAB_MONGODB_URI"):
    if uri_parser.parse_uri(os.getenv("BASHLAB_MONGODB_URI")):
        mdb_cli = MongoClient(os.getenv("BASHLAB_MONGODB_URI"), appname=PROGNAME, socketTimeoutMS=0, connectTimeoutMS=0, serverSelectionTimeoutMS=100)
        try:
            mdb_cli.admin.command("ping")
        except MongoDBConnectionFailure:
            mdb_cli = None
        else:
            mdb_col = mdb_cli[mdb_dtb_str]["programs"]
            mdb_col.update_one({"name": PROGNAME}, {"$set": program}, upsert=True)
mdb_col = None


# ==============================================================================
# argument parsing
# ==============================================================================
parser = argparse.ArgumentParser(prog=PROGNAME, description=program["desc"])
for arg in program["pargs"]:
    parser.add_argument(arg["name"], metavar=arg["name"], help=arg["desc"], nargs=arg["minc"])
for arg in program["oargs"]:
    parser.add_argument("--" + arg["long"], metavar=arg["name"], help=arg["desc"], nargs=arg["minc"])
for arg in program["opts"]:
    parser.add_argument("--" + arg["long"], metavar=arg["long"], help=arg["desc"])

parser.add_argument("--verbose", help="print processing details", action="store_true")
args = parser.parse_args()
# print(args.verbose)

# ==============================================================================
# workspace
# ==============================================================================
if mdb_cli != None:
    if os.getenv("BASHLAB_MONGODB_DATABASE"):
        mdb_dtb_str = os.env("BASHLAB_MONGODB_DATABASE")
    mdb_dtb = mdb_cli[mdb_dtb_str]

    if os.getenv("BASHLAB_MONGODB_COLLECTION"):
        mdb_col_str = os.env("BASHLAB_MONGODB_DATABASE")
    mdb_col = mdb_dtb[mdb_col_str]

# ==============================================================================
# main operation
# ==============================================================================

# ------------------------------------------------------------------------------
# INPUT
# ------------------------------------------------------------------------------
arg_file = args.file[0]

# ------------------------------------------------------------------------------
# OPERATION
# ------------------------------------------------------------------------------
pol, step, scan, freq, logmag, phase = np.loadtxt(arg_file, skiprows=1, delimiter=", ", unpack=True)
if args.verbose:
    print("[VERB] File read succesfully.")

Nfreq = 1
for i in range(1, len(freq)):
    if freq[0] == freq[i]:
        break
    else:
        Nfreq += 1
if args.verbose:
    print("[VERB] Frequency count:", Nfreq)

# ------------------------------------------------------------------------------
# OUTPUT
# ------------------------------------------------------------------------------
freqs = []
for i in range(Nfreq):
    freqs.append(str(freq[i] / 1e9))

if args.verbose:
    print("[VERB] Preparing per frequency near field datas for NSI2000 import:")
files = []
data_file_header = "pol\t\tstep\t\tscan\t\treal\t\timag"
for i in range(Nfreq):
    if args.verbose:
        print("\t%d MHz ... " % (int(freq[i] / 1e6)), end="")
    data = np.zeros((int(len(freq) / Nfreq), 5))
    for j in range(int(len(freq) / Nfreq)):
        polj = pol[j * Nfreq + i]
        stepj = step[j * Nfreq + i]
        scanj = scan[j * Nfreq + i]
        freqj = freq[j * Nfreq + i]
        logmagj = logmag[j * Nfreq + i]
        phasej = phase[j * Nfreq + i]
        realj = np.power(10, logmagj / 20) * np.cos(phasej / 180 * np.pi)
        imagj = np.power(10, logmagj / 20) * np.sin(phasej / 180 * np.pi)
        data[j] = [polj, stepj, scanj, realj, imagj]
    files.append("NF_" + str(int(freqj / 1e6)) + ".txt")
    np.savetxt(files[-1], data, fmt="%.6f", delimiter="\t\t", header="pol\t\tstep\t\tscan\t\treal\t\timag")
    if args.verbose:
        print("Done.")

# ------------------------------------------------------------------------------
# STDOUT
# ------------------------------------------------------------------------------
# print("Frequencies: ", end="")
for i in range(len(freqs) - 1):
    print(freqs[i] + ", ", end="")
print(freqs[-1])
# print("Files: ")
for file in files:
    print(file)

# ------------------------------------------------------------------------------
# WORKSPACE
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# HISTORY
# ------------------------------------------------------------------------------
if mdb_col != None:
    command = PROGNAME
    for i in range(len(sys.argv) - 1):
        command += " " + sys.argv[i + 1]
    mdb_col.update_one({"history": {"$exists": True}}, {"$push": {"history": command}})
