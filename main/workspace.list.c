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
/*============================================================================*/
/* Specifics                                                                  */
/*============================================================================*/
#define PROGNAME "workspace.list"
static const char *program_json =
    "{"
    "\"name\": \"workspace.list\","
    "\"desc\": \"lists workspace\","
    "\"pargs\": ["
    /*        */ ""
    /*       */ "],"
    "\"oargs\": ["
    /*        */ ""
    /*       */ "],"
    "\"opts\": ["
    /*       */ "{\"short\":\"h\", \"long\":\"history\", \"desc\":\"list history\"}"
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
  /* register program                                                         */
  /* ======================================================================== */
  if (getenv("BASHLAB_MONGODB_URI"))
  {
    mongoc_init();
    mdb_uri = mongoc_uri_new_with_error(getenv("BASHLAB_MONGODB_URI"), &mdb_err);
    if (mdb_uri)
    {
      mdb_cli = mongoc_client_new_from_uri(mdb_uri);
      if (mdb_cli)
      {
        mdb_col = mongoc_client_get_collection(mdb_cli, mdb_dtb_str, "programs");
        sprintf(buff, "{\"$set\": %s }", program_json);
        mdb_doc = bson_new_from_json((const uint8_t *)buff, -1, &mdb_err);
        mdb_qry = BCON_NEW("name", BCON_UTF8(PROGNAME));
        mdb_qry1 = BCON_NEW("upsert", BCON_BOOL(true));
        if (!mongoc_collection_update_one(mdb_col, mdb_qry, mdb_doc, mdb_qry1, NULL, &mdb_err))
        {
          fprintf(stderr, "%s: program info update/insert failed: %s(%u)\n", PROGNAME, mdb_err.message, mdb_err.code);
          exitcode = EXIT_FAILURE;
          goto EXIT;
        }
        bson_destroy(mdb_doc);
        bson_destroy(mdb_qry);
        bson_destroy(mdb_qry1);

        mongoc_collection_destroy(mdb_col);
      }
    }
  }

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
  if (mdb_col == NULL)
  {
    fprintf(stderr, "%s: function requires workspace database.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }

INPUTT:;

OPERATION:;
  size_t var_sizes[100][3];
  size_t var_sizeN[100];
  size_t var_sizeT[100];
  char var_names[100][100];

  size_t var_lngth = 0;

  mdb_qry = BCON_NEW("variables", "{", "$exists", BCON_BOOL(true), "}");
  mdb_qry1 = BCON_NEW("projection", "{", "variables.name", BCON_BOOL(true), "variables.size", BCON_BOOL(true), "}");
  mdb_crs = mongoc_collection_find_with_opts(mdb_col, mdb_qry, mdb_qry1, NULL);
  if (mongoc_cursor_error(mdb_crs, &mdb_err))
  {
    fprintf(stderr, "%s: error in listing workspace: %s.\n", PROGNAME, mdb_err.message);
    exitcode = EXIT_FAILURE;
    goto EXIT_OPERATION;
  }

  while (mongoc_cursor_next(mdb_crs, (const bson_t **)&mdb_doc))
  {
    bson_iter_t iter, iter1, iter2, iter3;
    if (bson_iter_init_find(&iter, mdb_doc, "variables") && BSON_ITER_HOLDS_ARRAY(&iter) && bson_iter_recurse(&iter, &iter1))
    {
      while (bson_iter_next(&iter1)) // iter through variables docs, while actually it is single doc
      {
        bson_iter_recurse(&iter1, &iter2); // step into variables array
        while (bson_iter_next(&iter2))     // iter through variables array
        {
          if (strcmp(bson_iter_key(&iter2), "name") == 0) // "name" key
            strcpy(var_names[var_lngth], bson_iter_value(&iter2)->value.v_utf8.str);
          else if (strcmp(bson_iter_key(&iter2), "size") == 0) // "size" key
          {
            var_sizeT[var_lngth] = 1;
            bson_iter_recurse(&iter2, &iter3);
            size_t i = 0;
            while (bson_iter_next(&iter3))
            {
              var_sizes[var_lngth][i] = (size_t)bson_iter_value(&iter3)->value.v_double;
              var_sizeT[var_lngth] *= var_sizes[var_lngth][i];
              i++;
            }
            var_sizeN[var_lngth] = i;
          }
        }
        var_lngth++;
      }
    }
  }
  bson_destroy(mdb_doc);
  mongoc_cursor_destroy(mdb_crs);
  bson_destroy(mdb_qry);
  bson_destroy(mdb_qry1);

  // Fetch values
  double var_valsd[100][5];
  char var_valss[100][5][16];
  bson_type_t var_types[100];
  var_lngth = 0;

  mdb_qry = BCON_NEW("variables", "{", "$exists", BCON_BOOL(true), "}");
  mdb_qry1 = BCON_NEW("projection", "{", "variables.value", "{", "$slice", BCON_INT32(5), "}", "}");
  mdb_crs = mongoc_collection_find_with_opts(mdb_col, mdb_qry, mdb_qry1, NULL);
  if (mongoc_cursor_error(mdb_crs, &mdb_err))
  {
    fprintf(stderr, "%s: error in listing workspace: %s.\n", PROGNAME, mdb_err.message);
    exitcode = EXIT_FAILURE;
    goto EXIT_OPERATION;
  }
  while (mongoc_cursor_next(mdb_crs, (const bson_t **)&mdb_doc))
  {
    bson_iter_t iter, iter1, iter2, iter3;
    if (bson_iter_init_find(&iter, mdb_doc, "variables") && BSON_ITER_HOLDS_ARRAY(&iter) && bson_iter_recurse(&iter, &iter1))
    {
      while (bson_iter_next(&iter1)) // iter through variables docs, while actually it is single doc
      {
        bson_iter_recurse(&iter1, &iter2); // step into variables array again for values
        while (bson_iter_next(&iter2))     // iter through variables array again for values
        {
          if (strcmp(bson_iter_key(&iter2), "value") == 0)
          {
            bson_iter_recurse(&iter2, &iter3);
            size_t i = 0;
            while (bson_iter_next(&iter3))
            {
              if (i == 0)
                var_types[var_lngth] = bson_iter_value(&iter3)->value_type;
              if (var_types[var_lngth] == BSON_TYPE_DOUBLE)
                var_valsd[var_lngth][i] = bson_iter_value(&iter3)->value.v_double;
              if (var_types[var_lngth] == BSON_TYPE_UTF8)
                strcpy(var_valss[var_lngth][i], bson_iter_value(&iter3)->value.v_utf8.str);
              i++;
            }
          }
        }
        var_lngth++;
      }
    }
  }
  bson_destroy(mdb_doc);
  mongoc_cursor_destroy(mdb_crs);
  bson_destroy(mdb_qry);
  bson_destroy(mdb_qry1);

  // fetch values reverse
  double var_valsd_b[100][5];
  char var_valss_b[100][5][16];
  var_lngth = 0;

  mdb_qry = BCON_NEW("variables", "{", "$exists", BCON_BOOL(true), "}");
  mdb_qry1 = BCON_NEW("projection", "{", "variables.value", "{", "$slice", BCON_INT32(-2), "}", "}");
  mdb_crs = mongoc_collection_find_with_opts(mdb_col, mdb_qry, mdb_qry1, NULL);
  if (mongoc_cursor_error(mdb_crs, &mdb_err))
  {
    fprintf(stderr, "%s: error in listing workspace: %s.\n", PROGNAME, mdb_err.message);
    exitcode = EXIT_FAILURE;
    goto EXIT_OPERATION;
  }
  while (mongoc_cursor_next(mdb_crs, (const bson_t **)&mdb_doc))
  {
    bson_iter_t iter, iter1, iter2, iter3;
    if (bson_iter_init_find(&iter, mdb_doc, "variables") && BSON_ITER_HOLDS_ARRAY(&iter) && bson_iter_recurse(&iter, &iter1))
    {
      while (bson_iter_next(&iter1)) // iter through variables docs, while actually it is single doc
      {
        bson_iter_recurse(&iter1, &iter2); // step into variables array again for values
        while (bson_iter_next(&iter2))     // iter through variables array again for values
        {
          if (strcmp(bson_iter_key(&iter2), "value") == 0)
          {
            bson_iter_recurse(&iter2, &iter3);
            size_t i = 0;
            while (bson_iter_next(&iter3))
            {
              if (i == 0)
                var_types[var_lngth] = bson_iter_value(&iter3)->value_type;
              if (var_types[var_lngth] == BSON_TYPE_DOUBLE)
                var_valsd_b[var_lngth][i] = bson_iter_value(&iter3)->value.v_double;
              if (var_types[var_lngth] == BSON_TYPE_UTF8)
                strcpy(var_valss_b[var_lngth][i], bson_iter_value(&iter3)->value.v_utf8.str);
              i++;
            }
          }
        }
        var_lngth++;
      }
    }
  }
  bson_destroy(mdb_doc);
  mongoc_cursor_destroy(mdb_crs);
  bson_destroy(mdb_qry);
  bson_destroy(mdb_qry1);

OUTPUT:;

STDOUT:;
  size_t max_var_name_length = 0;
  for (size_t i = 0; i < var_lngth; i++)
    if (strlen(var_names[i]) > max_var_name_length)
      max_var_name_length = strlen(var_names[i]);
  for (size_t i = 0; i < var_lngth; i++)
  {
    fprintf(stdout, "%-*s: ", (int)max_var_name_length, var_names[i]);
    fprintf(stdout, "%s[", (var_types[i] == BSON_TYPE_DOUBLE) ? "number" : "string");
    for (size_t j = 1; j < var_sizeN[i] - 1; ++j)
      fprintf(stdout, "%zux", var_sizes[i][j]);
    fprintf(stdout, "%zu]: ", var_sizes[i][var_sizeN[i] - 1]);

    if (var_sizeT[i] > 0)
      if (var_types[i] == BSON_TYPE_DOUBLE)
        fprintf(stdout, "%.16G", var_valsd[i][0]);
      else
        fprintf(stdout, "%.16s", var_valss[i][0]);

    if (var_sizeT[i] <= 5)
    {
      for (size_t j = 1; j < var_sizeT[i]; ++j)
        if (var_types[i] == BSON_TYPE_DOUBLE)
          fprintf(stdout, ", %.16G", var_valsd[i][j]);
        else
          fprintf(stdout, ", %.16s", var_valss[i][j]);
    }
    else
    {
      for (size_t j = 1; j < 3; ++j)
        if (var_types[i] == BSON_TYPE_DOUBLE)
          fprintf(stdout, ", %.16G", var_valsd[i][j]);
        else
          fprintf(stdout, ", %.16s", var_valss[i][j]);
      fprintf(stdout, ", ...");
      for (size_t j = 0; j < 2; ++j)
        if (var_types[i] == BSON_TYPE_DOUBLE)
          fprintf(stdout, ", %.16G", var_valsd_b[i][j]);
        else
          fprintf(stdout, ", %.16s", var_valss_b[i][j]);
    }
    fprintf(stdout, "\n");
  }
WORKSPACE:;

  // if (getenv("BL_WORKSPACE_ANS"))
  //   strcpy(mdb_var_str, getenv("BL_WORKSPACE_ANS"));

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
  //     BSON_APPEND_DOCUMENT_BEGIN(mdb_doc, "$push", &mdb_doc_child1);
  //     BSON_APPEND_DOCUMENT_BEGIN(&mdb_doc_child1, "variables", &mdb_doc_child2);
  //     BSON_APPEND_UTF8(&mdb_doc_child2, "name", mdb_var_str);
  //     BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child2, "value", &mdb_doc_child3);
  //     for (size_t i = 0; i < var_lngth; ++i)
  //       bson_append_utf8(&mdb_doc_child3, "no", -1, var_names[i], -1);
  //     bson_append_array_end(&mdb_doc_child2, &mdb_doc_child3);
  //     BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child2, "size", &mdb_doc_child3);
  //     bson_append_double(&mdb_doc_child3, "no", -1, (double)var_lngth);
  //     bson_append_array_end(&mdb_doc_child2, &mdb_doc_child3);
  //     bson_append_document_end(&mdb_doc_child1, &mdb_doc_child2);
  //     bson_append_document_end(mdb_doc, &mdb_doc_child1);

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
  //     BSON_APPEND_DOCUMENT_BEGIN(mdb_doc, "$set", &mdb_doc_child1);
  //     BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child1, "variables.$.value", &mdb_doc_child2);
  //     for (size_t i = 0; i < var_lngth; ++i)
  //       bson_append_utf8(&mdb_doc_child2, "no", -1, var_names[i], -1);
  //     bson_append_array_end(&mdb_doc_child1, &mdb_doc_child2);
  //     BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child1, "variables.$.size", &mdb_doc_child2);
  //     bson_append_double(&mdb_doc_child2, "no", -1, (double)var_lngth);
  //     bson_append_array_end(&mdb_doc_child1, &mdb_doc_child2);
  //     bson_append_document_end(mdb_doc, &mdb_doc_child1);

  //     if (!mongoc_collection_update_one(mdb_col, mdb_qry, mdb_doc, NULL, NULL, &mdb_err))
  //     {
  //       fprintf(stderr, "%s: variable update failed.\n", PROGNAME);
  //       exitcode = EXIT_FAILURE;
  //       goto EXIT_WORKSPACE;
  //     }
  //     bson_destroy(mdb_qry);
  //     bson_destroy(mdb_doc);
  //   }

HISTORY:
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