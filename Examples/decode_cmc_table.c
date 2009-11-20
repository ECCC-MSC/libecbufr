/**
@example decode_cmc_table.c
@english
Simple decode example which finds the most appropriate table version for
the message using Environment Canada CMC table nomenclature.

BUFR table locations are defined in the BUFR_TABLE environment variable.
Tables are called TABBm.l and TABDm.l, where m is the master version and l
is the local version. CMC tables combine both the master table with the
local descriptors used in Canada (center 54).

Before we get into the code, let's try to define "best" in a BUFR context.

We've got some hard constraints, and then we've got some preferences.

The master version number should be identical. If not, we don't
want it. The filters enforce this condition, ignoring anything which
has the wrong master version.

The local version number should be at least that of the requested local
table. Unfortunately, we're caught by a problematic CMC practice where
table's B and D weren't always released in sync. Which means you might have a
BUFR message which requires (say) master table 6 and local table 14
(real historical example; see Test/BUFR/isaa41_cybg_191900.bufr), but
someone never releases a table D with local version 14... version 0 was
still applicable. So in this case, the constraint is to _prefer_ local
versions greater than the desired, but _accept_ lower versions. In an ideal
world, we'd handle this with some kind of --pedantic flag.

For both master and local, the caller has the option of specifying a
"latest" by using a version>255. In that case, the constraints are
ignored and we hope for the best.

The "best" table, after we apply those constraints, is the one closest
to the requested version or (for the "latest" situation) the larger
version. Because of the variability of the target constraints, the sort
routine ends up being a little more general purpose than one would think
necessary for straight BUFR processing.

Note that there may be some argument about whether "latest" is EVER
appropriate. Reality being what it is, there are local practices and even
some in-the-wild traffic which we _cannot_ handle any other way.

@endenglish
@francais
@todo decode_cmc_table.c description should be translated
@endfrancais
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include "bufr_api.h"

/* what we're looking for */
static int cmp_master_vers = 256;
static int cmp_local_vers = 256;

/*
 * Given our files, we want them sorted in descending order of
 * "best" for the desired master and local versions, while trying to
 * enforce our constraints.
 * For qsort purposes, this function returns <0 where a is "better" than b,
 * ==0 when the same "goodness", and >0 where b is "better" than a.
 */
static int cmp_best_cmc(const struct dirent **a, const struct dirent **b) {
	const char* as = (*a)->d_name;
	const char* bs = (*b)->d_name;
	int amv = 0, bmv = 0;
	int alv = 0, blv = 0;

	sscanf( as, "TAB%*c%d.%d", &amv, &alv );
	sscanf( bs, "TAB%*c%d.%d", &bmv, &blv );

	if( amv == bmv ) {
		/* master tables match... as mentioned previously, we want to prefer
		 * at least the desired version but settle for something lesser.
		 */
		if( alv >= cmp_local_vers ) {
			if( blv >= cmp_local_vers ) {
				/* both at least what we want, use the "closest" */
				return alv - blv;
			} else {
				/* a is "better" */
				return -1;
			}
		} else if( blv >= cmp_local_vers ) {
			/* b is "better" */
			return 1;
		}
		
		/* Both versions are less than what we need, so prefer the higher
		 * version
		 */
		return blv - alv;
	}

	/* Not an exact match, which means we're probably looking for "latest",
	 * so the higher version number is "better"
	 */
	return bmv - amv;
}

/*
 * Enforce the constraint that the master table version match what
 * we need, unless we're looking for the latest.
 */
static int filter_cmc_tableb( const struct dirent * d ) {
	int m, l, n=0;
	return sscanf(d->d_name,"TABB%d.%d%n", &m, &l, &n) >= 2
		&& d->d_name[n]==0
		&& (cmp_master_vers>255 || m==cmp_master_vers)
		;
}

/*
 * Enforce the constraint that the local table version be at least what
 * we need, unless we're looking for the latest.
 */
static int filter_cmc_tabled( const struct dirent * d ) {
	int m, l, n=0;
	return sscanf(d->d_name,"TABD%d.%d%n", &m, &l, &n) >= 2
		&& d->d_name[n]==0
		&& (cmp_master_vers>255 || cmp_master_vers==m)
		;
}

/*
 * master_vers - use >255 for "latest"
 * local_vers - use >255 for "latest"
 */
static int find_best_cmc_table_filename(const char* tabledir, char B_or_D,
	int master_vers, int local_vers, char* filename, size_t flen
) {
	struct dirent **tables = NULL;
	int ntables = 0;

   B_or_D = toupper(B_or_D);

	filename[0] = 0;

	cmp_master_vers = master_vers;
	cmp_local_vers = local_vers;

	/* directory scannning is expensive. In operational code, we cache
	 * loaded tables and only directory scan periodically.
	 *
	 * This combines a filter to find the right table plus a sort function to
	 * make sure the "best" match is first in the list.
	 */
	ntables = scandir(tabledir, &tables,
		(B_or_D=='B') ? filter_cmc_tableb : filter_cmc_tabled,
		cmp_best_cmc );
	
	if( ntables > 0 ) {
		/* the list is sorted in descending order of "best", which means
		 * if we matched our constraints the first entry is the one we
		 * want.
		 */

		snprintf(filename, flen, "%s/%s", tabledir, tables[0]->d_name);

		/* scandir() expects us to do the cleanup */
		while( ntables -- ) {
			free( tables[ntables] );
		}
		free(tables);

		return 0;
	}

	return -1;
}


int main(int argc, char *argv[])
   {
   BUFR_Dataset  *dts;
   FILE          *fp;
   BUFR_Tables   *tables=NULL;
   char           buf[PATH_MAX];
   BUFR_Message  *msg;
   int            rtrn;
   int            count;

/*
 * open a file for reading
 */
   fp = fopen( argv[1], "r" );
   if (fp == NULL)
      {
      bufr_free_tables( tables );
      fprintf( stderr, "Error: can't open file \"%s\"\n", argv[1] );
      exit(-1);
      }
/*
 * read a message from the input file
 */
   rtrn = bufr_read_message( fp, &msg );
   if (rtrn <= 0)
      {
      fprintf( stderr, "Warning: there are no message\n" );
      exit(0);
      }

   tables = bufr_create_tables();

	if( 0==find_best_cmc_table_filename( getenv("BUFR_TABLES"), 'B',
		msg->s1.master_table_version, msg->s1.local_table_version,
		buf, sizeof(buf))
	) {
		fprintf( stderr, "Using Table B '%s'\n", buf );
      bufr_load_m_tableB( tables, buf );
      bufr_load_l_tableB( tables, buf );
	} else {
		fprintf( stderr, "Unable to find Table B for %d.%d\n",
			msg->s1.master_table_version, msg->s1.local_table_version );
	}

	if( 0==find_best_cmc_table_filename( getenv("BUFR_TABLES"), 'D',
		msg->s1.master_table_version, msg->s1.local_table_version,
		buf, sizeof(buf))
	) {
		fprintf( stderr, "Using Table D '%s'\n", buf );
      bufr_load_m_tableD( tables, buf );
      bufr_load_l_tableD( tables, buf );
	} else {
		fprintf( stderr, "Unable to find Table D for %d.%d\n",
			msg->s1.master_table_version, msg->s1.local_table_version );
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
/*
 * dump the content of the Message into a file
 */
	unlink("OUTPUT.TXT");
   bufr_dump_dataset( dts, "OUTPUT.TXT" );

/*
 * discard the message
 */
   bufr_free_message( msg );
   bufr_free_dataset( dts );
/*
 * close all file and cleanup
 */
   fclose( fp );

   bufr_free_tables( tables );
   }

