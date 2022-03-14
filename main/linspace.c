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
bson_iter_t iter, iter1, iter2, iter3;
/*============================================================================*/
/* Specifics                                                                  */
/*============================================================================*/
#define PROGNAME "linspace"
static const char *program_json =
    "{"
    "\"name\": \"linspace\","
    "\"desc\": \"generate linearly spaced array\","
    "\"pargs\": ["
    /*        */ "{\"name\":\"a\", \"minc\":1, \"maxc\":1, \"desc\":\"starting value of the sequence\"},"
    /*        */ "{\"name\":\"b\", \"minc\":1, \"maxc\":1, \"desc\":\"end value of the seqeunce (included)\"},"
    /*        */ "{\"name\":\"N\", \"minc\":1, \"maxc\":1, \"desc\":\"number of points\"}"
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
  struct arg_str *arg_a = (struct arg_str *)argtable[0];
  struct arg_str *arg_b = (struct arg_str *)argtable[1];
  struct arg_str *arg_N = (struct arg_str *)argtable[2];

INPUTT:;
  char *var_name;
  bool *var_found;
  size_t *var_dim;
  size_t *var_limitN;
  size_t *var_N;
  number_t *var_val;
  struct arg_str *var_arg;
  bson_type_t *var_type;

  // a
  char *a_name = (char *)arg_a->hdr.datatype;
  bool a_found = false;
  size_t a_dim = 1;
  size_t a_limitN[BL_WORKSPACE_MAX_DIM] = {1, 0, 0};
  size_t a_N[BL_WORKSPACE_MAX_DIM];
  bson_type_t a_type = BSON_TYPE_DOUBLE;
  number_t *a_val = NULL;

  var_arg = arg_a;
  var_name = a_name;
  var_found = &a_found;
  var_dim = &a_dim;
  var_limitN = a_limitN;
  var_N = a_N;
  var_type = &a_type;
  var_val = a_val;

  if (isnumber(var_arg->sval[0]))
  {
    var_N[0]++;
    var_val = (number_t *)calloc(var_N[0], sizeof(number_t));
    var_val[0] = atof(var_arg->sval[0]);
  }
  else if (mdb_col != NULL)
  {
    // name check
    mdb_qry = BCON_NEW("variables", "{", "$exists", BCON_BOOL(true), "}");
    mdb_qry1 = BCON_NEW("projection", "{", "variables.name", BCON_BOOL(true), "}");
    mdb_crs = mongoc_collection_find_with_opts(mdb_col, mdb_qry, mdb_qry1, NULL);
    if (mongoc_cursor_error(mdb_crs, &mdb_err))
    {
      fprintf(stderr, "%s: error in listing workspace: %s.\n", PROGNAME, mdb_err.message);
      exitcode = EXIT_FAILURE;
      goto EXIT_INPUT;
    }
    if (mongoc_cursor_next(mdb_crs, (const bson_t **)&mdb_doc) &&
        bson_iter_init_find(&iter, mdb_doc, "variables") &&
        BSON_ITER_HOLDS_ARRAY(&iter) &&
        bson_iter_recurse(&iter, &iter1))
    {
      while (bson_iter_next(&iter1)) // iter through variables docs, while actually it is single doc
      {
        bson_iter_recurse(&iter1, &iter2); // step into variables array
        bson_iter_next(&iter2);            // iter through variables array
        if (strcmp(bson_iter_value(&iter2)->value.v_utf8.str, var_name) == 0)
        {
          *var_found = true;
          break;
        }
      }
    }
    bson_destroy(mdb_doc);
    mongoc_cursor_destroy(mdb_crs);
    bson_destroy(mdb_qry);
    bson_destroy(mdb_qry1);

    // dim check
    mdb_qry = BCON_NEW("pipeline", "[",
                       "{", "$match", "{", "variables.name", BCON_UTF8(var_name), "}", "}",
                       "{", "$unwind", BCON_UTF8("$variables"), "}",
                       "{", "$match", "{", "variables.name", BCON_UTF8(var_name), "}", "}",
                       "{", "$unwind", BCON_UTF8("$variables.size"), "}",
                       "{", "$project", "{", "variables.size", BCON_BOOL(true), "}", "}",
                       "]");

    mdb_crs = mongoc_collection_aggregate(mdb_col, MONGOC_QUERY_NONE, mdb_qry, NULL, NULL);
    if (mongoc_cursor_error(mdb_crs, &mdb_err))
    {
      fprintf(stderr, "%s: error in listing workspace: %s.\n", PROGNAME, mdb_err.message);
      exitcode = EXIT_FAILURE;
      goto EXIT_INPUT;
    }
    size_t dim = 0;
    while (mongoc_cursor_next(mdb_crs, (const bson_t **)&mdb_doc))
      dim++;
    if (dim != *var_dim)
    {
      fprintf(stderr, "%s: variable \"%s\" dimension should be '%zu' instead of '%zu'.\n", PROGNAME, var_name, *var_dim, dim);
      exitcode = EXIT_FAILURE;
      goto EXIT_INPUT;
    }

    bson_destroy(mdb_doc);
    mongoc_cursor_destroy(mdb_crs);
    bson_destroy(mdb_qry);

    // size check
    mdb_qry = BCON_NEW("pipeline", "[",
                       "{", "$match", "{", "variables.name", BCON_UTF8(var_name), "}", "}",
                       "{", "$unwind", BCON_UTF8("$variables"), "}",
                       "{", "$match", "{", "variables.name", BCON_UTF8(var_name), "}", "}",
                       "{", "$unwind", BCON_UTF8("$variables.size"), "}",
                       "{", "$project", "{", "variables.size", BCON_BOOL(true), "}", "}",
                       "]");

    mdb_crs = mongoc_collection_aggregate(mdb_col, MONGOC_QUERY_NONE, mdb_qry, NULL, NULL);
    if (mongoc_cursor_error(mdb_crs, &mdb_err))
    {
      fprintf(stderr, "%s: error in listing workspace: %s.\n", PROGNAME, mdb_err.message);
      exitcode = EXIT_FAILURE;
      goto EXIT_INPUT;
    }

    if (mongoc_cursor_next(mdb_crs, (const bson_t **)&mdb_doc) &&
        bson_iter_init_find(&iter, mdb_doc, "variables") &&
        bson_iter_recurse(&iter, &iter1))
    {
      size_t i = 0;
      while (bson_iter_next(&iter1)) // iter through variables docs, while actually it is single doc
      {
        if ((size_t)bson_iter_value(&iter1)->value.v_double > var_limitN[i])
        {
          fprintf(stderr, "%s: variable \"%s\" size at dim '%zu' should exceeded '%zu'.\n", PROGNAME, var_name, i, var_limitN[i]);
          exitcode = EXIT_FAILURE;
          goto EXIT_INPUT;
        }
        var_N[i] = (size_t)bson_iter_value(&iter1)->value.v_double;
      }
    }

    bson_destroy(mdb_doc);
    mongoc_cursor_destroy(mdb_crs);
    bson_destroy(mdb_qry);

    // type check
    mdb_qry = BCON_NEW("pipeline", "[",
                       "{", "$match", "{", "variables.name", BCON_UTF8(var_name), "}", "}",
                       "{", "$unwind", BCON_UTF8("$variables"), "}",
                       "{", "$match", "{", "variables.name", BCON_UTF8(var_name), "}", "}",
                       "{", "$unwind", BCON_UTF8("$variables.value"), "}",
                       "{", "$limit", BCON_INT32(1), "}",
                       "{", "$project", "{", "variables.value", BCON_BOOL(true), "}", "}",
                       "{", "$project", "{", "value_type", "{", "$type", BCON_UTF8("$variables.value"), "}", "}", "}",
                       "]");

    mdb_crs = mongoc_collection_aggregate(mdb_col, MONGOC_QUERY_NONE, mdb_qry, NULL, NULL);
    if (mongoc_cursor_error(mdb_crs, &mdb_err))
    {
      fprintf(stderr, "%s: error in listing workspace: %s.\n", PROGNAME, mdb_err.message);
      exitcode = EXIT_FAILURE;
      goto EXIT_INPUT;
    }

    // while (mongoc_cursor_next(mdb_crs, (const bson_t **)&mdb_doc))
    // {
    //   char *str = bson_as_json(mdb_doc, NULL);
    //   printf("%s\n", str);
    //   bson_free(str);
    // }

    if (mongoc_cursor_next(mdb_crs, (const bson_t **)&mdb_doc) &&
        bson_iter_init_find(&iter, mdb_doc, "value_type"))
    {
        printf("%s: %s\n",  bson_iter_key(&iter), bson_iter_value(&iter)->value.v_utf8.str);
     
    }
    // if (mongoc_cursor_next(mdb_crs, (const bson_t **)&mdb_doc) &&
    //     bson_iter_init_find(&iter, mdb_doc, "variables") &&
    //     bson_iter_recurse(&iter, &iter1))
    // {
    //   size_t i = 0;
    //   while (bson_iter_next(&iter1)) // iter through variables docs, while actually it is single doc
    //   {
    //     if ((size_t)bson_iter_value(&iter1)->value.v_double > var_limitN[i])
    //     {
    //       fprintf(stderr, "%s: variable \"%s\" size at dim '%zu' should exceeded '%zu'.\n", PROGNAME, var_name, i, var_limitN[i]);
    //       exitcode = EXIT_FAILURE;
    //       goto EXIT_INPUT;
    //     }
    //     var_N[i] = (size_t)bson_iter_value(&iter1)->value.v_double;
    //   }
    // }

    bson_destroy(mdb_doc);
    mongoc_cursor_destroy(mdb_crs);
    bson_destroy(mdb_qry);
  }
  // while (mongoc_cursor_next(mdb_crs, (const bson_t **)&mdb_doc))
  // {
  //   char *str = bson_as_json(mdb_doc, NULL);
  //   printf("%s\n", str);
  //   bson_free(str);
  // }
  // if (a == NULL)
  // {
  // }

OPERATION:;

OUTPUT:;

STDOUT:;
  // size_t Nans = N1;
  // char **ans = out1;
  // if (verbose->count)
  // {
  //   for (int i = 0; i < Nans; i++)
  //     fprintf(stdout, "%s: %s\n", sp_get_port_name(port_list[i]), sp_get_port_description(port_list[i]));
  // }
  // else
  // {
  //   if (Nans > 0)
  //     fprintf(stdout, "%s", ans[0]);
  //   for (size_t i = 1; i < MIN(Nans, 3); ++i)
  //     fprintf(stdout, ", %s", ans[i]);
  //   if (Nans > 5)
  //     fprintf(stdout, ", ...");
  //   for (size_t i = MAX(MIN(Nans, 3), Nans - 2); i < Nans; ++i)
  //     fprintf(stdout, ", %s", ans[i]);
  //   fprintf(stdout, "\n");
  // }

WORKSPACE:;

  // if (getenv("BASHLAB_WORKSPACE_ANS"))
  //   strcpy(mdb_var_str, getenv("BASHLAB_WORKSPACE_ANS"));

  // if (mdb_col != NULL)
  // {
  //   bson_t *mdb_qry = BCON_NEW("variables.name", BCON_UTF8(mdb_var_str));
  //   int64_t mdb_cnt = mongoc_collection_count_documents(mdb_col, mdb_qry, NULL, NULL, NULL, &mdb_err);
  //   bson_destroy(mdb_qry);

  //   if (mdb_cnt < 0)
  //   {
  //     fprintf(stderr, "%s: counting variables failed.\n", PROGNAME);
  //     exitcode = EXIT_FAILURE;
  //     goto EXIT_WORKSPACE;
  //   }
  //   else if (mdb_cnt == 0)
  //   {
  //     bson_t *mdb_qry = BCON_NEW("variables", "{", "$exists", BCON_BOOL(true), "}");
  //     bson_t *mdb_doc = bson_new();
  //     bson_t mdb_doc_child1, mdb_doc_child2, mdb_doc_child3;
  //     BSOa_NPPEND_DOCUMENT_BEGIN(mdb_doc, "$push", &mdb_doc_child1);
  //     BSOa_NPPEND_DOCUMENT_BEGIN(&mdb_doc_child1, "variables", &mdb_doc_child2);
  //     BSOa_NPPEND_UTF8(&mdb_doc_child2, "name", mdb_var_str);
  //     BSOa_NPPEND_ARRAY_BEGIN(&mdb_doc_child2, "value", &mdb_doc_child3);
  //     for (size_t i = 0; i < N1; ++i)
  //       bsoa_Nppend_utf8(&mdb_doc_child3, "no", -1, out1[i], -1);
  //     bsoa_Nppend_array_end(&mdb_doc_child2, &mdb_doc_child3);
  //     BSOa_NPPEND_ARRAY_BEGIN(&mdb_doc_child2, "size", &mdb_doc_child3);
  //     bsoa_Nppend_double(&mdb_doc_child3, "no", -1, (double)N1);
  //     bsoa_Nppend_array_end(&mdb_doc_child2, &mdb_doc_child3);
  //     bsoa_Nppend_document_end(&mdb_doc_child1, &mdb_doc_child2);
  //     bsoa_Nppend_document_end(mdb_doc, &mdb_doc_child1);

  //     if (!mongoc_collection_update_one(mdb_col, mdb_qry, mdb_doc, NULL, NULL, &mdb_err))
  //     {
  //       fprintf(stderr, "%s: variable insertation failed.\n", PROGNAME);
  //       exitcode = EXIT_FAILURE;
  //       goto EXIT_WORKSPACE;
  //     }
  //     bson_destroy(mdb_qry);
  //     bson_destroy(mdb_doc);
  //   }
  //   else
  //   {
  //     bson_t *mdb_qry = BCON_NEW("variables.name", BCON_UTF8(mdb_var_str));
  //     bson_t *mdb_doc = bson_new();
  //     bson_t mdb_doc_child1, mdb_doc_child2, mdb_doc_child3;
  //     BSOa_NPPEND_DOCUMENT_BEGIN(mdb_doc, "$set", &mdb_doc_child1);
  //     BSOa_NPPEND_ARRAY_BEGIN(&mdb_doc_child1, "variables.$.value", &mdb_doc_child2);
  //     for (size_t i = 0; i < N1; ++i)
  //       bsoa_Nppend_utf8(&mdb_doc_child3, "no", -1, out1[i], -1);
  //     bsoa_Nppend_array_end(&mdb_doc_child1, &mdb_doc_child2);
  //     BSOa_NPPEND_ARRAY_BEGIN(&mdb_doc_child1, "variables.$.size", &mdb_doc_child2);
  //     bsoa_Nppend_double(&mdb_doc_child2, "no", -1, (double)N1);
  //     bsoa_Nppend_array_end(&mdb_doc_child1, &mdb_doc_child2);
  //     bsoa_Nppend_document_end(mdb_doc, &mdb_doc_child1);

  //     if (!mongoc_collection_update_one(mdb_col, mdb_qry, mdb_doc, NULL, NULL, &mdb_err))
  //     {
  //       fprintf(stderr, "%s: variable update failed.\n", PROGNAME);
  //       exitcode = EXIT_FAILURE;
  //       goto EXIT_WORKSPACE;
  //     }
  //     bson_destroy(mdb_qry);
  //     bson_destroy(mdb_doc);
  //   }
  // }

HISTORY:
  if (mdb_col != NULL)
  {
    strcpy(buff, PROGNAME);
    for (size_t i = 1; i < argc; i++)
    {
      strcat(buff, " ");
      strcat(buff, argv[i]);
    }
    bson_t *mdb_qry = BCON_NEW("history", "{", "$exists", BCON_BOOL(true), "}");
    bson_t *mdb_doc = BCON_NEW("$push", "{", "history", BCON_UTF8(buff), "}");

    if (!mongoc_collection_update_one(mdb_col, mdb_qry, mdb_doc, NULL, NULL, &mdb_err))
    {
      fprintf(stderr, "%s: history update failed.\n", PROGNAME);
      exitcode = EXIT_FAILURE;
      goto EXIT_HISTORY;
    }
    bson_destroy(mdb_doc);
    bson_destroy(mdb_qry);
  }

/* ======================================================================== */
/* exit                                                                     */
/* ======================================================================== */
EXIT_HISTORY:;

EXIT_WORKSPACE:;

EXIT_OUTPUT:;

EXIT_OPERATION:;

EXIT_INPUT:;
  if (a_val != NULL)
    free(a_val);

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
