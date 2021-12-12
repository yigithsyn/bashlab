
#define PROGNAME "linspace"
#define VERSION_MAJOR 1
#define VERSION_MINOR 2
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

#include "macros.h"
#include "linspace.h"

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
  json_t *workspace = NULL, *ivar, *ws_vars;
  size_t ivar_index;

  /* ======================================================================== */
  /* argument parse                                                           */
  /* ======================================================================== */

  /* the global arg_xxx structs are initialised within the argtable */
  int Nposargs = 3;
  struct arg_str *posargs[3];
  struct arg_lit *help = arg_lit0(NULL, "help", "display this help and exit");
  struct arg_lit *version = arg_lit0(NULL, "version", "display version info and exit");
  posargs[0] = arg_str1(NULL, NULL, "start", "the starting value of the sequence.");
  posargs[1] = arg_str1(NULL, NULL, "stop", "the last point, included by defaults.");
  posargs[2] = arg_str1(NULL, NULL, "N", "number of sampling points.");
  struct arg_str *wsout = arg_str0("o", NULL, "<wsout>", "output argument name for workspace");
  struct arg_end *end = arg_end(20);
  void *argtable[] = {posargs[0], posargs[1], posargs[2], wsout, help, version, end};

  int nerrors;
  nerrors = arg_parse(argc, argv, argtable);

  /* special case: '--help' takes precedence over error reporting */
  if (help->count > 0)
  {
  HELP:
    printf("%s: Generate linearly spaced vector.\n\n", PROGNAME);
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

  /* ======================================================================== */
  /* main operation                                                           */
  /* ======================================================================== */

  /* ------------------------------------------------------------------------ */
  /* fetch input                                                              */
  /* ------------------------------------------------------------------------ */
  for (int i = 0; i < Nposargs; ++i)
    dargs[i] = realloc(dargs[i], sizeof(double));

  /* positional argument as value */
  if (stat(WORKSPACE, &stat_buff) != 0)
  {
    for (int i = 0; i < Nposargs; ++i)
      dargs[i][Ndargs[i]++] = atof(posargs[i]->sval[0]);
    goto OUTPUT;
  }

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
  /* search for variable */
  json_t *dvar, *var_val;
  ws_vars = json_object_get(workspace, "variables");
  for (int i = 1; i < argc; ++i)
  {
    json_array_foreach(ws_vars, ivar_index, ivar)
    {
      if (strcmp(json_string_value(json_object_get(ivar, "name")), posargs[i]->sval[0]) == 0)
        break;
    }
    if (ivar_index == json_array_size(json_object_get(workspace, "variables")))
      dargs[i-1][Ndargs[i-1]++] = atof(argv[i]);
    else
    {
      var_val = json_object_get(ivar, "value");
      /* validity check */
      if (var_val == NULL)
      {
        fprintf(stderr, "%s: variable value not found.\n", PROGNAME);
        fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
        exitcode = EXIT_FAILURE;
        goto EXIT;
      }
      if (json_typeof(var_val) != JSON_ARRAY)
      {
        fprintf(stderr, "%s: unsupported variable from workspace.\n", PROGNAME);
        fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
        exitcode = EXIT_FAILURE;
        goto EXIT;
      }
      if (json_array_size(var_val) != 1)
      {
        fprintf(stderr, "%s: %s should be real single scalar.\n", PROGNAME, json_string_value(json_object_get(ivar, "name")));
        fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
        exitcode = EXIT_FAILURE;
        goto EXIT;
      }
      /* process variable */
      if (json_typeof(json_array_get(var_val, 0)) == JSON_INTEGER)
        dargs[i-1][Ndargs[i-1]++] = (double)json_integer_value(json_array_get(var_val, 0));
      else if (json_typeof(json_array_get(var_val, 0)) == JSON_REAL)
        dargs[i-1][Ndargs[i-1]++] = json_real_value(json_array_get(var_val, 0));
      else
      {
        fprintf(stderr, "%s: %s should be number.\n", PROGNAME, json_string_value(json_object_get(ivar, "name")));
        fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
        exitcode = EXIT_FAILURE;
        goto EXIT;
      }
    }
  }

  /* ------------------------------------------------------------------------ */
  /* write output                                                             */
  /* ------------------------------------------------------------------------ */
OUTPUT:
  dbuff = realloc(dbuff, (int)dargs[2][0] * sizeof(double));
  linspace(dargs[0][0], dargs[1][0], (int)dargs[2][0], dbuff);

  /* workspace */
  /* check for file structure */
  workspace = (workspace == NULL) ? json_load_file(WORKSPACE, 0, json_error): workspace;
  if (workspace == NULL || json_typeof(workspace) != JSON_OBJECT)
    workspace = json_loads("{\"variables\": []}", 0, NULL);
  /* search for variable */
  json_t *new_var, *new_var_val, *new_var_vals;
  ws_vars = json_object_get(workspace, "variables");
  if(ws_vars == NULL){
    json_object_set_new(workspace,"variables", json_array());
    ws_vars = json_object_get(workspace, "variables");
  }
  if (wsout->count)
    strcpy(buff, wsout->sval[0]);
  else
    strcpy(buff, "ans");
  json_array_foreach(ws_vars, ivar_index, ivar) if (strcmp(json_string_value(json_object_get(ivar, "name")), buff) == 0) break;
  /* delete existing */
  if (ivar_index != json_array_size(ws_vars))
    json_array_remove(ws_vars, ivar_index);
  /* create new */
  new_var = json_object();
  json_object_set_new(new_var, "name", json_string(buff));
  new_var_vals = json_array();
  /* append results */
  for (int i = 0; i < dargs[2][0]; ++i)
    json_array_append_new(new_var_vals, json_real(dbuff[i]));
  json_object_set_new(new_var, "value", new_var_vals);
  json_array_append_new(ws_vars, new_var);
  /* write workspace */
  json_dump_file(workspace, WORKSPACE, JSON_INDENT(2));

  /* stdout */
  for (int i = 0; i < MIN((int)dargs[2][0], 3); ++i)
    fprintf(fout, "%f\n", dbuff[i]);
  if ((int)dargs[2][0] > 5)
    fprintf(fout, "...\n");
  for (int i = MAX(MIN((int)dargs[2][0], 3), (int)dargs[2][0] - 2); i < (int)dargs[2][0]; ++i)
    fprintf(fout, "%f\n", dbuff[i]);

/* ======================================================================== */
/* history                                                                     */
/* ======================================================================== */
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

  /* ======================================================================== */
  /* exit                                                                     */
  /* ======================================================================== */
EXIT:
  if (dbuff != NULL)
    free(dbuff);
  for (int i = 0; i < MAX_ARG_NUM; i++)
    free(dargs[i]);
  if (workspace != NULL)
    json_decref(workspace);

  /* deallocate each non-null entry in argtable[] */
  arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
  return exitcode;
}

/*
Version history:
1.0.0: Initial release
1.1.0: Support negative range boundaries
1.2.0: Reuse workspace object
*/