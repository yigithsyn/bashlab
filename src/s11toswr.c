
#define PROGNAME "wsclr"
#define PROGDESC "Clear workspace"
#define PROGPOSA 0 // positional argument count

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 1

// #include <stdio.h>
// #include <stdbool.h>
// #include <stdlib.h>
#include <string.h>
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#include <sys/stat.h>
// #if defined(_WIN32)
// #include <io.h>
// #else
// #include <sys/io.h>
// #include <unistd.h>
// #include <errno.h>
// #endif

#include "argtable3.h"
#include "jansson.h"

#include "commands.h"
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
  /* fetch program definitions                                                */
  /* ======================================================================== */
printf("%s\n",__FILE__);
printf("%s\n",__FILENAME__);


  /* ======================================================================== */
  /* argument parse                                                           */
  /* ======================================================================== */

//   /* the global arg_xxx structs are initialised within the argtable */
//   struct arg_lit *help = arg_lit0(NULL, "help", "display this help and exit");
//   struct arg_lit *version = arg_lit0(NULL, "version", "display version info and exit");
//   struct arg_lit *history = arg_lit0("h", NULL, "clear history also");
//   struct arg_end *end = arg_end(20);
//   void *argtable[] = {help, history, version, end};

//   int nerrors;
//   nerrors = arg_parse(argc, argv, argtable);

//   /* special case: '--help' takes precedence over error reporting */
//   if (help->count > 0)
//   {
//   HELP:
//     printf("%s: %s.\n\n", PROGNAME, PROGDESC);
//     printf("Usage: %s", PROGNAME);
//     arg_print_syntaxv(stdout, argtable, "\n\n");
//     arg_print_glossary(stdout, argtable, "  %-25s %s\n");
//     goto EXIT;
//   }

//   /* special case: '--version' takes precedence over error reporting */
//   if (version->count > 0)
//   {
//     printf("%d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
//     goto EXIT;
//   }

//   /* If the parser returned any errors then display them and exit */
//   if (nerrors > 0)
//   {
//     /* Display the error details contained in the arg_end struct.*/
//     arg_print_errors(stdout, end, PROGNAME);
//     printf("Try '%s --help' for more information.\n", PROGNAME);
//     exitcode = EXIT_FAILURE;
//     goto EXIT;
//   }

//   /* ======================================================================== */
//   /* main operation                                                           */
//   /* ======================================================================== */

// INPUT:

// OUTPUT:

//   /* check for file structure */
//   workspace = (workspace == NULL) ? json_load_file(WORKSPACE, 0, json_error) : workspace;
//   if (!(workspace == NULL || json_typeof(workspace) != JSON_OBJECT))
//     json_object_del(workspace, "variables");

// HISTORY:
//   workspace = (workspace == NULL) ? json_load_file(WORKSPACE, 0, json_error) : workspace;
//   if (!(workspace == NULL || json_typeof(workspace) != JSON_OBJECT))
//   {
//     if (history->count > 0)
//     {
//       json_object_del(workspace, "history");
//       json_dump_file(workspace, WORKSPACE, JSON_INDENT(2));
//       goto EXIT;
//     }
//     if (workspace == NULL || json_typeof(workspace) != JSON_OBJECT)
//       workspace = json_loads("{\"history\": []}", 0, NULL);
//     if (json_object_get(workspace, "history") == NULL)
//       json_object_set_new(workspace, "history", json_array());
//     strcpy(buff, PROGNAME);
//     for (int i = 1; i < argc; i++)
//     {
//       strcat(buff, " ");
//       strcat(buff, argv[i]);
//     }
//     json_array_append_new(json_object_get(workspace, "history"), json_string(buff));
//     /* write workspace */
//     json_dump_file(workspace, WORKSPACE, JSON_INDENT(2));
//   }

//   /* ======================================================================== */
//   /* exit                                                                     */
//   /* ======================================================================== */
// EXIT:
//   if (dbuff != NULL)
//     free(dbuff);
//   for (int i = 0; i < MAX_ARG_NUM; i++)
//     free(dargs[i]);
//   if (workspace != NULL)
//     json_decref(workspace);

//   /* deallocate each non-null entry in argtable[] */
//   arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
  return exitcode;
}

/*
Version history:
1.0.0: Initial release
1.0.1: Fix for non-existing file case
*/