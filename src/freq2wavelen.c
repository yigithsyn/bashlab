
#define PROGNAME "freq2wavelen"
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
#include "freq2wavelen.h"

#define MAX_LINE_BUFFER 100
#define MAX_ERR_BUFF_LEN 250

int main(int argc, char *argv[])
{
  char err_buff[MAX_ERR_BUFF_LEN];
  FILE *fin = NULL, *fout = stdout;
  int exitcode = EXIT_SUCCESS;
  /* ======================================================================== */
  /* argument parse                                                           */
  /* ======================================================================== */

  /* the global arg_xxx structs are initialised within the argtable */
  struct arg_lit *help = arg_lit0(NULL, "help", "display this help and exit");
  struct arg_lit *version = arg_lit0(NULL, "version", "display version info and exit");
  struct arg_lit *human = arg_lit0("h", "human", "human readable output like 3.36 cm, 0.7 km");
  struct arg_dbl *freq = arg_dbl0(NULL, NULL, "<freq>", "frequency in Hertz [Hz]");
  struct arg_file *fileinp = arg_file0("f", "file", "<fileinp>", "input file for column-wise vector operation");
  struct arg_file *fileout = arg_file0("o", "out", "<fileout>", "output file for result storage");
  struct arg_end *end = arg_end(20);
  void *argtable[] = {freq, fileinp, human, fileout, help, version, end};

  int nerrors;
  nerrors = arg_parse(argc, argv, argtable);

  /* special case: '--help' takes precedence over error reporting */
  if (help->count > 0)
  {
HELP:
    printf("\nUsage: ");
    // arg_print_syntaxv(stdout, argtable, "\n");
    printf("%s <freq> [-h|--human] [-o|--out=<fileout>] \n", PROGNAME);
    printf("       %s -f|--file=<fileinp> [-h|--human] [-o|--out=<fileout>]\n", PROGNAME);
    printf(">>     %s [-h|--human] [-o|--out=<fileout>] (stdin pipe)\n", PROGNAME);
    printf("       %s [--help] [--version]\n\n", PROGNAME);
    printf("Convert frequency to wavelength.\n\n");
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
  /* Insufficient argument.*/
  if (argc == 1
#if defined(_WIN32)
      && _isatty(_fileno(stdin))
#else
      && isatty(fileno(stdin))
#endif
  )
  {
    goto HELP;
    printf("%s: insufficient argument.\n", PROGNAME);
    printf("Try '%s --help' for more information.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }
  if (freq->count == 1 && fileinp->count == 1
#if defined(_WIN32)
      && !_isatty(_fileno(stdin))
#else
      && !isatty(fileno(stdin))
#endif
  )
  {
    /* Display the error details contained in the arg_end struct.*/
    arg_print_errors(stdout, end, PROGNAME);
    printf("One of <freq>, <fileinp> or pipe should be supplied only.\n");
    printf("Try '%s --help' for more information.\n", PROGNAME);
    exitcode = EXIT_FAILURE;
    goto EXIT;
  }

  /* ======================================================================== */
  /* main operation                                                           */
  /* ======================================================================== */

  /* ------------------------------------------------------------------------ */
  /* switch output stream                                                     */
  /* ------------------------------------------------------------------------ */
  if (fileout->count == 1)
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
  /* ------------------------------------------------------------------------ */
  /* fetch input                                                              */
  /* ------------------------------------------------------------------------ */
  char buff[MAX_LINE_BUFFER];
  int N = 0, Nmax = 1;
  double *freqs = (double *)calloc(Nmax, sizeof(double));

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
        freqs = realloc(freqs, sizeof(double) * Nmax);
      }
      freqs[N++] = atof(buff);
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
        freqs = realloc(freqs, sizeof(double) * Nmax);
      }
      freqs[N++] = atof(buff);
    }
    fclose(fin);
    goto OUTPUT;
  }

  /* argument */
  if (freq->count == 1)
    freqs[N++] = freq->dval[0];

  /* ------------------------------------------------------------------------ */
  /* write output                                                             */
  /* ------------------------------------------------------------------------ */
OUTPUT:
  for (int i = 0; i < N; ++i)
    (human->count == 1) ? fprintf(fout, "%s\n", freq2wavelen_h(freqs[i], buff))
                        : fprintf(fout, "%f\n", freq2wavelen(freqs[i]));

  if (fileout->count == 1)
    fclose(fout);

  /* ======================================================================== */
  /* exit                                                                     */
  /* ======================================================================== */
EXIT:
  /* deallocate each non-null entry in argtable[] */
  arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
  return exitcode;
}