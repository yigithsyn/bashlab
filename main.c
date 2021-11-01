#include "argtable3.h"

#define C0 299792458  // speed of in m/s

/* global arg_xxx structs */
struct arg_lit *verb, *help, *version;
struct arg_int *level;
struct arg_file *o, *file;
struct arg_dbl *anbr;
struct arg_end *end;

int main(int argc, char *argv[])
{
  /* the global arg_xxx structs are initialised within the argtable */
  void *argtable[] = {
      help = arg_litn(NULL, "help", 0, 1, "display this help and exit"),
      version = arg_litn(NULL, "version", 0, 1, "display version info and exit"),
      anbr = arg_dbln(NULL, NULL, "<freq>", 1, 1, "frequency in Hertz [Hz]"),
      end = arg_end(20),
  };

  int exitcode = 0;
  char progname[] = "freq2wavelen";

  int nerrors;
  nerrors = arg_parse(argc, argv, argtable);

  /* special case: '--help' takes precedence over error reporting */
  if (help->count > 0)
  {
    printf("Usage: %s", progname);
    arg_print_syntaxv(stdout, argtable, "\n");
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

  double wavelen = C0/anbr->dval[0];
  printf("%f\n", wavelen);

  
exit:
  /* deallocate each non-null entry in argtable[] */
  arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
  return exitcode;
}