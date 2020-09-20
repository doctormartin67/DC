# This is a small awk file to find cells and their corresponding
# values in an excel file.
BEGIN {
    RS = "(<c r=\")|(</v></c>)";
    FS = "<v>";
}
{print $1 "<v>" $NF "</v>"}
