#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>

static struct stat stat_buff;
static bool valid_workspace = false;
static bool has_out_var = false;
static char out_name[20] = {'\0'};

int main(int argc, char *argv[])
{
  if (argc == 1)
  {
    FILE *fp;
    char row[255];
    fp = fopen(README, "r");
    while (fgets(row, sizeof(row), fp) != NULL)
      printf("%s",row);
    return 0;
  }

  return 0;
}