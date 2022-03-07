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
/*============================================================================*/
/* Specifics                                                                  */
/*============================================================================*/
#define PROGNAME "serialport.ping"
static const char *program_json =
    "{"
    "\"name\": \"serialport.ping\","
    "\"desc\": \"send command and read response from serial port\","
    "\"pargs\": ["
    /*       */ "],"
    "\"oargs\": ["
    /*       */ "],"
    "\"opts\": ["
    /*      */ "]"
    "}";

#include <libserialport.h>

/* Helper function for error handling. */
int check(enum sp_return result);

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

INPUTT:;

OPERATION:;
  struct sp_port **port_list;
  size_t N1 = 0;
  enum sp_return sp_result = sp_list_ports(&port_list);
  if (sp_result != SP_OK)
  {
    fprintf(stderr, "%s: serialport listing failed.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT_OPERATION;
  }
  while (port_list[N1] != NULL)
    ++N1;

OUTPUT:;
  char **out1 = (char **)malloc(N1 * sizeof(char *));
  for (int i = 0; i < N1; i++)
  {
    out1[i] = (char *)malloc(256 * sizeof(char));
    strcpy(out1[i], sp_get_port_name(port_list[i]));
  }

STDOUT:;
  size_t Nans = N1;
  char **ans = out1;
  if (verbose->count)
  {
    for (int i = 0; i < Nans; i++)
      fprintf(stdout, "%s: %s\n", sp_get_port_name(port_list[i]), sp_get_port_description(port_list[i]));
  }
  else
  {
    if (Nans > 0)
      fprintf(stdout, "%s", ans[0]);
    for (size_t i = 1; i < MIN(Nans, 3); ++i)
      fprintf(stdout, ", %s", ans[i]);
    if (Nans > 5)
      fprintf(stdout, ", ...");
    for (size_t i = MAX(MIN(Nans, 3), Nans - 2); i < Nans; ++i)
      fprintf(stdout, ", %s", ans[i]);
    fprintf(stdout, "\n");
  }

  struct sp_port *port;
  check(sp_get_port_by_name("COM1", &port));

  printf("Opening port.\n");
  check(sp_open(port, SP_MODE_READ_WRITE));

  printf("Setting port to 9600 8N1, no flow control.\n");
  check(sp_set_baudrate(port, 9600));
  check(sp_set_bits(port, 8));
  check(sp_set_parity(port, SP_PARITY_NONE));
  check(sp_set_stopbits(port, 1));
  check(sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE));

  char *data1 = "D1 220\r";
  int size1 = strlen(data1);
  char *data2 = "INC1\r";
  int size2 = strlen(data2);

  /* We'll allow a 1 second timeout for send and receive. */
  unsigned int timeout = 1000;

  /* On success, sp_blocking_write() and sp_blocking_read()
   * return the number of bytes sent/received before the
   * timeout expired. We'll store that result here. */
  int result;

  /* Send data. */
  printf("Sending '%s' (%d bytes) on port %s.\n", data1, size1, "COM1");
  result = check(sp_blocking_write(port, data1, size1, timeout));
  if (result == size2)
    printf("Sent %d bytes successfully.\n", size1);
  else
    printf("Timed out, %d/%d bytes sent.\n", result, size1);
  // printf("Sending '%s' (%d bytes) on port %s.\n", data2, size2, "COM1");
  // result = check(sp_blocking_write(port, data2, size2, timeout));
  // if (result == size2)
  //   printf("Sent %d bytes successfully.\n", size2);
  // else
  //   printf("Timed out, %d/%d bytes sent.\n", result, size2);
  /* Allocate a buffer to receive data. */
  char *buf = malloc(30 + 1);

  /* Try to receive the data on the other port. */
  printf("Receiving %d bytes on port.\n", 30);
  result = check(sp_blocking_read(port, buf, 30, timeout));

  /* Check whether we received the number of bytes we wanted. */

  printf("Timed out, %d bytes received.\n", result);

  /* Check if we received the same data we sent. */
  buf[result] = '\0';
  printf("Received '%s'.\n", buf);


  printf("Sending '%s' (%d bytes) on port %s. ", data2, size2, "COM1");
  result = check(sp_blocking_write(port, data2, size2, timeout));
  if (result == size2)
    printf("Sent %d bytes successfully.\n", size2);
  else
    printf("Timed out, %d/%d bytes sent.\n", result, size2);

  check(sp_close(port));
  sp_free_port(port);

WORKSPACE:;

  if (getenv("BASHLAB_MONGODB_VAR_STRING"))
    strcpy(mdb_var_str, getenv("BASHLAB_MONGODB_VAR_STRING"));

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
      bson_t *mdb_qry = BCON_NEW("variables", "{", "$exists", BCON_BOOL(true), "}");
      bson_t *mdb_doc = bson_new();
      bson_t mdb_doc_child1, mdb_doc_child2, mdb_doc_child3;
      BSON_APPEND_DOCUMENT_BEGIN(mdb_doc, "$push", &mdb_doc_child1);
      BSON_APPEND_DOCUMENT_BEGIN(&mdb_doc_child1, "variables", &mdb_doc_child2);
      BSON_APPEND_UTF8(&mdb_doc_child2, "name", mdb_var_str);
      BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child2, "value", &mdb_doc_child3);
      for (size_t i = 0; i < N1; ++i)
        bson_append_utf8(&mdb_doc_child3, "no", -1, out1[i], -1);
      bson_append_array_end(&mdb_doc_child2, &mdb_doc_child3);
      BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child2, "size", &mdb_doc_child3);
      bson_append_int64(&mdb_doc_child3, "no", -1, (int64_t)N1);
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
      bson_t *mdb_qry = BCON_NEW("variables.name", BCON_UTF8(mdb_var_str));
      bson_t *mdb_doc = bson_new();
      bson_t mdb_doc_child1, mdb_doc_child2, mdb_doc_child3;
      BSON_APPEND_DOCUMENT_BEGIN(mdb_doc, "$set", &mdb_doc_child1);
      BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child1, "variables.$.value", &mdb_doc_child2);
      for (size_t i = 0; i < N1; ++i)
        bson_append_utf8(&mdb_doc_child3, "no", -1, out1[i], -1);
      bson_append_array_end(&mdb_doc_child1, &mdb_doc_child2);
      BSON_APPEND_ARRAY_BEGIN(&mdb_doc_child1, "variables.$.size", &mdb_doc_child2);
      bson_append_int64(&mdb_doc_child2, "no", -1, (int64_t)N1);
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
  for (int i = N1; i < N1; i++)
    free(out1[i]);
  free(out1);

EXIT_OPERATION:;
  sp_free_port_list(port_list);

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

/* Helper function for error handling. */
int check(enum sp_return result)
{
  /* For this example we'll just exit on any error by calling abort(). */
  char *error_message;

  switch (result)
  {
  case SP_ERR_ARG:
    printf("Error: Invalid argument.\n");
    abort();
  case SP_ERR_FAIL:
    error_message = sp_last_error_message();
    printf("Error: Failed: %s\n", error_message);
    sp_free_error_message(error_message);
    abort();
  case SP_ERR_SUPP:
    printf("Error: Not supported.\n");
    abort();
  case SP_ERR_MEM:
    printf("Error: Couldn't allocate memory.\n");
    abort();
  case SP_OK:
  default:
    return result;
  }
}