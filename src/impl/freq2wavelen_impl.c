#include "freq2wavelen.h"

#include "constants.h"


double freq2wavelen(double freq)
{
  return C0 / freq;
}

char* freq2wavelen_h(double freq, char *buff)
{
  double wavelen = freq2wavelen(freq);
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