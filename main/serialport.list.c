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
static json_t *ivar, *ws_vars, *ws_hist, *var, *var_val, *var_size, *var_name;
static size_t ivar_index;
static FILE *fin;
static FILE *fout;
static size_t argcount = 0;
static char buff[250];
// mongodb
static mongoc_uri_t *mdb_uri = NULL;
static mongoc_client_t *mdb_cli = NULL;
static mongoc_database_t *mdb_dtb = NULL;
static mongoc_collection_t *mdb_col = NULL;
static bson_error_t bsn_err;
static bson_oid_t oid;

/*============================================================================*/
/* Specifics                                                                  */
/*============================================================================*/
#define PROGNAME "serialport.list"
static const char *program_json =
    "{"
    "\"name\": \"serialport.list\","
    "\"desc\": \"lists avaliable serialports on machine\","
    "\"pargs\": ["
    /*       */ "],"
    "\"oargs\": ["
    /*       */ "],"
    "\"opts\": ["
    /*      */ "]"
    "}";

#include <libserialport.h>

int main(int argc, char *argv[])
{
  int exitcode = EXIT_SUCCESS;
  fout = stdout;
  /* buffer variables */
  json_t *workspace = NULL, *program = NULL;
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
  struct arg_str *ws_out = arg_str0("o", "out", "name", "workspace output variable name");
  struct arg_lit *help = arg_lit0(NULL, "help", "display this help and exit");
  struct arg_lit *verbose = arg_lit0(NULL, "verbose", "print processing details");
  struct arg_end *end = arg_end(20);
  // argtable[argcount++] = database;
  argtable[argcount++] = ws_out;
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
  mdb_uri = mongoc_uri_new_with_error(getenv("BASHLAB_MONGODB_URI_STRING"), &bsn_err);
  if (!mdb_uri)
    goto MAIN;

  mdb_cli = mongoc_client_new_from_uri(mdb_uri);
  if (!mdb_cli)
    goto MAIN;

  mongoc_client_set_appname(mdb_cli, PROGNAME);

  char mdb_dtb_str[50] = "local";
  if (getenv("BASHLAB_MONGODB_DTB_STRING"))
    strcpy(mdb_dtb_str, getenv("BASHLAB_MONGODB_DTB_STRING"));
  mdb_dtb = mongoc_client_get_database(mdb_cli, mdb_dtb_str);

  char mdb_col_str[50] = "workspace";
  if (getenv("BASHLAB_MONGODB_COL_STRING"))
    strcpy(mdb_col_str, getenv("BASHLAB_MONGODB_COL_STRING"));
  mdb_col = mongoc_client_get_collection(mdb_cli, mdb_dtb_str, mdb_col_str);

/* ======================================================================== */
/* main operation                                                           */
/* ======================================================================== */
MAIN:;
  struct arg_str *parg1 = (struct arg_str *)argtable[0]; // frequency
  struct arg_str *parg2 = (struct arg_str *)argtable[1]; // aperture length
  struct arg_lit *opti1 = (struct arg_lit *)argtable[2]; // human

INPUTT:;

OPERATION:;
  struct sp_port **port_list;
  size_t N = 0;
  enum sp_return sp_result = sp_list_ports(&port_list);
  if (sp_result != SP_OK)
  {
    fprintf(stderr, "%s: serialport listing failed.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT_OPERATION;
  }
  while (port_list[N] != NULL)
    ++N;

OUTPUT:;
  char **out = (char **)malloc(N * sizeof(char *));
  for (int i = 0; i < N; i++)
  {
    out[i] = (char *)malloc(256 * sizeof(char));
    strcpy(out[i], sp_get_port_name(port_list[i]));
  }

  if (mdb_cli != NULL)
  {
    bson_t *mdb_qry = BCON_NEW("variables.name", BCON_UTF8("ans3"));
    int64_t mdb_cnt = mongoc_collection_count_documents(mdb_col, mdb_qry, NULL, NULL, NULL, &bsn_err);
    printf("%" PRId64 " documents counted.\n", mdb_cnt);
    bson_destroy(mdb_qry);

    if (mdb_cnt < 0)
    {
      fprintf(stderr, "%s: counting variables failed.\n", PROGNAME);
      exitcode = EXIT_FAILURE;
      goto EXIT_OUTPUT;
    }
    else if (mdb_cnt == 0)
    {
      bson_t *mdb_qry = BCON_NEW("variables", "{", "$exists", BCON_BOOL(true), "}");
      bson_t *mdb_doc = bson_new();
      bson_t mdb_doc_child1, mdb_doc_child2, mdb_doc_child3;
      BSON_APPEND_DOCUMENT_BEGIN(mdb_doc, "$push", &mdb_doc_child1);
      BSON_APPEND_DOCUMENT_BEGIN(&mdb_doc_child1, "variables", &mdb_doc_child2);
      BSON_APPEND_UTF8(&mdb_doc_child2, "name", "ans3");
      BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child2, "value", &mdb_doc_child3);
      for (int i = 0; i < 4; ++i)
      {
        char key[5];
        sprintf(key, "%d", i);
        bson_append_utf8(&mdb_doc_child3, "key", -1, key, -1);
      }
      bson_append_array_end(&mdb_doc_child2, &mdb_doc_child3);
      bson_append_document_end(&mdb_doc_child1, &mdb_doc_child2);
      bson_append_document_end(mdb_doc, &mdb_doc_child1);

      if (!mongoc_collection_update_one(mdb_col, mdb_qry, mdb_doc, NULL, NULL, &bsn_err))
      {
        fprintf(stderr, "%s\n", bsn_err.message);
      }
      bson_destroy(mdb_qry);
      bson_destroy(mdb_doc);
    }
    else
    {
      bson_t *mdb_qry = BCON_NEW("variables.name", BCON_UTF8("ans3"));
      bson_t *mdb_doc = bson_new();
      bson_t mdb_doc_child1, mdb_doc_child2, mdb_doc_child3;
      BSON_APPEND_DOCUMENT_BEGIN(mdb_doc, "$set", &mdb_doc_child1);
      BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child1, "variables.$.val", &mdb_doc_child2);
      for (int i = 0; i < 14; ++i)
      {
        char key[5];
        sprintf(key, "%d", i);
        bson_append_utf8(&mdb_doc_child2, "key", -1, key, -1);
      }
      bson_append_array_end(&mdb_doc_child1, &mdb_doc_child2);
      bson_append_document_end(mdb_doc, &mdb_doc_child1);

      if (!mongoc_collection_update_one(mdb_col, mdb_qry, mdb_doc, NULL, NULL, &bsn_err))
      {
        fprintf(stderr, "%s\n", bsn_err.message);
      }
      bson_destroy(mdb_qry);
      bson_destroy(mdb_doc);
    }

    // mdb_doc = bson_new();
    // bson_t mdb_doc_child1, mdb_doc_child2;
    // BSON_APPEND_DOCUMENT_BEGIN(mdb_doc, "$set", &mdb_doc_child1);
    // BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child1, "variables.$.val", &mdb_doc_child2);

    // // mdb_var = bson_new();
    // // BSON_APPEND_ARRAY_BEGIN(mdb_var, "value", &mdb_var_val);
    // for (int i = 0; i < 5; ++i)
    // {
    //   char key[5];
    //   sprintf(key, "%d", i);
    //   // bson_append_utf8(&mdb_var_val, "key", -1, key, -1);
    //   bson_append_utf8(&mdb_doc_child2, "key", -1, key, -1);
    // }
    // // bson_append_array_end(mdb_var, &mdb_var_val);
    // bson_append_array_end(&mdb_doc_child1, &mdb_doc_child2);
    // bson_append_array_end(mdb_doc, &mdb_doc_child1);

    // // char *str = bson_as_relaxed_extended_json(mdb_var, NULL);
    // char *str = bson_as_relaxed_extended_json(mdb_doc, NULL);
    // printf("%s\n", str);
    // bson_free(str);

    //  bson_t *query = BCON_NEW("variables.name",
    //                          BCON_UTF8("ans2"));

    // if (!mongoc_collection_update_one(mdb_col, query, mdb_doc, NULL, NULL, &bsn_err))
    // {
    //   fprintf(stderr, "%s\n", bsn_err.message);
    // }

    // if (!mongoc_collection_insert_one(mdb_col, mdb_var, NULL, NULL, &bsn_err))
    // {
    //   fprintf(stderr, "%s\n", bsn_err.message);
    // }

    // bson_t *query = BCON_NEW("name",
    //                          "{",
    //                          "$exists",
    //                          BCON_BOOL(true),
    //                          "}");
    // if (!mongoc_collection_replace_one(mdb_col, query, mdb_var, NULL, NULL, &bsn_err))
    // {
    //   fprintf(stderr, "%s\n", bsn_err.message);
    // }

    // bson_t *query = BCON_NEW("variables.name",
    //                          BCON_UTF8("ans")
    //                          );
    // const bson_t *doc;
    // mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(mdb_col, query, NULL, NULL);

    // str = bson_as_canonical_extended_json(query, NULL);
    // printf("%s\n", str);
    // bson_free(str);
    // while (mongoc_cursor_next(cursor, &doc))
    // {
    //   str = bson_as_canonical_extended_json(doc, NULL);
    //   printf("%s\n", str);
    //   bson_free(str);
    // }

    // bson_t *query = BCON_NEW("variables.name",
    //                          BCON_UTF8("ans"));
    // const bson_t *doc;
    // mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(mdb_col, query, NULL, NULL);

    // str = bson_as_canonical_extended_json(query, NULL);
    // printf("%s\n", str);
    // bson_free(str);
    // while (mongoc_cursor_next(cursor, &doc))
    // {
    //   str = bson_as_canonical_extended_json(doc, NULL);
    //   printf("%s\n", str);
    //   bson_free(str);
    // }

    // bson_t *query = BCON_NEW("variables.name",
    //                            BCON_UTF8("ans2"));
    //   bson_t *update = BCON_NEW("$set",
    //                             "{",
    //                             "variables.$.val",
    //                             BCON_UTF8("new_value"),
    //                             "}");

    //   if (!mongoc_collection_update_one(mdb_col, query, update, NULL, NULL, &bsn_err))
    //   {
    //     fprintf(stderr, "%s\n", bsn_err.message);
    //   }
  }

// HISTORY:
//   strcpy(buff, PROGNAME);
//   for (size_t i = 1; i < argc; i++)
//   {
//     strcat(buff, " ");
//     strcat(buff, argv[i]);
//   }
//   json_array_append_new(ws_hist, json_string(buff));
//   json_dump_file(workspace, BLAB_WS, JSON_COMPACT);

// STDOUT:
//   /* stream */
//   if (opti1->count > 0)
//   {
//     fprintf(fout, "%s", human(ans[0], buff));
//     for (size_t i = 1; i < MIN(Nans, 3); ++i)
//       fprintf(fout, ", %s", human(ans[i], buff));
//     if (Nans > 5)
//       fprintf(fout, ", ...");
//     for (size_t i = MAX(MIN(Nans, 3), Nans - 2); i < Nans; ++i)
//       fprintf(fout, ", %s", human(ans[i], buff));
//     fprintf(fout, "\n");
//   }
//   else
//   {
//     fprintf(fout, "%.16G", ans[0]);
//     for (size_t i = 1; i < MIN(Nans, 3); ++i)
//       fprintf(fout, ", %.16G", ans[i]);
//     if (Nans > 5)
//       fprintf(fout, ", ...");
//     for (size_t i = MAX(MIN(Nans, 3), Nans - 2); i < Nans; ++i)
//       fprintf(fout, ", %.16G", ans[i]);
//     fprintf(fout, "\n");
//   }

/* ======================================================================== */
/* exit                                                                     */
/* ======================================================================== */
EXIT_OUTPUT:;
  for (int i = N; i < N; i++)
    free(out[i]);
  free(out);
  // bson_destroy(mdb_var);

EXIT_OPERATION:;
  sp_free_port_list(port_list);

EXIT_INPUT:;
  // free(inp1);

EXIT:
  // mongoc cleanup
  mongoc_collection_destroy(mdb_col);
  mongoc_database_destroy(mdb_dtb);
  mongoc_client_destroy(mdb_cli);
  mongoc_uri_destroy(mdb_uri);
  mongoc_cleanup();

  // jansson cleanup
  if (program != NULL)
    json_decref(program);

  // argtable cleanup
  arg_freetable(argtable, argcount + 1); // +1 for end
  return exitcode;
}

char *human(number_t arg, char *buff)
{
  if (arg >= 1E3)
    sprintf(buff, "%.1f km", arg / 1E3);
  else if (arg >= 1)
    sprintf(buff, "%.1f m", arg / 1E0);
  else if (arg >= 1E-2)
    sprintf(buff, "%.1f cm", arg / 1E-2);
  else
    sprintf(buff, "%.1f mm", arg / 1E-3);
  return buff;
}
