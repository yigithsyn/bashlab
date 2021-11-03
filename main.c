
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_LINE_BUFFER 50
#define C0 299792458 // speed of in m/s
double freq2wavelen0(double freq){
  return C0/freq;
}
char* freq2wavelen1(double freq, char *buff){
  double wavelen = freq2wavelen0(freq);
  if(freq >= 1E12)
    sprintf(buff, "%.1f mm", wavelen*1E3);
  else if(freq >= 1E9)
    sprintf(buff, "%.1f cm", wavelen*1E2);
  else if(freq < 1E6)
    sprintf(buff, "%.1f km", wavelen/1E3);
  else
    sprintf(buff, "%.1f m", wavelen);
  return buff;
}

#include "argtable3.h"

int main(int argc, char *argv[])
{
  /*------------------------------------------*/
  /* argument parsing                         */
  /*------------------------------------------*/
  /* the global arg_xxx structs are initialised within the argtable */
  struct arg_lit *help      = arg_lit0(NULL, "help", "display this help and exit");
  struct arg_lit *version   = arg_lit0(NULL, "version", "display version info and exit");
  struct arg_lit *human     = arg_lit0("h", "human", "human readable output like 1.36 GHz, 512 MHz");
  struct arg_dbl *freq      = arg_dbl0(NULL, NULL, "<freq>", "frequency in Hertz [Hz]");
  struct arg_file *filename = arg_file0("f", "file", "<filename>", "input file for column-wise vector operation");
  struct arg_file *outfname = arg_file0("o", "out", "<output>", "output file for result storage");
  struct arg_end *end       = arg_end(20);
  void *argtable[]          = {freq, filename, human, outfname, help, version, end};

  int exitcode = 0;
  char progname[] = "freq2wavelen";

  int nerrors;
  nerrors = arg_parse(argc, argv, argtable);

  /* special case: '--help' takes precedence over error reporting */
  if (help->count > 0)
  {
    printf("Usage: ");
    // arg_print_syntaxv(stdout, argtable, "\n");
    printf("%s <freq> [-h|--human]\n", progname);
    printf("       %s -f|--file=<filename>\n", progname);
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
  if (argc == 1)
  {
    printf("%s: insufficient argument.\n", progname);
    printf("Try '%s --help' for more information.\n", progname);
    exitcode = 1;
    goto exit;
  }
  if (freq->count == 1 && filename->count == 1)
  {
    /* Display the error details contained in the arg_end struct.*/
    arg_print_errors(stdout, end, progname);
    printf("One of <freq> or <filename> should be supplied only.\n");
    printf("Try '%s --help' for more information.\n", progname);
    exitcode = 1;
    goto exit;
  }

  /*------------------------------------------*/
  /* main operation                           */
  /*------------------------------------------*/
  char buff[MAX_LINE_BUFFER], buff2[MAX_LINE_BUFFER];
  FILE *fp, *fp2;
  if (freq->count == 1){
    if(human->count == 1)
      fprintf(stdout, "%s\n", freq2wavelen1(freq->dval[0], buff));
    else
      fprintf(stdout, "%.6f\n", freq2wavelen0(freq->dval[0]));
  }
  else
  {
    fp = fopen(filename->filename[0], "r");
    if (fp == NULL) {
      fprintf(stderr, "%s: Unable to read input file.", progname);
      exitcode = 1;
      goto exit;
    }

    // -1 to allow room for NULL terminator for really long string
    while (fgets(buff, MAX_LINE_BUFFER - 1, fp))
    {
      if(outfname->count == 1){
      int errnum;
       fp2 = fopen(outfname->filename[0], "a");
         if (fp2 == NULL) {
          errnum = errno;
          fprintf(stderr, "%s: Unable to open output file.\n", progname);
          sprintf(buff2, "Error %d", errnum);
          perror(buff2);
          exitcode = 1;
          goto exit;
        }
        if(human->count == 0)
          fprintf(fp2, "%f\n", freq2wavelen0(atof(buff)));
        else
          fprintf(fp2, "%s\n", freq2wavelen1(atof(buff), buff2));
        fclose(fp2);
      }
      else{
        if(human->count == 0)
          fprintf(stdout, "%f\n", freq2wavelen0(atof(buff)));
        else
          fprintf(stdout, "%s\n", freq2wavelen1(atof(buff), buff2));
      }
    }
    fclose(fp);
  }
  

exit:
  /* deallocate each non-null entry in argtable[] */
  arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
  return exitcode;
}