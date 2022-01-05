#define PROGNAME "linspace"
static const char *program_json =
    "{"
    "\"name\": \"linspace\","
    "\"desc\": \"generate linearly spaced array\","
    "\"vers\": ["
    /*        */ "{\"num\":\"1.0.0\", \"msg\":\"initial release\"},"
    /*        */ "{\"num\":\"1.0.1\", \"msg\":\"minimum interval number check\"},"
    /*        */ "{\"num\":\"1.1.0\", \"msg\":\"print large arrays into file\"},"
    /*        */ "{\"num\":\"2.0.0\", \"msg\":\"integer number of points parsing\"}"
    /*      */ "],"
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

#include "typedefs.h"
#include "configs.h"
#include "macros.h"
#include "utility.h"

#include "linspace.h"

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

int main(int argc, char *argv[])
{
  int exitcode = EXIT_SUCCESS;
  fout = stdout;
  /* buffer variables */
  json_t *workspace = NULL, *program_list = NULL;
  void *argtable[100];

  /* ======================================================================== */
  /* fetch program definitions                                                */
  /* ======================================================================== */
  json_t *program = json_loads(program_json, 0, json_error);

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
  struct arg_lit *version = arg_lit0(NULL, "version", "display version number_t and exit");
  struct arg_lit *versions = arg_lit0(NULL, "versions", "display all version infos and exit");
  struct arg_end *end = arg_end(20);
  argtable[argcount++] = ws_out;
  argtable[argcount++] = help;
  argtable[argcount++] = version;
  argtable[argcount++] = versions;
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

  /* minus number_t back in positional args */
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
  struct arg_str *arg_a = (struct arg_str *)argtable[0];
  struct arg_str *arg_b = (struct arg_str *)argtable[1];
  struct arg_str *arg_N = (struct arg_str *)argtable[2];

INPUTT:;
  /* a */
  number_t a;
  json_array_foreach(ws_vars, ivar_index, ivar) if (strcmp(json_string_value(json_object_get(ivar, "name")), arg_a->sval[0]) == 0) break;
  if (ivar_index == json_array_size(ws_vars))
    if (!isnumber(arg_a->sval[0]))
    {
      fprintf(stderr, "%s: %s should be real number.\n", PROGNAME, arg_a->sval[0]);
      fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
      exitcode = EXIT_FAILURE;
      goto EXIT_INPUT;
    }
    else
      a = atof(arg_a->sval[0]);
  else
  {
    var_name = json_object_get(ivar, "name");
    var_size = json_object_get(ivar, "size");
    var_val = json_object_get(ivar, "value");
    /* size check */
    if (json_array_size(var_size) != 1 || json_integer_value(json_array_get(var_size, 0)) != 1)
    {
      fprintf(stderr, "%s: %s should be scalar (single element array).\n", PROGNAME, json_string_value(var_name));
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
    a = json_real_value(json_array_get(var_val, 0));
  }

  /* b */
  number_t b;
  json_array_foreach(ws_vars, ivar_index, ivar) if (strcmp(json_string_value(json_object_get(ivar, "name")), arg_b->sval[0]) == 0) break;
  if (ivar_index == json_array_size(ws_vars))
    if (!isnumber(arg_b->sval[0]))
    {
      fprintf(stderr, "%s: %s should be number.\n", PROGNAME, arg_b->sval[0]);
      fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
      exitcode = EXIT_FAILURE;
      goto EXIT_INPUT;
    }
    else
      b = atof(arg_b->sval[0]);
  else
  {
    var_name = json_object_get(ivar, "name");
    var_size = json_object_get(ivar, "size");
    var_val = json_object_get(ivar, "value");
    /* size check */
    if (json_array_size(var_size) != 1 || json_integer_value(json_array_get(var_size, 0)) != 1)
    {
      fprintf(stderr, "%s: %s should be scalar (single element array).\n", PROGNAME, json_string_value(var_name));
      fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
      exitcode = EXIT_FAILURE;
      goto EXIT_INPUT;
    }
    /* type check */
    if (json_typeof(json_array_get(var_val, 0)) != JSON_REAL)
    {
      fprintf(stderr, "%s: %s should be number.\n", PROGNAME, json_string_value(json_object_get(ivar, "name")));
      fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
      exitcode = EXIT_FAILURE;
      goto EXIT_INPUT;
    }
    /* process variable */
    b = json_real_value(json_array_get(var_val, 0));
  }

  /* N */
  size_t N;
  json_array_foreach(ws_vars, ivar_index, ivar) if (strcmp(json_string_value(json_object_get(ivar, "name")), arg_N->sval[0]) == 0) break;
  if (ivar_index == json_array_size(ws_vars))
  {
    if (!isinteger(arg_N->sval[0]))
    {
      fprintf(stderr, "%s: %s should be integer.\n", PROGNAME, arg_N->sval[0]);
      fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
      exitcode = EXIT_FAILURE;
      goto EXIT_INPUT;
    }
    string_t endptr;
    N = strtoumax(arg_N->sval[0], &endptr, 10);
  }
  else
  {
    var_name = json_object_get(ivar, "name");
    var_size = json_object_get(ivar, "size");
    var_val = json_object_get(ivar, "value");
    /* size check */
    if (json_array_size(var_size) != 1 || json_integer_value(json_array_get(var_size, 0)) != 1)
    {
      fprintf(stderr, "%s: %s should be scalar (single element array).\n", PROGNAME, json_string_value(var_name));
      fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
      exitcode = EXIT_FAILURE;
      goto EXIT_INPUT;
    }
    /* type check */
    if (json_typeof(json_array_get(var_val, 0)) != JSON_REAL)
    {
      fprintf(stderr, "%s: %s should be number.\n", PROGNAME, json_string_value(json_object_get(ivar, "name")));
      fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
      exitcode = EXIT_FAILURE;
      goto EXIT_INPUT;
    }
    /* process variable */
    N = (size_t)json_real_value(json_array_get(var_val, 0));
  }

  /* operational check */
  if (N < 2)
  {
    fprintf(stderr, "%s: N should be at least 2.\n", PROGNAME);
    fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT_INPUT;
  }

OPERATION:;
  number_t *arr = (number_t *)calloc(N, sizeof(number_t));
  linspace(a, b, (int)N, arr);

OUTPUT:;
  size_t Nans = N;
  number_t *ans = arr;
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
  sprintf(buff, "%s_%s.txt", BLAB_WS_WO_EXT, json_string_value(json_object_get(var, "name")));
  json_object_set_new(var, "size", json_array());
  json_array_append_new(json_object_get(var, "size"), json_integer(Nans));
  if (Nans > BLAB_WS_ARR_LIM)
  {
    FILE *f = fopen(buff, "w");
    for (size_t i = 0; i < Nans; ++i)
      fprintf(f, "%.16f\n", ans[i]);
    fclose(f);
    for (size_t i = 0; i < BLAB_WS_ARR_LIM - 1; ++i)
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

  /* stream */
  fprintf(fout, "%.16G", ans[0]);
  for (size_t i = 1; i < MIN(Nans, 3); ++i)
    fprintf(fout, ", %.16G", ans[i]);
  if (Nans > 5)
    fprintf(fout, ", ...");
  for (size_t i = MAX(MIN(Nans, 3), Nans - 2); i < Nans; ++i)
    fprintf(fout, ", %.16G", ans[i]);
  fprintf(fout, "\n");

HISTORY:
  strcpy(buff, PROGNAME);
  for (size_t i = 1; i < argc; i++)
  {
    strcat(buff, " ");
    strcat(buff, argv[i]);
  }
  json_array_append_new(ws_hist, json_string(buff));
  json_dump_file(workspace, BLAB_WS, JSON_COMPACT);

/* ======================================================================== */
/* exit                                                                     */
/* ======================================================================== */
EXIT_OUTPUT:;
  // free(ans);

EXIT_OPERATION:;
  free(arr);

EXIT_INPUT:;
  // free(s11);

EXIT:
  /* dereference json objects */
  if (workspace != NULL)
    json_decref(workspace);
  if (program_list != NULL)
    json_decref(program_list);

  /* deallocate each non-null entry in argtable[] */
  arg_freetable(argtable, argcount + 1); // +1 for end
  return exitcode;
}
