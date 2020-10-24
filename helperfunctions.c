#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "libraryheader.h"

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

/* the xml files for excel are often in the form:
   <v>...<\v> and so we want to value where ...
   lies. This is a small function to retrieve the
   ... given begin (<v>) and end (<\v>). 
   REMEMBER TO FREE THE RETURN VALUE WHEN YOU ARE
   FINISHED WITH IT!
*/
char *strinside(char *s, char *begin, char *end) {
  char *pb; //pointer to begin in s
  char *pe; //pointer to end in s
  char *value; //malloc result that we will return
  int i, length; 
  if ((pb = strstr(s, begin)) == NULL) {
    return NULL;
  }
  // pe should start looking for end starting at begin
  pe = pb;
  if ((pe = strstr(pe, end)) == NULL) {
    return NULL;
  }

  // move pointer to start of the value we want
  pb += strlen(begin);

  /* pb is pointing at the character just after
     the last character of the value we need and
     so pe - pb is exactly the amount of characters
     of value, we then need one extra value for '\0'.
  */
  length = pe - pb + 1;
  value = (char *)malloc(length);
  for (i = 0; i < length; i++) {
    value[i] = *pb++;
  }
  value[i-1] = '\0';
  return value;
  
}
