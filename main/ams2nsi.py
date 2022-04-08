import json
import os
import sys
import argparse
from pymongo import MongoClient
from pymongo import uri_parser
from pymongo.errors import ConnectionFailure as MongoDBConnectionFailure

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


mdb_dtb_str = "bashlab"
mdb_col_str = "workspace"
mdb_var_str = "ans"

# ==============================================================================
# fetch program definitions
# ==============================================================================
program = json.loads(program_json)
print(program)

# ==============================================================================
# register program
# ==============================================================================
if os.getenv("BASHLAB_MONGODB_URI_STRING"):
    if uri_parser.parse_uri(os.getenv("BASHLAB_MONGODB_URI_STRING")):
        mdb_cli = MongoClient(os.getenv("BASHLAB_MONGODB_URI_STRING"), socketTimeoutMS=0, connectTimeoutMS=0, serverSelectionTimeoutMS=100)
        try:
            mdb_cli.admin.command("ping")
        except MongoDBConnectionFailure:
            pass
        else:
            mdb_col = mdb_cli[mdb_dtb_str]["programs"]
            mdb_col.update_one({"name": PROGNAME}, {"$set": program}, upsert=True)

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

parser.add_argument("--verbose", metavar="", help="print processing details")
args = parser.parse_args()

# ==============================================================================
# workspace
# ==============================================================================

print(args.file)
