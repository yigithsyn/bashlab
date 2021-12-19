#define PROGNAME "s11toswr"

#include <stdio.h>
// #include <stdlib.h>
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

#include "configs.h"
#include "programs.h"
#include "macros.h"
#include "utility.h"

static struct stat statb; // file stat buffer
static json_error_t *json_error;
static json_t *ivar, *ws_vars, *ws_hist, *var_val;
static size_t ivar_index;
static FILE *fin;
static FILE *fout;
static int argcount = 0;
static char buff[250];
#define MAX_BUFF_ARR_LEN 50

int main(int argc, char *argv[])
{
  int exitcode = EXIT_SUCCESS;

  /* buffer variables */
  json_t *workspace = NULL, *program_list = NULL;
  void *argtable[MAX_ARG_NUM_ALL];

  double *dbuff[MAX_BUFF_ARR_LEN];
  int Ndargs[MAX_BUFF_ARR_LEN];
  for (int i = 0; i < MAX_BUFF_ARR_LEN; ++i)
  {
    dbuff[i] = (double *)calloc(0, sizeof(double));
    Ndargs[i] = 0;
  }

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
  for (int i = 1; i < argc; i++)
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
    int minc = json_integer_value(json_object_get(ivar, "minc"));
    int maxc = json_integer_value(json_object_get(ivar, "maxc"));
    const char *desc = json_string_value(json_object_get(ivar, "desc"));
    argtable[argcount++] = arg_litn(sh, ln, minc, maxc, desc);
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
  for (int i = 1; i < argc; i++)
    if (argv[i][0] == 126)
      argv[i][0] = 45; // '~' to '-'

  /* ======================================================================== */
  /* workspace                                                                */
  /* ======================================================================== */
  if (stat(WORKSPACE, &statb) == 0)
  {
    workspace = json_load_file(WORKSPACE, 0, json_error);
    if (workspace != NULL && json_typeof(workspace) != JSON_OBJECT)
    {
      fprintf(stderr, "%s: invalid workspace.\n", PROGNAME);
      fprintf(stderr, "Try '%s --help' for more information.\n\n", PROGNAME);
      exitcode = EXIT_FAILURE;
      goto EXIT;
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
  }
  /* ======================================================================== */
  /* main operation                                                           */
  /* ======================================================================== */
INPUT: ;
  /* s11 */
  struct arg_str *arg_s11 = (struct arg_str *)argtable[0];
  // struct arg_lit *arg_db = (struct arg_lit *)argtable[0];
  int Ns11 = 0, Nmaxs11 = 1;
  double *s11 = (double *)calloc(0, sizeof(double));
  for (int i = 0; i < arg_s11->count; ++i)
  {
    json_array_foreach(ws_vars, ivar_index, ivar) if (strcmp(json_string_value(json_object_get(ivar, "name")), arg_s11->sval[i]) == 0) break;
    if (ivar_index == json_array_size(ws_vars))
    {
      if (Ns11 >= Nmaxs11)
      {
        Nmaxs11 *= 2;
        s11 = realloc(s11, sizeof(double) * Nmaxs11);
      }
      s11[Ns11++] = atof(arg_s11->sval[i]);
    }
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
      /* process variable */
      for (int j = 0; j < json_array_size(var_val); ++j)
      {
        if (Ns11 >= Nmaxs11)
        {
          Nmaxs11 *= 2;
          s11 = realloc(s11, sizeof(double) * Nmaxs11);
        }
        if (json_typeof(json_array_get(var_val, j)) == JSON_INTEGER)
          s11[Ns11++] = (double)json_integer_value(json_array_get(var_val, j));
        else if (json_typeof(json_array_get(var_val, j)) == JSON_REAL)
          s11[Ns11++] = json_real_value(json_array_get(var_val, j));
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

OUTPUT: ;
  double *swr = (double *)calloc(0, sizeof(double) * Ns11);
  for (int i = 0; i < Ns11; i++)
  {
    printf("%.16f\n", s11[i]);
  }

HISTORY:
  strcpy(buff, PROGNAME);
  for (int i = 1; i < argc; i++)
  {
    strcat(buff, " ");
    strcat(buff, argv[i]);
  }
  json_array_append_new(ws_hist, json_string(buff));
  json_dump_file(workspace, WORKSPACE, JSON_INDENT(2));

/* ======================================================================== */
/* exit                                                                     */
/* ======================================================================== */
EXIT:
  /* release buffers */
  for (int i = 0; i < MAX_BUFF_ARR_LEN; i++)
    free(dbuff[i]);

  /* dereference json objects */
  if (workspace != NULL)
    json_decref(workspace);
  if (program_list != NULL)
    json_decref(program_list);

  /* deallocate each non-null entry in argtable[] */
  arg_freetable(argtable, argcount + 1); // +1 for end
  return exitcode;
}
