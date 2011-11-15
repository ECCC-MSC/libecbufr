/**
@example decode_sqlite_quals.c
@english
Decode datasets into a SQLite database using qualifier information
from RTMD.
@endenglish
@francais
@todo decode_sqlite_quals.c description should be translated
@endfrancais
@author Chris Beauregard
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sqlite3.h>
#include "bufr_api.h"

static void bufr_show_dataset( BUFR_Dataset *dts, char *filename );

/* return non-zero if "good", 0 if "missing" */
static int get_value_as_string( BufrDescriptor *bcv, char* str ) {
	str[0] = 0;
	if (bcv->value) {
		if( bufr_value_is_missing( bcv->value ) ) return 0;
		if (bcv->value->type == VALTYPE_INT32) {
			int32_t value = bufr_descriptor_get_ivalue( bcv );
			sprintf( str, "%d ", value );
		} else if (bcv->value->type == VALTYPE_INT64) {
			int64_t value = bufr_descriptor_get_ivalue( bcv );
			sprintf( str, "%lld ", value );
		} else if (bcv->value->type == VALTYPE_FLT32) {
			float value = bufr_descriptor_get_fvalue( bcv );
			sprintf( str, "%g", (double)value );
		} else if (bcv->value->type == VALTYPE_FLT64) {
			double value = bufr_descriptor_get_dvalue( bcv );
			sprintf( str, "%g", value );
		} else if (bcv->value->type == VALTYPE_STRING) {
			int len;
			char *s = bufr_descriptor_get_svalue( bcv, &len );
			strcpy(str, s);
		}
		return 1;
	}
	return 0;
}

static void usage() {
	printf( "Usage:\n"
		"decode_sqlite -j -d <sqlfile> [-c <qual1>] ... [-c <qualn>] <bufrfile>\n");
	printf( "-d <sqlfile>   SQLite database file\n" );
	printf( "-c <qualn>     Qualifier to place in own column\n" );
	printf( "-j             Don't create rows for column qualifiers\n" );
}

int main(int argc, char *argv[]) {
	extern char* optarg;
	extern int optind, opterr, optopt;
   BUFR_Dataset  *dts;
   FILE          *fp;
   BUFR_Tables   *tables=NULL;
   char           buf[256];
   BUFR_Message  *msg;
   int            rtrn;
   int            count;
	sqlite3* sqldb = NULL;
	sqlite3_stmt* insertion = NULL;
	int c;
	char sql[16000];	/* FIXME: yeah, harcoded limits...
	                      * I'm writing code on a train.
	                      */
	int just_data = 0;
	char* qual_cols[256];
	int nqual_cols = 0;
	char* errmsg;
   DataSubset    *subset;
   int            sscount, cvcount;
   int            i, j, k, m;
   BufrDescriptor      *bcv;

	/*
	 * load CMC Table B and D
	 * includes local descriptors
	 */
   tables = bufr_create_tables();
   bufr_load_cmc_tables( tables );  

	sprintf( sql, "CREATE TABLE IF NOT EXISTS bufr( ");

   while ( (c = getopt( argc, argv, "hjd:c:" )) != -1 )
       switch (c)
       {
           case 'h':
				  usage();
              exit(0);
				  break;

			  case 'j':
			     just_data = 1;
				  break;
           case 'c':
			     /* qualifier column as argument in optarg... it becomes
					* part of the sql */
              if( !bufr_is_qualifier(atoi(optarg)) ) {
				     fprintf(stderr,"%s is not a qualifier!\n", optarg);
					  exit(1);
				  }
				  /* FIXME: if this wasn't just an example, we'd
				   * use the element type to generate a proper type-safe
					* schema.
					*/
				  sprintf( buf, "qual%s TEXT, ", optarg );
				  strcat( sql, buf );
				  qual_cols[nqual_cols++] = optarg;	/* static, in argv */
              break;

           case 'd':
			     /* SQLite database filename in optarg */
				  if( sqldb ) {
				  	fprintf( stderr, "SQL database already open!\n" );
				   usage();
					exit(1);
				  }
				  if( SQLITE_OK != sqlite3_open(optarg,&sqldb) ) {
				  	perror(optarg);
					exit(1);
				  }
              break;

           default:
              usage();
              exit(1);
	 }

	if( sqldb == NULL ) {
		usage();
		exit(1);
	}

	/* finish up our sql and (conditionally) create the table */
	strcat( sql, "descriptor TEXT, value TEXT);" );
	sqlite3_exec( sqldb, sql, NULL, 0, &errmsg );

	/* prepare our insert statement. What we're going to do, in a
	 * nutshell, is build an insert statement where all the qualifier
	 * columns can be bound to values as we process each descriptor in
	 * the message. When we hit a descriptor which isn't a column, we add
	 * a row. We unbind columns when/if qualifiers are cancelled.
	 */
	sprintf( sql, "INSERT INTO bufr(" );
	for( i = 0; i < nqual_cols; i ++ ) {
		sprintf(buf,"qual%s, ", qual_cols[i]);
		strcat( sql, buf );
	}
	strcat(sql,"descriptor, value) VALUES(");
	for( i = 0; i < nqual_cols; i ++ ) {
		strcat( sql, "?," );
	}
	strcat(sql,"?, ?);");
	sqlite3_prepare(sqldb, sql, -1, &insertion, &errmsg);

	/*
	 * open a file for reading
	 */
	while( optind < argc ) {
		fp = fopen( argv[optind++], "r" );
		if (fp == NULL) {
			bufr_free_tables( tables );
			exit(1);
		}

		/*
		 * a new instance of BUFR_Message is assigned to msg at each invocation
		 */
		while ( (rtrn = bufr_read_message( fp, &msg )) > 0 ) {

			/* 
			 * BUFR_Message ==> BUFR_Dataset 
			 * decode the message using the BUFR Tables
			 * the decoded message is stored in BUFR_Dataset *dts structure
			 * a new instance of BUFR_Dataset is assigned to dts at each invocation
			 */
			dts = bufr_decode_message( msg, tables ); 
			if (dts == NULL) continue;

			/*
			 * see how many DataSubset are present in the Dataset, this is
			 * an extract from section3
			 * but actually kept as an array size
			 */
			sscount = bufr_count_datasubset( dts );

			/* loop through each of them */
			for (i = 0; i < sscount ; i++) {
				/*
				 * This is necessary to "build" the qualifier lists for
				 * the elements. Effectively, the datasubset needs to be
				 * flattened and walked.
				 */
				bufr_expand_datasubset( dts, i );

				/*
				 * get a pointer of a DataSubset of each
				 */
				subset = bufr_get_datasubset( dts, i );

				/*
				 * obtain the number of codes present in a Subset
				 */
				cvcount = bufr_datasubset_count_descriptor( subset );

				for (j = 0; j < cvcount ; j++) {

					/*
					 * obtain a pointer to a BufrDescriptor structure which
					 * contains the descriptor and value
					 */
					bcv = bufr_datasubset_get_descriptor( subset, j );
					
					if (bcv->flags & FLAG_SKIPPED ) {
						/* Table D expansions, etc. Nothing with a useful value */
						continue;
					}

					/* qualifiers which go into columns don't need their own
					 * rows in this mode.
					 */
					if( just_data ) {
						for( k = 0; k < nqual_cols; k ++ ) {
							if( bcv->descriptor == atoi(qual_cols[k]) ) break;
						}
						if( k < nqual_cols ) continue;
					}

					/* map all the desired qualifiers to their appropriate
					 * columns.
					 */
					for( k = 0; k < nqual_cols; k ++ ) {
						int qdesc = atoi(qual_cols[k]);

						/* in case it's not in the qualifier list... */
						sqlite3_bind_null( insertion, k+1 );

						/* search the descriptor qualifier list for a match */
						for( m = 0; bcv->meta && m < bcv->meta->nb_qualifiers; m ++ ) {
							if( qdesc == bcv->meta->qualifiers[m]->descriptor ) {
								/* it's a column... get the value and bind it to
								 * the appropriate parameter. It'll stick around
								 * until we insert something.
								 */
								if( get_value_as_string( bcv->meta->qualifiers[m], buf ) ) {
									sqlite3_bind_text( insertion, k+1, buf, -1,
										SQLITE_TRANSIENT );
								}
								break;
							}
						}
					}

					/* bind the descriptor and value to the query */

					sprintf(buf, "%06d", bcv->descriptor);
					sqlite3_bind_text( insertion, nqual_cols+1, buf, -1,
						SQLITE_TRANSIENT );	/* descriptor */

					if( get_value_as_string( bcv, buf ) ) {
						sqlite3_bind_text( insertion, nqual_cols+2, buf, -1,
							SQLITE_TRANSIENT );	/* value */
					} else {
						sqlite3_bind_null( insertion, nqual_cols+2 ); /* value */
					}

					/* insert into the database */
					{
						int rc;
						do {
							rc = sqlite3_step( insertion );
						} while( rc == SQLITE_ROW );
					}

					/* this doesn't clear bindings */
					sqlite3_reset( insertion );
				}
			}

			/* discard the message */
			bufr_free_message( msg );

			/* discard an instance of the Decoded message pointed to by dts */
			bufr_free_dataset( dts );
		}

		fclose( fp );
	}

	sqlite3_finalize( insertion );
	sqlite3_close( sqldb );

   bufr_free_tables( tables );
}
