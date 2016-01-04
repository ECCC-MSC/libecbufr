/**
@example encode_delayed_repl.c
@english
Encoding a message with delayed replication.

@endenglish
@francais
@todo encode_delayed_repl.c description should be translated
@endfrancais
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "bufr_api.h"

int main(int argc,char *argv[])
   {
   BUFR_Dataset  *dts = NULL;
   BUFR_Message *msg = NULL;
   BUFR_Tables  *tables = NULL;
	BUFR_Template *tmpl = NULL;
	DataSubset    *dss;
	BufrDescValue bdv;
	int n, m;

	/* load CMC Table B and D */
   tables = bufr_create_tables();
   bufr_load_cmc_tables( tables );

	tmpl = bufr_create_template( NULL, 0, tables, 4 );
	assert( tmpl != NULL );
	
	/* Create a simple template which contains nothing but a replicated
	 * 0-02-135 descriptor.
	 */
	bufr_init_DescValue( &bdv );
	bdv.descriptor = 321012;
	bufr_template_add_DescValue( tmpl, &bdv, 1 );
	bufr_finalize_template( tmpl );

	dts = bufr_create_dataset(tmpl);
	assert(dts != NULL);

	n = bufr_create_datasubset(dts);
	assert( n >= 0 );

	/* expand the datasubset... we need the template "blown up" so we can
	 * search for descriptors under it.
	 */
	bufr_expand_datasubset(dts,n);

	dss = bufr_get_datasubset( dts, n );
	assert( dss != NULL );

	/* In order to replicate, we need to find the replication factor. In
	 * a "real" template, of course, more than one replication factor might
	 * exist and we'd have to, say, search for the 321012, then find the
	 * replication factor from there.
	 */
	m = bufr_subset_find_descriptor( dss, 31001, 0 );
	if ( m >= 0)
		{
		BufrDescriptor* bcv = bufr_datasubset_get_descriptor( dss, m );
		assert(bcv != NULL);

		/* 8 replications */
		bufr_descriptor_set_dvalue( bcv, 8 );
		}

	/* re-expanding the datasubset after the replication has been set causes
	 * us to have a sequence of 8 0-02-135's with missing values.
	 */
	bufr_expand_datasubset(dts,n);

	/* Note that in theory, if we didn't do anything else except set the
	 * replication and expand, then the position of the 2135 descriptor
	 * would be at m+1. In that case, replace the next line with just
	 * m += 1;
	 */
	m = bufr_subset_find_descriptor( dss, 2135, m );
	if ( m >= 0)
		{
		int i;
		for( i = 0; i < 8; i ++ )
			{
			BufrDescriptor* bcv = bufr_datasubset_get_descriptor( dss, m+i );
			assert(bcv != NULL);
			bufr_descriptor_set_ivalue( bcv, 1 + 9*i );
			}
		}

   msg = bufr_encode_message ( dts, 0 /* no compression */ );
   if (msg != NULL)
      {
		/* open a file for writing the BUFR message */
		FILE* fpBufr = fopen( "delayed.BUFR", "w" );
      bufr_write_message( fpBufr, msg );
      bufr_free_message ( msg );
		fclose( fpBufr );
      }


   bufr_free_dataset( dts );
   bufr_free_template( tmpl );
   bufr_free_tables( tables );
   }

