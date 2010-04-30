/**
@example encode_freeform_seq.c
@english
Encoding a message from scratch without using a pre-defined template.

It should go without saying that this is NOT the recommended way to
encode BUFR messages and the API wasn't really designed with this mode
of operational in mind. On the other hand, if you have a legacy message
you need to replicate, what can you do?

This technique essentially builds a BUFR_Sequence and directly uses
the bufr_create_dataset_from_sequence() call to build a dataset
and, implicitly, a single datasubset. The technique described in
encode_freeform_tmpl.c uses an intermediate template.

Note, however, that this technique is only effective with simple
descriptor/value pairs (i.e. exactly as you'd expect in free-form
unstructured BUFR message).

@endenglish
@francais
@todo encode_freeform_seq.c description should be translated
@endfrancais
*/

#include <stdio.h>
#include <stdlib.h>
#include "bufr_api.h"

static void add_number( BUFR_Tables* tables, BUFR_Sequence* seq, int desc, double value ) {
	BufrDescriptor* d = bufr_create_descriptor( tables, desc );
	if( d==NULL ) {
		perror("bufr_create_descriptor");
		return;
	}
	if( bufr_descriptor_set_dvalue(d, value)<0 ) {
		perror("bufr_descriptor_set_dvalue");
		bufr_free_descriptor( d );
		return;
	}
	bufr_add_descriptor_to_sequence( seq, d );
}

static void add_string( BUFR_Tables* tables, BUFR_Sequence* seq, int desc, const char* value ) {
	BufrDescriptor* d = bufr_create_descriptor( tables, desc );
	if( d==NULL ) {
		perror("bufr_create_descriptor");
		return;
	}
	if( bufr_descriptor_set_svalue(d, value)<0 ) {
		perror("bufr_descriptor_set_svalue");
		bufr_free_descriptor( d );
		return;
	}
	bufr_add_descriptor_to_sequence( seq, d );
}

int main(int argc,char *argv[])
   {
   BUFR_Dataset  *dts = NULL;
   BUFR_Message *msg = NULL;
   BUFR_Tables  *tables = NULL;
	BUFR_Sequence *seq = NULL;

	/* load CMC Table B and D */
   tables = bufr_create_tables();
   bufr_load_cmc_tables( tables );

	/* Create an empty descriptor sequence */
	seq = bufr_create_sequence( NULL );

	/* start writing descriptor/value pairs. */
	add_number( tables, seq, bufr_fxy_to_descriptor(0,12,1), 300 );
	add_string( tables, seq, bufr_fxy_to_descriptor(0,1,19), "MESSAGE" );
	add_number( tables, seq, bufr_fxy_to_descriptor(0,12,30), 270.01 );

	/* Here we do the magic and turn the sequence into a dataset and
	 * datasubset.
	 */
	dts = bufr_create_dataset_from_sequence( seq, tables, 3 );

   bufr_free_sequence( seq );

	/* save the dataset to output file */
   msg = bufr_encode_message ( dts, 0 /* no compression */ );
   if (msg != NULL)
      {
		/* open a file for writing the BUFR message */
		FILE* fpBufr = fopen( "OUTPUT.BUFR", "w" );
      bufr_write_message( fpBufr, msg );
      bufr_free_message ( msg );
		fclose( fpBufr );
      }


   bufr_free_dataset( dts );
   bufr_free_tables( tables );
   }

