/**
@example encode_freeform_tmpl.c
@english
Encoding a message from scratch without using a pre-defined template.

It should go without saying that this is NOT the recommended way to
encode BUFR messages and the API wasn't really designed with this mode
of operational in mind. On the other hand, if you have a legacy message
you need to replicate, what can you do?

This technique takes advantage of the idea that LibECBUFR
templates can have "default" values _and_ that we can build such templates
dynamically, programatically. Then it's a matter of building the template
and making the dataset using it. This approach would be more effective than
the encode_freeform_seq.c example if one were building multiple datasubsets
from the same "schema", but for a single datasubset it's actually a bit more
complicated.

Note, however, that this technique is only effective with simple
descriptor/value pairs (i.e. exactly as you'd expect in free-form
unstructured BUFR message).  If you need to build a template free-form
using Table D operators and more complicate structures, you'll need
to generate the template and then populate the datasubset with values
later on.

@endenglish
@francais
@todo encode_freeform_tmpl.c description should be translated
@endfrancais
*/

#include <stdio.h>
#include <stdlib.h>
#include "bufr_api.h"

static void add_number( BUFR_Tables* tables, BUFR_Template* tmpl, int desc, double value ) {
	BufrDescValue d;
	EntryTableB* e;

	e = bufr_fetch_tableB( tables, desc );
	if( e == NULL ) {
		fprintf(stderr, "descriptor %d not in Table B!\n", desc);
		return;
	}
	if( e->encoding.type != TYPE_NUMERIC ) {
		fprintf( stderr, "descriptor %d encoding is not numeric!\n", desc );
	}

	bufr_init_DescValue( &d );
	d.descriptor = desc;

	/* allocate the BUFR value array */
	bufr_valloc_DescValue( &d, 1 );

	/* create and assign the BUFR value.  */
	d.values[0] = bufr_create_value( bufr_encoding_to_valtype(&(e->encoding)));
	bufr_value_set_double( d.values[0], value );

	/* put the entry in the template */
	bufr_template_add_DescValue( tmpl, &d, 1 );

	/* this implicitly frees the BUFR value(s), too. */
	bufr_vfree_DescValue( &d );
}

static void add_string( BUFR_Tables* tables, BUFR_Template* tmpl, int desc, const char* value ) {
	BufrDescValue d;
	EntryTableB* e;

	e = bufr_fetch_tableB( tables, desc );
	if( e == NULL ) {
		fprintf(stderr, "descriptor %d not in Table B!\n", desc);
		return;
	}
	if( e->encoding.type != TYPE_CCITT_IA5 ) {
		fprintf( stderr, "descriptor %d encoding is not CCITT IA5!\n", desc );
	}

	bufr_init_DescValue( &d );
	d.descriptor = desc;

	/* allocate the BUFR value array */
	bufr_valloc_DescValue( &d, 1 );

	/* create and assign the BUFR value. */
	d.values[0] = bufr_create_value( bufr_encoding_to_valtype(&(e->encoding)));
	bufr_value_set_string( d.values[0], value, e->encoding.nbits/8 );

	/* put the entry in the template */
	bufr_template_add_DescValue( tmpl, &d, 1 );

	/* this implicitly frees the BUFR value(s), too. */
	bufr_vfree_DescValue( &d );
}

int main(int argc,char *argv[])
   {
   BUFR_Dataset  *dts = NULL;
   BUFR_Message *msg = NULL;
   BUFR_Tables  *tables = NULL;
	BUFR_Template *tmpl = NULL;

	/* load CMC Table B and D */
   tables = bufr_create_tables();
   bufr_load_cmc_tables( tables );

	/* Create an empty template */
	tmpl = bufr_create_template( NULL, 0, tables, /* edition */ 3 );

	/* start writing descriptor/value pairs. */
	add_number( tables, tmpl, bufr_fxy_to_descriptor(0,12,1), 300 );
	add_string( tables, tmpl, bufr_fxy_to_descriptor(0,1,19), "MESSAGE" );
	add_number( tables, tmpl, bufr_fxy_to_descriptor(0,12,30), 270.01 );

	bufr_finalize_template( tmpl );

	/* We've populated the template, now we create a dataset */
	dts = bufr_create_dataset( tmpl );
	
	/* just create the datasubset. It initializes to the values
	 * in the template.
	 */
	bufr_create_datasubset( dts );

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
   bufr_free_template( tmpl );
   bufr_free_tables( tables );
   }

