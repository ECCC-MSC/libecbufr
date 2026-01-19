/***
Copyright Her Majesty The Queen in Right of Canada, Environment Canada, 2009.
Copyright Sa Majesté la Reine du Chef du Canada, Environnement Canada, 2009.

This file is part of libECBUFR.

    libECBUFR is free software: you can redistribute it and/or modify
    it under the terms of the Lesser GNU General Public License,
    version 3, as published by the Free Software Foundation.

    libECBUFR is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    Lesser GNU General Public License for more details.

    You should have received a copy of the Lesser GNU General Public
    License along with libECBUFR.  If not, see <http://www.gnu.org/licenses/>.

 * fichier : bufr_af.c
 *
 * author:  Vanh Souvanlasy 
 *
 * function: 
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "bufr_ieee754.h"
#include "bufr_api.h"
#include "bufr_io.h"
#include "bufr_af.h"
#include "bufr_afd.h"
#include "bufr_value.h"
#include "bufr_array.h"
#include "bufr_template.h"
#include "bufr_i18n.h"

#define   TLC_FLAG_BIT      0x80000
#define   QUAL_FLAG_BIT     0x40000
#define   CB_FLAG_BIT       0x20000
#define   FLAG_BITS (TLC_FLAG_BIT|QUAL_FLAG_BIT|CB_FLAG_BIT)

int  bufr_meta_enabled=1;

typedef struct {
	BufrValue val;
	int (*valcmp)(void* data, BufrDescriptor* bd);
	void* data;
} ValueCallback;

/**
 * @english
 * Initializes mathematical constants, calls bufr_init_limits.
 * Also, it initializes the internationalization features.
 * First function to call for using the BUFR API library
 * @warning Not thread-safe
 * @return void
 * @endenglish
 * @francais
 * Initialise les constantes mathématiques, appelle bufr_init_limits.
 * Sert aussi à initialiser les éléments d'internationalisation.
 * C'est la première fonction à appeler lors de l'utilisation de l'API BUFR.
 * @warning Le fil d'exécution n'est pas sécurisé (not thread-safe)
 * @return void 
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @ingroup api
 */
void bufr_begin_api(void)
   {
   // Internationalization
   bindtextdomain(PACKAGE, LOCALEDIR);

   if (bufr_is_debug())
      {
      char msg[256];

      bufr_print_debug( "###\n" );
      snprintf( msg, 256, _("### Debugging printout of BUFR API Version: %s\n"), 
                           BUFR_API_VERSION );
      bufr_print_debug( msg );
      bufr_print_debug( "###\n" );
      }
   bufr_init_limits();
   bufr_use_C_ieee754( 1 );
   }

/**
 * @english
 * Last function to call for using the BUFR API library.
 * Frees allocated memory used by api etc.
 * This may be a null function matching bufr_begin_api.
 * @warning This function will be reviewed
 * @warning Not thread-safe
 * @return void
 * @endenglish
 * @francais
 * Dernière fonction à appeler lors de l'utilisation de l'API BUFR.
 * Libère la mémoire allouée utilisée par l'API.
 * Ceci peut être une fonction nulle utilisée en conjonction avec bufr_begin_api.
 * @warning Cette fonction est sujette à révision
 * @warning Le fil d'exécution n'est pas sécurisé (not thread-safe)
 * @return void
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup api
 */
void bufr_end_api(void)
   {
   bufr_print_debug( NULL );
   bufr_print_output( NULL );
   bufr_set_debug_file( NULL );

   bufr_desc_end();
   bufr_value_end();
   bufr_linklist_end();
   }

/**
 * bufr_enable_meta
 * @english
 * enable or disable meta data collection while decoding
 * @todo translate
 * @endenglish
 * @francais
 * active ou desactive la collection de meta donnees durant decodage
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup debug
 */
void bufr_enable_meta(int mode)
   {
   bufr_meta_enabled = mode;
   }

/**
 * bufr_subset_find_descriptor
 * @english
 * find the position of a descriptor in a data subset
 * @endenglish
 * @francais
 * Trouver la position d'un descripteur dans un sous-jeu (subset) de données
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset
 */
int bufr_subset_find_descriptor( DataSubset *dts, int descriptor, int startpos )
   {
   BufrDescriptor  **pcb, *cb;
   int  count;
   int  i;

   if (dts == NULL) return -1;
   count = arr_count( dts->data );
   if (startpos < 0) startpos = 0;

   if (startpos >= count) return -1;

   pcb = (BufrDescriptor **)arr_get( dts->data, 0 );
   for (i = startpos; i < count ; i++)
      {
      cb = pcb[i];
      if (cb->descriptor == descriptor) return i;
      }
   return -1;
   }

/**
 * @english
 *    bufr_subset_find_values()
 *    (DataSubset *dts, BufrDescValue *codes, int nb, int startpos)
 *
 * To find the position of a sequence of descriptors with the code list.  Use when
 * looking for a particular sequence of descriptors within a code list. This only
 * returns the first sequence (which must be in order) from the starting
 * position that can be specified at any point. Values may be set in the
 * codes that are part of the search keys, and those values must be found
 * in the codes for this to be a successful call. All of the codes in the
 * sequence must be found in order for this to be a successful call, no
 * other values may be inserted.
 * @warning Not thread-safe
 * @return Int, If the values are all found, it is greater to or equal to
 * 0, else nothing is found.
 * @endenglish
 * @francais
 *   bufr_subset_find_values()
 *   (DataSubset *dts, BufrDescValue *codes, int nb, int startpos)
 * Trouver la position d'une séquence de descriptors avec la liste de codes.
 * À utiliser lorsqu'on recherche une séquence de descripteurs particulière au sein d'une
 * liste de codes. Ceci ne retourne que la première séquence trouvée (qui doit être en ordre)
 * à partir du point de départ qui peut être spécifiée à tout endroit. Des valeurs peuvent
 * être données dans les codes qui font partie des clés de recherche, et ces valeurs doivent
 * être retrouvées dans les codes pour que l'appel à la fonction réussisse. Tous les codes
 * dans la séquence de recherche doivent être trouvés pour que l'appel réussisse, aucune
 * autre valeur ne peut être insérée.
 * @warning Le fil d'exécution n'est pas sécurisé (not thread-safe)
 * @return Int Une valeur positive ou égale à zéro indique le succès. 
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset
 */
int bufr_subset_find_values( DataSubset *dts, BufrDescValue *codes, int nb, int startpos )
   {
   BufrDescriptor  **pcb, *cb;
   int         count;
   int         i, j, lj, k, jj;
   double      scale;
   double      epsilon;
	int nb_tlc = 0, nb_qual = 0, nb_desc = 0;
	BufrDescValue* tlc = NULL;
	BufrDescValue* qual = NULL;
	BufrDescValue* desc = NULL;

   if (dts == NULL) return -1;
   count = arr_count( dts->data );
	if( count == 0 ) return -1;

   if (startpos < 0) 
      startpos = 0;
   else if (startpos >= count) 
      return -1;
   if (nb == 0) return startpos;

	/* Break the query keys into three lists. We check TLC and qualifiers
	 * against every element, and check descriptors sequentially.
	 */
	for( j = 0; j < nb; j ++ )
		{
		if( codes[j].descriptor & TLC_FLAG_BIT )
			{
			/* NOTE: we're overallocating, but we're nicely bounded by nb */
			if( tlc == NULL ) tlc = calloc(nb,sizeof(BufrDescValue));
			if( tlc == NULL ) return -1;
			memcpy(&tlc[nb_tlc], &codes[j], sizeof(BufrDescValue));
			tlc[nb_tlc].descriptor &= ~TLC_FLAG_BIT;
			nb_tlc ++;
			}
		else if( codes[j].descriptor & QUAL_FLAG_BIT )
			{
			if( qual == NULL ) qual = calloc(nb,sizeof(BufrDescValue));
			if( qual == NULL ) return -1;
			memcpy(&qual[nb_qual], &codes[j], sizeof(BufrDescValue));
			/* Note: leave CB_FLAG_BIT alone */
			qual[nb_qual].descriptor &= ~QUAL_FLAG_BIT;
			nb_qual ++;
			}
		else
			{
			if( desc == NULL ) desc = calloc(nb,sizeof(BufrDescValue));
			if( desc == NULL ) return -1;
			memcpy(&desc[nb_desc++], &codes[j], sizeof(BufrDescValue));
			}
		}

	/* Basic strategy for searching and handling searching failures...
	 * When we find a "first" match for the sequence, we want jj to
	 * indicate this starting position. j indicates which descriptor in
	 * the search list we're currently supposed to see in the subset, and
	 * this gets incremented for each subsequent match.
	 * We return jj once we've found all nb_desc descriptors.
	 * When we fail a match, jj has to be cleared. _But_ we don't want
	 * to just continue the search at the current "i" position. We
	 * actually want to bounce back to jj+1, which is accomplished
	 * by assigning i=jj and letting the loop increment.
	 * Note that the trivial case, nb_desc==0, pretty much bypasses the
	 * loop and we return startpos.
	 */

   pcb = (BufrDescriptor **)arr_get( dts->data, 0 );
   lj = j = 0;
   jj = startpos;
   for (i = startpos; j < nb_desc && i < count ; i++)
      {
      cb = pcb[i];

		/* this is a cheap test... do it before we check the lists */
      if (cb->descriptor != (desc[j].descriptor&(~FLAG_BITS))) 
			{
			if( jj>=0 ) i = jj;
			jj = -1;
			j = 0;
			continue;
			}

		/* check TLC. Note that if we're looking for a TLC code and the
		 * descriptor has no meta-data, it becomes a fail.
		 */
		for( k=0; cb->meta && k<nb_tlc; k++ )
			{
			float v1 = bufr_value_get_float( tlc[k].values[0] );
			float v2 = bufr_fetch_rtmd_location( tlc[k].descriptor, cb->meta );
         if( v1 != v2 )
				{
				break;
				}
			}
		if( k<nb_tlc )
			{
			if( jj>=0 ) i = jj;
			jj = -1;
			j = 0;
			continue;
			}

		/* check qualifiers (and non TLC meta-data) */
		for( k=0; k<nb_qual; k++ )
			{
			if( qual[k].descriptor & CB_FLAG_BIT )
				{
				ValueCallback* bv = (ValueCallback*) qual[k].values[0];
				if( bv== NULL || bv->valcmp(bv->data, cb) ) break;
				}
			else if(cb->meta)
				{
				BufrDescriptor* qd = bufr_fetch_rtmd_qualifier(
					qual[k].descriptor&(~FLAG_BITS), cb->meta);
				if( qd == NULL ) break;

				scale = cb->encoding.scale ? pow(10,(double)cb->encoding.scale) : 1;
				epsilon = 0.5 / scale;
					
				if( bufr_compare_value( qd->value, qual[k].values[0], epsilon ) )
					{
					break;
					}
				}
			else
				{
				break;
				}
			}
		if( k<nb_qual )
			{
			if( jj>=0 ) i = jj;
			jj = -1;
			j = 0;
			continue;
			}

		/* If we got here, all our qualifiers match. Now it's just straight
		 * value checking. If there's no value defined for the match, it's
		 * obviously good. Otherwise, we get into the weeds.
		 */

		/* if we find a match, j should be incremented. If it isn't, no match,
		 * and we don't have to pepper failure handling in all our conditions.
		 */
		lj = j;

		if( desc[j].descriptor&CB_FLAG_BIT && desc[j].nbval>0 )
			{
			/* callback match */
			ValueCallback* bv = (ValueCallback*) desc[j].values[0];
			if( 0 == bv->valcmp(bv->data, cb) )
				{
				/* callback returned zero, so it's a match */
				++j;
				}
			}
		else if (desc[j].nbval > 0)
			{
			scale = cb->encoding.scale ? pow(10, (double)cb->encoding.scale) : 1;
			epsilon = 0.5 / scale;

			if (cb->value)
				{
				if (desc[j].nbval != 2)
					{
					for (k = 0 ; k < desc[j].nbval ; k++)
						{
						if (bufr_compare_value( cb->value, desc[j].values[k],
								epsilon ) == 0)
							{
							++j;
							break;
							}
						}
					}
				else if (desc[j].nbval == 2)
					{
					if (bufr_between_values( desc[j].values[0], cb->value,
							desc[j].values[1] ) == 1)
						{
						++j;
						}
					}
				}
			}
		else
			{
			++j;
			}

		if( lj == j )
			{
			/* j never got incremented, which means we failed to match */
			if( jj>=0 ) i = jj;
			jj = -1;
			j = 0;
			}
		else
			{
			/* we had a match */
			if( jj<0 ) jj = i;
			}
      }

	if( desc ) free(desc);
	if( qual ) free(qual);
	if( tlc ) free(tlc);

   return (j==nb_desc) ? jj : -1;
   }

/**
 * @english
 *    bufr_set_key_string(BufrDescValue *cv, int descriptor, first *values, int nbval)
 *    (BufrDescValue *cv, int descriptor, char **values, int nbval)
 * To define a search key in the form of a character string for use on descriptors whose unit
 * is CCITT IA5. The parameter nbval can be defined as a single
 * value to select or, if more than one value are in nbval, then it is a list of values to
 * select from. If it is coded as NULL, then any value is chosen.
 * The search key defined by bufr_set_key_string may be used by bufr_subset_find_values().
 * This call is thread-safe.
 * @warning Call bufr_vfree_DescValueto free storage after use.
 * @return void
 * @endenglish
 * @francais
 *    bufr_set_key_string(BufrDescValue *cv, int descriptor, first *values, int nbval)
 *    (BufrDescValue *cv, int descriptor, char **values, int nbval)
 * Définir une clé de recherche sous la forme d'une chaîne de caractères afin d'effectuer
 * une recherche sur des descripteurs dont l'unité est CCITT IA5. 
 * Le paramètre nbval peut être défini par une valeur unique 
 * ou, si plus d'une valeur sont contenues dans nbval il s'agit alors d'une liste de valeurs
 * valides comme clés de sélection. Dans le cas où nbval est codé comme NULL, toute valeur rencontrée
 * lors de la recherche est acceptée. La clé de recherche définie par bufr_set_key_string peut
 * être utilisée par bufr_subset_find_values().
 * Le fil d'exécution de cet appel est sécurisé (thread-safe).
 * @warning Appelez bufr_vfree_DescValue pour libérer le stockage après usage.
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
void bufr_set_key_string( BufrDescValue *cv, int descriptor, const char **values, int nbval )
   {
   int i;

	bufr_init_DescValue(cv);
   cv->descriptor = descriptor;
   bufr_valloc_DescValue( cv, nbval );
   if (nbval <= 0) return;

   for (i = 0; i < nbval ; i++)
      {
      cv->values[i] = bufr_create_value( VALTYPE_STRING );
      bufr_value_set_string( cv->values[i], values[i], strlen(values[i]) );
      }
   }

/**
 * @english
 *    bufr_set_key_int32(BufrDescValue *cv, int descriptor, first *values, int nbval)
 *    (BufrDescValue *cv, int descriptor, int *values, int nbval)
 * To define a search key in the form of an integer for use on descriptors whose unit
 * is represented by integers. The parameter nbval can
 * be defined as a single value to select, as a range of two within which
 * values are selected, or if more than two values are in nbval, then
 * it is a list of values to select from. If it is coded as NULL, then any
 * value is chosen. This call is thread-safe.
 * @warning Call bufr_vfree_DescValueto free storage after use.
 * @return void
 * @endenglish
 * @francais
 *    bufr_set_key_int32(BufrDescValue *cv, int descriptor, first *values, int nbval)
 *    (BufrDescValue *cv, int descriptor, int *values, int nbval)
 * Définir une clé de recherche sous la forme d'un entier afin d'effectuer une recherche sur des
 * descripteurs dont l'unité est représentée par le type entier.
 * Le paramètre nbval peut être défini par une valeur unique
 * ou, si plus d'une valeur sont contenues dans nbval il s'agit alors d'une liste de valeurs
 * valides comme clés de sélection. Dans le cas où nbval est codé comme NULL, toute valeur rencontrée
 * lors de la recherche est acceptée. La clé de recherche définie par bufr_set_key_int32 peut
 * être utilisée par bufr_subset_find_values().
 * Le fil d'exécution de cet appel est sécurisé (thread-safe).
 * @warning Appelez bufr_vfree_DescValue pour libérer le stockage après usage.
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
void bufr_set_key_int32( BufrDescValue *cv, int descriptor, int *values, int nbval )
   {
   int i;

	bufr_init_DescValue(cv);
   cv->descriptor = descriptor;

   bufr_valloc_DescValue( cv, nbval );
   if (nbval <= 0) return;

   for (i = 0; i < nbval ; i++)
      {
		if( bufr_is_missing_int(values[i]) )
			cv->values[i] = bufr_create_value( VALTYPE_FLT32 );
		else
			cv->values[i] = bufr_create_value( VALTYPE_INT32 );
      bufr_value_set_int32( cv->values[i], values[i] );
      }
   }

/**
 * @english
 *    bufr_set_key_flt32(BufrDescValue *cv, int descriptor, first *values, int nbval)
 *    (BufrDescValue *cv, int descriptor, float *values, int nbval)
 * To define a search key in the form of an float for use on descriptors whose unit
 * is represented by floats. The parameter nbval can
 * be defined as a single value to select, as a range of two within which
 * values are selected, or if more than two values are in nbval, then
 * it is a list of values to select from. If it is coded as NULL, then any
 * value is chosen.
 * This call is thread-safe.
 * @warning Call bufr_vfree_DescValueto free storage after use.
 * @return void
 * @endenglish
 * @francais
 * Définir une clé de recherche sous la forme d'un float afin d'effectuer une recherche sur des
 * descripteurs dont l'unité est représentée par le type float.
 * Le paramètre nbval peut être défini par une valeur unique
 * ou, si plus d'une valeur sont contenues dans nbval il s'agit alors d'une liste de valeurs
 * valides comme clés de sélection. Dans le cas où nbval est codé comme NULL, toute valeur rencontrée
 * lors de la recherche est acceptée. La clé de recherche définie par bufr_set_key_int32 peut
 * être utilisée par bufr_subset_find_values().
 * Le fil d'exécution de cet appel est sécurisé (thread-safe).
 * @warning Appelez bufr_vfree_DescValue pour libérer le stockage après usage.
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
void bufr_set_key_flt32( BufrDescValue *cv, int descriptor, float *values, int nbval  )
   {
   int i;

	bufr_init_DescValue(cv);
   cv->descriptor = descriptor;
   bufr_valloc_DescValue( cv, nbval );
   if (nbval <= 0) return;

   for (i = 0; i < nbval ; i++)
      {
      cv->values[i] = bufr_create_value( VALTYPE_FLT32 );
      bufr_value_set_float( cv->values[i], values[i] );
      }
   }

/**
 * bufr_set_key_location
 * @english
 * define a key with a value representing time or location to search
 * in datasubset
 * @note BufrDescValue structures initialized by this function shouldn't
 * be used outside of key search operations.
 * @warning Call bufr_vfree_DescValueto free storage after use.
 * @endenglish
 * @francais
 * définir une clé représentant une valeur de coordonnée spatiale ou temporelle
 * afin d'effectuer une recherche dans un sous-ensemble de données (data subset)
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
void bufr_set_key_location( BufrDescValue *cv, int descriptor, float value  )
   {
	bufr_init_DescValue(cv);
   cv->descriptor = descriptor | TLC_FLAG_BIT;
   bufr_valloc_DescValue( cv, 1 );

   cv->values[0] = bufr_create_value( VALTYPE_FLT32 );
   bufr_value_set_float( cv->values[0], value );
   }

/**
 * @english
 * define a key with a value representing qualifier to search for
 * in datasubset.
 * @note Qualifier values are the last entry in the datasubset. If you need
 * to search for a qualifier value which might have been derived from a
 * replication increment, you need to use a time/location key via
 * bufr_set_key_location().
 * @note BufrDescValue structures initialized by this function shouldn't
 * be used outside of key search operations.
 * @note If value is "missing", results are undefined.
 * @warning Call bufr_vfree_DescValueto free storage after use.
 * @param cv descriptor/value to allocation
 * @param descriptor descriptor to search for
 * @param value what we're looking for. Will be copied.
 * @endenglish
 * @francais
 * définir une clé représentant une valeur de coordonnée spatiale ou temporelle
 * afin d'effectuer une recherche dans un sous-ensemble de données (data subset)
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 * @see bufr_set_key_qualifier_int32, bufr_set_key_qualifier_flt32
 */
void bufr_set_key_qualifier( BufrDescValue *cv, int descriptor, 
	const BufrValue* value)
   {
	bufr_init_DescValue(cv);
   cv->descriptor = descriptor | QUAL_FLAG_BIT;
   bufr_valloc_DescValue( cv, value ? 1 : 0 );

	if( value ) cv->values[0] = bufr_duplicate_value( value );
   }

/**
 * @english
 * define a key with a value representing qualifier to search for
 * in datasubset.
 * @note Qualifier values are the last entry in the datasubset. If you need
 * to search for a qualifier value which might have been derived from a
 * replication increment, you need to use a time/location key via
 * bufr_set_key_location().
 * @note BufrDescValue structures initialized by this function shouldn't
 * be used outside of key search operations.
 * @note If value is "missing", results are undefined.
 * @warning Call bufr_vfree_DescValueto free storage after use.
 * @param cv descriptor/value to allocation
 * @param descriptor descriptor to search for
 * @param value what we're looking for. Will be copied.
 * @endenglish
 * @francais
 * définir une clé représentant une valeur de coordonnée spatiale ou temporelle
 * afin d'effectuer une recherche dans un sous-ensemble de données (data subset)
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 * @see bufr_set_key_qualifier, bufr_set_key_qualifier_flt32
 */
void bufr_set_key_qualifier_int32( BufrDescValue *cv, int descriptor, 
	int value)
   {
	bufr_init_DescValue(cv);
   cv->descriptor = descriptor | QUAL_FLAG_BIT;
   bufr_valloc_DescValue( cv, 1 );
	if( bufr_is_missing_int(value) )
		cv->values[0] = bufr_create_value( VALTYPE_FLT32 );
	else
		cv->values[0] = bufr_create_value( VALTYPE_INT32 );
	bufr_value_set_int32( cv->values[0], value );
   }

/**
 * @english
 * define a key with a value representing qualifier to search for
 * in datasubset.
 * @note Qualifier values are the last entry in the datasubset. If you need
 * to search for a qualifier value which might have been derived from a
 * replication increment, you need to use a time/location key via
 * bufr_set_key_location().
 * @note BufrDescValue structures initialized by this function shouldn't
 * be used outside of key search operations.
 * @note If value is "missing", results are undefined.
 * @warning Call bufr_vfree_DescValueto free storage after use.
 * @param cv descriptor/value to allocation
 * @param descriptor descriptor to search for
 * @param value what we're looking for. Will be copied.
 * @endenglish
 * @francais
 * définir une clé représentant une valeur de coordonnée spatiale ou temporelle
 * afin d'effectuer une recherche dans un sous-ensemble de données (data subset)
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 * @see bufr_set_key_qualifier, bufr_set_key_qualifier_flt32
 */
void bufr_set_key_qualifier_flt32( BufrDescValue *cv, int descriptor, 
	float value)
   {
	bufr_init_DescValue(cv);
   cv->descriptor = descriptor | QUAL_FLAG_BIT;
   bufr_valloc_DescValue( cv, 1 );
	cv->values[0] = bufr_create_value( VALTYPE_FLT32 );
	bufr_value_set_float( cv->values[0], value );
   }

/**
 * @english
 * define a key with a comparison callback. This is matched at the same "rank"
 * as a descriptor key value, but the callback can also check qualifiers and
 * other meta-data. The callback should return zero for a match.
 *
 * This type of search key is useful for matching against flag/code tables
 * where more than one option might be acceptable.
 *
 * @note BufrDescValue structures initialized by this function shouldn't
 * be used outside of key search operations.
 * @warning Call bufr_vfree_DescValue to free storage after use.
 * @param cv descriptor/value to allocation
 * @param descriptor descriptor to search for
 * @param valcmp callback function pointer
 * @param data first parameter passed to callback function
 * @endenglish
 * @francais
 * @todo translate.
 * @endfrancais
 * @author Chris Beauregard
 * @ingroup descriptor
 */
void bufr_set_key_callback( BufrDescValue *cv, int descriptor,
	int (*valcmp)(void* data, BufrDescriptor* bd), void* data)
	{
	ValueCallback* bv;

	bufr_init_DescValue(cv);
   cv->descriptor = descriptor | CB_FLAG_BIT;

	bv = calloc(1,sizeof(ValueCallback));
	if( bv )
		{
		/* NOTE: this only works because the library _allows_ undefined
		 * types and knows enough not to mess with the contents. In the
		 * good 'ol days we'd just stick the pointer into a 32 or 64 bit integer,
		 * but it's no longer safe to make assumptions about that sort of thing.
		 */
		bv->val.type = VALTYPE_UNDEFINE;
		bv->val.af = NULL;

		bv->valcmp = valcmp;
		bv->data = data;

		bufr_valloc_DescValue( cv, 1 );
		cv->values[0] = (BufrValue*) bv;
		}
	}

/**
 * @english
 * Define a meta-data key with a comparison callback. The callback is run
 * against every descriptor and would normally check meta-data such as
 * qualifiers. There is no descriptor argument since no specific descriptor
 * is being tested. The callback should return zero for a match.
 *
 * Note that this key is intended to test against meta-data, but since
 * the base descriptor is being used other tests (i.e. "not missing") are
 * also feasible.
 *
 * @note BufrDescValue structures initialized by this function shouldn't
 * be used outside of key search operations.
 * @warning Call bufr_vfree_DescValue to free storage after use.
 * @param cv descriptor/value to allocation
 * @param descriptor descriptor to search for
 * @param valcmp callback function pointer
 * @param data first parameter passed to callback function
 * @endenglish
 * @francais
 * @todo translate.
 * @endfrancais
 * @author Chris Beauregard
 * @ingroup descriptor
 */
void bufr_set_key_meta_callback( BufrDescValue *cv,
	int (*valcmp)(void* data, BufrDescriptor* bd), void* data)
	{
	ValueCallback* bv;

	bufr_init_DescValue(cv);

	/* Note that we use the qualifier bit flag... it doesn't really
	 * matter which, so much. We have no "real" descriptor.
	 */
   cv->descriptor = CB_FLAG_BIT | QUAL_FLAG_BIT;

	bv = calloc(1,sizeof(ValueCallback));
	if( bv )
		{
		/* NOTE: this only works because the library _allows_ undefined
		 * types and knows enough not to mess with the contents. In the
		 * good 'ol days we'd just stick the pointer into a 32 or 64 bit integer,
		 * but it's no longer safe to make assumptions about that sort of thing.
		 */
		bv->val.type = VALTYPE_UNDEFINE;
		bv->val.af = NULL;

		bv->valcmp = valcmp;
		bv->data = data;

		bufr_valloc_DescValue( cv, 1 );
		cv->values[0] = (BufrValue*) bv;
		}
	}
