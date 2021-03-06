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
static char buff[500];

// mongodb
static mongoc_uri_t *mdb_uri = NULL;
static mongoc_client_t *mdb_cli = NULL;
static mongoc_database_t *mdb_dtb = NULL;
static mongoc_collection_t *mdb_col = NULL;
static bson_error_t mdb_err;
static bson_oid_t mdb_oid;
static char mdb_dtb_str[50] = "bashlab";
static char mdb_col_str[50] = "workspace";
static char mdb_var_str[50] = "ans";
bson_t *mdb_qry, *mdb_qry1, *mdb_doc, *mdb_doc1, *mdb_pip;
mongoc_cursor_t *mdb_crs, *mdb_crs1;
bson_iter_t iter, iter1, iter2, iter3;
/*============================================================================*/
/* Specifics                                                                  */
/*============================================================================*/
#include "linspace.h"
#define PROGNAME "linspace"
static const char *program_json = "{\"name\":\"linspace\",\"desc\":\"generate linearly spaced array\",\"osOnly\":false,\"pargs\":[{\"name\":\"a\",\"minc\":1,\"maxc\":1,\"desc\":\"starting value of the sequence\"},{\"name\":\"b\",\"minc\":1,\"maxc\":1,\"desc\":\"end value of the seqeunce (included)\"},{\"name\":\"N\",\"minc\":1,\"maxc\":1,\"desc\":\"number of points\"}],\"oargs\":[],\"opts\":[]}";

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
  if (!getenv("BASHLAB_MONGODB_URI"))
    goto MAIN;

  mongoc_init();
  mdb_uri = mongoc_uri_new_with_error(getenv("BASHLAB_MONGODB_URI"), &mdb_err);
  if (!mdb_uri)
    goto MAIN;

  mdb_cli = mongoc_client_new_from_uri(mdb_uri);
  if (!mdb_cli)
    goto MAIN;

  mongoc_client_set_appname(mdb_cli, PROGNAME);

  if (getenv("BASHLAB_MONGODB_DATABASE"))
    strcpy(mdb_dtb_str, getenv("BASHLAB_MONGODB_DATABASE"));
  mdb_dtb = mongoc_client_get_database(mdb_cli, mdb_dtb_str);

  if (getenv("BASHLAB_MONGODB_COLLECTION"))
    strcpy(mdb_col_str, getenv("BASHLAB_MONGODB_COLLECTION"));
  mdb_col = mongoc_client_get_collection(mdb_cli, mdb_dtb_str, mdb_col_str);

/* ======================================================================== */
/* main operation                                                           */
/* ======================================================================== */
MAIN:;
  struct arg_str *arg_a = (struct arg_str *)argtable[0];
  struct arg_str *arg_b = (struct arg_str *)argtable[1];
  struct arg_str *arg_N = (struct arg_str *)argtable[2];

INPUTT:;
  size_t Nvar = 3;
  struct arg_str *var_args[3] = {arg_a, arg_b, arg_N};
  char_t var_names[3][BL_WORKSPACE_MAX_VARLENGTH];
  for (size_t i = 0; i < Nvar; i++)
    strcpy(var_names[i], (char *)var_args[i]->sval[0]);
  bool var_founds[3] = {false, false, false};
  size_t var_dims[3] = {1, 1, 1};
  size_t var_size_limits_upper[3][BL_WORKSPACE_MAX_DIM] = {
      {1},
      {1},
      {1}};
  size_t var_size_limits_lower[3][BL_WORKSPACE_MAX_DIM] = {
      {1},
      {1},
      {1}};
  size_t var_sizes[3][BL_WORKSPACE_MAX_DIM] = {
      {0},
      {0},
      {0}};
  size_t var_total_sizes[3] = {1, 1, 1};
  char_t var_types[3][10] = {"double", "double", "double"};
  number_t *var_vals[3] = {NULL, NULL, NULL};

  for (size_t i = 0; i < Nvar; i++)
  {
    if (isnumber(var_args[i]->sval[0]))
    {
      var_founds[i] = true;
      var_sizes[i][0]++;
      var_vals[i] = (number_t *)calloc(var_sizes[i][0], sizeof(number_t));
      var_vals[i][0] = atof(var_args[i]->sval[0]);
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
          if (strcmp(bson_iter_value(&iter2)->value.v_utf8.str, var_names[i]) == 0)
          {
            var_founds[i] = true;
            break;
          }
        }
      }
      bson_destroy(mdb_doc);
      mongoc_cursor_destroy(mdb_crs);
      bson_destroy(mdb_qry);
      bson_destroy(mdb_qry1);

      if (!var_founds[i])
      {
        fprintf(stderr, "%s: variable \"%s\" not found.\n", PROGNAME, var_names[i]);
        exitcode = EXIT_FAILURE;
        goto EXIT_INPUT;
      }

      // dim check
      mdb_qry = BCON_NEW("pipeline", "[",
                         "{", "$match", "{", "variables.name", BCON_UTF8(var_names[i]), "}", "}",
                         "{", "$unwind", BCON_UTF8("$variables"), "}",
                         "{", "$match", "{", "variables.name", BCON_UTF8(var_names[i]), "}", "}",
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
      bson_destroy(mdb_doc);
      mongoc_cursor_destroy(mdb_crs);
      bson_destroy(mdb_qry);

      if (dim != var_dims[i])
      {
        fprintf(stderr, "%s: variable \"%s\" dimension should be '%zu' instead of '%zu'.\n", PROGNAME, var_names[i], var_dims[i], dim);
        exitcode = EXIT_FAILURE;
        goto EXIT_INPUT;
      }

      // size check
      mdb_qry = BCON_NEW("pipeline", "[",
                         "{", "$match", "{", "variables.name", BCON_UTF8(var_names[i]), "}", "}",
                         "{", "$unwind", BCON_UTF8("$variables"), "}",
                         "{", "$match", "{", "variables.name", BCON_UTF8(var_names[i]), "}", "}",
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
        size_t j = 0;
        while (bson_iter_next(&iter1)) // iter through variables docs, while actually it is single doc
          var_sizes[i][j++] = (size_t)bson_iter_value(&iter1)->value.v_double;
      }

      bson_destroy(mdb_doc);
      mongoc_cursor_destroy(mdb_crs);
      bson_destroy(mdb_qry);

      for (size_t j = 0; j < var_dims[i]; j++)
        if (var_sizes[i][j] > var_size_limits_upper[i][j] || var_sizes[i][j] < var_size_limits_lower[i][j])
        {
          fprintf(stderr, "%s: variable \"%s\" size at dim '%zu' should not exceeded '%zu' and below '%zu'.\n", PROGNAME, var_names[i], j, var_size_limits_upper[i][j], var_size_limits_lower[i][j]);
          exitcode = EXIT_FAILURE;
          goto EXIT_INPUT;
        }

      // type check
      mdb_qry = BCON_NEW("pipeline", "[",
                         "{", "$match", "{", "variables.name", BCON_UTF8(var_names[i]), "}", "}",
                         "{", "$unwind", BCON_UTF8("$variables"), "}",
                         "{", "$match", "{", "variables.name", BCON_UTF8(var_names[i]), "}", "}",
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

      bool is_matched = false;
      if (mongoc_cursor_next(mdb_crs, (const bson_t **)&mdb_doc) &&
          bson_iter_init_find(&iter, mdb_doc, "value_type"))
        is_matched = !strcmp(bson_iter_value(&iter)->value.v_utf8.str, var_types[i]);

      bson_destroy(mdb_doc);
      mongoc_cursor_destroy(mdb_crs);
      bson_destroy(mdb_qry);

      if (!is_matched)
      {
        fprintf(stderr, "%s: variable \"%s\" should be \"%s\" type.\n", PROGNAME, var_names[i], var_types[i]);
        exitcode = EXIT_FAILURE;
        goto EXIT_INPUT;
      }

      // determine total size and allocate memory
      for (size_t j = 0; j < var_dims[i]; j++)
        var_total_sizes[i] *= var_sizes[i][j];
      var_vals[i] = (number_t *)calloc(var_total_sizes[i], sizeof(number_t));

      // value fetch
      mdb_qry = BCON_NEW("pipeline", "[",
                         "{", "$match", "{", "variables.name", BCON_UTF8(var_names[i]), "}", "}",
                         "{", "$unwind", BCON_UTF8("$variables"), "}",
                         "{", "$match", "{", "variables.name", BCON_UTF8(var_names[i]), "}", "}",
                         "{", "$project", "{", "variables.value", BCON_BOOL(true), "}", "}",
                         "{", "$project", "{", "value", BCON_UTF8("$variables.value"), "}", "}",
                         "]");

      mdb_crs = mongoc_collection_aggregate(mdb_col, MONGOC_QUERY_NONE, mdb_qry, NULL, NULL);
      if (mongoc_cursor_error(mdb_crs, &mdb_err))
      {
        fprintf(stderr, "%s: error in listing workspace: %s.\n", PROGNAME, mdb_err.message);
        exitcode = EXIT_FAILURE;
        goto EXIT_INPUT;
      }

      if (mongoc_cursor_next(mdb_crs, (const bson_t **)&mdb_doc) &&
          bson_iter_init_find(&iter, mdb_doc, "value") &&
          BSON_ITER_HOLDS_ARRAY(&iter) &&
          bson_iter_recurse(&iter, &iter1))
      {
        size_t j = 0;
        while (bson_iter_next(&iter1))
          var_vals[i][j++] = bson_iter_value(&iter1)->value.v_double;
      }

      bson_destroy(mdb_doc);
      mongoc_cursor_destroy(mdb_crs);
      bson_destroy(mdb_qry);
    }
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

  // post check
  if ((size_t)var_vals[2][0] < 2)
  {
    fprintf(stderr, "%s: \"%s\" should be at least '%zu'.\n", PROGNAME, var_names[2], (size_t)2);
    exitcode = EXIT_FAILURE;
    goto EXIT_INPUT;
  }

OPERATION:;
  number_t a = var_vals[0][0], b = var_vals[1][0];
  size_t N = (size_t)var_vals[2][0];
  number_t *out = (number_t *)calloc(N, sizeof(number_t));
  if (out == NULL)
  {
    fprintf(stderr, "%s: Not enough memory\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT_OPERATION;
  }
  if (verbose->count)
  {
    fprintf(stdout, "Step size: %.16G\n", (b - a) / (N - 1));
    fprintf(stdout, "Operation: %ld ... ", tic());
  }
  linspace(a, b, (int)N, out);
  if (verbose->count)
    fprintf(stdout, "%ld [ms]\n", toc());

OUTPUT:;

STDOUT:;
  size_t Nans = N;
  number_t *ans = out;
  if (verbose->count)
  {
    sprintf(buff, "%zu", Nans - 1);
    for (size_t i = 0; i < MIN(Nans, 3); ++i)
      fprintf(stdout, "[%-*zu]: %.16G\n", (int)strlen(buff), i, ans[i]);
    if (Nans > 5)
      fprintf(stdout, "...\n");
    for (size_t i = MAX(MIN(Nans, 3), Nans - 2); i < Nans; ++i)
      fprintf(stdout, "[%-*zu]: %.16G\n", (int)strlen(buff), i, ans[i]);
  }
  else
  {
    if (Nans > 0)
      fprintf(stdout, "%.16G", ans[0]);
    for (size_t i = 1; i < MIN(Nans, 3); ++i)
      fprintf(stdout, ", %.16G", ans[i]);
    if (Nans > 5)
      fprintf(stdout, ", ...");
    for (size_t i = MAX(MIN(Nans, 3), Nans - 2); i < Nans; ++i)
      fprintf(stdout, ", %.16G", ans[i]);
    fprintf(stdout, "\n");
  }

WORKSPACE:;

  if (getenv("BL_WORKSPACE_ANS"))
    strcpy(mdb_var_str, getenv("BL_WORKSPACE_ANS"));

  if (mdb_col != NULL)
  {
    bson_t *mdb_qry = BCON_NEW("variables.name", BCON_UTF8(mdb_var_str));
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
      for (size_t i = 0; i < N; ++i)
        bson_append_double(&mdb_doc_child3, "no", -1, out[i]);
      bson_append_array_end(&mdb_doc_child2, &mdb_doc_child3);
      BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child2, "size", &mdb_doc_child3);
      bson_append_double(&mdb_doc_child3, "no", -1, (double)N);
      bson_append_array_end(&mdb_doc_child2, &mdb_doc_child3);
      bson_append_document_end(&mdb_doc_child1, &mdb_doc_child2);
      bson_append_document_end(mdb_doc, &mdb_doc_child1);
    }
    else
    {
      mdb_qry = BCON_NEW("variables.name", BCON_UTF8(mdb_var_str));
      mdb_doc = bson_new();
      bson_t mdb_doc_child1, mdb_doc_child2, mdb_doc_child3;
      BSON_APPEND_DOCUMENT_BEGIN(mdb_doc, "$set", &mdb_doc_child1);
      BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child1, "variables.$.value", &mdb_doc_child2);
      for (size_t i = 0; i < N; ++i)
        bson_append_double(&mdb_doc_child2, "no", -1, out[i]);
      bson_append_array_end(&mdb_doc_child1, &mdb_doc_child2);
      BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child1, "variables.$.size", &mdb_doc_child2);
      bson_append_double(&mdb_doc_child2, "no", -1, (double)N);
      bson_append_array_end(&mdb_doc_child1, &mdb_doc_child2);
      bson_append_document_end(mdb_doc, &mdb_doc_child1);
    }
    if (mdb_doc->len > BL_WORSKPACE_MAX_RAW_SIZE)
    {
      fprintf(stderr, "%s: variable \"%s\" size '%u' exceeds database document limit '%u'\n", PROGNAME, mdb_var_str, mdb_doc->len, BL_WORSKPACE_MAX_RAW_SIZE);
      exitcode = EXIT_FAILURE;
    }
    else if (!mongoc_collection_update_one(mdb_col, mdb_qry, mdb_doc, NULL, NULL, &mdb_err))
    {
      if (strstr(mdb_err.message, "BSONObj size") != NULL)
      {
        ; // total object size exceeds 16MB
      }
      fprintf(stderr, "%s: variable update failed: %s(%u)\n", PROGNAME, mdb_err.message, mdb_err.code);
      exitcode = EXIT_FAILURE;
    }
    bson_destroy(mdb_qry);
    bson_destroy(mdb_doc);
    if (exitcode == EXIT_FAILURE)
      goto EXIT_WORKSPACE;
  }

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
  free(out);

EXIT_INPUT:;
  for (size_t i = 0; i < Nvar; i++)
    if (var_vals[i] != NULL)
      free(var_vals[i]);

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
