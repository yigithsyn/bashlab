#ifndef H_UTILITY
#define H_UTILITY

#include <string.h>
#include <stdbool.h>
#include <time.h>

bool isinteger(const char *str)
{
  /* How to check if a string is a number? */
  /* https://stackoverflow.com/a/58585995/13305144 */
  int decpos = -1, pmpos = -1, engpos = strlen(str) - 1, epmpos = strlen(str) - 1;
  for (int i = 0; i < strlen(str); i++)
    /* check if it is integer */
    if (str[i] > 47 && str[i] < 58)
      continue;
    else
      return false;
  return true;
}

bool isnumber(const char *str)
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
      return false;
  return true;
}

/* chronometer */
static struct timespec t_start, t_end;
unsigned long tic()
{
  timespec_get(&t_start, TIME_UTC);
  return 0;
}
unsigned long toc()
{
  timespec_get(&t_end, TIME_UTC);
  double duration = (((double)t_end.tv_sec * 1e9 + t_end.tv_nsec) - ((double)t_start.tv_sec * 1e9 + t_start.tv_nsec));
  return (unsigned long)(duration / 1E6);
}

#endif