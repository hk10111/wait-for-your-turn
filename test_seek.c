#include <stdio.h>
#include <stdlib.h>
int main()
{
  char line[500];
  FILE* file=fopen("random.txt","r");
  //fseek(file, 8, SEEK_SET);
  fgets(line, sizeof(line), file);
  fgets(line,sizeof(line),file);
  printf("%s",line);
}
