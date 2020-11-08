# This is a small awk file to manipulate the sharedStrings.xml file that is used to extract
# the strings that are in the excel file
BEGIN {
    RS = "<si>";
    FS = "( xml:space=\"preserve\">)|(</t>)";
}
NF > 1 {print cnt++ ":\t" $2}
