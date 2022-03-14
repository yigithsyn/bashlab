#define PROGNAME "freq2wavelen"
static const char *program_json =
    "{"
    "\"name\": \"freq2wavelen\","
    "\"desc\": \"convert frequency to wavelength\","
    "\"pargs\": ["
    /*        */ "{\"name\":\"freq\", \"minc\":1, \"maxc\":100, \"desc\":\"freqeuncy value(s)\"}"
    /*       */ "],"
    "\"oargs\": ["
    /*        */ ""
    /*       */ "],"
    "\"opts\": ["
    /*       */ "{\"short\":\"h\", \"long\":\"human\", \"desc\":\"human readable display like 3.36 cm, 0.7 km\"}"
    /*      */ "]"
    "}";

#include "typedefs.h"
#include "configs.h"
#include "macros.h"
#include "utility.h"
#include "constants.h"

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
static size_t ivar_index;
static FILE *fin;
static FILE *fout;
static size_t argcount = 0;
static char buff[250];

char *human(number_t arg, char *buff);

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

  /* option arg structs*/
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
  workspace = json_load_file(BL_WORKSPACE, 0, json_error);
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
  struct arg_str *arg1 = (struct arg_str *)argtable[0]; // freqs
  struct arg_lit *arg2 = (struct arg_lit *)argtable[1]; // human

INPUTT:;
  /* freqs */
  int N = 0, Nmax = 1;
  number_t *inp1 = (number_t *)calloc(Nmax, sizeof(number_t));
  for (size_t i = 0; i < arg1->count; ++i)
  {
    json_array_foreach(ws_vars, ivar_index, ivar) if (strcmp(json_string_value(json_object_get(ivar, "name")), arg1->sval[i]) == 0) break;
    if (ivar_index == json_array_size(ws_vars))
    {
      if (!isnumber(arg1->sval[i]))
      {
        fprintf(stderr, "%s: %s should be real number.\n", PROGNAME, arg1->sval[i]);
        fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
        exitcode = EXIT_FAILURE;
        goto EXIT_INPUT;
      }
      else
      {
        if (N >= Nmax)
        {
          Nmax *= 2;
          inp1 = realloc(inp1, sizeof(number_t) * Nmax);
        }
        inp1[N++] = atof(arg1->sval[i]);
      }
    }
    else
    {
      var_name = json_object_get(ivar, "name");
      var_size = json_object_get(ivar, "size");
      var_val = json_object_get(ivar, "value");
      /* size check */
      if (json_array_size(var_size) != 1)
      {
        fprintf(stderr, "%s: %s should be scalar or 1D array.\n", PROGNAME, json_string_value(var_name));
        fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
        exitcode = EXIT_FAILURE;
        goto EXIT_INPUT;
      }
      /* type check */
      if (json_typeof(json_array_get(var_val, 0)) != JSON_REAL)
      {
        fprintf(stderr, "%s: %s should be real number.\n", PROGNAME, json_string_value(json_object_get(ivar, "name")));
        fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
        exitcode = EXIT_FAILURE;
        goto EXIT_INPUT;
      }
      /* process variable */
      Nmax = N + json_integer_value(json_array_get(var_size, 0));
      inp1 = realloc(inp1, sizeof(number_t) * Nmax);
      if (json_integer_value(json_array_get(var_size, 0)) > BL_WORKSPACE_ARRAY_LIMIT)
      {
        sprintf(buff, "%s_%s.txt", BL_WORKSPACE_WO_EXT, json_string_value(var_name));
        number_t *arr = (number_t *)calloc(Nmax, sizeof(number_t));
        read_number_data_file(buff, arr);
        for (size_t j = 0; j < json_integer_value(json_array_get(var_size, 0)); ++j)
          inp1[N++] = arr[j];
        free(arr);
      }
      else
        for (size_t j = 0; j < json_array_size(var_val); ++j)
          inp1[N++] = json_real_value(json_array_get(var_val, j));
    }
  }

OPERATION:;
  number_t *out1 = (number_t *)calloc(N, sizeof(number_t));
  if (verbose->count)
    fprintf(stdout, "Operation: %ld ... ", tic());
  for (int i = 0; i < N; ++i)
    out1[i] = BLAB_C0 / inp1[i];
  if (verbose->count)
    fprintf(stdout, "%ld [ms]\n", toc());

OUTPUT:;
  size_t Nans = N;
  number_t *ans = out1;
  /* workspace */
  if (ws_out->count)
    strcpy(buff, ws_out->sval[0]);
  else
    strcpy(buff, "ans");
  json_array_foreach(ws_vars, ivar_index, ivar) if (strcmp(json_string_value(json_object_get(ivar, "name")), buff) == 0) break;
  /* delete existing */
  if (ivar_index != json_array_size(ws_vars))
    json_array_remove(ws_vars, ivar_index);
  /* create new */
  var = json_object();
  json_object_set_new(var, "name", json_string(buff));
  var_val = json_array();
  /* write out large arrays seperately */
  sprintf(buff, "%s_%s.txt", BL_WORKSPACE_WO_EXT, json_string_value(json_object_get(var, "name")));
  json_object_set_new(var, "size", json_array());
  json_array_append_new(json_object_get(var, "size"), json_integer(Nans));
  if (Nans > BL_WORKSPACE_ARRAY_LIMIT)
  {
    if (verbose->count)
      fprintf(stdout, "Output: File: %ld ... ", tic());
    FILE *f = fopen(buff, "w");
    for (size_t i = 0; i < Nans; ++i)
      fprintf(f, "%.16E\n", ans[i]);
    fclose(f);
    if (verbose->count)
      fprintf(stdout, "%ld [ms]\n", toc());
    for (size_t i = 0; i < BL_WORKSPACE_ARRAY_LIMIT - 1; ++i)
      json_array_append_new(var_val, json_real(ans[i]));
    for (size_t i = Nans - 2; i < Nans; ++i)
      json_array_append_new(var_val, json_real(ans[i]));
  }
  else
  {
    if (stat(buff, &stat_buff) == 0)
    {
      if (remove(buff) != 0)
      {
        fprintf(stderr, "%s: Error in deleting workspace variable '%s' file: %s\n", PROGNAME, json_string_value(json_object_get(var, "name")), strerror(errno));
        exitcode = EXIT_FAILURE;
        goto EXIT_OUTPUT;
      }
    }
    /* append results */
    for (size_t i = 0; i < Nans; ++i)
      json_array_append_new(var_val, json_real(ans[i]));
  }
  json_object_set_new(var, "value", var_val);
  json_array_append_new(ws_vars, var);

HISTORY:
  strcpy(buff, PROGNAME);
  for (size_t i = 1; i < argc; i++)
  {
    strcat(buff, " ");
    strcat(buff, argv[i]);
  }
  json_array_append_new(ws_hist, json_string(buff));
  json_dump_file(workspace, BL_WORKSPACE, JSON_COMPACT);

STDOUT:
  /* stream */
  if (arg2->count > 0)
  {
    fprintf(fout, "%s", human(ans[0], buff));
    for (size_t i = 1; i < MIN(Nans, 3); ++i)
      fprintf(fout, ", %s", human(ans[i], buff));
    if (Nans > 5)
      fprintf(fout, ", ...");
    for (size_t i = MAX(MIN(Nans, 3), Nans - 2); i < Nans; ++i)
      fprintf(fout, ", %s", human(ans[i], buff));
    fprintf(fout, "\n");
  }
  else
  {
    fprintf(fout, "%.16G", ans[0]);
    for (size_t i = 1; i < MIN(Nans, 3); ++i)
      fprintf(fout, ", %.16G", ans[i]);
    if (Nans > 5)
      fprintf(fout, ", ...");
    for (size_t i = MAX(MIN(Nans, 3), Nans - 2); i < Nans; ++i)
      fprintf(fout, ", %.16G", ans[i]);
    fprintf(fout, "\n");
  }

/* ======================================================================== */
/* exit                                                                     */
/* ======================================================================== */
EXIT_OUTPUT:;
  // free(ans);

EXIT_OPERATION:;
  free(out1);

EXIT_INPUT:;
  free(inp1);

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
