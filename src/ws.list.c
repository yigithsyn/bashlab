#define PROGNAME "ws.list"

#include "argtable3.h"
#include "jansson.h"

#include "configs.h"
#include "definitions.h"
#include "programs.h"
#include "macros.h"
#include "utility.h"

#include <stdio.h>
#include <stdlib.h>

static json_error_t *json_error;
static json_t *ivar, *ws_vars, *ws_hist, *var, *var_val;
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
  json_t *workspace = NULL, *workspace_val = NULL, *program_list = NULL;
  void *argtable[100];

  /* ======================================================================== */
  /* fetch program definitions                                                */
  /* ======================================================================== */
  program_list = json_loads(programs, 0, json_error);
  json_array_foreach(program_list, ivar_index, ivar)
  {
    if (strcmp(PROGNAME, json_string_value(json_object_get(ivar, "name"))) == 0)
      break;
  }
  json_t *program = ivar;

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
  struct arg_lit *version = arg_lit0(NULL, "version", "display version number and exit");
  struct arg_lit *versions = arg_lit0(NULL, "versions", "display all version infos and exit");
  struct arg_end *end = arg_end(20);
  argtable[argcount++] = help;
  argtable[argcount++] = version;
  argtable[argcount++] = versions;
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

  /* special case: '--version' takes precedence over error reporting */
  if (version->count > 0)
  {
    json_t *vers = json_object_get(program, "vers");
    json_t *last_ver = json_array_get(vers, json_array_size(vers) - 1);
    printf("%s\n", json_string_value(json_object_get(last_ver, "num")));
    goto EXIT;
  }

  /* special case: '--versions' takes precedence over error reporting */
  if (versions->count > 0)
  {
    json_t *vers = json_object_get(program, "vers");
    json_array_foreach(vers, ivar_index, ivar)
    {
      printf("%s: %s\n", json_string_value(json_object_get(ivar, "num")), json_string_value(json_object_get(ivar, "msg")));
    }
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
  workspace = json_load_file(WORKSPACE, 0, json_error);
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
  struct arg_lit *arg_hist = (struct arg_lit *)argtable[json_array_size(pargs) + json_array_size(oargs)];

INPUT:;

OPERATION:;

OUTPUT:;

  /* stream */
  if (arg_hist->count < 1)
  {
    for (size_t i = 0; i < json_array_size(ws_vars); ++i)
    {
      var = json_array_get(ws_vars, i);
      var_val = json_object_get(var, "name");
      fprintf(fout, "%-10s: ", json_string_value(var_val));
      var_val = json_object_get(var, "value");
      if (json_typeof(var_val) != JSON_ARRAY)
      {
        fprintf(stderr, "%s: unsupported variable from workspace.\n", PROGNAME);
        fprintf(stderr, "Check and fix workspace.\n\n");
        exitcode = EXIT_FAILURE;
        goto EXIT_OUTPUT;
      }
      if (json_array_size(var_val) > BLAB_WS_ARR_LIM)
      {
        sprintf(buff, "number[%zu]", (size_t)json_real_value(json_object_get(var, "size")));
        fprintf(fout, "%-15s: ", buff);
      }
      else
      {
        sprintf(buff, "number[%zu]", json_array_size(var_val));
        fprintf(fout, "%-15s: ", buff);
      }
      if (json_typeof(json_array_get(var_val, 0)) == JSON_REAL)
      {
        fprintf(fout, "%G", json_real_value(json_array_get(var_val, 0)));
        for (size_t i = 1; i < MIN(json_array_size(var_val), 3); ++i)
          fprintf(fout, ", %G", json_real_value(json_array_get(var_val, i)));
        if (json_array_size(var_val) > 5)
          fprintf(fout, ", ...");
        for (size_t i = MAX(MIN(json_array_size(var_val), 3), json_array_size(var_val) - 2); i < json_array_size(var_val); ++i)
          fprintf(fout, ", %G", json_real_value(json_array_get(var_val, i)));
        fprintf(fout, "\n");
      }
      else
      {
        fprintf(stderr, "%s: unsupported variable value.\n", PROGNAME);
        fprintf(stderr, "Check workspace.\n\n");
        exitcode = EXIT_FAILURE;
        goto EXIT_OUTPUT;
      }
    }
  }
  else
  {
    for (size_t i = 0; i < json_array_size(ws_hist); ++i)
      fprintf(fout, "%s\n", json_string_value(json_array_get(ws_hist, i)));
  }

HISTORY:
  // strcpy(buff, PROGNAME);
  // for (size_t i = 1; i < argc; i++)
  // {
  //   strcat(buff, " ");
  //   strcat(buff, argv[i]);
  // }
  // json_array_append_new(ws_hist, json_string(buff));
  // json_dump_file(workspace, WORKSPACE, JSON_COMPACT);

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
  if (workspace_val != NULL)
    json_decref(workspace_val);
  if (program_list != NULL)
    json_decref(program_list);

  /* deallocate each non-null entry in argtable[] */
  arg_freetable(argtable, argcount + 1); // +1 for end
  return exitcode;
}
