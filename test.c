#include <stdio.h>
#include <stdlib.h>
#include "libraryheader.h"

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("syntax: ./a.out day");
    exit(0);
  }

  Date date;

  for (int i = 0; i < 367; i++) {
    date.XLday = atoi(*(argv+1)) + i;
    setdate(&date);
    setdate(&date);
    printf("%d is day %d\n", date.XLday, date.day);
    printf("%d is in month %d\n", date.XLday, date.month);
    printf("%d is in year %d\n", date.XLday, date.year);
  }
  Date *temp = newDate(1, 1, 2020);
  printf("%d, %d, %d\n", temp->day, temp->month, temp->year);
  return 0;
}
