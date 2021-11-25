
#define PROGNAME "freq2wavelen"
#define VERSION_MAJOR 2
#define VERSION_MINOR 0
#define VERSION_PATCH 0

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
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

#include "freq2wavelen.h"

#define WORKSPACE "workspace.json"
#define MAX_LINE_BUFFER 100
#define MAX_ERR_BUFF_LEN 250

int main(int argc, char *argv[])
{
  char buff[MAX_LINE_BUFFER];
  char err_buff[MAX_ERR_BUFF_LEN];
  FILE *fin = NULL, *fout = stdout;
  int exitcode = EXIT_SUCCESS;
  double *darr = (double *)calloc(0, sizeof(double));
  struct stat stat_buff;
  json_error_t *json_error = NULL;
  json_t *workspace, *ivar, *ws_vars;
  size_t ivar_index;

  /* ======================================================================== */
  /* argument parse                                                           */
  /* ======================================================================== */

  /* the global arg_xxx structs are initialised within the argtable */
  struct arg_lit *help = arg_lit0(NULL, "help", "display this help and exit");
  struct arg_lit *version = arg_lit0(NULL, "version", "display version info and exit");
  struct arg_lit *human = arg_lit0("h", "human", "human readable display like 3.36 cm, 0.7 km");
  struct arg_str *posargs = arg_str1(NULL, NULL, "<freq>", "frequency in Hertz [Hz]");
  struct arg_str *wsout = arg_str0("o", NULL, "<wsout>", "output argument name for workspace");
  struct arg_end *end = arg_end(20);
  void *argtable[] = {posargs, human, wsout, help, version, end};

  int nerrors;
  nerrors = arg_parse(argc, argv, argtable);

  /* special case: '--help' takes precedence over error reporting */
  if (help->count > 0)
  {
  HELP:
    printf("%s: Convert frequency to wavelength.\n\n", PROGNAME);
    printf("Usage: %s", PROGNAME);
    arg_print_syntaxv(stdout, argtable, "\n");
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
  if (nerrors > 0)
  {
    /* Display the error details contained in the arg_end struct.*/
    arg_print_errors(stdout, end, PROGNAME);
    printf("Try '%s --help' for more information.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }

  /* ======================================================================== */
  /* main operation                                                           */
  /* ======================================================================== */

  /* ------------------------------------------------------------------------ */
  /* fetch input                                                              */
  /* ------------------------------------------------------------------------ */
  int N = 0, Nmax = 1;
  darr = realloc(darr, sizeof(double) * Nmax);

  /* positional argument as value */
  if (stat(WORKSPACE, &stat_buff) != 0)
  {
  ARGASVAL:
    darr[N++] = atof(posargs->sval[0]);
    goto OUTPUT;
  }

  /* workspace */
  /* check for file and structure */
  workspace = json_load_file(WORKSPACE, 0, json_error);
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
  json_array_foreach(ws_vars, ivar_index, ivar) if (strcmp(json_string_value(json_object_get(ivar, "name")), posargs->sval[0]) == 0) break;
  if (ivar_index == json_array_size(json_object_get(workspace, "variables")))
  {
    json_decref(workspace);
    goto ARGASVAL;
  }
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
  json_array_foreach(json_object_get(ivar, "value"), ivar_index, dvar)
  {
    if (N >= Nmax)
    {
      Nmax *= 2;
      darr = realloc(darr, sizeof(double) * Nmax);
    }
    darr[N++] = json_real_value(dvar);
  }
  json_decref(workspace);
  goto OUTPUT;

  /* ------------------------------------------------------------------------ */
  /* write output                                                             */
  /* ------------------------------------------------------------------------ */
OUTPUT:
  /* workspace */
  /* check for file structure */
  workspace = json_load_file(WORKSPACE, 0, json_error);
  if (workspace == NULL || json_typeof(workspace) != JSON_OBJECT)
    workspace = json_loads("{\"variables\": []}", 0, NULL);
  /* search for variable */
  json_t *new_var, *new_var_val, *new_var_vals;
  ws_vars = json_object_get(workspace, "variables");
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
  for (int i = 0; i < N; ++i)
    json_array_append_new(new_var_vals, json_real(freq2wavelen(darr[i])));
  json_object_set_new(new_var, "value", new_var_vals);
  json_array_append_new(ws_vars, new_var);
  /* write workspace */
  json_dump_file(workspace, WORKSPACE, JSON_INDENT(2));
  json_decref(workspace);

  /* stdout */
  for (int i = 0; i < min(N, 3); ++i)
    (human->count == 1) ? fprintf(fout, "%s\n", freq2wavelen_h(darr[i], buff))
                        : fprintf(fout, "%f\n", freq2wavelen(darr[i]));
  if (N > 5)
    fprintf(fout, "...\n");
  for (int i = max(min(N, 3), N - 2); i < N; ++i)
    (human->count == 1) ? fprintf(fout, "%s\n", freq2wavelen_h(darr[i], buff))
                        : fprintf(fout, "%f\n", freq2wavelen(darr[i]));

  /* ======================================================================== */
  /* history                                                                     */
  /* ======================================================================== */
HISTORY:
  workspace = json_load_file(WORKSPACE, 0, json_error);
  if (workspace == NULL || json_typeof(workspace) != JSON_OBJECT)
    workspace = json_loads("{\"history\": []}", 0, NULL);
  if (json_object_get(workspace, "history") == NULL)
    json_object_set_new(workspace, "history", json_array());
  strcpy(buff, PROGNAME);
  for (int i = 1; i < argc; i++)
    sprintf(buff, "%s %s\0", buff, argv[i]);
  json_array_append_new(json_object_get(workspace, "history"), json_string(buff));
  /* write workspace */
  json_dump_file(workspace, WORKSPACE, JSON_INDENT(2));
  json_decref(workspace);

  /* ======================================================================== */
  /* exit                                                                     */
  /* ======================================================================== */
EXIT:
  /* deallocate each non-null entry in argtable[] */
  free(darr);
  arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
  return exitcode;
}

/*
Version history:
1.0.0: Initial release
1.0.2: Input fetch fix
1.1.0: Argument support from workspace
1.2.0: Add command to workspace history
1.3.0: Always add result to history
2.0.0: Argument simplification by using only positional or workspace argument
*/