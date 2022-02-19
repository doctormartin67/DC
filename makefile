excel: excel.c errorexit.c helperfunctions.c common.c
	gcc -g -pedantic -Wall excel.c errorexit.c helperfunctions.c common.c `xml2-config --cflags` `xml2-config --libs`
