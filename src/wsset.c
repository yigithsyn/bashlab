
#define PROGNAME "wsset"
#define PROGDESC "Set workspace variable"
#define PROGPOSA 2 // positional argument count
#define PROGPOS0DEF "name"
#define PROGPOS0DESC "variable name"
#define PROGPOS1DEF "value"
#define PROGPOS1DESC "variable value(s) [string|number]"

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#if defined(_WIN32)
#include <io.h>
#else
#include <sys/io.h>
#include <unistd.h>
#include <errno.h>
#endif

#include "argtable3.h"
#include "jansson.h"

#include "utility.h"

#define WORKSPACE "workspace.json"
#define MAX_ARG_NUM 5
#define MAX_LINE_BUFFER 100
#define MAX_ERR_BUFF_LEN 250

int main(int argc, char *argv[])
{
  char buff[MAX_LINE_BUFFER];
  char err_buff[MAX_ERR_BUFF_LEN];
  FILE *fin = NULL, *fout = stdout;
  int exitcode = EXIT_SUCCESS;
  double *dbuff = NULL;
  double *dargs[MAX_ARG_NUM];
  int Ndargs[MAX_ARG_NUM];
  for (int i = 0; i < MAX_ARG_NUM; ++i)
  {
    dargs[i] = (double *)calloc(0, sizeof(double));
    Ndargs[i] = 0;
  }

  struct stat stat_buff;
  json_error_t *json_error = NULL;
  json_t *workspace = NULL, *ivar, *ws_vars, *var, *var_vals;
  size_t ivar_index;

  /* ======================================================================== */
  /* argument parse                                                           */
  /* ======================================================================== */

  /* the global arg_xxx structs are initialised within the argtable */
  struct arg_str *posargs[PROGPOSA];
  struct arg_lit *help = arg_lit0(NULL, "help", "display this help and exit");
  struct arg_lit *version = arg_lit0(NULL, "version", "display version info and exit");
  posargs[0] = arg_str1(NULL, NULL, PROGPOS0DEF, PROGPOS0DESC);
  posargs[1] = arg_strn(NULL, NULL, PROGPOS1DEF, 1, 100, PROGPOS1DESC);
  struct arg_end *end = arg_end(20);
  void *argtable[] = {posargs[0], posargs[1], help, version, end};

  int nerrors;
  nerrors = arg_parse(argc, argv, argtable);

  /* special case: '--help' takes precedence over error reporting */
  if (help->count > 0)
  {
  HELP:
    printf("%s: %s.\n\n", PROGNAME, PROGDESC);
    printf("Usage: %s", PROGNAME);
    arg_print_syntaxv(stdout, argtable, "\n\n");
    arg_print_glossary(stdout, argtable, "  %-25s %s\n");
    goto EXIT;
  }

  /* special case: '--version' takes precedence over error reporting */
  if (version->count > 0)
  {
    printf("%d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    goto EXIT;
  }

  /* If the parser returned any errors then display them and exit */
  if (posargs[0]->count == 0)
  {
    printf("%s: missing argument '%s'\n", PROGNAME, PROGPOS0DEF);
    printf("Try '%s --help' for more information.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }
  if (posargs[1]->count == 0)
  {
    printf("%s: missing argument '%s'\n", PROGNAME, PROGPOS0DEF);
    printf("Try '%s --help' for more information.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }

  /* ======================================================================== */
  /* main operation                                                           */
  /* ======================================================================== */

INPUT:
  int Nmax = 1;
  dargs[0] = realloc(dargs[0], sizeof(double) * Nmax);
  Ndargs[0] = 0;

  /* workspace */
  /* check for file and structure */
  workspace = (workspace == NULL) ? json_load_file(WORKSPACE, 0, json_error): workspace;
  if (workspace == NULL || json_typeof(workspace) != JSON_OBJECT)
  {
    fprintf(stderr, "%s: invalid workspace.\n", PROGNAME);
    fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }
  // /* search for variable */
  json_t *dvar, *var_val;
  ws_vars = json_object_get(workspace, "variables");
  for (int i = 2; i < argc; ++i)
  {
    json_array_foreach(ws_vars, ivar_index, ivar)
    {
      if (strcmp(json_string_value(json_object_get(ivar, "name")), argv[i]) == 0)
        break;
    }
    /* process command line argument */
    if (ivar_index == json_array_size(json_object_get(workspace, "variables")))
    {
      if (!isnumber(argv[i]))
      {
        fprintf(stderr, "%s: inconsistent array values.\n", PROGNAME);
        fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
        json_decref(workspace);
        exitcode = EXIT_FAILURE;
        goto EXIT;
      }
      if (Ndargs[0] >= Nmax)
      {
        Nmax *= 2;
        dargs[0] = realloc(dargs[0], sizeof(double) * Nmax);
      }
      dargs[0][Ndargs[0]++] = atof(argv[i]);
    }
    /* process argument from workspace */
    else
    {
      var_val = json_object_get(ivar, "value");
      /* validity check */
      if (var_val == NULL)
      {
        fprintf(stderr, "%s: variable value not found.\n", PROGNAME);
        fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
        json_decref(workspace);
        exitcode = EXIT_FAILURE;
        goto EXIT;
      }
      if (json_typeof(var_val) != JSON_ARRAY)
      {
        fprintf(stderr, "%s: unsupported variable from workspace.\n", PROGNAME);
        fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
        json_decref(workspace);
        exitcode = EXIT_FAILURE;
        goto EXIT;
      }
      /* process variable */
      for (int j = 0; j < json_array_size(var_val); ++j)
      {
        if (Ndargs[0] >= Nmax)
        {
          Nmax *= 2;
          dargs[0] = realloc(dargs[0], sizeof(double) * Nmax);
        }
        if (json_typeof(json_array_get(var_val, j)) == JSON_INTEGER)
          dargs[0][Ndargs[0]++] = (double)json_integer_value(json_array_get(var_val, j));
        else if (json_typeof(json_array_get(var_val, j)) == JSON_REAL)
          dargs[0][Ndargs[0]++] = json_real_value(json_array_get(var_val, j));
        else
        {
          fprintf(stderr, "%s: %s should be number.\n", PROGNAME, json_string_value(json_object_get(ivar, "name")));
          fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
          json_decref(workspace);
          exitcode = EXIT_FAILURE;
          goto EXIT;
        }
      }
    }
  }

OUTPUT:

OUTPUT_WORKSPACE:
  /* check for file structure */
  workspace = (workspace == NULL) ? json_load_file(WORKSPACE, 0, json_error): workspace;
  if (workspace == NULL || json_typeof(workspace) != JSON_OBJECT)
    workspace = json_loads("{\"variables\": []}", 0, NULL);
  ws_vars = json_object_get(workspace, "variables");

  /* delete existing */
  json_array_foreach(ws_vars, ivar_index, ivar)
  {
    if (strcmp(json_string_value(json_object_get(ivar, "name")), posargs[0]->sval[0]) == 0)
      break;
  }
  if (ivar_index != json_array_size(ws_vars))
    json_array_remove(ws_vars, ivar_index);
  /* create new */
  var = json_object();
  json_object_set_new(var, "name", json_string(posargs[0]->sval[0]));
  /* assign values */
  var_vals = json_array();
  for (int i = 0; i < Ndargs[0]; ++i)
  {
    json_array_append_new(var_vals, json_real(dargs[0][i]));
  }
  json_object_set_new(var, "value", var_vals);
  json_array_append_new(ws_vars, var);
  /* write workspace */
  json_dump_file(workspace, WORKSPACE, JSON_INDENT(2));

HISTORY:
  workspace = (workspace == NULL) ? json_load_file(WORKSPACE, 0, json_error): workspace;
  if (workspace == NULL || json_typeof(workspace) != JSON_OBJECT)
    workspace = json_loads("{\"history\": []}", 0, NULL);
  if (json_object_get(workspace, "history") == NULL)
    json_object_set_new(workspace, "history", json_array());
  strcpy(buff, PROGNAME);
  for (int i = 1; i < argc; i++)
  {
    strcat(buff, " ");
    strcat(buff, argv[i]);
  }
  json_array_append_new(json_object_get(workspace, "history"), json_string(buff));
  /* write workspace */
  json_dump_file(workspace, WORKSPACE, JSON_INDENT(2));
  json_decref(workspace);

  /* ======================================================================== */
  /* exit                                                                     */
  /* ======================================================================== */
EXIT:
  if (dbuff != NULL)
    free(dbuff);
  for (int i = 0; i < MAX_ARG_NUM; i++)
    free(dargs[i]);

  /* deallocate each non-null entry in argtable[] */
  arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
  return exitcode;
}

/*
Version history:
1.0.0: Initial release
*/