#define PROGNAME "ws.list"
static const char *program_json =
    "{"
    "\"name\": \"ws.list\","
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

#include "typedefs.h"
#include "configs.h"
#include "macros.h"
#include "utility.h"

#include "argtable3.h"
#include "jansson.h"

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
static json_type var_type;
static size_t ivar_index;
static FILE *fin;
static FILE *fout;
static size_t argcount = 0;
static char buff[250];

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
      argv[i][0] = 126; // '-' to '~' avoiding argtable literal behaviour for negative numbers

  /* positional arg structs*/
  json_t *pargs = json_object_get(program, "pargs");
  json_array_foreach(pargs, ivar_index, ivar)
  {
    const char *name = json_string_value(json_object_get(ivar, "name"));
    int minc = json_integer_value(json_object_get(ivar, "minc"));
    int maxc = json_integer_value(json_object_get(ivar, "maxc"));
    const char *desc = json_string_value(json_object_get(ivar, "desc"));
    argtable[argcount++] = arg_strn(NULL, NULL, name, minc, maxc, desc);
  }

  /* optional arg structs*/
  json_t *oargs = json_object_get(program, "oargs");
  json_array_foreach(oargs, ivar_index, ivar)
  {
    const char *sh = json_string_value(json_object_get(ivar, "short"));
    const char *ln = json_string_value(json_object_get(ivar, "long"));
    const char *name = json_string_value(json_object_get(ivar, "name"));
    int minc = json_integer_value(json_object_get(ivar, "minc"));
    int maxc = json_integer_value(json_object_get(ivar, "maxc"));
    const char *desc = json_string_value(json_object_get(ivar, "desc"));
    argtable[argcount++] = arg_strn(sh, ln, name, minc, maxc, desc);
  }

  /* option arg structs*/
  json_t *opts = json_object_get(program, "opts");
  json_array_foreach(opts, ivar_index, ivar)
  {
    const char *sh = json_string_value(json_object_get(ivar, "short"));
    const char *ln = json_string_value(json_object_get(ivar, "long"));
    const char *desc = json_string_value(json_object_get(ivar, "desc"));
    argtable[argcount++] = arg_lit0(sh, ln, desc);
  }

  /* commong arg structs */
  struct arg_lit *help = arg_lit0(NULL, "help", "display this help and exit");
  struct arg_end *end = arg_end(20);
  argtable[argcount++] = help;
  argtable[argcount] = end;

  int arg_errors;
  arg_errors = arg_parse(argc, argv, argtable);

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

  /* minus number back in positional args */
  for (size_t i = 1; i < argc; i++)
    if (argv[i][0] == 126)
      argv[i][0] = 45; // '~' to '-'

  /* ======================================================================== */
  /* workspace                                                                */
  /* ======================================================================== */
  workspace = json_load_file(BLAB_WS, 0, json_error);
  if (workspace != NULL && json_typeof(workspace) != JSON_OBJECT)
  {
    fprintf(stderr, "%s: invalid workspace.\n", PROGNAME);
    fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }
  if (workspace == NULL)
  {
    workspace = json_object();
  }
  ws_vars = json_object_get(workspace, "variables");
  if (ws_vars != NULL && json_typeof(ws_vars) != JSON_ARRAY)
  {
    fprintf(stderr, "%s: invalid workspace variables.\n", PROGNAME);
    fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }
  if (ws_vars == NULL)
  {
    json_object_set_new(workspace, "variables", json_array());
    ws_vars = json_object_get(workspace, "variables");
  }
  ws_hist = json_object_get(workspace, "history");
  if (ws_hist != NULL && json_typeof(ws_hist) != JSON_ARRAY)
  {
    fprintf(stderr, "%s: invalid workspace variables.\n", PROGNAME);
    fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }
  if (ws_hist == NULL)
  {
    json_object_set_new(workspace, "history", json_array());
    ws_hist = json_object_get(workspace, "history");
  }

  /* ======================================================================== */
  /* main operation                                                           */
  /* ======================================================================== */
  struct arg_lit *arg_hist = (struct arg_lit *)argtable[0];

INPUT:;

OPERATION:;

OUTPUT:;
  size_t Nans = 1;

  /* stream */
  if (arg_hist->count < 1)
  {
    for (size_t i = 0; i < json_array_size(ws_vars); ++i)
    {
      var = json_array_get(ws_vars, i);
      var_name = json_object_get(var, "name");
      var_size = json_object_get(var, "size");
      var_val = json_object_get(var, "value");
      var_type = json_typeof(json_array_get(var_val, 0));
      fprintf(fout, "%-10s: ", json_string_value(var_name));
      sprintf(buff, "%s", var_type == JSON_REAL ? "number[" : "string[");
      for (size_t i = 0; i < json_array_size(var_size) - 1; ++i)
        sprintf(buff, "%s%lldx", buff, json_integer_value(json_array_get(var_size, i)));
      sprintf(buff, "%s%lld]", buff,json_integer_value(json_array_get(var_size, json_array_size(var_size) - 1)));
      fprintf(fout, "%-15s: ", buff);

      Nans = json_array_size(var_val);
      if (var_type == JSON_REAL)
      {
        fprintf(fout, "%.16G", json_real_value(json_array_get(var_val, 0)));
        for (size_t i = 1; i < MIN(Nans, 3); ++i)
          fprintf(fout, ", %.16G", json_real_value(json_array_get(var_val, i)));
        if (Nans > 5)
          fprintf(fout, ", ...");
        for (size_t i = MAX(MIN(Nans, 3), Nans - 2); i < Nans; ++i)
          fprintf(fout, ", %.16G", json_real_value(json_array_get(var_val, i)));
        fprintf(fout, "\n");
      }
      else
      {
        fprintf(fout, "%s", json_string_value(json_array_get(var_val, 0)));
        for (size_t i = 1; i < MIN(Nans, 3); ++i)
          fprintf(fout, ", %s", json_string_value(json_array_get(var_val, i)));
        if (Nans > 5)
          fprintf(fout, ", ...");
        for (size_t i = MAX(MIN(Nans, 3), Nans - 2); i < Nans; ++i)
          fprintf(fout, ", %s", json_string_value(json_array_get(var_val, i)));
        fprintf(fout, "\n");
      }
    }
  }
  else
  {
    for (size_t i = 0; i < json_array_size(ws_hist); ++i)
      fprintf(fout, "%s\n", json_string_value(json_array_get(ws_hist, i)));
  }

HISTORY:

/* ======================================================================== */
/* exit                                                                     */
/* ======================================================================== */
EXIT_OPERATION:;

EXIT_INPUT:;

EXIT_OUTPUT:;

EXIT:
  /* dereference json objects */
  if (workspace != NULL)
    json_decref(workspace);
  if (program != NULL)
    json_decref(program);

  /* deallocate each non-null entry in argtable[] */
  arg_freetable(argtable, argcount + 1); // +1 for end
  return exitcode;
}
