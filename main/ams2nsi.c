#define PROGNAME "ams2nsi"
static const char *program_json =
    "{"
    "\"name\": \"ams2nsi\","
    "\"desc\": \"Convert ASYSOL AMS CSV data file to NSI importable formats\","
    "\"pargs\": ["
    /*        */ "{\"name\":\"file\", \"minc\":1, \"maxc\":1, \"desc\":\"AMS csv data file\"}"
    /*       */ "],"
    "\"oargs\": ["
    /*       */ "],"
    "\"opts\": ["
    /*      */ "]"
    "}";

#include <math.h>

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
static size_t Nans = 0;
static char buff[250];

int main(int argc, char *argv[])
{
  int exitcode = EXIT_SUCCESS;
  fout = stdout;
  /* buffer variables */
  json_t *workspace = NULL, *program = NULL;
  void *argtable[100];
  number_t *ans = NULL;

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
  struct arg_str *arg_file = (struct arg_str *)argtable[0];

INPUTT:;
  /* check input file */
  if (stat(arg_file->sval[0], &stat_buff) != 0)
  {
    fprintf(stderr, "%s: %s: %s\n", PROGNAME, strerror(errno), arg_file->sval[0]);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }
  fin = fopen(arg_file->sval[0], "r");
  if (fin == NULL)
  {
    fprintf(stderr, "%s: %s: %s\n", PROGNAME, strerror(errno), arg_file->sval[0]);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }

  size_t len = 250;
  char line[250];
  char *token = 0;
  /* calculate number of frequencies */
  size_t Nfreq = 0;
  number_t fstart = 0;
  token = fgets(line, len, fin); // trivial output for compiler
  while (fgets(line, len, fin))
  {
    token = strtok(line, ", ");
    for (size_t i = 0; i < 3; ++i)
      token = strtok(NULL, ", ");
    if (fstart == 0.0)
      fstart = atof(token);
    else if (fstart == atof(token))
      break;
    Nfreq++;
  }

  /* calculate step size */
  size_t Nscan = 0;
  rewind(fin);
  token = fgets(line, len, fin); // trivial output for compiler
  number_t step_prev = 0, step_next = 0;
  while (fgets(line, len, fin))
  {
    token = strtok(line, ", ");
    for (size_t i = 0; i < 1; ++i)
      token = strtok(NULL, ", ");
    if (step_prev == 0.0)
      step_prev = atof(token);
    else if (step_prev != atof(token))
      break;
    Nscan++;
  }
  Nscan /= Nfreq;
  step_next = atof(token);

  /* calculate scan size */
  rewind(fin);
  token = fgets(line, len, fin); // trivial output for compiler
  number_t scan_prev = 0, scan_next = 0;
  while (fgets(line, len, fin))
  {
    token = strtok(line, ", ");
    for (size_t i = 0; i < 2; ++i)
      token = strtok(NULL, ", ");
    if (scan_prev == 0.0)
      scan_prev = atof(token);
    else if (scan_prev != atof(token))
      break;
  }
  scan_next = atof(token);

  /* fetch data */
  size_t Nline = 0, Nmax = 1;
  number_t *pol = (number_t *)calloc(Nmax, sizeof(number_t));
  number_t *step = (number_t *)calloc(Nmax, sizeof(number_t));
  number_t *scan = (number_t *)calloc(Nmax, sizeof(number_t));
  number_t *freq = (number_t *)calloc(Nmax, sizeof(number_t));
  number_t *amp = (number_t *)calloc(Nmax, sizeof(number_t));
  number_t *phas = (number_t *)calloc(Nmax, sizeof(number_t));
  rewind(fin);
  token = fgets(line, len, fin); // trivial output for compiler
  while (fgets(line, len, fin))
  {
    if (Nline >= Nmax)
    {
      Nmax *= 2;
      pol = realloc(pol, Nmax * sizeof(number_t));
      step = realloc(step, Nmax * sizeof(number_t));
      scan = realloc(scan, Nmax * sizeof(number_t));
      freq = realloc(freq, Nmax * sizeof(number_t));
      amp = realloc(amp, Nmax * sizeof(number_t));
      phas = realloc(phas, Nmax * sizeof(number_t));
    }
    token = strtok(line, ", ");
    if (token != NULL)
      pol[Nline] = atof(token);
    token = strtok(NULL, ", ");
    if (token != NULL)
      step[Nline] = atof(token);
    token = strtok(NULL, ", ");
    if (token != NULL)
      scan[Nline] = atof(token);
    token = strtok(NULL, ", ");
    if (token != NULL)
      freq[Nline] = atof(token);
    token = strtok(NULL, ", ");
    if (token != NULL)
      amp[Nline] = atof(token);
    token = strtok(NULL, ", ");
    if (token != NULL)
      phas[Nline++] = atof(token);
  }
  fclose(fin);

  if (verbose->count > 0)
  {
    for (size_t i = 0; i < Nfreq; i++)
      for (size_t j = 0; j < Nline / Nfreq; j++)
        printf("%f %f %f %f %f %f\n", pol[j * Nfreq + i], step[j * Nfreq + i], scan[j * Nfreq + i], freq[j * Nfreq + i], pow(10.0, amp[j * Nfreq + i] / 20) * cos(phas[j * Nfreq + i] / 180 * BLAB_PI), pow(10.0, amp[j * Nfreq + i] / 20) * sin(phas[j * Nfreq + i] / 180 * BLAB_PI));
  }
  printf("Number of frequencies: %zu\n", Nfreq);
  printf("Step size: %f\n", step_next - step_prev);
  printf("Scan size: %f\n", scan_next - scan_prev);
  printf("Number of datas: %zu\n", Nline);
  printf("Number of steps: %zu\n", Nline / (Nfreq * Nscan * 2));
  printf("Number of scans: %zu\n", Nscan);

OPERATION:;

OUTPUT:;
  for (size_t i = 0; i < Nfreq; i++)
  {
    sprintf(buff, "Beam_%zu.txt", i + 1);
    fout = fopen(buff, "w");
    fprintf(fout, "#pol\t\tstep\t\tscan\t\treal\t\timag\n");
    for (size_t j = 0; j < Nline / Nfreq; j++)
      fprintf(fout, "%f\t\t%f\t\t%f\t\t%f\t\t%f\n", pol[j * Nfreq + i], step[j * Nfreq + i], scan[j * Nfreq + i], pow(10.0, amp[j * Nfreq + i] / 20) * cos(phas[j * Nfreq + i] / 180 * BLAB_PI), pow(10.0, amp[j * Nfreq + i] / 20) * sin(phas[j * Nfreq + i] / 180 * BLAB_PI));
    fclose(fout);
  }

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
// /* stream */
// fprintf(fout, "%.16G", ans[0]);
// for (size_t i = 1; i < MIN(Nans, 3); ++i)
//   fprintf(fout, ", %.16G", ans[i]);
// if (Nans > 5)
//   fprintf(fout, ", ...");
// for (size_t i = MAX(MIN(Nans, 3), Nans - 2); i < Nans; ++i)
//   fprintf(fout, ", %.16G", ans[i]);
// fprintf(fout, "\n");

/* ======================================================================== */
/* exit                                                                     */
/* ======================================================================== */
EXIT_OUTPUT:;

EXIT_OPERATION:;

EXIT_INPUT:;
  free(pol);
  free(step);
  free(scan);
  free(freq);
  free(amp);
  free(phas);

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
