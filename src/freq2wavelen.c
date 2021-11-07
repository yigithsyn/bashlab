
#include <stdio.h>
#include <io.h>
#include <stdbool.h>
#include <stdlib.h>
#if defined(_WIN32)
#else
#include <unistd.h>
#endif

#define MAX_LINE 100
#define MAX_LINE_BUFFER 100

#define C0 299792458 // speed of in m/s
double freq2wavelen0(double freq);
char *freq2wavelen1(double freq, char *buff);

#include "argtable3.h"

int main(int argc, char *argv[])
{
  /*------------------------------------------*/
  /* argument parsing                         */
  /*------------------------------------------*/
  /* the global arg_xxx structs are initialised within the argtable */
  struct arg_lit *help = arg_lit0(NULL, "help", "display this help and exit");
  struct arg_lit *version = arg_lit0(NULL, "version", "display version info and exit");
  struct arg_lit *human = arg_lit0("h", "human", "human readable output like 1.36 GHz, 512 MHz");
  struct arg_dbl *freq = arg_dbl0(NULL, NULL, "<freq>", "frequency in Hertz [Hz]");
  struct arg_file *filename = arg_file0("f", "file", "<filename>", "input file for column-wise vector operation");
  struct arg_file *outfile = arg_file0("o", "out", "<outfile>", "output file for result storage");
  struct arg_end *end = arg_end(20);
  void *argtable[] = {freq, filename, human, outfile, help, version, end};

  int exitcode = 0;
  char progname[] = "freq2wavelen";

  int nerrors;
  nerrors = arg_parse(argc, argv, argtable);

  /* special case: '--help' takes precedence over error reporting */
  if (help->count > 0)
  {
    printf("Usage: ");
    // arg_print_syntaxv(stdout, argtable, "\n");
    printf("%s <freq> [-h|--human] [-o|--out=<outfile>] \n", progname);
    printf("       %s -f|--file=<filename> [-o|--out=<outfile>]\n", progname);
    printf("       %s [--help] [--version]\n", progname);
    printf("Convert frequency to wavelength.\n\n");
    arg_print_glossary(stdout, argtable, "  %-25s %s\n");
    exitcode = 0;
    goto exit;
  }

  /* special case: '--version' takes precedence over error reporting */
  if (version->count > 0)
  {
    printf("v1.0.0\n");
    exitcode = 0;
    goto exit;
  }

  /* If the parser returned any errors then display them and exit */
  if (nerrors > 0)
  {
    /* Display the error details contained in the arg_end struct.*/
    arg_print_errors(stdout, end, progname);
    printf("Try '%s --help' for more information.\n", progname);
    exitcode = 1;
    goto exit;
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
    printf("%s: insufficient argument.\n", progname);
    printf("Try '%s --help' for more information.\n", progname);
    exitcode = 1;
    goto exit;
  }
  if (freq->count == 1 && filename->count == 1
#if defined(_WIN32)
      && !_isatty(_fileno(stdin))
#else
      && !isatty(fileno(stdin))
#endif
  )
  {
    /* Display the error details contained in the arg_end struct.*/
    arg_print_errors(stdout, end, progname);
    printf("One of <freq>, <filename> or pipe should be supplied only.\n");
    printf("Try '%s --help' for more information.\n", progname);
    exitcode = 1;
    goto exit;
  }

  /*------------------------------------------*/
  /* main operation                           */
  /*------------------------------------------*/
  char buff[MAX_LINE_BUFFER], buff_err[MAX_LINE_BUFFER];
  double freqs[MAX_LINE];
  int N = 0;
  FILE *fp, *fout;

  if (!_isatty(_fileno(stdin)))
    while (fgets(buff, 100, stdin)) /* break with ^D or ^Z */
      freqs[N++] = atof(buff);
  else if (freq->count == 1)
    freqs[N++] = freq->dval[0];
  else
  {
    fp = fopen(filename->filename[0], "r");
    if (fp == NULL)
    {
      sprintf(buff_err, "%s: Error %d: Unable to open input file '%s'", progname, errno, filename->filename[0]);
      perror(buff_err);
      exitcode = 1;
      goto exit;
    }

    // -1 to allow room for NULL terminator for really long string
    while (fgets(buff, MAX_LINE_BUFFER - 1, fp))
      freqs[N++] = atof(buff);
    fclose(fp);
  }

  if (outfile->count == 1)
  {
    fp = fopen(outfile->filename[0], "w");
    if (fp == NULL)
    {
      sprintf(buff_err, "%s: Error %d: Unable to open output file '%s'", progname, errno, outfile->filename[0]);
      perror(buff_err);
      exitcode = 1;
      goto exit;
    }
    fout = fp;
  }
  else
    fout = stdout;

  for (int i = 0; i < N; ++i)
    (human->count == 1) ? fprintf(fout, "%s\n", freq2wavelen1(freqs[i], buff)) : fprintf(fout, "%f\n", freq2wavelen0(freqs[i]));

  if (outfile->count == 1)
    fclose(fp);
exit:
  /* deallocate each non-null entry in argtable[] */
  arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
  return exitcode;
}

double freq2wavelen0(double freq)
{
  return C0 / freq;
}
char *freq2wavelen1(double freq, char *buff)
{
  double wavelen = freq2wavelen0(freq);
  if (freq >= 1E12)
    sprintf(buff, "%.1f mm", wavelen * 1E3);
  else if (freq >= 1E9)
    sprintf(buff, "%.1f cm", wavelen * 1E2);
  else if (freq < 1E6)
    sprintf(buff, "%.1f km", wavelen / 1E3);
  else
    sprintf(buff, "%.1f m", wavelen);
  return buff;
}