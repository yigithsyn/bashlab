#ifndef H_UTILITY
#define H_UTILITY

#include <string.h>

char isnumber(const char *str)
{
  /* How to check if a string is a number? */
  /* https://stackoverflow.com/a/58585995/13305144 */
  int decpos = -1, pmpos = -1, engpos = strlen(str) - 1, epmpos = strlen(str) - 1;
  for (int i = 0; i < strlen(str); i++)
    /* check if it is integer */
    if (str[i] > 47 && str[i] < 58)
      continue;
    /* check if it is decimal seperator and used once*/
    else if (str[i] == 46 && decpos == -1)
    {
      decpos = i;
      continue;
    }
    /* check if it is +/-, at the begining*/
    else if ((str[i] == 43 || str[i] == 45) && i == 0)
    {
      pmpos = 1;
      continue;
    }
    /* check if it is engineering format e/E, used once, after decimal and before +/-*/
    else if ((str[i] == 69 || str[i] == 101) && engpos == strlen(str) - 1 && i > 0 && i > decpos && i < epmpos)
    {
      engpos = 1;
      continue;
    }
    /* check if it is engineering format +/-, used once, after decimal and after engineering e/E*/
    else if ((str[i] == 43 || str[i] == 45) && epmpos == strlen(str) - 1 && i > 0 && i > decpos && i > engpos)
    {
      epmpos = 1;
      continue;
    }
    else
      return 0;
  return 1;
}

#endif