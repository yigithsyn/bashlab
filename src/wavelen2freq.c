
#define PROGNAME "wavelen2freq"
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 1

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#if defined(_WIN32)
#include <io.h>
#else
#include <sys/io.h>
#include <unistd.h>
#include <errno.h>
#endif

#include "argtable3.h"
#include "wavelen2freq.h"

#define MAX_LINE_BUFFER 100
#define MAX_ERR_BUFF_LEN 250

int main(int argc, char *argv[])
{
  char err_buff[MAX_ERR_BUFF_LEN];
  FILE *fin = NULL, *fout = stdout;
  int exitcode = EXIT_SUCCESS;
  double *darr = (double *)calloc(0, sizeof(double));

  /* ======================================================================== */
  /* argument parse                                                           */
  /* ======================================================================== */

  /* the global arg_xxx structs are initialised within the argtable */
  struct arg_lit *help = arg_lit0(NULL, "help", "display this help and exit");
  struct arg_lit *version = arg_lit0(NULL, "version", "display version info and exit");
  struct arg_lit *human = arg_lit0("h", "human", "human readable output like 1.36 GHz, 512 MHz");
  struct arg_dbl *dpos = arg_dbl0(NULL, NULL, "<wavelen>", "wavelength in meters [m]");
  struct arg_file *fileinp = arg_file0(NULL, "finp", "<fileinp>", "input file for column-wise vector operation");
  struct arg_file *fileout = arg_file0(NULL, "fout", "<fileout>", "output file for result storage");
  struct arg_str *wsinp = arg_str0("i", NULL, "<wsinp>", "input argument name from workspace");
  struct arg_str *wsout = arg_str0("o", NULL, "<wsout>", "output argument name for workspace");
  struct arg_end *end = arg_end(20);
  void *argtable[] = {dpos, wsinp, fileinp, human, wsout, fileout, help, version, end};

  int nerrors;
  nerrors = arg_parse(argc, argv, argtable);

  /* special case: '--help' takes precedence over error reporting */
  if (help->count > 0)
  {
HELP:
    printf("\nUsage: ");
    // arg_print_syntaxv(stdout, argtable, "\n");
    printf("%s <wavelen> [-h|--human] [-o|--out=<fileout>] \n", PROGNAME);
    printf("       %s -i=<wsinp> [-h|--human] [-o=<wsout>] [--fout<fileout>]\n", PROGNAME);
    printf("       %s --finp=<fileinp> [-h|--human] [-o=<wsout>] [--fout<fileout>]\n", PROGNAME);
    printf(">>     %s [-h|--human] [-o=<wsout>] [--fout<fileout>] (stdin pipe)\n", PROGNAME);
    printf("       %s [--help] [--version]\n\n", PROGNAME);
    printf("Convert wavelength to frequency.\n\n");
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
    // printf("Try '%s --help' for more information.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto HELP;
  }
  /* Insufficient argument.*/
  if (argc == 1
#if defined(_WIN32)
      && _isatty(_fileno(stdin))
#else
      && isatty(fileno(stdin))
#endif
  )
  {
    printf("%s: insufficient argument.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto HELP;
  }

  /* Argument conflict.*/
  if ((dpos->count + wsinp->count + fileinp->count
#if defined(_WIN32)
      + !_isatty(_fileno(stdin))
#else
      + !isatty(fileno(stdin))
#endif
  ) > 1)
  {
    printf("%s: input argument conflict.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto HELP;
  }
  /* ======================================================================== */
  /* main operation                                                           */
  /* ======================================================================== */

 /* ------------------------------------------------------------------------ */
  /* fetch input                                                              */
  /* ------------------------------------------------------------------------ */
  char buff[MAX_LINE_BUFFER];
  int N = 0, Nmax = 1;
  darr = realloc(darr, sizeof(double) * Nmax);

INPUT:
  /* argument */
  if (dpos->count == 1)
  {
    darr[N++] = dpos->dval[0];
    goto OUTPUT;
  }

  /* stdin */
#if defined(_WIN32)
  if (!_isatty(_fileno(stdin)))
#else
  if (!isatty(fileno(stdin)))
#endif
  {
    while (fgets(buff, 100, stdin))
    {
      if (N >= Nmax)
      {
        Nmax *= 2;
        darr = realloc(darr, sizeof(double) * Nmax);
      }
      darr[N++] = atof(buff);
    }
    goto OUTPUT;
  }

  /* file argument*/
  if (fileinp->count == 1)
  {
    fin = fopen(fileinp->filename[0], "r");
    if (fin == NULL)
    {
      sprintf(err_buff, "%s: Error %d: Unable to open input file '%s'", PROGNAME, errno, fileinp->filename[0]);
      perror(err_buff);
      exitcode = EXIT_FAILURE;
      goto EXIT;
    }

    // -1 to allow room for NULL terminator for really long string
    while (fgets(buff, MAX_LINE_BUFFER - 1, fin))
    {
      if (N >= Nmax)
      {
        Nmax *= 2;
        darr = realloc(darr, sizeof(double) * Nmax);
      }
      darr[N++] = atof(buff);
    }
    fclose(fin);
    goto OUTPUT;
  }

  /* ------------------------------------------------------------------------ */
  /* write output                                                             */
  /* ------------------------------------------------------------------------ */
OUTPUT:
  /* file */
  if (fileout->count == 1 && wsout->count == 0)
  {
    fout = fopen(fileout->filename[0], "w");
    if (fout == NULL)
    {
      sprintf(err_buff, "%s: Error %d: Unable to open output file '%s'",
              PROGNAME, errno, fileout->filename[0]);
      perror(err_buff);
      exitcode = 1;
      goto EXIT;
    }
  }

  for (int i = 0; i < N; ++i)
    (human->count == 1) ? fprintf(fout, "%s\n", wavelen2freq_h(darr[i], buff))
                        : fprintf(fout, "%f\n", wavelen2freq(darr[i]));

  if (fileout->count == 1)
    fclose(fout);

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
1.0.1: Input fetch fix
*/