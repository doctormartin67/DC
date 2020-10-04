#This is a small awk file to print out the sheet names
#of an excel file.
BEGIN {
    RS = "(Id[0-9]\"/>)|(<sheets>)";
    FS = "\"";
}
/sheet name/ {print $2};
