#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "libraryheader.h"

#define LENGTH BUFSIZ * 1000

static struct stat buf;

char *trim(char *s) {
  char *t;
  t = s;
  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r'){
    t = ++s;
  }
  while (*s != ' ' && *s != '\t' && *s != '\n' && *s != '\r'){
    s++;
  }
  *s = '\0';
  return t;
}

// replace all occurences of string oldW with newW in s
char *replace(const char *s, const char *oldW, 
	      const char *newW) 
{ 
  char *result; 
  int i, cnt = 0; 
  int newWlen = strlen(newW); 
  int oldWlen = strlen(oldW); 
  
  // Counting the number of times old word 
  // occur in the string 
  for (i = 0; s[i] != '\0'; i++) { 
    if (strstr(&s[i], oldW) == &s[i]) { 
      cnt++; 
  
      // Jumping to index after the old word. 
      i += oldWlen - 1; 
    } 
  } 
  
  // Making new string of enough length 
  result = (char *) malloc(i + cnt * (newWlen - oldWlen) + 1); 
  
  i = 0; 
  while (*s) { 
    // compare the substring with the result 
    if (strstr(s, oldW) == s) { 
      strcpy(&result[i], newW); 
      i += newWlen; 
      s += oldWlen; 
    } 
    else
      result[i++] = *s++; 
  } 
  
  result[i] = '\0'; 
  return result; 
}

// Check if file exists
int FILEexists(const char *fname)
{
  FILE *file;
  if ((file = fopen(fname, "r")))
    {
      fclose(file);
      return 1;
    }
  return 0;
}

// Check if DIR exists
int DIRexists(const char *dname) {
  DIR* dir = opendir(dname);
  if (dir) {
    /* Directory FILEexists. */
    return 1;
    closedir(dir);
  } else if (ENOENT == errno) {
    /* Directory does not exist. */
    return 0;
  } else {
    /* opendir() failed for some other reason. */
    return -1;
  }
}
