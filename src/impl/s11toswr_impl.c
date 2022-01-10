#include "s11toswr.h"

#include <math.h>

double s11toswr(double s11)
{
  return (1+s11)/(1-s11);
}

double s11toswr_db(double s11)
{
  return s11toswr(pow(10.0,s11/20));
}