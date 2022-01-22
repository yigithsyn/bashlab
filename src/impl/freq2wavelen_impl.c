#include "freq2wavelen.h"
#include "constants.h"

#include <stdio.h>

double freq2wavelen(double freq)
{
  return C0 / freq;
}