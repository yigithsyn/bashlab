/*============================================================================*/
/* Commons                                                                    */
/*============================================================================*/
#include "typedefs.h"
#include "configs.h"
#include "macros.h"
#include "utility.h"
#include "constants.h"

#include "argtable3.h"
#include "jansson.h"
#include "mongoc/mongoc.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/stat.h>
#if defined(_WIN32)
#else
#include <unistd.h>
#endif

static struct stat stat_buff;
static json_error_t *json_error;
static json_t *ivar;
static size_t ivar_index;
static size_t argcount = 0;
static char buff[250];

// mongodb
static mongoc_uri_t *mdb_uri = NULL;
static mongoc_client_t *mdb_cli = NULL;
static mongoc_database_t *mdb_dtb = NULL;
static mongoc_collection_t *mdb_col = NULL;
static bson_error_t mdb_err;
static bson_oid_t mdb_oid;
static char mdb_dtb_str[50] = "local";
static char mdb_col_str[50] = "workspace";
static char mdb_var_str[50] = "ans";
bson_t *mdb_qry, *mdb_qry1, *mdb_doc, *mdb_doc1, *mdb_pip;
mongoc_cursor_t *mdb_crs, *mdb_crs1;
/*============================================================================*/
/* Specifics                                                                  */
/*============================================================================*/
#define PROGNAME "workspace.set"
static const char *program_json =
    "{"
    "\"name\": \"workspace.set\","
    "\"desc\": \"sets worskpace variable\","
    "\"pargs\": ["
    /*        */ "{\"name\":\"val(s)\", \"minc\":1, \"maxc\":100, \"desc\":\"value to be set\"}"
    /*       */ "],"
    "\"oargs\": ["
    /*        */ ""
    /*       */ "],"
    "\"opts\": ["
    /*      */ "]"
    "}";

int main(int argc, char *argv[])
{
  int exitcode = EXIT_SUCCESS;
  /* buffer variables */
  json_t *program = NULL;
  void *argtable[100];

  /* ======================================================================== */
  /* fetch program definitions                                                */
  /* ======================================================================== */
  program = json_loads(program_json, 0, json_error);

  /* ======================================================================== */
  /* argument parse                                                           */
  /* ======================================================================== */
  /* copy positional args */
  for (size_t i = 1; i < argc; i++)
    if (argv[i][0] == 45 && isnumber(argv[i]))
      argv[i][0] = 126; // '-' to '~' avoiding argtable literal behaviour

  /* positional arg structs*/
  json_t *pargs = json_object_get(program, "pargs");
  json_array_foreach(pargs, ivar_index, ivar)
  {
    const_string_t name = json_string_value(json_object_get(ivar, "name"));
    int_t minc = json_integer_value(json_object_get(ivar, "minc"));
    int_t maxc = json_integer_value(json_object_get(ivar, "maxc"));
    const_string_t desc = json_string_value(json_object_get(ivar, "desc"));
    argtable[argcount++] = arg_strn(NULL, NULL, name, minc, maxc, desc);
  }

  /* optional arg structs*/
  json_t *oargs = json_object_get(program, "oargs");
  json_array_foreach(oargs, ivar_index, ivar)
  {
    const_string_t sh = json_string_value(json_object_get(ivar, "short"));
    const_string_t ln = json_string_value(json_object_get(ivar, "long"));
    const_string_t name = json_string_value(json_object_get(ivar, "name"));
    int_t minc = json_integer_value(json_object_get(ivar, "minc"));
    int_t maxc = json_integer_value(json_object_get(ivar, "maxc"));
    const_string_t desc = json_string_value(json_object_get(ivar, "desc"));
    argtable[argcount++] = arg_strn(sh, ln, name, minc, maxc, desc);
  }

  /* options structs*/
  json_t *opts = json_object_get(program, "opts");
  json_array_foreach(opts, ivar_index, ivar)
  {
    const_string_t sh = json_string_value(json_object_get(ivar, "short"));
    const_string_t ln = json_string_value(json_object_get(ivar, "long"));
    const_string_t desc = json_string_value(json_object_get(ivar, "desc"));
    argtable[argcount++] = arg_lit0(sh, ln, desc);
  }

  /* commong arg structs */
  struct arg_lit *help = arg_lit0(NULL, "help", "display this help and exit");
  struct arg_lit *verbose = arg_lit0(NULL, "verbose", "print processing details");
  struct arg_end *end = arg_end(20);
  argtable[argcount++] = help;
  argtable[argcount++] = verbose;
  argtable[argcount] = end;

  int arg_errors = arg_parse(argc, argv, argtable);

  /* special case: '--help' takes precedence over error reporting */
  if (help->count > 0)
  {
    printf("%s: %s.\n\n", PROGNAME, json_string_value(json_object_get(program, "desc")));
    printf("Usage: %s", PROGNAME);
    arg_print_syntaxv(stdout, argtable, "\n\n");
    arg_print_glossary(stdout, argtable, "  %-25s %s\n");
    goto EXIT;
  }

  /* If the parser returned any errors then display them and exit */
  if (arg_errors > 0)
  {
    /* Display the error details contained in the arg_end struct.*/
    arg_print_errors(stdout, end, PROGNAME);
    printf("Try '%s --help' for more information.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }

  /* minus number_t back in positional args */
  for (size_t i = 1; i < argc; i++)
    if (argv[i][0] == 126)
      argv[i][0] = 45; // '~' to '-'

  /* ======================================================================== */
  /* workspace                                                                */
  /* ======================================================================== */
  if (!getenv("BASHLAB_MONGODB_URI_STRING"))
    goto MAIN;

  mongoc_init();
  mdb_uri = mongoc_uri_new_with_error(getenv("BASHLAB_MONGODB_URI_STRING"), &mdb_err);
  if (!mdb_uri)
    goto MAIN;

  mdb_cli = mongoc_client_new_from_uri(mdb_uri);
  if (!mdb_cli)
    goto MAIN;

  mongoc_client_set_appname(mdb_cli, PROGNAME);

  if (getenv("BASHLAB_MONGODB_DTB_STRING"))
    strcpy(mdb_dtb_str, getenv("BASHLAB_MONGODB_DTB_STRING"));
  mdb_dtb = mongoc_client_get_database(mdb_cli, mdb_dtb_str);

  if (getenv("BASHLAB_MONGODB_COL_STRING"))
    strcpy(mdb_col_str, getenv("BASHLAB_MONGODB_COL_STRING"));
  mdb_col = mongoc_client_get_collection(mdb_cli, mdb_dtb_str, mdb_col_str);

/* ======================================================================== */
/* main operation                                                           */
/* ======================================================================== */
MAIN:;
  if (mdb_col == NULL)
  {
    fprintf(stderr, "%s: function requires workspace database.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }
  struct arg_str *arg_val = (struct arg_str *)argtable[0];

INPUTT:;
  double var_valsd[100];
  char var_valss[100][1000];
  size_t var_lngth = arg_val->count;
  bson_type_t var_type = isnumber(arg_val->sval[0]) ? BSON_TYPE_DOUBLE : BSON_TYPE_UTF8;
  for (size_t i = 0; i < arg_val->count; ++i)
  {
    if (var_type == BSON_TYPE_DOUBLE)
      var_valsd[i] = atof(arg_val->sval[i]);
    else
      strcpy(var_valss[i], arg_val->sval[i]);
  }

OPERATION:;
  if (getenv("BL_WORKSPACE_ANS"))
    strcpy(mdb_var_str, getenv("BL_WORKSPACE_ANS"));

  mdb_qry = BCON_NEW("variables.name", BCON_UTF8(mdb_var_str));
  int64_t mdb_cnt = mongoc_collection_count_documents(mdb_col, mdb_qry, NULL, NULL, NULL, &mdb_err);
  bson_destroy(mdb_qry);

  if (mdb_cnt < 0)
  {
    fprintf(stderr, "%s: counting variables failed.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT_WORKSPACE;
  }
  else if (mdb_cnt == 0)
  {
    mdb_qry = BCON_NEW("variables", "{", "$exists", BCON_BOOL(true), "}");
    mdb_doc = bson_new();
    bson_t mdb_doc_child1, mdb_doc_child2, mdb_doc_child3;
    BSON_APPEND_DOCUMENT_BEGIN(mdb_doc, "$push", &mdb_doc_child1);
    BSON_APPEND_DOCUMENT_BEGIN(&mdb_doc_child1, "variables", &mdb_doc_child2);
    BSON_APPEND_UTF8(&mdb_doc_child2, "name", mdb_var_str);
    BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child2, "value", &mdb_doc_child3);
    for (size_t i = 0; i < var_lngth; ++i)
      if (var_type == BSON_TYPE_DOUBLE)
        bson_append_double(&mdb_doc_child3, "no", -1, var_valsd[i]);
      else
        bson_append_utf8(&mdb_doc_child3, "no", -1, var_valss[i], -1);
    bson_append_array_end(&mdb_doc_child2, &mdb_doc_child3);
    BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child2, "size", &mdb_doc_child3);
    bson_append_double(&mdb_doc_child3, "no", -1, (double)var_lngth);
    bson_append_array_end(&mdb_doc_child2, &mdb_doc_child3);
    bson_append_document_end(&mdb_doc_child1, &mdb_doc_child2);
    bson_append_document_end(mdb_doc, &mdb_doc_child1);

    if (!mongoc_collection_update_one(mdb_col, mdb_qry, mdb_doc, NULL, NULL, &mdb_err))
    {
      fprintf(stderr, "%s: variable insertation failed.\n", PROGNAME);
      exitcode = EXIT_FAILURE;
      goto EXIT_WORKSPACE;
    }
    bson_destroy(mdb_qry);
    bson_destroy(mdb_doc);
  }
  else
  {
    mdb_qry = BCON_NEW("variables.name", BCON_UTF8(mdb_var_str));
    mdb_doc = bson_new();
    bson_t mdb_doc_child1, mdb_doc_child2, mdb_doc_child3;
    BSON_APPEND_DOCUMENT_BEGIN(mdb_doc, "$set", &mdb_doc_child1);
    BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child1, "variables.$.value", &mdb_doc_child2);
    for (size_t i = 0; i < var_lngth; ++i)
      if (var_type == BSON_TYPE_DOUBLE)
        bson_append_double(&mdb_doc_child2, "no", -1, var_valsd[i]);
      else
        bson_append_utf8(&mdb_doc_child2, "no", -1, var_valss[i], -1);
    bson_append_array_end(&mdb_doc_child1, &mdb_doc_child2);
    BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child1, "variables.$.size", &mdb_doc_child2);
    bson_append_double(&mdb_doc_child2, "no", -1, (double)var_lngth);
    bson_append_array_end(&mdb_doc_child1, &mdb_doc_child2);
    bson_append_document_end(mdb_doc, &mdb_doc_child1);

    if (!mongoc_collection_update_one(mdb_col, mdb_qry, mdb_doc, NULL, NULL, &mdb_err))
    {
      fprintf(stderr, "%s: variable update failed.\n", PROGNAME);
      exitcode = EXIT_FAILURE;
      goto EXIT_WORKSPACE;
    }
    bson_destroy(mdb_qry);
    bson_destroy(mdb_doc);
  }

OUTPUT:;

STDOUT:;

  fprintf(stdout, "%-10s: ", mdb_var_str);
  fprintf(stdout, "%s[%zu]: ", (var_type == BSON_TYPE_DOUBLE) ? "number" : "string", var_lngth);
  if (var_type == BSON_TYPE_DOUBLE)
    fprintf(stdout, "%.16G", var_valsd[0]);
  else
    fprintf(stdout, "%.16s", var_valss[0]);
  for (size_t i = 1; i < MIN(var_lngth, 3); ++i)
    if (var_type == BSON_TYPE_DOUBLE)
      fprintf(stdout, ", %.16G", var_valsd[i]);
    else
      fprintf(stdout, ", %.16s", var_valss[i]);
  if (var_lngth > 5)
    fprintf(stdout, ", ...");
  for (size_t i = MAX(MIN(var_lngth, 3), var_lngth - 2); i < var_lngth; ++i)
    if (var_type == BSON_TYPE_DOUBLE)
      fprintf(stdout, ", %.16G", var_valsd[i]);
    else
      fprintf(stdout, ", %.16s", var_valss[i]);
  fprintf(stdout, "\n");

WORKSPACE:;

HISTORY:
  strcpy(buff, PROGNAME);
  for (size_t i = 1; i < argc; i++)
  {
    strcat(buff, " ");
    strcat(buff, argv[i]);
  }
  mdb_qry = BCON_NEW("history", "{", "$exists", BCON_BOOL(true), "}");
  mdb_doc = BCON_NEW("$push", "{", "history", BCON_UTF8(buff), "}");

  if (!mongoc_collection_update_one(mdb_col, mdb_qry, mdb_doc, NULL, NULL, &mdb_err))
  {
    fprintf(stderr, "%s: history update failed.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT_HISTORY;
  }
  bson_destroy(mdb_doc);
  bson_destroy(mdb_qry);

/* ======================================================================== */
/* exit                                                                     */
/* ======================================================================== */
EXIT_HISTORY:;

EXIT_WORKSPACE:;

EXIT_STDOUT:;

EXIT_OUTPUT:;

EXIT_OPERATION:;

EXIT_INPUT:;

EXIT:;
  // mongoc cleanup
  mongoc_collection_destroy(mdb_col);
  mongoc_database_destroy(mdb_dtb);
  mongoc_client_destroy(mdb_cli);
  mongoc_uri_destroy(mdb_uri);
  mongoc_cleanup();

  // jansson cleanup
  json_decref(program);

  // argtable cleanup
  arg_freetable(argtable, argcount + 1); // +1 for end
  return exitcode;
}