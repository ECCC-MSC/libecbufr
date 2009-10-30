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

 * fichier : bufr_template.c
 *
 * author:  Vanh Souvanlasy 
 *
 * function: 
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include "bufr_array.h"
#include "bufr_linklist.h"
#include "bufr_io.h"
#include "bufr_ieee754.h"
#include "bufr_desc.h"
#include "bufr_sequence.h"
#include "bufr_tables.h"
#include "bufr_template.h"

static void bufr_copy_DescValue ( BufrDescValue *dest, BufrDescValue *src );
static void bufr_free_desc_array( char *list );

/**
 * @english
 * Deallocate an array of descriptors
 * @param  list  array to free
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup template internal
 */
static void bufr_free_desc_array( char *list )
   {
   int i, count;
   BufrDescriptor  **pcb;

   count = arr_count( list );
   pcb = (BufrDescriptor **)arr_get( list, 0 );
   for (i = 0; i < count ; i++)
      {
      bufr_free_descriptor( pcb[i] );
      }
   arr_free( &list );
   }

/**
 * @english
 *
 * Constructor of new template object from a list of descriptors. 
 * Each of the descriptor will be validated using the BUFR tables given.
 *
 * @param descs  an array of descriptors of type BufrDescValue
 * @param nb     size of descs
 * @param tables pointer to BUFR_Tables that will be used
 * @param edition  version number of BUFR used
 * @return BUFR_Template, Will return NULL if errors are found
 * @endenglish
 * @francais
 * Reçoit une liste de descripteurs avec les tables BUFR et retourne
 * un gabarit.  Ceci diffère de bufr_load_template en ce que le gabarit
 * est créé d'une liste de descripteurs au moment de l'exécution et
 * ne nécessite pas de fichier de définition de gabarit.
 * Vous pourriez utiliser ceci si vous aviez décodé un message et souhaitiez
 * créer un message BUFR similaire à partir de la section 3 de l'autre
 * message.
 * @endfrancais
 * @ingroup template
 * @author Vanh Souvanlasy
 * @see bufr_load_template()
 * @see bufr_finalize_template()
 * @see bufr_copy_template()
 * @see bufr_free_template()
 * @see bufr_save_template()
 * @see bufr_finalize_template()
 * @see bufr_compare_template()
 * @see bufr_template_add_DescValue()
 */
BUFR_Template *bufr_create_template  
   ( BufrDescValue *descs, int nb, BUFR_Tables *tbls, int edition )
   {
   BUFR_Template   *tmplt;
   EntryTableB     *e;
   int              i;
   char            errmsg[256];
   int             has_error=0;
/*
 * first make sure that all descriptor exist in BUFR table
 */
   for ( i = 0; i < nb ; i++ )
      {
      if (!bufr_is_descriptor( descs[i].descriptor ) )
         {
         has_error = 1;
         sprintf( errmsg, "Error: not a valid descriptor %d\n", 
               descs[i].descriptor );
         bufr_print_debug( errmsg );
         fprintf( stderr, "%s", errmsg );
         }
      else if (bufr_is_table_b( descs[i].descriptor ))
         {
         e = bufr_fetch_tableB( tbls, descs[i].descriptor );
         if (e == NULL) 
            {
            if ((i > 0) && bufr_is_sig_datawidth(descs[i-1].descriptor))
               { 
               /* known bit width, acceptable  */
               if (bufr_is_debug())
                  {
                  sprintf( errmsg, "Descriptor %d has sig data width %d\n", 
                        descs[i].descriptor, descs[i-1].descriptor );
                  bufr_print_debug( errmsg );
                  }
               }
            else
               {
               sprintf( errmsg, "Descriptor %d ??\n", descs[i].descriptor );
               bufr_print_debug( errmsg );
               has_error = 1;
               sprintf( errmsg, "Error: unknown descriptor %d\n", descs[i].descriptor );
               bufr_print_debug( errmsg );
               }
            }
         }
      }
   
   if (has_error)
      {
      bufr_print_debug( "Error: Template definition contains error(s)\n" );
      bufr_print_debug( "Error: Unable to create Template\n" );
      return NULL;
      }

   tmplt               = (BUFR_Template *)malloc(sizeof(BUFR_Template));
   tmplt->gabarit      = NULL;
   tmplt->edition      = edition;
   tmplt->flags        = 0;
   tmplt->codets        = NULL;
   tmplt->ddo_tbe      = (EntryTableBArray)arr_create( 32, sizeof(EntryTableB *), 128 );;
   tmplt->tables       = bufr_create_tables();

   bufr_merge_tables( tmplt->tables, tbls );
   bufr_set_tables_category( tmplt->tables, 
         tbls->data_cat, tbls->data_cat_desc );

   if ((descs == NULL)||(nb <= 0))
      return tmplt;

   bufr_template_add_DescValue( tmplt, descs, nb );

   if (bufr_finalize_template( tmplt ) < 0)
      {
      bufr_print_debug( "Error: Template definition contains error(s)\n" );
      bufr_print_debug( "Error: Unable to create Template\n" );
      bufr_free_template( tmplt );
      tmplt = NULL;
      }

   return tmplt;
   }

/**
 * @english
 *
 * Add descriptor(s) to a template, the new descriptor(s) will append at 
 * end of existing list.
 *
 * @param  tmplt  pointer to a BUFR_Template where to add
 * @param  descs  new descriptors to be added
 * @param  nb     size of descs
 * @warning After been added to, the template will need to be (re)finalized before uses.
 * @return void
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @see bufr_finalize_template()
 * @see bufr_create_template()
 * @see bufr_load_template()
 * @see bufr_copy_template()
 * @see bufr_free_template()
 * @see bufr_finalize_template()
 * @see bufr_compare_template()
 * @ingroup template
 */
void bufr_template_add_DescValue ( BUFR_Template *tmplt, BufrDescValue *descs, int nb )
   {
   int  i;
   BufrDescValue  descriptor;
/*
 * a finalized template will need to be finalized again if it is changed

*/
   if (tmplt->gabarit != NULL)
      {
      bufr_free_desc_array( tmplt->gabarit );
      tmplt->gabarit = NULL;
      }

   if (tmplt->codets == NULL)
      tmplt->codets = (BufrDescValueArray)arr_create( nb, sizeof(BufrDescValue), 100 );

   bufr_init_DescValue( &descriptor );
/*
 * make a copy of descriptors list

*/
   for (i = 0; i < nb ; i++)
      {
      bufr_copy_DescValue( &descriptor, &(descs[i]) );
      arr_add( tmplt->codets, (char *)&descriptor );
      bufr_init_DescValue( &descriptor );
      }
   }

/**
 * @english
 * Copy the value of an DescValue 
 * @param  dest output 
 * @param  src input
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup template internal
 */
static void bufr_copy_DescValue
   ( BufrDescValue *dest, BufrDescValue *src )
   {
   int  i;
   if ((dest == NULL)||(src == NULL)) return;

   dest->descriptor = src->descriptor;

   bufr_vfree_DescValue( dest );
   
   if (src->nbval > 0)
      {
      bufr_valloc_DescValue( dest, src->nbval );

      for (i = 0; i < src->nbval ; i++)
         dest->values[i] = bufr_duplicate_value( src->values[i] );
      }
   }

/**
 * @english
 * Free all allocations for values set inside the descriptor value object
 *
 * @param   desc  pointer to object of type BufrDescValue 
 * @return void
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @ingroup descriptor internal
 * @see bufr_init_DescValue()
 * @see bufr_valloc_DescValue()
 */
void bufr_vfree_DescValue ( BufrDescValue *desc )
   {
   int  i;
   
   if ( desc == NULL ) return;
   
   if (desc->values)
      {
      for ( i = 0; i < desc->nbval ; i++ )
         {
         bufr_free_value ( desc->values[i] );
         }
      free( desc->values );
      desc->values = NULL;
      }
   desc->nbval = 0;
   }

/**
 * @english
 *
 * Finalize a newly created template object and makes it
 * ready for use. If a finalized template is altered, it has to be processed
 * again by this function. Which will generates internal data needed for
 * creating new BUFR messages for encoding and decoding.
 *
 * @param  tmplt  pointer to a BUFR_Template 
 * @return 1 if no problem found or  -1 in case there is an error
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup template
 */
int bufr_finalize_template( BUFR_Template *tmplt )
   {
   BufrDescValue    *code;
   BufrDescriptor   *bc;
   BUFR_Sequence    *gabarit;
   int              i, count;
   int              flags;
   EntryTableB     *e;
   char             errmsg[256];

	if( NULL == tmplt ) return errno=EINVAL, -1;

   gabarit = bufr_create_sequence(NULL);
	if( NULL == gabarit ) return -1;

   count = arr_count( tmplt->codets );

   for (i = 0; i < count ; i++)
      {
      code = (BufrDescValue *)arr_get( tmplt->codets, i );
      if (!bufr_is_descriptor( code->descriptor ) )
         {
         sprintf( errmsg, "Error: not a valid descriptor %d\n", 
               code->descriptor );
         bufr_print_debug( errmsg );
         fprintf( stderr, "%s", errmsg );
         bufr_free_sequence( gabarit );
         return -1;
         }
      else if (bufr_is_table_b( code->descriptor ))
         {
         e = bufr_fetch_tableB( tmplt->tables, code->descriptor );
         if (e == NULL)
            {
            BufrDescValue    *pcode;
            char             errmsg[256];

            pcode = (BufrDescValue *)arr_get( tmplt->codets, i-1 );
            if ((i > 0) && bufr_is_sig_datawidth( pcode->descriptor ))
               { 
               /* known bit width, acceptable  */
               if (bufr_is_debug())
                  {
                  sprintf( errmsg, "Descriptor %d has sig data width %d\n", 
                        code->descriptor, pcode->descriptor );
                  bufr_print_debug( errmsg );
                  }
               }
            else
               {
               bufr_print_debug( "Error: template contains errors\n" );
               bufr_free_sequence( gabarit );
               return -1;
               }
            }
         }
      bc = bufr_create_descriptor ( tmplt->tables, code->descriptor );
      if ( code->values )
         bc->value = bufr_duplicate_value( code->values[0] );
      else
         bc->value = NULL;

      bufr_add_descriptor_to_sequence( gabarit, bc );
      }
   
/*
 * invalid templates are not allowed

*/
   flags = 0;
   if (bufr_check_sequence( gabarit, tmplt->edition, &flags, tmplt->tables, 0 ) <= 0 ) 
      {
      bufr_print_debug( "Error: template contains errors\n" );
      bufr_free_sequence( gabarit );
      return -1;
      }

   bufr_expand_sequence( gabarit, 0, tmplt->tables ); /* do not expand delayed replication yet */

   tmplt->flags |= flags;
   if ( tmplt->gabarit )
      {
      bufr_free_desc_array( tmplt->gabarit );
      tmplt->gabarit = NULL;
      }
   tmplt->gabarit = bufr_sequence_to_array( gabarit, 0 );
   lst_dellist( gabarit->list );
   gabarit->list = NULL;
   bufr_free_sequence( gabarit );

   return 1;
   }

/**
 * @english
 *
 * Destructor of an template object. This will free all allocations used
 * by the BUFR_Template object.
 *
 * @param  tmplt  pointer to a BUFR_Template 
 * @return void
 * @warning Beware not destroy an object that is still in use.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup template
 */
void bufr_free_template ( BUFR_Template *tmplt )
   {
   int             i, count;
   BufrDescValue   *code;

   if (tmplt == NULL) return;

   if ( tmplt->gabarit )
      {
      bufr_free_desc_array( tmplt->gabarit );
      tmplt->gabarit = NULL;
      }

   count = arr_count( tmplt->codets );
   for (i = 0; i < count ; i++ )
      {
      code = (BufrDescValue *)arr_get( tmplt->codets, i );
      if (code->values)
         {
         bufr_vfree_DescValue( code );
         }
      }
   arr_free( &(tmplt->codets) );

   if ( tmplt->tables )
      {
      bufr_free_tables( tmplt->tables );
      tmplt->tables = NULL;
      }

   if (tmplt->ddo_tbe)
      {
      bufr_tableb_free( tmplt->ddo_tbe );
      tmplt->ddo_tbe = NULL;
      }

   free( tmplt );
   }

/**
 * @english
 * Make a copy of a template object (BUFR_Template)
 * @param  tmplt  pointer to a BUFR_Template 
 * @endenglish
 * @return BUFR_Template, Will return NULL if errors are found
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup template

 */
BUFR_Template *bufr_copy_template( BUFR_Template *tmplt )
   {
   BUFR_Template *tmplt2;
   BufrDescValue  *descs;
   int            count;
   
   descs = (BufrDescValue *)arr_get( tmplt->codets, 0 );
   count = arr_count( tmplt->codets );

   tmplt2 = bufr_create_template( descs, count, tmplt->tables, tmplt->edition );

   return tmplt2;
   }

/**
 * @english
 *
 * Create a template object and loads a BUFR template definition file 
 * into it. Its content are check for validity.
 *
 * @param     filename input filename
 * @param     mtbls pointer to a master BUFR_Tables 
 *            (optional if template definition file already refers to master tables)
 * @return BUFR_Template, If the definition file is invalid, then it will
 * return NULL.
 * @endenglish
 * @francais
 * Cette fonction creer une structure pour contenir un gabarit et charge 
 * le contenu d'un fichier de gabarit BUFR. Une vérification interne pour valider 
 * le gabarit afin de vérifier que la structure de réplication peut-etre 
 * résolue correctement.
 * @param     filename fichier d'entree
 * @param     mtbls pointeur à BUFR_Tables (table maîtresse) 
 *            (optionnel si le fichier de définition de gabarit réfère déjà à la table maîtresse)
 * @return BUFR_Template, si le fichier de définition est invalide on retourne NULL.
 * @endfrancais
 * @author Vanh Souvanlasy
 * @see bufr_finalize_template()
 * @see bufr_create_template()
 * @see bufr_copy_template()
 * @see bufr_free_template()
 * @see bufr_finalize_template()
 * @see bufr_compare_template()
 * @ingroup io template
 */
BUFR_Template *bufr_load_template( const char *filename, BUFR_Tables *mtbls )
   {
   FILE *fp ;
   char ligne[2048] ;
   char *tok;
   int  icode;
   BUFR_Tables   *tbls;
   EntryTableB   *e;
   char         *sequence;
   BufrDescValue  code;
   BUFR_Template  *tmplt;
   int          edition=4;
   int          vlen;
   ValueType    vtype;
   int64_t      ival64;
   int32_t      ival32;
   float        fval;
   char        *kptr = NULL, *ptr;
   int          debug;
   char         errmsg[256];
   int          error=0;

   if (filename == NULL) return NULL;

   debug = bufr_is_debug();
   if (debug)
      {
      sprintf( errmsg, "### Loading template file \"%s\"\n", filename );
      bufr_print_debug( errmsg );
      }

   fp = fopen ( filename, "r" ) ;
   if (fp == NULL) 
      {
      sprintf( errmsg, "Error: can't open template file %s\n", filename );
      bufr_print_debug( errmsg );
      return NULL;
      }

   edition = 4; /* current default BUFR edition */

   tbls = bufr_create_tables();

   sequence = (BufrDescValueArray)arr_create( 200, sizeof(BufrDescValue), 100 );

   while ( fgets(ligne,2048,fp) != NULL ) 
      {
      if ( ligne[0] == '#' ) continue;
      if ( ligne[0] == '*' ) continue;

      if (strncmp( ligne, "LOCAL_TABLEB", 12 ) == 0)
         {
         tok = strtok( ligne+12, " =\t\n" );
         if (tok)
            {
            if (debug)
               {
               sprintf( errmsg, "### template has a local table B: %s\n", tok );
               bufr_print_debug( errmsg );
               }
            bufr_load_l_tableB( tbls, tok );
            }
         continue ;
         }
      else if (strncmp( ligne, "MASTER_TABLEB", 13 ) == 0)
         {
         tok = strtok( ligne+13, " =\t\n" );
         if (tok)
            {
            if (debug)
               {
               sprintf( errmsg, "### template has a master table B: %s\n", tok );
               bufr_print_debug( errmsg );
               }
            bufr_load_m_tableB( tbls, tok );
            }
         continue ;
         }
      else if (strncmp( ligne, "LOCAL_TABLED", 12 ) == 0)
         {
         tok = strtok( ligne+12, " =\t\n" );
         if (tok)
            {
            if (debug)
               {
               sprintf( errmsg, "### template has a local table D: %s\n", tok );
               bufr_print_debug( errmsg );
               }
            bufr_load_l_tableD( tbls, tok );
            }
         continue ;
         }
      else if (strncmp( ligne, "MASTER_TABLED", 13 ) == 0)
         {
         tok = strtok( ligne+13, " =\t\n" );
         if (tok)
            {
            if (debug)
               {
               sprintf( errmsg, "### template has a master table D: %s\n", tok );
               bufr_print_debug( errmsg );
               }
            bufr_load_m_tableD( tbls, tok );
            }
         continue ;
         }
      else if (strncmp( ligne, "BUFR_EDITION", 12 ) == 0)
         {
         tok = strtok( ligne+12, " =\t\n" );
         if (tok)
            {
            edition = atoi( tok );
            if (debug)
               {
               sprintf( errmsg, "### template BUFR Version: %d\n", edition );
               bufr_print_debug( errmsg );
               }
            }
         continue ;
         }

      if (tbls->master.tableB == NULL)
         {
         if (mtbls == NULL)
            {
            sprintf( errmsg, "Error: require a master BUFR Tables\n" );
            bufr_print_debug( errmsg );
            sprintf( errmsg, "Error: can't load template file %s\n", filename );
            bufr_print_debug( errmsg );

            bufr_free_tables( tbls );
            arr_free( &(sequence) );
            return NULL;
            }
         else
            {
            bufr_merge_tables( tbls, mtbls );
            }
         }

      if (kptr != NULL) 
         {
         free( kptr );
         kptr = NULL;
         }
      kptr = strdup( ligne );
      ptr = NULL;
      tok = strtok_r( kptr, " \t\n,=", &ptr );
      if (tok == NULL) continue;
      icode = atoi(tok);
      code.descriptor = icode ;
      code.values = NULL;
      code.nbval = 0;

      tok = strtok_r( NULL, " \t\n,=", &ptr );
      if (tok)
         {
         if (strcmp( tok, "VALUE")==0)
            {
            int  vpos;
            tok = strtok_r( NULL, "\t\n,=", &ptr );
            while ( tok )
               {
               if (code.values == NULL)
                  {
                  code.nbval  = 1;
                  code.values = (BufrValue **)malloc( code.nbval * sizeof(BufrValue *) );
                  }
               else
                  {
                  code.nbval  += 1;
                  code.values = (BufrValue **)realloc( code.values, code.nbval* sizeof(BufrValue *) );
                  }
               vpos = code.nbval - 1;
               code.values[vpos] = NULL;
               if (e != NULL)
                  {
                  vtype = bufr_encoding_to_valtype( &(e->encoding) ); 
                  vlen = e->encoding.nbits / 8;
                  }
               else
                  {
                  vtype = bufr_datatype_to_valtype( bufr_descriptor_to_datatype( tbls, e, icode, &vlen ), 32, 0 ); 
                  }
               switch( vtype )
                  {
                  case VALTYPE_STRING :
                     code.values[vpos] = bufr_create_value( vtype );
                     bufr_value_set_string( code.values[vpos], tok, vlen );
                     break;
                  case VALTYPE_INT64 :
                     ival64 = atol(tok);
                     code.values[vpos] = bufr_create_value( vtype );
                     bufr_value_set_int64( code.values[vpos], ival64 );
                     break;
                  case VALTYPE_INT32  :
                     ival32 = atoi(tok);
                     code.values[vpos] = bufr_create_value( vtype );
                     bufr_value_set_int32( code.values[vpos], ival32 );
                     break;
                  case VALTYPE_FLT64  :
                  case VALTYPE_FLT32  :
                     if (strcmp( tok, "MSNG" ) != 0)
                        {
                        fval = strtof( tok, NULL );
                        if (!bufr_is_missing_float( fval ))
                           {
                           code.values[vpos] = bufr_create_value( vtype );
                           bufr_value_set_float( code.values[vpos], fval );
                           }
                        }
                     break;
                  default :
                     break;
                  }
               tok = strtok_r( NULL, "\t\n,", &ptr );
               }
            }
         }

      arr_add( sequence, (char *)&code );
      }

   fclose ( fp ) ;

   if (kptr != NULL) 
      free( kptr );

   tmplt = bufr_create_template( NULL, 0, tbls, edition );
   tmplt->codets = sequence;
   if ((bufr_finalize_template( tmplt ) < 0)|| error)
      {
      sprintf( errmsg, "Error: Template file %s contains error(s)\n", filename );
      bufr_print_debug( errmsg );
      bufr_print_debug( "Error: Unable to create Template\n" );
      bufr_free_template( tmplt );
      return NULL;
      }

   bufr_free_tables( tbls );

   if (debug)
      {
      int count;
      bufr_print_debug   ( "### Finished loading template file\n" );
      count = arr_count( tmplt->codets );
      sprintf( errmsg, "### Template contains %d descriptors\n", count  );
      bufr_print_debug( errmsg );
      bufr_print_debug   ( NULL );
      }

   return tmplt;
   }

/**
 * @english
 *    bufr_save_template( str_template, bufr_get_dataset_template(dts)  
 *    (char *filename, BUFR_Template *tmplt)
 * Save a template object to a template definition file. Which can be
 * reloaded by encoders to create new BUFR message object.
 * @param filename  output filename
 * @param tmplt pointer to a BUFR_Template 
 * @return -1 if there is an error, otherwise return 0
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @see bufr_load_template()
 * @see bufr_finalize_template()
 * @see bufr_copy_template()
 * @see bufr_free_template()
 * @see bufr_create_template()
 * @see bufr_finalize_template()
 * @see bufr_compare_template()
 * @see bufr_template_add_DescValue()
 * @author Vanh Souvanlasy
 * @ingroup io template
 */
int bufr_save_template( const char *filename, BUFR_Template *tmplt )
   {
   int           i, j;
   BufrDescValue *code;
   int           count;
   char          errmsg[256];
   FILE         *fp;

   fp = fopen( filename, "w" );
   if (fp == NULL) return -1;

   count = arr_count( tmplt->codets );
   fprintf( fp, "# This file contains %d Codes of section 3 or Template\n", count );
   fprintf( fp, "BUFR_EDITION=%d\n", tmplt->edition );
   fprintf( fp, "#\n" );
   for (i = 0; i < count ; i++)
      {
      code = (BufrDescValue *)arr_get( tmplt->codets, i );
      fprintf( fp, "%d", code->descriptor );
      if ( code->values )
         {
         if (code->nbval > 1)
            fprintf( fp, ",VALUES=" );
         else
            fprintf( fp, ",VALUE=" );
         for (j = 0; j < code->nbval ; j++ )
            {
            errmsg[0] = '\0';
            if (bufr_print_value( errmsg, code->values[j] ))
               {
               fprintf( fp, errmsg );
               }
            if ((j > 0)&&((j+1) < code->nbval))
               fprintf( fp, "," );
            }
         }
      fprintf( fp, "\n" );
      }
   fprintf( fp, "#\n" );
   fclose( fp );
   return 0;
   }

/**
 * @english
 * Allocate an array of values for descriptor
 * @param bdv pointer to BufrDescValue object
 * @param nb_values  values count inside bdv
 * @return void
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @see bufr_init_DescValue()
 * @see bufr_vfree_DescValue()
 * @ingroup template
 */
void bufr_valloc_DescValue     ( BufrDescValue *bdv, int nb_values )
   {
   int i;

   bdv->values = (BufrValue **)malloc( sizeof(BufrValue *) * nb_values );
   bdv->nbval  = nb_values;

   for (i = 0; i < nb_values ; i++)
      bdv->values[i] = NULL;
   }

/**
 * @english
 *   initialize the BufrDescValue structure
 * @param bdv pointer to BufrDescValue object
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @see bufr_valloc_DescValue()
 * @see bufr_vfree_DescValue()
 * @ingroup descriptor
 */
void bufr_init_DescValue ( BufrDescValue *bdv )
   {
   bdv->values = NULL;
   bdv->nbval  = 0;
   }

/**
 * @english
 * Compare 2 template to see if they are the same
 * @param t1 : pointers to first template to compare
 * @param t2 : pointers to second template to compare
 * @return  0 if both are same, otherwise return -1
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @see bufr_load_template()
 * @see bufr_finalize_template()
 * @see bufr_copy_template()
 * @see bufr_free_template()
 * @see bufr_save_template()
 * @see bufr_finalize_template()
 * @see bufr_create_template()
 * @see bufr_template_add_DescValue()
 * @ingroup template
 */
int bufr_compare_template( BUFR_Template *t1, BUFR_Template *t2 )
   {
   int         i, cnt1, cnt2;
   BufrDescriptor  **pcb1, **pcb2;

   cnt1 = arr_count( t1->gabarit );
   cnt2 = arr_count( t2->gabarit );

   if (cnt1 != cnt2) return -1;


   pcb1 = (BufrDescriptor **)arr_get( t1->gabarit, 0 );
   pcb2 = (BufrDescriptor **)arr_get( t2->gabarit, 0 );
   for (i = 0; i < cnt1 ; i++)
      {
      if ( pcb1[i]->descriptor != pcb2[i]->descriptor ) return -1;
      }
   return 0;
   }
