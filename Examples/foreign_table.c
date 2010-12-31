/**
@example foreign_table.c
@english

This is an example of how to load a table from a "foreign" format into LibECBUFR
table structures.

The specific table format we're going to import will be ECMWF Table B and D.
Altering the logic to other table sources should be straighforward.

@endenglish
@francais
@todo foreign_table.c description should be translated
@endfrancais
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <assert.h>
#include "bufr_api.h"

static int compare_tabled(const void *p1, const void *p2)
   {
	/* Sort by ascending order of descriptor */
	return (*(const EntryTableD **)p1)->descriptor
		- (*(const EntryTableD **)p2)->descriptor;
   }

static int load_ecmwf_table_d( BUFR_Tables* tables, const char *filename)
	{
	FILE* fin = NULL;
	int n;
	int descriptor = 0;	/* current descriptor */
	int count = 0;			/* how many in this sequence */
	int *descriptors = NULL;	/* sequence array */
	int left = 0;	/* how many more descriptors to read */

   if (tables->master.tableD == NULL)
		{
      tables->master.tableD = (EntryTableDArray)arr_create( 50, sizeof(EntryTableD *), 10 );
		}

	/* FIXME: it would be useful to extract the table version from the
	 * table filename.
	 */

	fin = fopen(filename,"r");
	if( fin == NULL )
		{
		perror(filename);
		return -1;
		}

	while( 1 == fscanf( fin, "%d", &n ) )
		{
		if( descriptor == 0 )
			{
			descriptor = n;
			}
		else if( count == 0 )
			{
				count = left = n;
				descriptors = calloc(count,sizeof(int));
				assert( count > 0 );
			}
		else
			{
			int i = count - left;
			assert( left > 0 );
			assert( descriptors != NULL );
			descriptors[i] = n;

			/* after every descriptor there can be a comment describing the
			 * sequence. Strip that out by reading to the newline
			 */
			while( !feof(fin) && !ferror(fin) )
				{
				int c = fgetc(fin);
				if( c < 0 || c=='\n' || c=='\r' ) break;
				}

			left --;
			if( left == 0 )
				{
				/* got all the entries */
				EntryTableD *etd = bufr_new_EntryTableD(descriptor,
					NULL, 0, descriptors, count);

				if( 0 == arr_add( tables->master.tableD, (char *)&etd ) )
					{
					perror("arr_add");
					}
				free( descriptors );
				descriptors = NULL;
				left = descriptor = count = 0;
				}
			}
		}

	fclose(fin);

	arr_sort( tables->master.tableD, compare_tabled );
	tables->master.tableDtype = TYPE_REFERENCED;
	}

static int compare_tableb(const void *p1, const void *p2)
   {
	/* Sort by ascending order of descriptor */
	return (*(const EntryTableB **)p1)->descriptor
		- (*(const EntryTableB **)p2)->descriptor;
   }

static int load_ecmwf_table_b( BUFR_Tables* tables, const char *filename)
	{
	FILE* fin = NULL;
	char line[PATH_MAX];

   if (tables->master.tableB == NULL)
		{
      tables->master.tableB = (EntryTableBArray)arr_create( 50, sizeof(EntryTableB *), 10 );
		}

	/* FIXME: it would be useful to extract the table version from the
	 * table filename.
	 */

	fin = fopen(filename,"r");
	if( fin == NULL )
		{
		perror(filename);
		return -1;
		}

	while( fgets(line,sizeof(line),fin) != NULL )
		{
		int i;
		EntryTableB *etb = bufr_new_EntryTableB();
		int sign = 1;

		etb->descriptor = atoi( &line[1] );

		for( i = 72; i >= 8 && isspace(line[i]); i -- ) line[i] = 0;
		if( line[8] == 0 ) continue;
		etb->description = strdup( &line[8] );

		for( i = 96; i >= 73 && isspace(line[i]); i -- ) line[i] = 0;
		if( line[73] == 0 ) continue;
		etb->unit = strdup( &line[73] );

		/* The implementation of bufr_unit_to_datatype() just happens
		 * to perform partial matches which are good enough for the
		 * conventions followed by the ECMWF. If that wasn't the case
		 * we'd need to implement our own mapping
		 */
		etb->encoding.type = bufr_unit_to_datatype( etb->unit );
		assert( TYPE_UNDEFINED != etb->encoding.type );

		/* we'd rather just use atoi() for all this stuff, but the
		 * ECMWF apparently allows whitespace between the sign
		 * and value for scale and reference.
		 */

		/* scale is between 97 and 101 */
		line[101] = 0;
		for( i = 97; line[i]; i ++ ) {
			if( isdigit(line[i]) ) {
				etb->encoding.scale = sign * atoi(&line[i]);
				break;
			}
			if( line[i] == '-' ) sign = -1;
		}

		/* reference is between 102 and 114 */
		line[114] = 0;
		for( i = 102; line[i]; i ++ ) {
			if( isdigit(line[i]) ) {
				etb->encoding.reference = sign * atoi(&line[i]);
				break;
			}
			if( line[i] == '-' ) sign = -1;
		}

		/* data width is 115 to 118 */
		etb->encoding.nbits = atoi( &line[115] );

		assert( 0 != etb->encoding.nbits );

		if( 0 == arr_add( tables->master.tableB, (char *)&etb ) )
			{
			perror("arr_add");
			}
		}

	fclose(fin);

	arr_sort( tables->master.tableB, compare_tableb );
	tables->master.tableBtype = TYPE_REFERENCED;
	}

int main(int argc, char *argv[])
   {
	BUFR_Tables* tables = bufr_create_tables();
	int i;
	FILE *fp;
   BUFR_Dataset  *dts;
   BUFR_Message  *msg;
   int            rtrn;

	for( i = 1; i < argc; i ++ )
		{
		if( argv[i][0]=='-' && strstr(argv[i],"tableb") )
			{
				load_ecmwf_table_b( tables, argv[++i] );
			}
		else if( argv[i][0]=='-' && strstr(argv[i],"tabled") )
			{
				load_ecmwf_table_d( tables, argv[++i] );
			}
		else
			{
			break;
			}
		}

	/* open a file for reading */
   fp = fopen( argv[i], "r" );
   if (fp == NULL)
      {
		perror( argv[i] );
      exit(-1);
      }

	/* read a message from the input file */
   rtrn = bufr_read_message( fp, &msg );
   if (rtrn <= 0)
      {
      fprintf( stderr, "Warning: cannot read message\n" );
      exit(0);
      }

	/* 
	 * BUFR_Message ==> BUFR_Dataset 
	 * decode the message using the BUFR Tables
	 */
   dts = bufr_decode_message( msg, tables ); 
   if (dts == NULL) 
      {
      fprintf( stderr, "Error: can't decode messages\n" );
      exit(-1);
      }

	/* dump the content of the Message into a file */
	unlink("OUTPUT.TXT");
   bufr_dump_dataset( dts, "OUTPUT.TXT" );

	/* discard the message */
   bufr_free_message( msg );
   bufr_free_dataset( dts );

   fclose( fp );

   bufr_free_tables( tables );
   }
