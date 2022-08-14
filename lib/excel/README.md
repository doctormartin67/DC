This library is used to compile the files necessary to read an excel file.
It uses xml by saving the excel file as a zip file and then extracting the
necessary information.
The functions defined in include/excel.h are rather straightforward if you
would like to take a look.

Here is an example. Let's say you have an excel file with a sheet as in the 
below picture
![alt text](https://github.com/doctormartin67/DC/blob/master/lib/excel/excel_example.png?raw=true)
Running the following line of code would create a `Database` and return a pointer to it:

`const Database *db = open_database("example.xlsx", "Sheet1", "C3");`

where "C3" is the cell where the database begins. Leaving `sheet_name` as `NULL` would default to the first (and only sheet) in the file. Leaving it `NULL` when there are multiple sheets is a caught error.

After this if you would then want the value 4, you could run:

`int four = record_int(db, 1, "a");`

If you really just want a value from a cell without creating a `Database`, you could use the following functions:

```
Excel *open_excel(const char *file_name, const char *sheet_name);
void close_excel(Excel *excel);
Content *cell_content(Excel *excel, const char *sheet_name, const char *cell);
```

Using these functions are rather straightforward:

```
Excel *e = open_excel("example.xlsx", "Sheet1");
Content *c = cell_content(e, "Sheet1", "C5");
assert(CONTENT_INT == c->kind);
int four = c->val.i; // Val is defined in "common.h"
```
