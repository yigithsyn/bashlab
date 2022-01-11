#include "wavelen2freq.h"

#include <stdio.h>

char *wavelen2freq_h(double wavelen, char *buff)
{
  double freq = wavelen2freq(wavelen);
  if (freq > 1E12)
    sprintf(buff, "%.1f THz", freq / 1E12);
  else if (freq > 1E9)
    sprintf(buff, "%.1f GHz", freq / 1E9);
  else if (freq > 1E6)
    sprintf(buff, "%.1f MHz", freq / 1E6);
  else if (freq > 1E3)
    sprintf(buff, "%.1f kHz", freq / 1E3);
  else
    sprintf(buff, "%.1f Hz", freq / 1E12);
  return buff;
}