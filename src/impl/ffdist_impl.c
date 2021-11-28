#include "macros.h"
#include "freq2wavelen.h"

#include <stdio.h>

double ffdist(double freq, double D)
{
  /* Warren L. Stutzman, Antenna Theory and Design, 3rd Ed., Page 43 */
  double wavelen = freq2wavelen(freq);
  return MAX(2*D*D/wavelen,MAX(1.6*wavelen,5*D));
}

char* ffdist_h(double freq, double D, char *buff)
{
  double ffdist_val = ffdist(D, freq);
  if (ffdist_val >= 1E3)
    sprintf(buff, "%.1f km", ffdist_val * 1E-3);
  else if (ffdist_val >= 1E0)
    sprintf(buff, "%.1f m", ffdist_val * 1E0);
  else if (ffdist_val >= 1E-2)
    sprintf(buff, "%.1f cm", ffdist_val * 1E2);
  else
    sprintf(buff, "%.1f mm", ffdist_val * 1E3);
  return buff;
}