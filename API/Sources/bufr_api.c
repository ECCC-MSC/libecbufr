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
 * @ingroup descriptor dataset
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
   int         i, j, k, jj;
   int         desc1, j1;
   float       val1, val2;
   double      scale;
   double      epsilon;

   if (dts == NULL) return -1;
   count = arr_count( dts->data );
   if (startpos < 0) 
      startpos = 0;
   else if (startpos >= count) 
      return -1;
   if (nb == 0) return startpos;

   pcb = (BufrDescriptor **)arr_get( dts->data, 0 );
   j = 0;
   jj = -1;
   j1 = 0;
   if (codes[j].descriptor & TLC_FLAG_BIT)
      {
      desc1 = codes[j].descriptor & ~TLC_FLAG_BIT;
      val1 = bufr_value_get_float( codes[j].values[0] );
      }
   for (i = startpos; i < count ; i++)
      {
      cb = pcb[i];
      if (cb->encoding.scale == 0)
         scale = 1.0;
      else
         scale = pow( 10.0, (double)cb->encoding.scale );
      epsilon = 0.5 / scale;
      if ((j1 != j) && (codes[j].descriptor & TLC_FLAG_BIT))
         {
         desc1 = codes[j].descriptor & ~TLC_FLAG_BIT;
         val1 = bufr_value_get_float( codes[j].values[0] );
         }
      if (codes[j].descriptor & TLC_FLAG_BIT)
         {
         val2 = bufr_fetch_rtmd_location( desc1, cb->meta );
         if (val1 == val2) 
            ++j;
         }
      else if (cb->descriptor == codes[j].descriptor) 
         {
         if (codes[j].nbval > 0)
            {
            if (cb->value)
               {
               if (codes[j].nbval != 2)
                  {
                  for (k = 0 ; k < codes[j].nbval ; k++)
                     {
                     if (bufr_compare_value( cb->value, codes[j].values[k], epsilon ) == 0)
                        {
                        ++j;
                        break;
                        }
                     }
                  }
               else if (codes[j].nbval == 2)
                  {
                  if (bufr_between_values( codes[j].values[0], cb->value, codes[j].values[1] ) == 0)
                     ++j;
                  }
               }
            }
         else
            {
            ++j;
            }
         if (jj == -1) jj = i;
         if (j == nb)
            return jj;
         }
      else
         {
         j = 0;
         jj = -1;
         }
      }
   return -1;
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

   cv->descriptor = descriptor;

   bufr_valloc_DescValue( cv, nbval );
   if (nbval <= 0) return;

   for (i = 0; i < nbval ; i++)
      {
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
 * define a key with a value representing time or location to search in datasubset
 * @endenglish
 * @francais
 * définir une clé représentant une valeur de coordonnée spatiale ou temporelle
 * afin d'effectuer une recherche dans un sous-ensemble de données (data subset)
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup decode descriptor
 */
void bufr_set_key_location( BufrDescValue *cv, int descriptor, float value  )
   {
   cv->descriptor = descriptor | TLC_FLAG_BIT;
   bufr_valloc_DescValue( cv, 1 );

   cv->values[0] = bufr_create_value( VALTYPE_FLT32 );
   bufr_value_set_float( cv->values[0], value );
   }
