/***
Copyright Her Majesty The Queen in Right of Canada, Environment Canada, 2009.
Copyright Sa Majest� la Reine du Chef du Canada, Environnement Canada, 2009.

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

 * fichier : bufr_dataset.c
 *
 * author:  Vanh Souvanlasy 
 *
 * function: 
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#include <gettext.h>
#include <errno.h>

#include "bufr_util.h"
#include "bufr_array.h"
#include "bufr_linklist.h"
#include "bufr_io.h"
#include "bufr_api.h"
#include "bufr_ieee754.h"
#include "bufr_value.h"
#include "bufr_desc.h"
#include "bufr_sequence.h"
#include "bufr_dataset.h"
#include "bufr_i18n.h"

static uint64_t    bufr_value2bits           ( BufrDescriptor *code );
static void        bufr_empty_datasubsets    ( BUFR_Dataset *dts );
static void        bufr_free_datasubsets     ( BUFR_Dataset *dts );
static void        bufr_free_datasubset      ( DataSubset *subset );
static void        bufr_put_desc_value       ( BUFR_Message *bufr, BufrDescriptor *bc );
static int         bufr_get_desc_value       ( BUFR_Message *bufr, BufrDescriptor *bc );
static int         bufr_get_desc_ieeefp     ( BUFR_Message *bufr, BufrDescriptor *code );
static int         bufr_get_desc_ccittia5   ( BUFR_Message *bufr, BufrDescriptor *code, int );

static void        bufr_put_af_compressed    ( BUFR_Message *msg, BUFR_Dataset *, BufrDescriptor *, int j );
static void        bufr_put_ccitt_compressed ( BUFR_Message *msg, BUFR_Dataset *dts, int j );
static void        bufr_put_ieeefp_compressed( BUFR_Message *msg, BUFR_Dataset *dts, int j );
static DataSubset *bufr_duplicate_datasubset ( DataSubset *dss );
static void        bufr_put_numeric_compressed
                           ( BUFR_Message *msg, BUFR_Dataset *dts, BufrDescriptor *bcv, int j );
static int         bufr_get_ccitt_compressed 
                           ( BufrDescriptor *cb, int nbsubset, BUFR_Message *msg, ListNode **, int subset_from, int subset_to );
static int         bufr_get_ieeefp_compressed 
                           ( BufrDescriptor *cb, int nbsubset, BUFR_Message *msg, ListNode **, int subset_from, int subset_to );
static int         bufr_get_numeric_compressed
                           ( BufrDescriptor *cb, int nbsubset, BUFR_Message *msg, ListNode **, int subset_from, int subset_to );
static int         bufr_get_af_compressed
                           ( BufrDescriptor *cb, int nbsubset, BUFR_Message *msg, ListNode **, int subset_from, int subset_to );
static DataSubset *bufr_allocate_datasubset  ( void );
static void        bufr_fill_datasubset      ( DataSubset *subset, BUFR_Sequence *bsq );
static int         bufr_load_header( FILE *fp, BUFR_Dataset *dts );
static int         bufr_load_datasubsets( FILE *fp, BUFR_Dataset *dts, int lineno, BUFR_Enforcement enforce );
static void        bufr_mkval_rest_sequence(BUFR_Tables   *tbls, BUFR_Sequence *bsq2, ListNode *node, int *errflg );

extern int         bufr_meta_enabled;

/**
 * @english
 * instantiate a DataSubset object
 * @endenglish
 * @francais
 * creer un nouveau objet de type DataSubset
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset
 */
static DataSubset *bufr_allocate_datasubset(void)
   {
   DataSubset  *subset;

   subset = (DataSubset *)malloc( sizeof(DataSubset ));
   subset->dpbm     = NULL;
   subset->data     = NULL;
   return  subset;
   }

/**
 * @english
 *    dts = bufr_create_dataset( tmplt )
 *    (BUFR_Template *tmplt)
 * This function creates a data structure for storing BUFR message's values 
 * in the decoded form. For data viewing or placing a set of values for encoding.
 * The returned structure will hold only data defined by the specified template
 * @param tmplt  pointer to a BUFR_Template
 * @return BUFR_Dataset
 * @endenglish
 * @francais
 *    dts = bufr_create_dataset( tmplt )
 *    (BUFR_Template *tmplt)
 * Cette fonction creer une structure de donnees pour contenir les valeurs
 * d'un message BUFR sous la forme non-encode, pour consulter les donnees ou pour
 * inserer les valeurs pour l'encodage.
 * La structure retourne ne pourra contenir que les donnees defini par le gabarit (Template)
 * @param tmplt  un pointeur a BUFR_Template
 * @return BUFR_Dataset
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @ingroup dataset
 */
BUFR_Dataset *bufr_create_dataset  ( BUFR_Template *tmplt )
   {
   BUFR_Dataset    *dts;

   if ((tmplt == NULL) || (tmplt->gabarit == NULL))
      {
      bufr_print_debug( _("Warning: cannot create dataset, template not finalized\n") );
      return NULL;
      }

   dts = (BUFR_Dataset *)malloc(sizeof(BUFR_Dataset));
   dts->tmplte = bufr_copy_template( tmplt );

   bufr_init_sect1( &(dts->s1), tmplt->edition );
   bufr_set_gmtime( &(dts->s1) );
   dts->data_flag  = 0;

   if (dts->tmplte == NULL)
      {
      free( dts );
      dts = NULL;
      }
   else
      {
      dts->datasubsets = (DataSubsetArray)arr_create( 500, sizeof(DataSubset *), 500 );
      }
   dts->header_string = NULL;
   return dts;
   }


/**
 * @english
 * free memory allocation of a BUFR_Dataset 
 * @param     dts : pointer to a BUFR_Dataset
 * @endenglish
 * @francais
 * liberer l'espace memoire d'un BUFR_Dataset
 * @param     dts : adresse pointeur a un BUFR_Dataset
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset
 */
void bufr_free_dataset ( BUFR_Dataset *dts )
   {
   if (dts == NULL) return;

   bufr_free_datasubsets( dts );
   bufr_free_template( dts->tmplte );
   dts->tmplte = NULL;
   if ( dts->header_string )
      {
      free( dts->header_string );
      dts->header_string = NULL;
      }
   free( dts );
   }

/**
 * @english
 *    bufr_get_dataset_template(dts)
 *    (BUFR_Dataset *dts)
 * This return a read only reference to a BUFR_template used by 
 * the BUFR message stored within a BUFR_Dataset structure.
 * @warning What is returned here should not be freed, it is only a
 * reference.
 * @param dts pointer to a BUFR_Dataset
 * @return BUFR_Template
 * @endenglish
 * @francais
 *    bufr_get_dataset_template(dts)
 *    (BUFR_Dataset *dts)
 * Cette fonction retourne un pointeur du BUFR_template associe
 * au BUFR_Dataset.
 * @warning Ne pas liberer ou alterer le contenu du pointeur 
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @ingroup dataset template
 */
BUFR_Template *bufr_get_dataset_template   ( BUFR_Dataset *dts )
   {
   return dts->tmplte;
   }

/**
 * @english
 *    pos = bufr_create_datasubset( dts )
 *    (BUFR_Dataset *dts)
 * This adds a new datasubset to a dataset. A datasubset is a storage
 * for values of descriptors sequence as defined by its template. 
 * This is used for encoding a new BUFR message with multiple datasubset.
 * @param dts pointer to a BUFR_Dataset
 * @return int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @ingroup dataset
 */
int bufr_create_datasubset( BUFR_Dataset *dts )
   {
   int             i;
   BUFR_Sequence   *bsq;
   BufrDescriptor      **pbcd;
   BufrDescriptor       *bd;
   BUFR_Template  *tmplt;
   int             count;
   int             pos;
   BufrDDOp       *ddo;
   int             errcode;

   if (dts == NULL) return -1;

   if (dts->tmplte == NULL)
      {
      bufr_print_debug( _("Error: cannot create datasubset, template not defined in dataset\n") );
      return -1;
      }

   tmplt = dts->tmplte;

   count = arr_count( tmplt->gabarit );
   bsq = bufr_create_sequence(NULL);
   pbcd = (BufrDescriptor **)arr_get( tmplt->gabarit, 0 );
   for (i = 0; i < count ; i++)
      {
      bd = bufr_dupl_descriptor( pbcd[i] );
      bufr_add_descriptor_to_sequence( bsq, bd );
      }

/*
 * what is in gabarit is already expanded but,
 * delayed replications are not, all delayed field of the template "gabarit"
 * should be filled prior to this  

*/
   if (tmplt->flags & HAS_DELAYED_REPLICATION)
      {
      if (bufr_expand_sequence( bsq, OP_EXPAND_DELAY_REPL | OP_ZDRC_SKIP, 
               tmplt->tables ) < 0)
         return -1;
      }
/*
 * applying Table C

*/
   ddo = bufr_create_BufrDDOp( BUFR_STRICT );
   bufr_apply_Tables( ddo, bsq, tmplt, NULL, &errcode ); 
   if (errcode < 0)
	   dts->data_flag |= BUFR_FLAG_INVALID;
   bufr_free_BufrDDOp( ddo );
   pos = bufr_add_datasubset( dts, bsq, NULL );
   return pos;
   }

int bufr_expand_qualifiers( DataSubset* dss )
   {
   int            i, j, count;
   BufrDescriptor     **pbcd;
	BufrDescriptor **quals;	/* scratchpad */
	int nb_quals = 0;

	if( dss==NULL ) return errno=EINVAL, -1;

   pbcd = (BufrDescriptor **)arr_get( dss->data, 0 );
	if( pbcd == NULL ) return -1;

   count = bufr_datasubset_count_descriptor( dss );
	if( count <= 0 ) return 0;

	quals = calloc(count,sizeof(BufrDescriptor *));
	if( quals == NULL ) return 0;

   for (i = 0; i < count ; i++)
      {
		/* For some descriptors, qualifiers aren't terribly useful. It
		 * probably wouldn't _hurt_ to track qualifiers, but it does end
		 * up being a waste of memory. Replication values aren't really
		 * directly interesting, and quality flags are more appropriately
		 * qualified by the elements they reference.
		 * NOTE: don't add new flags without checking this assumption.
		 */
		if( pbcd[i]->flags & (FLAG_CLASS31|FLAG_CLASS33) ) continue;

		/* Assign copy of current qualifier list to the descriptor.
		 * This may require allocating new (empty) RTMD.
		 */
		if  (( nb_quals>0 ) && bufr_meta_enabled )
			{
			BufrDescriptor* bd = pbcd[i];
			if( bd->meta == NULL ) bd->meta = bufr_create_rtmd(0);
			if( bd->meta == NULL ) break;

			if( bd->meta->qualifiers ) free(bd->meta->qualifiers);

			bd->meta->qualifiers
				= (BufrDescriptor**)calloc(nb_quals, sizeof(BufrDescriptor*));
			bd->meta->nb_qualifiers = 0;
			if( bd->meta->qualifiers )
				{
				for( j = 0; j < nb_quals; j ++ )
					{
					if( quals[j] && quals[j]->value
							&& !bufr_value_is_missing(quals[j]->value) )
						{
						bd->meta->qualifiers[bd->meta->nb_qualifiers++] = quals[j];
						}
					}
				}
			}

		/* If we've got a qualifier, we need to add it into the list
		 * or cancel it
		 */
		if( bufr_is_qualifier(pbcd[i]->descriptor) && pbcd[i]->value )
			{
			/* see if there's already an instance in the list. If so,
			 * we replace/cancel it. Note that we search backwards, like
			 * a stack. Recently seen qualifiers will normally be the
			 * first to be cancelled. Qualifiers near the start of the
			 * sequence are almost never cancelled.
			 */
			int qpos;
			for( qpos = nb_quals-1; qpos >= 0; qpos -- )
				{
				if( pbcd[qpos]==NULL ) continue;
				if( pbcd[i]->descriptor == quals[qpos]->descriptor ) break;
				}

			/* Not in list. We'll append to the list, or we'll end up
			 * ignoring it if it's missing.
			 */
			if( qpos<0 ) qpos = nb_quals;

			if( bufr_value_is_missing(pbcd[i]->value) )
				{
				/* cancel the existing qualifier, if any, and fill in
				 * the resulting "hole".
				 */
				quals[qpos] = NULL;
				if( qpos < nb_quals )
					{
					nb_quals --;
					for(j = qpos; j < nb_quals; j ++ )
						{
						quals[j] = quals[j+1];
						}
					}
				}
			else
				{
				quals[qpos] = pbcd[i];
				if( qpos == nb_quals ) nb_quals ++;
				}
			}
      }

	free(quals);
	return i;
   }


/**
 * @english
 *    bufr_expand_datasubset(dts, i)
 *    (BUFR_Dataset *dts, int pos) 
 * Expands a descriptors sequence of a datasubset by resolving any Table D,
 * replications or delayed replications once the delayed replication 
 * counter has been set (value of descriptor 31001 that follows)
 *
 * In addition, this calculates the qualifier list in the RTMD.
 *
 * @param dts      :  pointer to a BUFR_Dataset
 * @param dss_ops  :  the position of datasubset to expand
 *
 * @warning Once a delayed replication has been expanded, it can no longer
 * be reset.
 * @return int, the new number of descriptors within the subset that have
 * been expanded. If an error, -1 is returned.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset
 */
int bufr_expand_datasubset( BUFR_Dataset *dts, int dss_pos )
   {
   BUFR_Sequence  *bsq;
   int            i, count;
   BufrDescriptor     **pbcd;
   DataSubset    *dss;
   BufrDDOp       *ddo;
   int             errcode;

   dss = bufr_get_datasubset( dts, dss_pos );
   if (dss == NULL) return -1;

   bsq = bufr_create_sequence(NULL);
   pbcd = (BufrDescriptor **)arr_get( dss->data, 0 );
   count = bufr_datasubset_count_descriptor( dss );
   for (i = 0; i < count ; i++)
      {
      bufr_add_descriptor_to_sequence( bsq, pbcd[i] );
      }
   if (bufr_expand_sequence( bsq, OP_EXPAND_DELAY_REPL | OP_ZDRC_SKIP, 
            dts->tmplte->tables ) < 0 )
      {
      bufr_free_sequence( bsq );
      return -1;
      }
   ddo = bufr_create_BufrDDOp( BUFR_STRICT );
   bufr_apply_Tables( ddo, bsq, dts->tmplte, NULL, &errcode ); 
   bufr_free_BufrDDOp( ddo );
   if (errcode < 0)
	   dts->data_flag |= BUFR_FLAG_INVALID;

   arr_free( &(dss->data) );
   dss->data = bufr_sequence_to_array( bsq, 1 );
   if (dss->dpbm != NULL)
      {
      bufr_free_BufrDPBM( dss->dpbm );
      dss->dpbm = NULL;
      }
   dss->dpbm = bufr_index_dpbm( NULL, bsq );
   lst_dellist( bsq->list );
   bsq->list = NULL;
   bufr_free_sequence( bsq );

   if ( bufr_meta_enabled )
	   bufr_expand_qualifiers( dss );

   return bufr_datasubset_count_descriptor( dss );
   }

/**
 * @english
 * makes a copy of the DataSubset 
 * @param  dss  : pointer to a DataSubset to copy
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset
 */
static DataSubset *bufr_duplicate_datasubset( DataSubset *dss )
   {
   DataSubset  *subset;
   int          i, count;
   BufrDescriptor     *bc;

   subset = bufr_allocate_datasubset();

   count = bufr_datasubset_count_descriptor( dss );
   subset->data = (BufrDescriptorArray)arr_create ( count, sizeof(BufrDescriptor *), 100 );
   for (i = 0; i < count ; i++ )
      {
      bc = bufr_datasubset_get_descriptor( dss, i );
      bc = bufr_dupl_descriptor( bc );
      arr_add( subset->data , (char *)&bc );
      }

   return subset;
   }

/**
 * @english
 * @brief add a Datasubset to a dataset from a BUFR_Sequence
 *
 * This sequence must share the same template as the Dataset
 * This is used internally for decoding but may be used for encoding also.
 *
 * @param dts pointer to a BUFR_Dataset
 * @param bsq pointer to a BUFR_Sequence to be add, once added, its memory management
 * belongs to the dataset, it should not be freed from outside
 * @param ddo pointer to BufrDDOp, if it has the dpbm instance defined, move it into DataSubset 
 * @return int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset
 */
int bufr_add_datasubset( BUFR_Dataset *dts, BUFR_Sequence *bsq, BufrDDOp *ddo )
   {
   DataSubset  *subset;
   int          pos;

   subset = bufr_allocate_datasubset();
   if ( (ddo == NULL) || (ddo->dpbm == NULL) )
      {
      subset->dpbm = bufr_index_dpbm( NULL, bsq );
      }
   else
      {
      subset->dpbm = ddo->dpbm;
      ddo->dpbm = NULL;
      }

   bufr_fill_datasubset( subset, bsq );
   pos = arr_count( dts->datasubsets );
   arr_add( dts->datasubsets, (char *)&subset );

   return pos;
   }

/**
 * @english
 * add a Datasubset to a dataset from a BUFR_Sequence,
 * this sequence must share the same template as the Dataset
 * @param   subset :  pointer to a BUFR_Dataset
 * @param   bsq :  pointer to a BUFR_Sequence to be add, once added, its
 *                 memory management belongs to the dataset, it should not be freed from outside
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset
 */
static void bufr_fill_datasubset( DataSubset *subset, BUFR_Sequence *bsq )
   {
   subset->data =  bufr_sequence_to_array( bsq, 1 );
/*
 * all items already transfered to datasubset array
 * so just free the list

*/
   lst_dellist( bsq->list );
   bsq->list = NULL;
   bufr_free_sequence( bsq );
   }

/**
 * @english
 *    sscount = bufr_count_datasubset( dts )
 *    (BUFR_Dataset *dts)
 * This call returns the number of data subsets in the BUFR message.
 * as indicated in section 3.
 * @param dts pointer to a BUFR_Dataset
 * @return the number of DataSubset inside a Dataset
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset
 */
int bufr_count_datasubset( BUFR_Dataset *dts )
   {
   if (dts == NULL) return 0;

   return arr_count( dts->datasubsets );
   }

/**
 * @english
 *
 * return a pointer to a BufrDescriptor located at a position in the data subset.
 * @warning The returned structure should not be freed; it will be freed
 * when the entire dataset is freed.
 * @param dts pointer to a DataSubset
 * @param pos position from 0 to n-1
 * @return pointer to a BufrDescriptor located at the given position
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset descriptor
 */
BufrDescriptor *bufr_datasubset_get_descriptor( DataSubset *dts, int pos )
   {
   BufrDescriptor  **pcb;
   
   if (dts == NULL) return NULL;

   pcb = (BufrDescriptor **)arr_get( dts->data, pos );
   return (pcb ? *pcb : NULL);
   }

/**
 * @english
 * @return a pointer to a next unskipped BufrDescriptor
 * located at/after the given position. Note that this will also skip
 * over anything except Table B descriptors.
 * @param   dts :  pointer to a DataSubset
 * @param   pos :  position from 0 to n-1
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor dataset
 */
BufrDescriptor *bufr_datasubset_next_descriptor( DataSubset *dts, int *pos )
   {
   BufrDescriptor  **pcb, *cb;
   int  count;
   int  f;
   
   if (dts == NULL) return NULL;

   count = arr_count( dts->data );
   if ((*pos < 0)||(*pos >= count)) return NULL;

   while (*pos < count)
      {
      pcb = (BufrDescriptor **)arr_get( dts->data, *pos );
      *pos += 1;
      cb = *pcb;
      f = DESC_TO_F( cb->descriptor );
      if (f != 0) continue;
      if ( cb && !(cb->flags&FLAG_SKIPPED))
         {
         return cb;
         }
      }

   return NULL;
   }

/**
 * @english
 * @return the number of BufrDescriptor inside a DataSubset.
 * Each subset of a Dataset may have a different descriptors count.
 * If there are delayed replications of different count in each subset.
 * For each data subset there will be a counter of the number of elements
 * within it, which is extracted from Section 4 of the BUFR message.
 *
 * @param dts pointer to a DataSubset
 * @return number of BufrDescriptor inside a DataSubset
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset descriptor
 */
int bufr_datasubset_count_descriptor ( DataSubset *dts )
   {
   int  count;
   
   if (dts == NULL) return 0;
   count = arr_count( dts->data );
   return count;
   }

/**
 * @english
 * free all DataSubset stored in a Dataset
 * @param   dts :  pointer to a BUFR_Dataset
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset
 */
static void bufr_free_datasubsets( BUFR_Dataset *dts )
   {
   if (dts == NULL) return;

   if (dts->datasubsets)
      {
      bufr_empty_datasubsets( dts );
      arr_free( &(dts->datasubsets) );
      dts->datasubsets = NULL;
      }
   }

/**
 * @english
 * free all DataSubset stored in a Dataset
 * @param   dts :  pointer to a BUFR_Dataset
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset
 */
static void bufr_empty_datasubsets( BUFR_Dataset *dts )
   {
   int  i, count;
   DataSubset *subset, **ptr;

   if (dts == NULL) return;

   if (dts->datasubsets)
      {
      count = arr_count( dts->datasubsets );
      for ( i = 0; i < count; i++ )
         {
         ptr = (DataSubset **)arr_get( dts->datasubsets, i );
         if (ptr && *ptr)
            {
            subset = *ptr;
            bufr_free_datasubset( subset );
            }
         }
      arr_del( dts->datasubsets, count );
      }
   }

/**
 * @english
 * free a DataSubset
 * @param  dts :  pointer to a DataSubset
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset
 */
static void bufr_free_datasubset( DataSubset *subset )
   {
   int i, count;
   BufrDescriptor  **pcb, *bd;
   char  *list;

   if (subset == NULL) return;
   if (subset->data == NULL) return;

   list = subset->data;
   count = arr_count( list );
   for (i = 0; i < count ; i++)
      {
      pcb = (BufrDescriptor **)arr_get( list, i );
      bd = *pcb;
      if (bd)
         {
         if (bd->value)
            {
            bufr_free_value( bd->value );
            bd->value = NULL;
            }
         bufr_free_descriptor( bd );
         }
      }
   arr_free( &list );
   subset->data = NULL;

   if (subset->dpbm)
      {
      bufr_free_BufrDPBM( subset->dpbm );
      subset->dpbm = NULL;
      }

   free( subset );
   }

/**
 * @english
 * Takes values defined within the dataset and applies the Table C defined
 * within the dataset upon the values before encoding the dataset into a
 * BUFR message. Table C allows values of elements to be stored to
 * different width, scaling, or reference value than normally defined. This
 * may be a way of avoiding the definition of local descriptors.
 * @param dts pointer to a BUFR_Dataset containing data
 * @param x_compress boolean requiring compression if possible
 * @return BUFR_Message
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup message encode
 */
BUFR_Message *bufr_encode_message( BUFR_Dataset *dts , int x_compress )
   {
   BufrDescriptor       *bcv;
   int             descriptor;
   uint64_t        blen, nbits;
   int             i, j, it;
   int             nb_subsets;
   int             count;
   BUFR_Template  *tmplt;
   BufrDescValue   *bdv;
   DataSubset     *subset;
   BUFR_Message   *msg;
   int             debug, verbose;
   char            errmsg[256];

   if (dts == NULL) return NULL;

   debug = bufr_is_debug();
   verbose = bufr_is_verbose();

   strcpy( errmsg, _("### Creating BUFR Message with Dataset\n") );
   if (debug || verbose)
      bufr_print_debug( errmsg );

   msg = bufr_create_message( dts->tmplte->edition );
   bufr_copy_sect1( &(msg->s1), &(dts->s1) );
   tmplt = dts->tmplte;
   if (dts->header_string)
      {
      msg->header_string = strdup( dts->header_string );
      msg->header_len = strlen( dts->header_string );
      }

   if (x_compress > 0) 
      {
      x_compress = bufr_dataset_compressible( dts );
      }
   else if (x_compress < 0) 
      {
      x_compress = dts->data_flag & BUFR_FLAG_COMPRESSED;
      if (x_compress)
         {
         x_compress = bufr_dataset_compressible( dts );
         if (debug && !x_compress)
         bufr_print_debug( _("### Data not compressible\n") );
         }
      }

   if (dts->data_flag >= 0)
      {
      msg->s3.flag = (dts->data_flag & (BUFR_FLAG_OBSERVED|BUFR_FLAG_COMPRESSED));
      }

   if (x_compress)
      {
      /* other data, compressed */
      BUFR_SET_COMPRESSED( msg );
      }
   else 
      {
      /* other data, non-compressed */
      BUFR_SET_UNCOMPRESSED( msg );
      }
/*
 * empty all

*/
   bufr_begin_message( msg );
/*
 * construire la liste descripteurs de la section 3

*/
   nb_subsets = bufr_count_datasubset( dts );
   BUFR_SET_NB_DATASET(msg, nb_subsets);

   it = 0;

   count = arr_count( tmplt->codets );
   for (i = 0; i < count ; i++ )
      {
      bdv = (BufrDescValue *)arr_get( tmplt->codets, i );
      descriptor = bdv->descriptor;
      arr_add( msg->s3.desc_list, (char *)&descriptor ); ++it;
      }

/*
 * compute section 4 maximum length
 */
   blen = 0;
   nbits = 0;
   for (i = 0; i < nb_subsets ; i++)
      {
      subset = bufr_get_datasubset( dts, i );
      count = bufr_datasubset_count_descriptor( subset );
      for ( j = 0 ; j < count ; j++ )
         {
         bcv = bufr_datasubset_get_descriptor( subset, j );
         if (bcv->flags & FLAG_SKIPPED) continue;
         nbits += bcv->encoding.nbits + bcv->encoding.af_nbits;
         }
      blen += nbits / 8 ;
      nbits = nbits % 8;
      }

   blen += (nbits > 0) ? 1 : 0;
   bufr_alloc_sect4( msg, blen );

/*
 * encoder les donnees de la section 4

*/
   if (!BUFR_IS_COMPRESSED(msg))  /* sans compress */
      {
      for (i = 0; i < nb_subsets ; i++)
         {
         subset = bufr_get_datasubset( dts, i );
         count = bufr_datasubset_count_descriptor( subset );
         if (debug)
            {
            sprintf( errmsg, _("Storing Subset # %d\n"), i+1 );
            bufr_print_debug( errmsg );
            }
         for ( j = 0 ; j < count ; j++ )
            {
            bcv = bufr_datasubset_get_descriptor( subset, j );
            if (bcv->flags & FLAG_SKIPPED) continue;
            bufr_put_desc_value( msg, bcv );
            }
         }
      }
   else 
      { 
/*
 * all datasubset must have the same descriptor values count for compression, checked above 

*/
      subset = bufr_get_datasubset( dts, 0 );
      count = bufr_datasubset_count_descriptor( subset );
      if (debug)
         {
         sprintf( errmsg, _("Storing %d compressed Subsets of %d items\n"), nb_subsets, count );
         bufr_print_debug( errmsg );
         }
      for ( j = 0 ; j < count ; j++ )
         {
         bcv = bufr_datasubset_get_descriptor( subset, j );
         if (debug)
            {
            bufr_print_descriptor( errmsg, bcv );
            bufr_print_debug( errmsg );
            bufr_print_debug( "\n" );
            }

         if (bcv->flags & FLAG_SKIPPED) 
            {
            if (debug)
               bufr_print_debug( "\n" );
            continue;
            }

         bufr_put_af_compressed( msg, dts, bcv, j );

         switch (bcv->encoding.type)
            {
            case TYPE_CCITT_IA5 :
               bufr_put_ccitt_compressed( msg, dts, j );
               break;
            case TYPE_IEEE_FP :
               bufr_put_ieeefp_compressed( msg, dts, j );

               break;
            case TYPE_NUMERIC :
            case TYPE_CODETABLE :
            case TYPE_FLAGTABLE :
            case TYPE_CHNG_REF_VAL_OP :
               if (bcv->encoding.nbits <= 0) continue;

               bufr_put_numeric_compressed( msg, dts, bcv, j );

               break;
            default :
               if (debug)
                  bufr_print_debug( "\n" );
               break;
            }
         }
      }

   bufr_end_message( msg );

   if (msg->len_msg > BUFR_MAX_MSG_LEN)
      {
      sprintf( errmsg, _n("Warning: BUFR message length is %u octet. ", "Warning: BUFR message length is %u octets. ", msg->len_msg), 
               msg->len_msg );
      bufr_print_debug( errmsg );
      sprintf( errmsg, _n("It exceeds the maximum allowed of %d octet\n", "It exceeds the maximum allowed of %d octets\n", BUFR_MAX_MSG_LEN), 
               BUFR_MAX_MSG_LEN );
      bufr_print_debug( errmsg );
      }

   if (debug)
      {
      bufr_print_debug( "\n" );
      bufr_print_debug( NULL );
      }
   return msg;
   }

/**
 * @english
 * store a numeric of every subset in a dataset with compression
 * @param  msg : pointer to BUFR_Message where data are stored
 * @param  dts : pointer to BUFR_Dataset containing data to be stored
 * @param  bcv : BurCode of the numeric to be stored
 * @param  j   : position of bcv within each subset.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup message encode dataset internal
 */
static void bufr_put_numeric_compressed( BUFR_Message *msg, BUFR_Dataset *dts, BufrDescriptor *bcv, int j )
   {
   uint64_t    ival, ival2;
   uint64_t    imin, imax;
   int         i, nbinc;
   int         nb_subsets;
   DataSubset *subset;
   char        errmsg[256];
   int         debug = bufr_is_debug();
   uint64_t    missing, msng;
   int         nb_msng;

   missing = bufr_missing_ivalue( bcv->encoding.nbits );

   nb_subsets = bufr_count_datasubset( dts );
/*
 * find 1st valid min and max , not missing
 */
   nb_msng = 0;
   imin = imax = bufr_value2bits( bcv );
   for (i = 0; i < nb_subsets ; i++)
      {
      subset = bufr_get_datasubset( dts, i );
      bcv = bufr_datasubset_get_descriptor( subset, j );
      ival = bufr_value2bits( bcv );
      if (ival == missing) 
         {
         ++nb_msng;
         continue;
         }
      if (imin == missing)
         {
         imin = imax = ival;
         continue;
         }
      if (ival < imin) imin = ival;
      if (ival > imax) imax = ival;
      }
/*
 * same value for all subsets, ie all missing or a value
 */
   if (((imin == imax)&&(nb_msng == 0))||(nb_msng == nb_subsets))
      {
      bufr_putbits( msg, imin, bcv->encoding.nbits );  /* REF */
      bufr_putbits( msg, 0, 6 );                       /* NBINC */
      if (debug)
         {
         bufr_print_debug( "   " );
         if (bufr_print_dscptr_value( errmsg, bcv ))
            bufr_print_debug( errmsg );
         sprintf( errmsg, _n(" -> R0=0x%llx (%d bit) ", " -> R0=0x%llx (%d bits) ", bcv->encoding.nbits), 
               (unsigned long long)imin, bcv->encoding.nbits );
         bufr_print_debug( errmsg );
         sprintf( errmsg, _n("NBINC=%d (%d bit)\n", "NBINC=%d (%d bits)\n", 6 ), 0, 6 );
         bufr_print_debug( errmsg );
         }
      }
   else
      {
      bufr_putbits( msg, imin, bcv->encoding.nbits );  /* REF */
      imax -= imin;
      nbinc = bufr_value_nbits( imax );
      msng = bufr_missing_ivalue( nbinc );
      bufr_putbits( msg, nbinc, 6 );          /* NBINC */
      if (debug)
         {
         bufr_print_debug( "   " );
         sprintf( errmsg, _n(" REF=%ld -> R0=0x%llx (%d bit) ", " REF=%ld -> R0=0x%llx (%d bits) ", bcv->encoding.nbits), 
               imin, (long long)imin, bcv->encoding.nbits );
         bufr_print_debug( errmsg );
         sprintf( errmsg, _n("NBINC=%d (%d bit)\n", "NBINC=%d (%d bits)\n", 6), nbinc, 6 );
         bufr_print_debug( errmsg );
         }
      for (i = 0; i < nb_subsets ; i++)
         {
         subset = bufr_get_datasubset( dts, i );
         bcv = bufr_datasubset_get_descriptor( subset, j );
         ival2 = bufr_value2bits( bcv );
         if (ival2 == missing)
            ival = msng;
         else
            ival = ival2 - imin;
         bufr_putbits( msg, ival, nbinc );     /* Inc 's */
         if (debug)
            {
            bufr_print_debug( "   " );
            if (bufr_print_dscptr_value( errmsg, bcv ))
               bufr_print_debug( errmsg );
            sprintf( errmsg, _n(" -> R(%d)=0x%llx (%d bit)\n", " -> R(%d)=0x%llx (%d bits)\n", nbinc), i+1, (unsigned long long)ival, nbinc );
            bufr_print_debug( errmsg );
            }
         }
      }
   }

/**
 * bufr_put_ieeefp_compressed
 * @english
 * store an ieeefp of every subset of a Dataset
 * @param  msg : pointer to BUFR_Message where data are stored
 * @param  dts : pointer to BUFR_Dataset containing data to be stored
 * @param  j   : position of bcv within each subset.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void bufr_put_ieeefp_compressed( BUFR_Message *msg, BUFR_Dataset *dts, int j )
   {
   uint64_t    uval;
   int         i;
   int         nb_subsets;
   int         differs;
   DataSubset *subset;
   char        errmsg[256];
   double      dval0, dval;
   float       fval0, fval;
   int         debug = bufr_is_debug();
   BufrDescriptor   *bcv;

   nb_subsets = bufr_count_datasubset( dts );
   subset = bufr_get_datasubset( dts, 0 );
   bcv = bufr_datasubset_get_descriptor( subset, j );
   if (bcv->encoding.nbits == 64)
      dval0 = bufr_value_get_double( bcv->value );
   else
      fval0 = bufr_value_get_float( bcv->value );
   differs=0;
   for (i = 0; (i < nb_subsets)&&(differs == 0) ; i++)
      {
      subset = bufr_get_datasubset( dts, i );
      bcv = bufr_datasubset_get_descriptor( subset, j );
      if (bcv->encoding.nbits == 64)
         {
         dval = bufr_value_get_double( bcv->value );
         if (dval0 != dval) differs = 1;
         }
      else
         {
         fval = bufr_value_get_float( bcv->value );
         if (fval0 != fval) differs = 1;
         }
      }
   if (differs == 0)
      {
      if (bcv->encoding.nbits == 64)
         {
         uval = bufr_ieee_encode_double( dval0 );
         bufr_putbits( msg, uval, 64 );     /* REF */
         if (debug)
            {
            sprintf( errmsg, _n("   %E -> R0=%llx (%d bit) ", "   %E -> R0=%llx (%d bits) ", 64), 
                  dval0, (unsigned long long)uval, 64 );
            bufr_print_debug( errmsg );
            sprintf( errmsg, _n("NBINC=0 (%d bit)\n", "NBINC=0 (%d bits)\n", 6), 6 );
            bufr_print_debug( errmsg );
            }
         }
      else
         {
         uval = bufr_ieee_encode_single( fval0 );
         bufr_putbits( msg, uval, 32 );     /* REF */
         if (debug)
            {
            sprintf( errmsg, _n("   %E -> R0=%llx (%d bit) ", "   %E -> R0=%llx (%d bits) ", 32), 
                  fval0, (unsigned long long)uval, 32 );
            bufr_print_debug( errmsg );
            sprintf( errmsg, _n("NBINC=0 (%d bit)\n", "NBINC=0 (%d bits)\n", 6), 6);
            bufr_print_debug( errmsg );
            }
         }
      bufr_putbits( msg, 0, 6 );         /* NBINC */
      }
   else
      {
      if (bcv->encoding.nbits == 64)
         {
         uval = 0;
         bufr_putbits( msg, uval, 64 );     /* REF */
         bufr_putbits( msg, 8, 6 );         /* NBINC */
         if (debug)
            {
            sprintf( errmsg, _n("   %E -> R0=0x%llx (%d bit) ", "   %E -> R0=0x%llx (%d bits) ", 64), 
                  dval0, (unsigned long long)uval, 64 );
            bufr_print_debug( errmsg );
            sprintf( errmsg, _n("NBINC=%d (%d bit)\n", "NBINC=%d (%d bits)\n", 6), 8, 6);
            bufr_print_debug( errmsg );
            }
         }
      else
         {
         uval = 0;
         bufr_putbits( msg, uval, 32 );     /* REF */
         bufr_putbits( msg, 4, 6 );         /* NBINC */
         if (debug)
            {
            sprintf( errmsg, _n("   %E -> R0=0x%llx (%d bit) ", "   %E -> R0=0x%llx (%d bits) ", 32), 
                  fval0, (unsigned long long)uval, 32);
            bufr_print_debug( errmsg );
            sprintf( errmsg, _n("NBINC=%d (%d bit)\n", "NBINC=%d (%d bits)\n", 6), 4, 6);
            bufr_print_debug( errmsg );
            }
         }

      for (i = 0; i < nb_subsets ; i++)
         {
         subset = bufr_get_datasubset( dts, i );
         bcv = bufr_datasubset_get_descriptor( subset, j );
         if (bcv->encoding.nbits == 64)
            {
            dval = bufr_value_get_double( bcv->value );
            uval = bufr_ieee_encode_double( dval );
            bufr_putbits( msg, uval, 64 );
            if (debug)
               {
               sprintf( errmsg, _("   %E -> R(%d)=0x%llx (%d bits)\n"),
											  dval, i+1, (unsigned long long)uval, 64 );
               bufr_print_debug( errmsg );
               }
            }
         else
            {
            fval = bufr_value_get_float( bcv->value );
            uval = bufr_ieee_encode_single( fval );
            bufr_putbits( msg, uval, 32 );
            if (debug)
               {
               sprintf( errmsg, _("   %E ->  R(%d)=0x%llx (%d bits)\n"),
						fval, i, (unsigned long long)uval, 32 );
               bufr_print_debug( errmsg );
               }
            }
         }
      }
   }

/**
 * bufr_put_af_compressed
 * @english
 * store the associated field of a BufrDescriptor of every subset in a 
 *           dataset with compression
 * @param  msg : pointer to BUFR_Message where data are stored
 * @param  dts : pointer to BUFR_Dataset containing data to be stored
 * @param  bcv : BurCode of the numeric to be stored
 * @param  j   : position of bcv within each subset.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void bufr_put_af_compressed( BUFR_Message *msg, BUFR_Dataset *dts, BufrDescriptor *bcv, int j )
   {
   uint64_t    umin, umax, uval, uval2;
   int         i;
   int         nbinc;
   int         nb_subsets;
   DataSubset *subset;
   int         debug = bufr_is_debug();
   char        errmsg[256];

   if ((bcv->encoding.af_nbits <= 0)||(bcv->value->af == NULL)) return;

   umin = umax = bcv->value->af->bits;
   nb_subsets = bufr_count_datasubset( dts );
   for (i = 0; i < nb_subsets ; i++)
      {
      subset = bufr_get_datasubset( dts, i );
      bcv = bufr_datasubset_get_descriptor( subset, j );
      uval = bcv->value->af->bits;
      if (uval < umin) umin = uval;
      if (uval > umax) umax = uval;
      }
   if (umin == umax) 
      {
      bufr_putbits( msg, umin, bcv->value->af->nbits );  /* REF */
      bufr_putbits( msg, 0, 6 );                       /* NBINC */
      if (debug)
         {
         sprintf( errmsg, _n("   AF0=0x%llx (%d bit) ", "   AF0=0x%llx (%d bits) ", bcv->value->af->nbits), 
                     (unsigned long long)umin, bcv->value->af->nbits);
         bufr_print_debug( errmsg );
         sprintf( errmsg, _n("NBINC=%d (%d bit)\n", "NBINC=%d (%d bits)\n", 6), 0, 6 );
         bufr_print_debug( errmsg );
         }
      }
   else
      {
      bufr_putbits( msg, umin, bcv->encoding.nbits );  /* REF */
      umax -= umin;
      nbinc = bufr_value_nbits( umax );
      bufr_putbits( msg, nbinc, 6 );          /* NBINC */
      if (debug)
         {
         sprintf( errmsg, _n("   AF0=0x%llx (%d bit) ", "   AF0=0x%llx (%d bits) ", bcv->encoding.nbits), 
                     (unsigned long long)umin, bcv->encoding.nbits );
         bufr_print_debug( errmsg );
         sprintf( errmsg, _n("NBINC=%d (%d bit)\n", "NBINC=%d (%d bits)\n", 6), nbinc, 6 );
         bufr_print_debug( errmsg );
         }
      for (i = 0; i < nb_subsets ; i++)
         {
         subset = bufr_get_datasubset( dts, i );
         bcv = bufr_datasubset_get_descriptor( subset, j );
         if (bcv->value->af)
            {
            uval2 = bcv->value->af->bits;
            uval = uval2 - umin;
            bufr_putbits( msg, uval, nbinc );     /* Inc 's */
            if (debug)
               {
               sprintf( errmsg, _n("   0x%llx -> AF(%d)=0x%llx (%d bit)\n", " 0x%llx -> AF(%d)=0x%llx (%d bits)\n", nbinc), (unsigned long long)uval2, i+1, (unsigned long long)uval, nbinc );
               bufr_print_debug( errmsg );
               }
            }
         }
      }
   }

/*
 * @english
 * Return zero if string a is equivalent to string b according to
 * BUFR conventions. Which means ignore space-filling on the right,
 * and anything over the encoding length is truncated.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Chris Beauregard
 * @ingroup internal
 */
static int strbufrcmp(const char* a, const char* b, int alen, int blen,
	int enclen)
	{
	if( alen > enclen ) alen = enclen;
	if( blen > enclen ) blen = enclen;
	while(alen > 0 && a[alen-1]==' ') alen --;
	while(blen > 0 && b[blen-1]==' ') blen --;
	if (alen != blen) return alen - blen;
	return strncmp(a, b, alen);
	}

/**
 * bufr_put_ccitt_compressed
 * @english
 * store a string of all subset in a dataset with compression
 * @param  msg : pointer to BUFR_Message where data are stored
 * @param  dts : pointer to BUFR_Dataset containing data to be stored
 * @param  j   : position of bcv within each subset.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void bufr_put_ccitt_compressed(BUFR_Message *msg, BUFR_Dataset *dts, int j )
   {
   int       i;
   int             blen, blen0;
   int             nb_subsets;
   DataSubset     *subset;
   const char     *strval, *strval0;
   int             debug;
   int             differs;
   char            errmsg[2048];
   BufrDescriptor       *bcv;

   debug = bufr_is_debug();
/*
 * see if all string are the same
 */
   nb_subsets = bufr_count_datasubset( dts );
   subset = bufr_get_datasubset( dts, 0 );
   bcv = bufr_datasubset_get_descriptor( subset, j );
   strval0 = bufr_value_get_string( bcv->value, &blen0 );
   differs = 0;
   for (i = 0 ; (i < nb_subsets)&&(differs == 0) ; i++)
      {
      subset = bufr_get_datasubset( dts, i );
      bcv = bufr_datasubset_get_descriptor( subset, j );
      strval = bufr_value_get_string( bcv->value, &blen );
		if( strbufrcmp(strval0, strval, blen0, blen, bcv->encoding.nbits/8))
			{
			differs = 1;
			}
      }
/*
 * compression of CCITT_IA5 increase by 1 element + 6
 * writing R0 REF VALUE, this or a blank 
 */
   subset = bufr_get_datasubset( dts, 0 );
   bcv = bufr_datasubset_get_descriptor( subset, j );
   strval = bufr_value_get_string( bcv->value, &blen );
   if (differs == 0)
      {
		bufr_put_padstring( msg, strval, blen, bcv->encoding.nbits/8);
      bufr_putbits( msg, 0, 6 );              /* NBINC */
      if (debug)
         {
         bufr_print_debug( " R0=" );
         if (bufr_print_dscptr_value( errmsg, bcv ))
            bufr_print_debug( errmsg );
         sprintf( errmsg, _n(" NBINC=0 (%d bit)\n", " NBINC=0 (%d bits)\n", 6), 6 );
         bufr_print_debug( errmsg );
         }
      }
   else
      {
      BufrValue *bv = bufr_create_value( VALTYPE_STRING );
      bufr_value_set_string( bv, NULL, bcv->encoding.nbits/8 );
		bufr_putstring( msg, bufr_value_get_string( bv, &blen ), bcv->encoding.nbits/8 );
      bufr_putbits( msg, bcv->encoding.nbits/8, 6 );              /* NBINC */
      if (debug)
         {
         bufr_print_debug( " R0=" );
         if (bufr_print_value( errmsg, bv ))
            bufr_print_debug( errmsg );
         sprintf( errmsg, _n(" NBINC=%d (%d bit)\n", " NBINC=%d (%d bits)\n", 6), bcv->encoding.nbits/8, 6 );
         bufr_print_debug( errmsg );
         }
      bufr_free_value( bv );

      for (i = 0; i < nb_subsets ; i++)
         {
         subset = bufr_get_datasubset( dts, i );
         bcv = bufr_datasubset_get_descriptor( subset, j );
         strval = bufr_value_get_string( bcv->value, &blen );
			bufr_put_padstring( msg, strval, blen, bcv->encoding.nbits/8);
         if (debug)
            {
            sprintf( errmsg, "   R(%d)=", i+1 );
            bufr_print_debug( errmsg );
            if (bufr_print_dscptr_value( errmsg, bcv ))
               bufr_print_debug( errmsg );
            sprintf( errmsg, _n(" (%d bit)\n", " (%d bits)\n", bcv->encoding.nbits), bcv->encoding.nbits );
            bufr_print_debug( errmsg );
            }
         }
      }
   }

/**
 * @english
 *    subset = bufr_get_datasubset( dts, i )
 *    (BUFR Dataset *dts, int pos)
 * This call returns a data subset structure at the location pointed to by
 * “i” in the data subset array. This is called iteratively from the
 * count obtained in bufr_count_datasubset. The data read is in Section 4
 * of the BUFR message.
 * @warning The return value should not be freed; it will be freed when the
 * dataset (dts) is freed on its own.
 * @param  dts pointer to BUFR_Dataset 
 * @param  pos position of subset from 0 to n-1
 * @return a pointer to a DataSubset located at position
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset
 */
DataSubset *bufr_get_datasubset( BUFR_Dataset *dts, int pos )
   {
   DataSubset **ptr;

   ptr = (DataSubset **)arr_get( dts->datasubsets, pos );
   if (ptr) 
      return *ptr;
   else
      return (DataSubset *)NULL;
   }

/**
 * @english
 * encode a numerical value into bits for storage into output bitsream
 * @param  bd : pointer to BufrDescriptor containing value to encode
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static uint64_t bufr_value2bits( BufrDescriptor *bd )
   {
   double          dval;
   int64_t         ival;

   switch (bd->encoding.type)
      {
      case TYPE_NUMERIC :
         if (bd->encoding.nbits <= 32)
            {
            if (bd->value->type == VALTYPE_INT32)
               {
               ival = bufr_value_get_int32( bd->value );
/*
 * FAILSAFE: INT type may have reference or scale 

*/
               if ((bd->encoding.reference != 0)||(bd->encoding.scale != 0))
                  ival = bufr_cvt_fval_to_i32( bd->descriptor, &(bd->encoding), (float)ival );
               }
            else
               {
               dval = bufr_value_get_double( bd->value );
               ival = bufr_cvt_dval_to_i64( bd->descriptor, &(bd->encoding), dval );
               }
            }
         else
            {
            if (bd->value->type == VALTYPE_INT64)
               {
               ival = bufr_value_get_int64( bd->value );
/*
 * FAILSAFE: INT type may have reference or scale 

*/
               if ((bd->encoding.reference != 0)||(bd->encoding.scale != 0))
                  ival = bufr_cvt_dval_to_i64( bd->descriptor, &(bd->encoding), (double)ival );
               }
            else
               {
               dval = bufr_value_get_double( bd->value );
               ival = bufr_cvt_dval_to_i64( bd->descriptor, &(bd->encoding), dval );
               }
            }
         break;
      case TYPE_CHNG_REF_VAL_OP :
         ival = bufr_value_get_int32( bd->value );
         if (ival < 0)
            ival = bufr_negative_ivalue( ival, bd->encoding.nbits );
         break;
      case TYPE_CODETABLE :
      case TYPE_FLAGTABLE :
         ival = bufr_value_get_int32( bd->value );
         if (ival < -1)
            {
            ival = -1;
            }
         break;
      case TYPE_IEEE_FP   :
      case TYPE_CCITT_IA5 :
      default :
         {
         char    errmsg[2048];
         sprintf( errmsg, _("Error: internal problem in bufr_value2bits(), wrong type:%.6d (%d)"), 
                  bd->descriptor, bd->encoding.type );
	 bufr_print_debug( errmsg );
         bufr_print_descriptor( errmsg, bd );
         if (bufr_print_value( errmsg, bd->value ))
	    bufr_print_debug( errmsg );
         exit(1);
         }
      break;
      }

   if (ival == -1)
      ival = bufr_missing_ivalue( bd->encoding.nbits );

   return ival;
   }

/**
 * @english
 * store value of a BufrDescriptor into the Message bitstream
 * @param   bufr  :  pointer to output Message 
 * @param   bd    :  pointer to descriptor container with value
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void bufr_put_desc_value ( BUFR_Message *bufr, BufrDescriptor *bd )
   {
   const char     *strval;
   float           fval;
   double          dval;
   int             blen;
   uint64_t        ui64val;
   int32_t         i32val;
   int64_t         i64val;
   char            errmsg[256];
   int             isdebug=bufr_is_debug();
   BufrValue      *bv;

   if (bd == NULL) return;

   if ( bd->flags & FLAG_SKIPPED )
      {
      if (isdebug)
         {
         sprintf( errmsg, _("#Code: %.6d (0 bits)\n"), bd->descriptor );
         bufr_print_debug( errmsg );
         }
      return;
      }
   else
      {
      if (isdebug)
         {
         sprintf( errmsg, _("Code: %.6d "), bd->descriptor );
         bufr_print_debug( errmsg );
         if ( bd->s_descriptor != 0 )
            {
            sprintf( errmsg, "{%.6d} ", bd->s_descriptor );
            bufr_print_debug( errmsg );
            }
         }
      }

   if ((bd->encoding.af_nbits > 0)&&(bd->value->af != NULL))
      {
      if (isdebug)
         {
         sprintf( errmsg,
				_n("AFD: 0x%llx (%u bit) ", "AFD: 0x%llx (%u bits) ",
						bd->value->af->nbits), 
				(long long)(bd->value->af->bits), (unsigned)(bd->value->af->nbits));
         bufr_print_debug( errmsg );
         }
      bufr_putbits( bufr, bd->value->af->bits, bd->value->af->nbits );
      }

   switch (bd->encoding.type)
      {
      case TYPE_CCITT_IA5 :
         strval = bufr_value_get_string( bd->value, &blen );
         bv = NULL;
         if (strval == NULL)
            {
            if (isdebug)
               {
               sprintf( errmsg, _("Warning: expecting string but is null: descriptor=%d\n"), 
                                 bd->descriptor );
               bufr_print_debug( errmsg );
               }
            bv = bufr_create_value( VALTYPE_STRING );
            bufr_value_set_string( bv, NULL, bd->encoding.nbits/8 );
            strval = bufr_value_get_string( bv, &blen );
            }
         if (isdebug)
            {
            char  errmsgl[2048];

            sprintf( errmsgl, _n("STR: \"%s\" (%d bit) ", "STR: \"%s\" (%d bits) ", bd->encoding.nbits), strval, bd->encoding.nbits );
            bufr_print_debug( errmsgl );
            }
         bufr_put_padstring( bufr, strval, blen, bd->encoding.nbits/8 );
         if (bv) 
            {
            bufr_free_value( bv );
            bv = NULL;
            }
         break;
      case TYPE_IEEE_FP :
         if (isdebug)
            {
            sprintf( errmsg, _n("IEEE FP(%d bit) ", "IEEE FP(%d bits) ", bd->encoding.nbits), bd->encoding.nbits );
            bufr_print_debug( errmsg );
            }
         if (bd->encoding.nbits == 64)
            {
            dval = bufr_value_get_double( bd->value );
            ui64val = bufr_ieee_encode_double( dval );
            bufr_putbits( bufr, ui64val, bd->encoding.nbits );
            if (isdebug)
               {
               sprintf( errmsg, "%E", dval );
               bufr_print_debug( errmsg );
               }
            }
         else
            {
            fval = bufr_value_get_float( bd->value );
            ui64val = bufr_ieee_encode_single( fval );
            bufr_putbits( bufr, ui64val, 32 );
            if (isdebug)
               {
               sprintf( errmsg, "%E", fval );
               bufr_print_debug( errmsg );
               }
            }
         break;
      case TYPE_NUMERIC :
         if (isdebug)
            bufr_print_debug( _("NUM: ") );
         if (bd->encoding.nbits <= 32)
            {
            if (bd->value->type == VALTYPE_INT32)
               {
               i32val = bufr_value_get_int32( bd->value );
               if ((bd->encoding.reference != 0)||(bd->encoding.scale != 0))
                  {
                  ui64val = bufr_cvt_fval_to_i32( bd->descriptor, &(bd->encoding), (float)i32val );
                  }
               else if (i32val < 0)
                  {
                  ui64val = bufr_missing_ivalue(  bd->encoding.nbits );
                  }
               else
                  {
                  ui64val = i32val;
                  }
               if (isdebug)
                  {
                  sprintf( errmsg, "%llu", (unsigned long long)ui64val );
                  bufr_print_debug( errmsg );
                  }
               }
            else if (bd->value->type == VALTYPE_FLT32)
               {
               fval = bufr_value_get_float( bd->value );
               ui64val = bufr_cvt_fval_to_i32( bd->descriptor, &(bd->encoding), fval );
               if (isdebug)
                  {
                  if (bufr_is_missing_float( fval ))
                     sprintf( errmsg, _("MSNG --> %llu"), (unsigned long long)ui64val );
                  else
                     sprintf( errmsg, _("%f --> %llu"), fval, (unsigned long long)ui64val );
                  bufr_print_debug( errmsg );
                  }
               }
            else if (bd->value->type == VALTYPE_FLT64)
               {
               dval = bufr_value_get_double( bd->value );
               ui64val = bufr_cvt_dval_to_i64( bd->descriptor, &(bd->encoding), dval );
               if (isdebug)
                  {
                  if (bufr_is_missing_double( dval ))
                     sprintf( errmsg, _("MSNG --> %llu"), (unsigned long long)ui64val );
                  else
                     sprintf( errmsg, _("%f --> %llu"), dval, (unsigned long long)ui64val );
                  bufr_print_debug( errmsg );
                  }
               }
            }
         else
            {
            if (bd->value->type == VALTYPE_INT64)
               {
               i64val = bufr_value_get_int64( bd->value );
               if (isdebug)
                  {
                  sprintf( errmsg, _("%lld"), (long long)i64val );
                  bufr_print_debug( errmsg );
                  }
               if ((bd->encoding.reference != 0)||(bd->encoding.scale != 0))
                  {
/* FAILSAFE: INT type may have reference or scale */
                  ui64val = bufr_cvt_dval_to_i64( bd->descriptor, &(bd->encoding), (double)i64val );
                  }
               else
                  {
                  ui64val = i64val;
                  }
               }
            else
               {
               dval = bufr_value_get_double( bd->value );
               ui64val = bufr_cvt_dval_to_i64( bd->descriptor, &(bd->encoding), dval );
               if (isdebug)
                  {
                  if (bufr_is_missing_double( fval ))
                     sprintf( errmsg, _("MSNG --> %llu"), (unsigned long long)ui64val );
                  else
                     sprintf( errmsg, _("%f --> %llu"), dval, (unsigned long long)ui64val );
                  bufr_print_debug( errmsg );
                  }
               }
            }
         if (isdebug)
            {
            sprintf( errmsg, _n(" (%d bit) ", " (%d bits) ", bd->encoding.nbits), bd->encoding.nbits );
            bufr_print_debug( errmsg );
            }
         bufr_putbits( bufr, ui64val, bd->encoding.nbits );
         break;
      case TYPE_CHNG_REF_VAL_OP :
         i32val = bufr_value_get_int32( bd->value );
         if (isdebug)
            {
            sprintf( errmsg, _("INT: %lld "), (long long)i32val );
            bufr_print_debug( errmsg );
            }
         if (i32val < 0)	
            {
            ui64val = bufr_negative_ivalue( i32val, bd->encoding.nbits );
            if (isdebug)
               {
               sprintf( errmsg, _("--> %llu "), (unsigned long long)ui64val );
               bufr_print_debug( errmsg );
               }
            }
         else 
            {
            ui64val = i32val;
            }
         if (isdebug)
            {
            sprintf( errmsg, _n("(%d bit) ", "(%d bits) ", bd->encoding.nbits), bd->encoding.nbits );
            bufr_print_debug( errmsg );
            }
         bufr_putbits( bufr, ui64val, bd->encoding.nbits );
         break;
      case TYPE_CODETABLE :
      case TYPE_FLAGTABLE :
         i32val = bufr_value_get_int32( bd->value );
         if (isdebug)
            {
            sprintf( errmsg, _("INT: %lld "), (long long)i32val );
            bufr_print_debug( errmsg );
            }
         if (isdebug)
            {
            sprintf( errmsg, _n("(%d bit) ", "(%d bits) ", bd->encoding.nbits), bd->encoding.nbits );
            bufr_print_debug( errmsg );
            }
         if  (i32val < 0)
            {
            ui64val = bufr_missing_ivalue( bd->encoding.nbits );
            }
         else
            {
            ui64val = i32val;
            }
         bufr_putbits( bufr, ui64val, bd->encoding.nbits );
         break;
      default :
         if (isdebug)
            {
            bufr_print_debug( _("(0 bits) ") );
            }
      break;
      }

   if (isdebug)
      {
      bufr_print_debug( "\n" );
      bufr_print_debug( NULL );
      }
   }

/*
 * name: bufr_get_desc_value
 *
 * author:  Vanh Souvanlasy
 *
 * function: extract a value of a code from a BUFR Message
 *
 * parametres:
 * 
 *    bufr  :  pointer to output Message 
 *    bd    :  pointer to descriptor container with value

 */
static int bufr_get_desc_value ( BUFR_Message *bufr, BufrDescriptor *bd )
   {
   float           fval;
   double          dval;
   uint64_t        ival;
   uint64_t        ival2;
   int64_t         val;
   char            errmsg[512];
   int             errcode;

   int             isdebug=bufr_is_debug();

   if (bd == NULL) return 0;

   if ( bd->flags & FLAG_SKIPPED )
      {
      if (isdebug)
         {
         sprintf( errmsg, _("#Code: %.6d (0 bits)\n"), bd->descriptor );
         bufr_print_debug( errmsg );
         }
      return 0;
      }
   else
      {
      if (isdebug)
         {
         sprintf( errmsg, _("Code: %.6d "), bd->descriptor );
         bufr_print_debug( errmsg );
         }
      }

   if (bd->value == NULL)
      bd->value = bufr_mkval_for_descriptor( bd );
/*
 * this bd has no value
 */
   if ( bd->value == NULL ) 
      {
      if (isdebug)
         bufr_print_debug( "\n" );
      bufr_print_debug( NULL );
      return 0;
      }

   if (isdebug)
      {
      sprintf( errmsg, _n("AFD: (%d bit) ", "AFD: (%d bits) ", bd->encoding.af_nbits), 
            (int)bd->encoding.af_nbits );
      bufr_print_debug( errmsg );
      }

   if ((bd->encoding.af_nbits > 0)&&(bd->value->af != NULL))
      {
      bd->value->af->bits = bufr_getbits( bufr, bd->value->af->nbits, &errcode );
      if (isdebug)
         {
         sprintf( errmsg,
				_n("AFD: 0x%llx (%u bit) ", "AFD: 0x%llx (%u bits) ",
						bd->value->af->nbits), 
				(long long)(bd->value->af->bits), (unsigned)(bd->value->af->nbits));
         bufr_print_debug( errmsg );
         }
      if ( errcode < 0 ) return errcode;
      }

   switch (bd->encoding.type)
      {
      case TYPE_CCITT_IA5 :
         errcode = bufr_get_desc_ccittia5( bufr, bd, -1 );
         if ( errcode < 0 ) return errcode;
         break;
      case TYPE_IEEE_FP :
         errcode = bufr_get_desc_ieeefp( bufr, bd );
         if ( errcode < 0 ) return errcode;
         break;
      case TYPE_NUMERIC :
         val = ival = bufr_getbits( bufr, bd->encoding.nbits, &errcode );
         if ( errcode < 0 ) return errcode;
         ival2 = bufr_missing_ivalue(  bd->encoding.nbits );
         if  ((bd->value->type == VALTYPE_INT32)||(bd->value->type == VALTYPE_INT64))
            {
            if (ival == ival2)
               {
               int x;
               x = DESC_TO_X( bd->descriptor );
/* 
 * regulation 94.1.5 does not apply to class 31 
 */
               if (x != 31) 
                  val = -1;
               else
                  val = ival;
               }
            else
               {
               if (bd->encoding.reference != 0)
                  val = ival + bd->encoding.reference;
               else
                  val = ival;
               }
            bufr_value_set_int32( bd->value, val );
            if (isdebug)
               {
               sprintf( errmsg, _("IVAL=%lld "), (long long)val );
               bufr_print_debug( errmsg );
               }
            }
         else if (bd->value->type == VALTYPE_FLT32)
            {
            if (ival == ival2)
               fval = bufr_missing_float();
            else
               fval = bufr_cvt_i32_to_fval( &(bd->encoding), ival );
            bufr_value_set_float( bd->value, fval );
            if (isdebug)
               {
               if (bufr_is_missing_float( fval ))
                  strcpy( errmsg, _("FVAL=MISSING ") );
               else
                  sprintf( errmsg, _("FVAL=%E IVAL=%llu "), fval, (unsigned long long)ival );
               bufr_print_debug( errmsg );
               }
            }
         else if (bd->value->type == VALTYPE_FLT64)
            {
            if (ival == ival2)
               dval = bufr_missing_double();
            else
               dval = bufr_cvt_i64_to_dval( &(bd->encoding), ival );
            bufr_value_set_double( bd->value, dval );
            if (isdebug)
               {
               sprintf( errmsg, _("DVAL=%E"), dval );
               bufr_print_debug( errmsg );
               }
            }

         if (isdebug)
            {
            sprintf( errmsg, _n(" (%d bit)", " (%d bits)", bd->encoding.nbits), bd->encoding.nbits );
            bufr_print_debug( errmsg );
            }
         break;
      case TYPE_CHNG_REF_VAL_OP :
         ival = bufr_getbits( bufr, bd->encoding.nbits, &errcode );
         if ( errcode < 0 ) return errcode;
         val = bufr_cvt_ivalue( ival, bd->encoding.nbits );
         if (isdebug)
            {
            sprintf( errmsg, _n("IVAL=%llu INT: %lld (%d bit) ", "IVAL=%llu INT: %lld (%d bits) ", bd->encoding.nbits), 
                  (unsigned long long)ival, (long long)val, bd->encoding.nbits);
            bufr_print_debug( errmsg );
            }
         bufr_value_set_int32( bd->value, val );
         break;
      case TYPE_CODETABLE :
      case TYPE_FLAGTABLE :
         ival = bufr_getbits( bufr, bd->encoding.nbits, &errcode );
         if ( errcode < 0 ) return errcode;
         ival2 = bufr_missing_ivalue(  bd->encoding.nbits );
         if (ival == ival2) 
            {
/* 
 * exception: descriptor 20011's value can be same as missing value
 * No need to show as 15 and leave it as missing:
 * "For use of code figure 15, see Regulation 12.1.4." which is a section of the FM-12 code form that basically says 
 * "if the auto isn't * equipped to report a mandatory section, just code '/' instead" 
 * this equating of 15 to /// is just a fancy way of saying that the code figure 15 in 020011 means "missing"
 */
#if 0
            if (bd->descriptor != 20011)
               ival = -1;
#else
            ival = -1;
#endif
            }
         if (isdebug)
            {
            sprintf( errmsg, _n("IVAL=%llu (%d bit) ", "IVAL=%llu (%d bits) ", bd->encoding.nbits), (unsigned long long)ival, bd->encoding.nbits );
            bufr_print_debug( errmsg );
            }
         bufr_value_set_int32( bd->value, ival );
         break;
      default :
         if (isdebug)
            {
            bufr_print_debug( _("(0 bits) ") );
            }
      break;
      }
   if (isdebug)
      {
      if (isdebug)
         bufr_print_debug( "\n" );
      bufr_print_debug( NULL );
      }
   return 1;
   }

/**
 * @english
 * extract a string of a code from BUFR Message
 * @param   bufr  :  pointer to output Message 
 * @param   bd    :  pointer to descriptor container with value
 * @param   nb_octet : nb of octets to read overriding bd->encoding.nbits
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int bufr_get_desc_ccittia5
   ( BUFR_Message *bufr, BufrDescriptor *bd, int nb_octet )
   {
   char           *strval;
   int             errcode;

   if (nb_octet < 0) nb_octet = bd->encoding.nbits/8;
   strval = (char *)malloc( (nb_octet+1) * sizeof(char) );
   errcode = bufr_getstring( bufr, strval, nb_octet );
   bufr_descriptor_set_svalue( bd, strval );
   if (bufr_is_debug())
      {
      char errmsg[2048];

      sprintf( errmsg, _n("STR: [%s] (%d bit) ", "STR: [%s] (%d bits) ", 
               nb_octet*8), strval, nb_octet*8 );
      bufr_print_debug( errmsg );
      }
   free( strval );
   return errcode;
   }

/**
 * @english
 * extract an ieee fp value of a descriptor from BUFR Message
 * @param   bufr  :  pointer to output Message 
 * @param   bd    :  pointer to descriptor container of value
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int  bufr_get_desc_ieeefp( BUFR_Message *bufr, BufrDescriptor *bd )
   {
   int64_t         ival;
   char            errmsg[256];
   int             errcode;

   if (bd->value == NULL)
      bd->value = bufr_mkval_for_descriptor( bd );

   ival = bufr_getbits( bufr, bd->encoding.nbits, &errcode );
   if ( errcode < 0 ) return errcode;
   if ( bd->encoding.nbits == 64 )
      {
      double dval = bufr_ieee_decode_double( ival );
      bufr_value_set_double( bd->value, dval );
      if (bufr_is_debug())
         {
         sprintf( errmsg, _n("DVAL=%E (%d bit)", "DVAL=%E (%d bits)", 64), dval, 64 );
         bufr_print_debug( errmsg );
         }
      }
   else
      {
      float fval = bufr_ieee_decode_single( ival );
      bufr_value_set_float( bd->value, fval );
      if (bufr_is_debug())
         {
         sprintf( errmsg, _n("FVAL=%E (%d bit)", "FVAL=%E (%d bits)", 32), fval, 32 );
         bufr_print_debug( errmsg );
         }
      }
   return 1;
   }

/**
 * @english
 * Decode every subsets of a BUFR message
 * This is a higher-level call than bufr_read_message, the tables that have
 * been passed to this call should contain all of the elements referred to
 * by the message or it will fail and return NIL. The returned “dts” is
 * an object containing the returned decoded data and if an error occurs it
 * will contain nothing. This decodes a single entire message one at a
 * time.
 * @param msg the Message to decode
 * @param tables use the tables to resolve descriptors of the Message
 * @return data stored in a BUFR_Dataset
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup message decode
 */
BUFR_Dataset  *bufr_decode_message( BUFR_Message *msg, BUFR_Tables *tables )
   {
   return bufr_decode_message_subsets( msg, tables, 0, 0 );
   }

/**
 * @english
 * Decode selected range of subsets from a BUFR message
 * @param msg the Message to decode
 * @param tables use the tables to resolve descriptors of the Message
 * @param start position of subset, set to 0 for all
 * @param end position of subset
 * @return data stored in a BUFR_Dataset
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup message decode
 */
BUFR_Dataset  *bufr_decode_message_subsets( BUFR_Message *msg, BUFR_Tables *tables, int subset_from, int subset_to )
   {
   BufrDescValue   *codets;
   int             count;
   int            *code;
   BUFR_Template  *template;
   BUFR_Dataset    *dts;
   int             nbsubset;
   int             i, j;
   BUFR_Sequence   *bsq, *bsq2;
   BufrDescriptor      **pbcd;
   BufrDescriptor       *cb;
   ListNode       *node, *nnode, *node2;
   int             compressed;
   int             has_delayed_replication=0;
   int             f, x, y;
   int             debug=bufr_is_debug();
   char            errmsg[256];
   DataSubset     *subset = NULL;
   BufrDDOp       *ddo;
   int             errcode, skip;
   LinkedList     *tmplist;
   int             len, dts_len;
   int             flags;
   BUFR_DecodeInfo s4;


   if (bufr_table_is_empty( tables ))
      {
      bufr_print_debug( _("Error: BUFR Tables contains no entry, cannot decode message\n") );
      return NULL;
      }

   nbsubset   = msg->s3.no_data_subsets;
   if (subset_from > nbsubset) 
      {
      sprintf( errmsg, _("Error: Bad subset #%d, will not decode message\n"), subset_from );
      bufr_print_debug( errmsg );
      return NULL;
      }
   if (subset_from < 0) subset_from = 1;
   if (subset_to < subset_from) subset_to = subset_from;
   if (subset_to > nbsubset) subset_to = nbsubset;

   if (debug)
      bufr_print_debug( _("### Converting BUFR Message into Dataset\n") );

   if (tables->master.version <= 13)
      {
      if (msg->s1.master_table_version > tables->master.version)
         {
         if (debug)
            {
            sprintf( errmsg, _("Warning: master Table B version: %d less than message: %d\n"), 
                  tables->master.version, msg->s1.master_table_version );
            bufr_print_debug( errmsg );
            }
         }
      }
   else if (msg->s1.master_table_version != tables->master.version )
      {
      if (debug)
         {
         sprintf( errmsg, _("Warning: master Table B version: %d differs from message's: %d\n"), 
               tables->master.version, msg->s1.master_table_version );
         bufr_print_debug( errmsg );
         }
      }

   if (msg->enforce == BUFR_STRICT)
      {
      if (msg->s1.bufr_master_table == 10)
         {
         bufr_print_output( errmsg );
         bufr_print_debug( _("Error: Oceanography Tables are not supported by libecbufr\n") );
         return NULL;
         }
      if (msg->s1.bufr_master_table != 0)
         {
         sprintf( errmsg, _("Error: This Master Table number %d is not supported by WMO BUFR regulation. Check Octet 4 in section 1.\n"), 
                  msg->s1.bufr_master_table );
         bufr_print_output( errmsg );
         bufr_print_debug( errmsg );
         return NULL;
         }
      }

   count = arr_count( msg->s3.desc_list );
   codets = (BufrDescValue *)malloc( sizeof(BufrDescValue) * count );
   for (i = 0; i < count ; i++)
      {
      codets[i].nbval = 0;
      codets[i].values = NULL;
      code = (int *)arr_get( msg->s3.desc_list, i );
      codets[i].descriptor = *code;
      }

   if (tables == NULL) 
      {
      bufr_print_debug( _("Error: a BUFR_Tables is required to decode BUFR Message into Dataset\n") );
      return NULL;
      }
/*
 * create a template using only codes from message section 3

*/
   template = bufr_create_template( codets, count, tables, msg->edition );
   if (template == NULL)
      {
      free( codets );
      return NULL;
      }

   dts = bufr_create_dataset( template );
   if (dts == NULL)
      {
      bufr_free_template ( template );
      free( codets );
      return NULL;
      }

   s4.max_len = msg->s4.len;
   s4.len = 0;

   if ( msg->header_string )
      {
      if ( dts->header_string ) free( dts->header_string );
      dts->header_string = strdup( msg->header_string );
      }

   free( codets );
   bufr_free_template ( template );

   compressed = msg->s3.flag & BUFR_FLAG_COMPRESSED;

   count = arr_count( dts->tmplte->gabarit );
   bsq = bufr_create_sequence(NULL);
   pbcd = (BufrDescriptor **)arr_get( dts->tmplte->gabarit, 0 );
   if (debug)
      bufr_print_debug( _("### Descriptors in section 3\n") );
   for (i = 0; i < count ; i++)
      {
      cb = bufr_dupl_descriptor( pbcd[i] );
      if (debug)
         {
         sprintf( errmsg, "   %.6d\n", cb->descriptor );
         bufr_print_debug( errmsg );
         }
      bufr_add_descriptor_to_sequence( bsq, cb );
      }

   if (bufr_expand_sequence( bsq, OP_EXPAND_DELAY_REPL | OP_ZDRC_SKIP, dts->tmplte->tables ) < 0) return NULL;
   ddo = bufr_create_BufrDDOp( msg->enforce );
   bufr_apply_Tables( ddo, bsq,  dts->tmplte, NULL, &errcode ); 
   bufr_free_BufrDDOp( ddo );
   if (errcode < 0) dts->data_flag |= BUFR_FLAG_INVALID;

   flags = 0;
   bufr_check_sequence( bsq, NULL, &flags, dts->tmplte->tables, 0 );
   if (!compressed)
      {
      int nbits_seq=0, seq_len_is_const=1;
      int nbsubset1, j1;

      if (debug)
         bufr_print_debug( _("### Message is not compressed\n") );
      
      if (flags & HAS_DELAYED_REPLICATION)
         {
         if (subset_from > 0) 
            {
            if (debug)
                bufr_print_debug( _("### Warning: Message contains delayed replications, cannot skip\n") );
	    seq_len_is_const = 0;
            fprintf( stderr, "### Warning: Message contains delayed replications, cannot skip\n");
	    }
         }

      nbsubset1 = nbsubset;
      nbits_seq = bufr_estimate_seq_length( bsq, dts->tmplte->tables );
      if ( seq_len_is_const )
         {
         if (subset_from > 0) 
	    nbsubset1 = subset_to - subset_from + 1;
         if (subset_from > 1)
            bufr_skip_bits( msg, nbits_seq*(subset_from-1), &errcode );
	 }
/*
 * loop as many times as specified to fill in all the datasubsets
 */
      for (j = 0; j < nbsubset1 ; j++ )
         {
         bsq2 = bufr_copy_sequence( bsq );
         node = lst_firstnode( bsq2->list );
         ddo = bufr_create_BufrDDOp( msg->enforce );
         while ( node )
            {
            cb = (BufrDescriptor *)node->data;
            if (cb->flags & FLAG_SKIPPED)
               {
               ddo->current = node;
               bufr_apply_tables2node( ddo, bsq2, dts->tmplte, node, &errcode );
               if (errcode < 0)
	               dts->data_flag |= BUFR_FLAG_INVALID;
               node = lst_nextnode( node );
               continue;
               }

            f = DESC_TO_F( cb->descriptor );
            y = DESC_TO_Y( cb->descriptor );

            ddo->current = node;
            bufr_apply_tables2node( ddo, bsq2, dts->tmplte, node, &errcode ); 
            if (errcode < 0)
	            dts->data_flag |= BUFR_FLAG_INVALID;
            if (bufr_get_desc_value( msg, cb ) < 0)
               {
/*
 * terminate the loop and bail out nicely
*/
		snprintf( errmsg, sizeof(errmsg),
			_("Warning: premature end-of-data reading subset %d\n"), j);
		bufr_print_debug( errmsg );
		dts->data_flag |= BUFR_FLAG_INVALID;

               node = NULL;
               j = nbsubset;
               continue;
               }
            bufr_init_location( ddo, cb );
            bufr_apply_op_crefval( ddo, cb, dts->tmplte );

            if ((f == 1)&&(y == 0)) 
               {
               BufrDescriptor       *cb31;
               int             f2, x2;

               nnode = lst_nextnode( node );
               cb31 = (BufrDescriptor *)nnode->data;
               f2 = DESC_TO_F( cb31->descriptor );
               x2 = DESC_TO_X( cb31->descriptor );

               if ((f2 == 0)&&(x2 == 31))
                  {
                  int ival;
                  int  errcode=0;

                  if (bufr_get_desc_value( msg, cb31 ) < 0)
                     {
			snprintf( errmsg, sizeof(errmsg),
								_("Warning: premature end-of-data reading subset %d\n"), j);
			bufr_print_debug( errmsg );
			dts->data_flag |= BUFR_FLAG_INVALID;
                     node = NULL;
                     j = nbsubset;
                     continue;
                     }
                  ival = bufr_descriptor_get_ivalue( cb31 );
                  tmplist = bufr_expand_node_descriptor( bsq2->list, node, OP_EXPAND_DELAY_REPL|OP_ZDRC_IGNORE, tables, &skip, &errcode, &s4 );
                  if (errcode != 0)
                     {
                     dts->data_flag |= BUFR_FLAG_INVALID;
                     }
/*
 * check for bad replication that means section 4 should be much bigger 
 * since this is a very coarse estimation, we accept no more than 3 times its size
 */
                  if (tmplist)
                     {
                     int  msglen;
                     bsq2->list = tmplist;
                     len = bufr_estimate_seq_length( bsq2, tables );
                     msglen = (s4.len+len)/8;

                     if (msglen > (s4.max_len*3) )
                        {
                        char errmsg[256];

                        sprintf( errmsg, _("Error: BUFR Message is too short: %d > maxlen=%d\n"), msglen, s4.max_len );

                        bufr_print_debug( errmsg );
                        tmplist = NULL;
                        }
                     }

                  if (tmplist == NULL)
                     {
                     subset = bufr_allocate_datasubset();
                     bufr_fill_datasubset( subset, bsq2 );
                     arr_add( dts->datasubsets, (char *)&subset );

                     bufr_free_BufrDDOp( ddo );
                     bufr_free_sequence( bsq );
                     return dts;
                     }
                  node = lst_nextnode( node ); /* skip over class 31 code */
                  }
               }

            node = lst_nextnode( node );
            }

         bufr_free_BufrDDOp( ddo );
         ddo = NULL;

	 if (seq_len_is_const)
            s4.len += nbits_seq;
	 else
            s4.len += bufr_estimate_seq_length( bsq2, tables );

         j1 = j+1;
         /* bsq2 is freed by bufr_fill_datasubset */
	 if (seq_len_is_const || (subset_from<=0) || (j1 >= subset_from)&&(j1 <= subset_to))
            {
            subset = bufr_allocate_datasubset();
            arr_add( dts->datasubsets, (char *)&subset );
            bufr_fill_datasubset( subset, bsq2 );
	    subset = NULL;
	    }
         else
            {
            bufr_free_sequence( bsq2 );
	    }
         }
         if (seq_len_is_const && (subset_from > 0))
            {
	    nbsubset1 = subset_to - subset_from + 1;
            if (subset_from > 1)
            bufr_skip_bits( msg, nbits_seq*(nbsubset - subset_to), &errcode );
	    }
      }
   else
      {
      BUFR_Sequence   **bseq;
      BufrDDOp       **ddos;
      BufrDescriptor       *cb2;
      ListNode       **nodes;
      int             nbsubset1;

      if (debug)
         bufr_print_debug( _("### Message is compressed\n") );

      nbsubset1 = nbsubset;
      if (subset_from > 0) 
	 nbsubset1 = subset_to - subset_from + 1;
/*
 * allocates all subsets
 */
      bseq = (BUFR_Sequence **)malloc ( nbsubset1 * sizeof(BUFR_Sequence *) );
      ddos = (BufrDDOp **)malloc ( nbsubset1 * sizeof(BufrDDOp *) );
      nodes = (ListNode **)malloc ( nbsubset1 * sizeof(ListNode *) );

      for ( i = 0; i < nbsubset1 ; i++ )
         {
         bseq[i] = bufr_copy_sequence( bsq );
         subset = bufr_allocate_datasubset();
         arr_add( dts->datasubsets, (char *)&subset );
         ddos[i] = bufr_create_BufrDDOp( msg->enforce );
         nodes[i] = lst_firstnode( bseq[i]->list );
         }

      j = 1;
      node = lst_firstnode( bseq[0]->list );
      while ( node )
         {
         cb = (BufrDescriptor *)node->data;

         if (debug)
            {
            bufr_print_descriptor( errmsg, cb );
            bufr_print_debug( errmsg );
            }
         if (cb->flags & FLAG_SKIPPED)
            {
            for ( i = 0; i < nbsubset1 ; i++ )
               {
               node2 = nodes[i];
               ddos[i]->current = node2;
               bufr_apply_tables2node( ddos[i], bseq[i], dts->tmplte, node2, &errcode ); 
               if (errcode < 0)
	               dts->data_flag |= BUFR_FLAG_INVALID;
               nodes[i] = nodes[i]->next;
               }
            ++j;
            node = lst_nextnode( node );
            if (debug) bufr_print_debug( "\n" );
            continue;
            }

         for ( i = 0; i < nbsubset1 ; i++ )
            {
            node2 = nodes[i];
            ddos[i]->current = node2;
            bufr_apply_tables2node( ddos[i], bseq[i], dts->tmplte, node2, &errcode ); 
            if (errcode < 0)
	            dts->data_flag |= BUFR_FLAG_INVALID;
            }

         errcode = bufr_get_af_compressed( cb, nbsubset, msg, nodes, subset_from, subset_to );
         if (errcode < 0)
            {
            dts->data_flag |= BUFR_FLAG_INVALID;
	    node = NULL;
	    }

	/* assert( errcode >= 0 ) */
         switch (cb->encoding.type)
            {
            case TYPE_CCITT_IA5 :
               errcode = bufr_get_ccitt_compressed( cb, nbsubset, msg, nodes, subset_from, subset_to );
               break;
            case TYPE_IEEE_FP :
               errcode = bufr_get_ieeefp_compressed( cb, nbsubset, msg, nodes, subset_from, subset_to );
               break;
            case TYPE_NUMERIC :
            case TYPE_CODETABLE :
            case TYPE_FLAGTABLE :
            case TYPE_CHNG_REF_VAL_OP :
               errcode = bufr_get_numeric_compressed( cb, nbsubset, msg, nodes, subset_from, subset_to );
               break; 
            default : 
               if (debug) bufr_print_debug( "\n" );
               break; 
            }
         if (errcode < 0)
	    {
  	    dts->data_flag |= BUFR_FLAG_INVALID;
	    node = NULL;
	    }

/*
 * apply post value operations on BufrDescriptor

*/
         for ( i = 0; i < nbsubset1 ; i++ )
            {
            node2 = nodes[i];
            ddos[i]->current = node2;
            cb2 = (BufrDescriptor *)node2->data;
            bufr_init_location( ddos[i], cb2 );
            bufr_apply_op_crefval( ddos[i], cb2, dts->tmplte );
            }

         f = DESC_TO_F( cb->descriptor );
         x = DESC_TO_X( cb->descriptor );
         y = DESC_TO_Y( cb->descriptor );
         if (has_delayed_replication)
            {
            has_delayed_replication = 0;
            if ((f == 0)&&(x == 31))
               {
               int errcode, skip;

               errcode = 0;
               for (i = 0; i < nbsubset1 ; i++)
                  {
                  node2 = nodes[i]->prev;
                  tmplist = bufr_expand_node_descriptor( bseq[i]->list, node2, OP_EXPAND_DELAY_REPL|OP_ZDRC_IGNORE, tables, &skip, &errcode, &s4 );
                  if (errcode != 0)
                     {
                     dts->data_flag |= BUFR_FLAG_INVALID;                    
                     }
                  if (tmplist == NULL)
                     {
//                     bufr_free_dataset( dts );
                     for ( i = 0; i < nbsubset1 ; i++ )
                        {
                        bufr_free_BufrDDOp( ddos[i] );
                        bufr_free_sequence( bseq[i] );
                        }
                     free( bseq );
                     free( ddos );
                     free( nodes );
                     return dts;
                     }
                  bseq[i]->list = tmplist;
                  ddos[i]->current = node2;
                  }
               }
            }
         else if ((f == 1)&&(y == 0)) 
            {
            has_delayed_replication = 1;
            }

         ++j;
         for ( i = 0; i < nbsubset1 ; i++ )
            nodes[i] = nodes[i]->next;
         node = lst_nextnode( node );
         }

      for (i = 0; i < nbsubset1 ; i++)
         {
         subset = bufr_get_datasubset( dts, i );
/*
 * transfer ddo->dpbm to subset->dpbm
 */
         subset->dpbm = ddos[i]->dpbm;
         ddos[i]->dpbm = NULL;
/*
 * bseq[i] is freed by bufr_fill_datasubset 
 */
         bufr_reindex_sequence( bseq[i] );
         bufr_fill_datasubset( subset, bseq[i] ); 
         bseq[i] = NULL;
         bufr_free_BufrDDOp( ddos[i] );
         }
      free( bseq );
      free( ddos );
      free( nodes );
      }

   if (debug)
      {
      bufr_print_debug( "\n" );
      bufr_print_debug( NULL );
      }

   if ( bsq )
      {
      bufr_free_sequence( bsq );
      bsq = NULL;
      }

   bufr_copy_sect1( &(dts->s1), &(msg->s1) );

   dts->data_flag |= msg->s3.flag;

   if (msg->s1.bufr_master_table != 0)
      {
      dts->data_flag |= BUFR_FLAG_SUSPICIOUS; 
      sprintf( errmsg, _("Warning: Master Table number %d is not supported by WMO BUFR regulation.\n"), 
                  msg->s1.bufr_master_table );
      bufr_print_output( errmsg );
      bufr_print_debug( errmsg );
      }

   return dts;
   }

/**
 * @english
 * extract a block of compressed string elements of all subsets of a message
 * @param  cb    : pointer to BufrDescriptor
 * @param  bseq  : array of codelist of every subset
 * @param  nbsubset : number of subset in the message
 * @param  msg      : the Message containing bitstream data to decode
 * @param  j        : position of the current code in the subset.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int bufr_get_ccitt_compressed
   ( BufrDescriptor *cb, int nbsubset, BUFR_Message *msg, ListNode **nodes, int subset_from, int subset_to )
   {
   int             i, count, nbits;
   ListNode       *node2;
   int             debug=bufr_is_debug();
   char            *errmsg;
   char            *dstrptr;
   BufrDescriptor       *cb2;
   int             nbinc;
   int             errcode;
   int             missing;

   count = (subset_from > 0) ? subset_to - subset_from + 1 : nbsubset;

   if (debug)
      bufr_print_debug( "   R0=" );

   errcode = bufr_get_desc_ccittia5( msg, cb, -1 );
   if (errcode < 0) return errcode;

   nbinc = bufr_getbits( msg, 6, &errcode );

   if ( errcode < 0 ) return errcode;

/*
 * initial allocation for string buffer
 */
   dstrptr = arr_create( 2048, sizeof(char), 1024 );
   arr_inc( dstrptr, 2000 );
   errmsg = (char *) arr_get( dstrptr, 0 );

   if (debug)
      {
      if (bufr_print_value( errmsg, cb->value ))
         bufr_print_debug( errmsg );
      sprintf( errmsg, _n(" NBINC=%d (%d bit)\n", " NBINC=%d (%d bits)\n", 6), nbinc, 6 );
      bufr_print_debug( errmsg );
      }
/*
 * all values are missing
 */
   missing = bufr_missing_ivalue( 6 );
   if (nbinc == missing)
      {
      nbinc = 0;
      }

   nbits = nbinc * 8 ;

   if (subset_from > 1)
      bufr_skip_bits( msg, nbits*(subset_from-1), &errcode );

   for (i = 0; i < count ; i++)
      {
      node2 = nodes[i];
      cb2 = (BufrDescriptor *)node2->data;
      if (nbinc > 0)
         {
         if (debug)
            {
            sprintf( errmsg, "   R(%d)=", i+1 );
            bufr_print_debug( errmsg );
            }
         errcode = bufr_get_desc_ccittia5( msg, cb2, nbinc );
         if (errcode < 0) 
            {
            arr_free( &dstrptr );
            return errcode;
            }
         if (debug)
            {
            int slen;
/*
 * make sure buffer string is big enough
 */         
            bufr_value_get_string( cb2->value, &slen );
            if (slen >= arr_count(dstrptr) )
               {
               arr_inc( dstrptr, slen );
               errmsg = arr_get( dstrptr, 0 );
               }
            if (bufr_print_value( errmsg, cb2->value ))
               bufr_print_debug( errmsg );
            bufr_print_debug( "\n" );
            }
         }
      else
         {
         if (cb2->value == NULL)
            cb2->value = bufr_mkval_for_descriptor( cb );
         bufr_copy_value( cb2->value, cb->value );
         }
      }

   if (subset_from > 0)
       bufr_skip_bits( msg, nbits*(nbsubset-subset_to), &errcode );

   arr_free( &dstrptr );
   return 1;
   }

/**
 * @english
 * extract a block of compressed values of ieee fp
 * @param  cb       : pointer to BufrDescriptor
 * @param  nbsubset : number of subset in the message
 * @param  msg      : the Message containing bitstream data to decode
 * @param  nodes    : list of current node for eah subset
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int bufr_get_ieeefp_compressed
   ( BufrDescriptor *cb, int nbsubset, BUFR_Message *msg, ListNode **nodes, int subset_from, int subset_to )
   {
   int             i, count, nbits;
   ListNode       *node2;
   int             debug=bufr_is_debug();
   char            errmsg[256];
   BufrDescriptor       *cb2;
   int             nbinc;
   int             errcode;

   if (subset_from > 0)
      {
      node2 = nodes[0];
      cb2 = (BufrDescriptor *)node2->data;
      nbits = cb2->encoding.nbits;
      count = subset_to - subset_from + 1;
      }
   else 
      count = nbsubset;

   if (debug)
      bufr_print_debug( "   R0=" );
   errcode = bufr_get_desc_ieeefp( msg, cb );
   if (errcode < 0) return errcode;
   nbinc = bufr_getbits( msg, 6, &errcode );
   if (debug)
      {
      if (bufr_print_value( errmsg, cb->value ))
         bufr_print_debug( errmsg );
      sprintf( errmsg, _n(" NBINC=%d (%d bit)\n", " NBINC=%d (%d bits)\n", 6), nbinc, 6 );
      bufr_print_debug( errmsg );
      }
   if (errcode < 0) return errcode;

   if (subset_from > 1)
      bufr_skip_bits( msg, nbits*(subset_from-1), &errcode );

   for (i = 0; i < count ; i++)
      {
      node2 = nodes[i];
      cb2 = (BufrDescriptor *)node2->data;
      if (nbinc > 0)
         {
         if (debug)
            {
            sprintf( errmsg, "   R(%d)=", i+1 );
            bufr_print_debug( errmsg );
            }
         errcode = bufr_get_desc_ieeefp( msg, cb2 );
         if (debug)
            {
            if (bufr_print_value( errmsg, cb2->value ))
               bufr_print_debug( errmsg );
            bufr_print_debug( "\n" );
            }
         if (errcode < 0) return errcode;
         }
      else
         {
         if (cb2->value == NULL)
            cb2->value = bufr_mkval_for_descriptor( cb );
         bufr_copy_value( cb2->value, cb->value );
         }
      }

   if (subset_from > 0)
       bufr_skip_bits( msg, nbits*(nbsubset-subset_to), &errcode );

   return 1;
   }

/**
 * @english
 * extract a block of compressed numeric
 * @param  cb    : pointer to BufrDescriptor
 * @param  bseq  : array of codelist of every subset
 * @param  nbsubset : number of subset in the message
 * @param  msg      : the Message containing bitstream data to decode
 * @param  j        : position of the current code in the subset.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int bufr_get_numeric_compressed
   ( BufrDescriptor *cb, int nbsubset, BUFR_Message *msg, ListNode **nodes, int subset_from, int subset_to )
   {
   uint64_t        ival, ival2, imin;
   int             i, count;
   ListNode       *node2;
   int             debug=bufr_is_debug();
   char            errmsg[256];
   BufrDescriptor       *cb2;
   int             nbinc;
   int             errcode;
   uint64_t        missing, msng;

   count = (subset_from > 0) ? subset_to - subset_from + 1 : nbsubset;
   missing = bufr_missing_ivalue(  cb->encoding.nbits );
   imin = bufr_getbits( msg, cb->encoding.nbits, &errcode );
   if (debug)
      {
      sprintf( errmsg, _n("   R0=%llu (%d bit)", "   R0=%llu (%d bits)", cb->encoding.nbits), 
            (unsigned long long)imin, cb->encoding.nbits );
      bufr_print_debug( errmsg );
      }
   if ( errcode < 0 ) return errcode;
   nbinc = bufr_getbits( msg, 6, &errcode );
   if (debug)
      {
      sprintf( errmsg, _n(" NBINC=%d (%d bit)\n", " NBINC=%d (%d bits)\n", 6), 
            nbinc, 6 );
      bufr_print_debug( errmsg );
      }
   if (nbinc > cb->encoding.nbits)
      {
/* ERROR: NBINC can't be bigger than NBITS, corrupted ?? */
      sprintf( errmsg, _n("Warning: NBINC=%d is bigger than (%d bit)\n", 
                          "Warning: NBINC=%d is bigger than (%d bits)\n", 
                          cb->encoding.nbits), 
               nbinc, cb->encoding.nbits );
/* let it continue, this allow to see the message partially  */
/*    nbinc = 0; */
      bufr_print_debug( errmsg );
      errcode = -2;
      }
/*
 * all value are missing
 */
   msng = bufr_missing_ivalue( 6 );
   if (nbinc == msng)
      nbinc = 0; 

   if ( errcode < 0 ) return errcode;


   if (nbinc == 0) 
      {
      for (i = 0; i < count ; i++)
         {
         node2 = nodes[i];
         cb2 = (BufrDescriptor *)node2->data;
         if (cb2->value == NULL)
            cb2->value = bufr_mkval_for_descriptor( cb2 );
         bufr_descriptor_set_bitsvalue( cb2, imin );
         if (debug && cb2->value)
            {
            sprintf( errmsg, "   R(%d)=", i+1 );
            bufr_print_debug( errmsg );
            if (bufr_print_dscptr_value( errmsg, cb2 ))
               bufr_print_debug( errmsg );
            bufr_print_debug( "\n" );
            }
         }
      }
   else
      {
      if (subset_from > 1)
         bufr_skip_bits( msg, nbinc*(subset_from-1), &errcode );
      msng = bufr_missing_ivalue( nbinc );
      for (i = 0; i < count ; i++)
         {
         node2 = nodes[i];
         cb2 = (BufrDescriptor *)node2->data;
         ival = bufr_getbits( msg, nbinc, &errcode ); 
         if ( errcode < 0 ) return errcode;
         if (ival == msng)
            ival2 = missing;
         else
            ival2 = ival + imin;
         if (cb2->value == NULL)
            cb2->value = bufr_mkval_for_descriptor( cb2 );
         bufr_descriptor_set_bitsvalue( cb2, ival2 );
         if (debug)
            {
            sprintf( errmsg, _n("   R(%d)=%llx(%llx) (%d bit)", " R(%d)=%llx(%llx) (%d bits)", nbinc), i+1, (unsigned long long)ival2, (unsigned long long)ival, nbinc );
            bufr_print_debug( errmsg );
            if (bufr_print_dscptr_value( errmsg, cb2 ))
               bufr_print_debug( errmsg );
            bufr_print_debug( "\n" );
            }
         }
      if (subset_from > 0)
         bufr_skip_bits( msg, nbinc*(nbsubset-subset_to), &errcode );
      }
   return 1;
   }

/**
 * @english
 * extract a block of compressed associated fields
 * @param  cb    : pointer to BufrDescriptor
 * @param  bseq  : array of codelist of every subset
 * @param  nbsubset : number of subset in the message
 * @param  msg      : the Message containing bitstream data to decode
 * @param  j        : position of the current code in the subset.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor internal

 */
static int bufr_get_af_compressed
   ( BufrDescriptor *cb, int nbsubset, BUFR_Message *msg, ListNode **nodes, int subset_from, int subset_to )
   {
   uint64_t        ival, imin;
   int             i, count, nbits;
   ListNode       *node2;
   int             debug;
   char            errmsg[256];
   BufrDescriptor       *cb2;
   int             nbinc;
   int             errcode;

   if (cb->encoding.af_nbits <= 0) return 0;

   if (cb->value == NULL)
      cb->value = bufr_mkval_for_descriptor( cb );

   count = (subset_from > 0) ? subset_to - subset_from + 1 : nbsubset;
   debug = bufr_is_debug();
   imin = bufr_getbits( msg, cb->value->af->nbits, &errcode );
   if (debug)
      {
      sprintf( errmsg, _n(" A0=%llx (%d bit)", " A0=%llx (%d bits)", cb->value->af->nbits), 
            (unsigned long long)imin, cb->value->af->nbits );
      bufr_print_debug( errmsg );
      }
   if (errcode < 0) return errcode;
   nbinc = bufr_getbits( msg, 6, &errcode );
   if (debug)
      {
      sprintf( errmsg, _n(" NBINC=%d (%d bit)\n", " NBINC=%d (%d bits)\n", 6), 
            nbinc, 6 );
      bufr_print_debug( errmsg );
      }
   if (errcode < 0) return errcode;
   if (nbinc == 0) 
      {
      for (i = 0; i < count ; i++)
         {
         node2 = nodes[i];
         cb2 = (BufrDescriptor *)node2->data;
         if (cb2->value == NULL)
            cb2->value = bufr_mkval_for_descriptor( cb2 );
         cb2->value->af->bits = imin;
         if (debug && cb2->value)
            {
            sprintf( errmsg, _(" A(%d)=%llx"), i+1, (unsigned long long)imin );
            bufr_print_debug( errmsg );
            bufr_print_debug( "\n" );
            }
         }
      }
   else
      {
      if (subset_from > 1)
         bufr_skip_bits( msg, nbinc*(subset_from-1), &errcode );
      for (i = 0; i < count ; i++)
         {
         node2 = nodes[i];
         cb2 = (BufrDescriptor *)node2->data;
         ival = bufr_getbits( msg, nbinc, &errcode ); 
         if (errcode < 0) return errcode;
         ival = ival + imin;
         if (cb2->value == NULL)
            cb2->value = bufr_mkval_for_descriptor( cb2 );
         cb2->value->af->bits = ival;
         if (debug)
            {
            sprintf( errmsg, _n("   A(%d)=%llx (%d bit)", "   A(%d)=%llx (%d bits)", nbinc), i+1, (unsigned long long)ival, nbinc );
            bufr_print_debug( errmsg );
            bufr_print_debug( "\n" );
            }
         }
      if (subset_from > 0)
         bufr_skip_bits( msg, nbinc*(nbsubset-subset_to), &errcode );
      }
   return 1;
   }

/**
 * @english
 * @brief check if a dataset is compressible
 *
 * It is compressible when there are more than 1 datasubsets and if there are
 * delayed replication, all count must be identical codes list
 
 * @param dts pointer to a BUFR_Dataset
 * @return int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset encode
 */
int  bufr_dataset_compressible( BUFR_Dataset *dts )
   {
   int          i, j, count;
   DataSubset  *subsetref, *subset;
   BufrDescriptor    *coderef, *code;
   int          next_31=0;
   int          nrep1, nrep2;
   int          nb_subsets;
   int          d1, d2;
   int          f, x, y;
   char         errmsg[256];

/*
 * no need to compress with 1 single subset

*/
   nb_subsets = bufr_count_datasubset( dts );
   if (nb_subsets <= 1) return 0;

/*
 * data with delayed replication is compressible only when
 * all replication number are identical

*/
   subsetref = bufr_get_datasubset( dts, 0 );
   count = bufr_datasubset_count_descriptor( subsetref );
   for (i = 1; i < nb_subsets ; i++)
      {
      subset = bufr_get_datasubset( dts, i );
/*
 * first indication that delayed replication count are not the same
 * make sure all datasubsets have the same code values count, 

*/
      if (count != bufr_datasubset_count_descriptor( subset ))
         return 0;
/*
 * the above test should be enough, will skip the following test is slowing down very large dataset

*/
#if 0
      for (j = 0; j < count ; j++ )
         {
         coderef = bufr_datasubset_get_descriptor( subsetref, j );
         if (next_31)
            {
            next_31 = 0;
            d2 = coderef->descriptor;
            nrep1 = bufr_descriptor_get_ivalue( coderef );
            code = bufr_datasubset_get_descriptor( subset, j );
            nrep2 = bufr_descriptor_get_ivalue( code );
            if (nrep1 != nrep2) 
               {
               if (bufr_is_debug())
                  {
                  bufr_print_debug( _("### Dataset not compressible, delayed replication count differs\n"));
                  sprintf( errmsg, "### %d  %d (0)=%d  (%d)=%d\n", d1, d2, nrep1, j, nrep2 );
                  bufr_print_debug( errmsg );
                  }
               return 0;
               }
            }
         else
            {
            bufr_descriptor_to_fxy( coderef->descriptor, &f, &x, &y );
            if ((f == 1)&&(y == 0))
               {
               d1 = coderef->descriptor;
               next_31 = 1;
               }
            }
         }
#endif
      }
   return 1;
   }

/**
 * @english
 * Used to bundle data subsets into a larger dataset (e.g. one has ten
 * empty slots and wished to choose a particular slot. It is a requirement
 * for this call that the data subsets are following a single template.
 * This internally compares the templates of the two BUFR datasets to
 * verify if they are compatible; if not, an error is retuned. The
 * parameter ‘dts’ is the destination dataset where the data subset
 * will be copied. The second parameter is the destination position of the
 * subset starting from 0. The third parameter is the source dataset, the
 * fourth parameter is the starting position of the source dataset and the
 * fifth one is the number of data subsets to be copied.
 * @param   dest  destination Dataset
 * @param   dest_pos starting subset location where Datasubset from source will be copied
 *                if (dest_pos > number of subset in dest) then extra blank subset will be added
 * @param   src  source Dataset
 * @param   src_pos  starting subset location of Datasubset from source
 * @param   nb  number of subset to copy
 * @return int, It returns -1 if there is a template mismatch or return the
 * number of data subsets that have been copied if successful.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup encode dataset
 */
int bufr_merge_dataset ( BUFR_Dataset *dest, int dest_pos, BUFR_Dataset *src,  int src_pos, int nb )
   {
   int          destcount;
   int          srccount;
   int          pos;
   DataSubset  *ss, *dss, *oss;
   int          i;
   int          total=0;

   if (bufr_compare_template( dest->tmplte, src->tmplte ) != 0) return -1;

   srccount = bufr_count_datasubset( src );
   destcount = bufr_count_datasubset( dest );
   if (nb > srccount) nb = srccount;
   if (dest_pos >= destcount)
      {
      for (i = destcount; i <= dest_pos ; i++ )
         bufr_create_datasubset( dest );
      destcount = bufr_count_datasubset( dest );
      }

   for ( i = 0; i < nb ; i++ )
      {
      pos = i + dest_pos;
      ss = bufr_get_datasubset( src, src_pos + i );
      dss = bufr_duplicate_datasubset( ss );
      if ( pos < destcount )
         {
         oss = bufr_get_datasubset( dest, pos );
         bufr_free_datasubset( oss );
         arr_set( dest->datasubsets, pos, (char *)&dss );
         ++total;
         }
      else
         {
         arr_add( dest->datasubsets, (char *)&dss );
         ++total;
         }
      }
   return total;
   }


/**
 * @english
 *    bufr_load_dataset( dts, str_datafile )
 *    (BUFR_Dataset *dts, char *infile)
 * This loads a dataset initialised with bufr_create_dataset using data from
 * a dump file (cite Code Example later)from the function
 * bufr_dump_dataset. 
 * @warning The dump file should exactly match the template or there will
 * be a loading error, “dts” will not be valid (but neither set to
 * NULL). 
 * @warning The dump file may contain only 1 single dataset. for multiple
 * datasets in a dump file, use the function  bufr_read_dataset_text
 * @param   dts      destination Dataset
 * @param   infile   filename containing data to load, the data should match
 *                the template defined in the Dataset
 * @return int, Upon failure an integer return code will be set less than
 * or equal to zero (zero is where there’s no error but nothing’s
 * loaded).
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @deprecated by bufr_read_dataset_dump
 * @author Vanh Souvanlasy
 * @ingroup io encode dataset
 */
int bufr_load_dataset( BUFR_Dataset *dts,  const char *infile )
   {
   FILE          *fp;
   char           errmsg[256];
   int            status;
   int            lineno;

   if (infile == NULL) return -1;

   fp = fopen ( infile, "rb" ) ;
   if (fp == NULL) 
      {
      sprintf( errmsg, _("Error: can't open Datafile %s\n"), infile );
      bufr_print_debug( errmsg );
      return -1;
      }

   if ((lineno = bufr_load_header( fp, dts )) > 0)
      status = bufr_load_datasubsets( fp, dts, lineno, BUFR_STRICT );

   fclose ( fp ) ;
   if (status >= 0)
      return bufr_count_datasubset( dts );
   else
      return status;
   }

/**
 * @english
 *    bufr_read_dataset_dump( dts, fp )
 *    (BUFR_Dataset *dts, FILE *fp)
 * This reads 1 dataset from a dump file (output of bufr_dump_dataset())
 * @warning The dump file should exactly match the template or there will
 * be a loading error, will not be valid (but neither set to
 * NULL). 
 * @param   dts      destination Dataset
 * @param   fp       pointer to file containing data to read, the data should match
 *                   the template defined in the Dataset
 * @return int, Upon failure an integer return code will be set less than
 * or equal to zero (zero is where there’s no error but nothing’s
 * loaded).
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup io encode dataset
 */
int bufr_read_dataset_dump( BUFR_Dataset *dts, FILE *fp )
   {
   int status = 0;

   bufr_empty_datasubsets( dts );

   if ((status = bufr_load_header( fp, dts )) > 0)
      status = bufr_load_datasubsets( fp, dts, status, BUFR_STRICT );
   return status;
   }

/**
 * @english
 * load data stored in a file into a Dataset
 * @param   fp       :  file pointer containing data to load
 * @param   dts      :  destination Dataset
 * @param   ligne    :  string buffer for reading a line of string from the file
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup io dataset internal
 */
static int bufr_load_datasubsets( FILE *fp, BUFR_Dataset *dts, int lineno, BUFR_Enforcement enforce )
   {
   char            *errmsg;
   char            *dstrptr;
   BufrDescriptor     **pbcd;
   int            count;
   BUFR_Sequence  *bsq, *bsq2 = NULL;
   BufrDescriptor      *cb;
   ListNode      *node;
   BufrDDOp      *ddo=NULL;
   BUFR_Tables   *tbls;
   char          ligne[2048];
   char          msg[2048];

   char          *tok;
   int            icode;
   int64_t        ival64;
   int            ival;
   unsigned int   uival;
   float          fval;
   double         dval;
   char          *kptr = NULL, *ptr;
   int            i, len;
   int            errcode;
   LinkedList    *tmplist;
   int            debug;
   int            errflg=0;

   debug = bufr_is_debug();
   count = arr_count( dts->tmplte->gabarit );
   bsq = bufr_create_sequence(NULL);
   pbcd = (BufrDescriptor **)arr_get( dts->tmplte->gabarit, 0 );
   for (i = 0; i < count ; i++)
      {
      cb = bufr_dupl_descriptor( pbcd[i] );
      bufr_add_descriptor_to_sequence( bsq, cb );
      }
   ddo = bufr_create_BufrDDOp( BUFR_STRICT );
   bufr_apply_Tables( ddo, bsq,  dts->tmplte, NULL, &errcode );
   if (errcode < 0)
	   dts->data_flag |= BUFR_FLAG_INVALID;
   bufr_free_BufrDDOp( ddo );
   ddo = NULL;

   tbls = dts->tmplte->tables;

   node = NULL;

/*
 * initial allocation for string buffer
 */
   dstrptr = arr_create( 2048, sizeof(char), 1024 );
   arr_inc( dstrptr, 2000 );
   errmsg = (char *) arr_get( dstrptr, 0 );

   while ( fgets(ligne,2048,fp) != NULL )
      {
      ++lineno;
      if ( ligne[0] == '#' ) continue;
      if ( ligne[0] == '*' ) continue;

      if (strncmp( ligne, "BUFR_EDITION=", 13 ) == 0)
         {
         if (bsq2)
            {
            bufr_mkval_rest_sequence( tbls, bsq2, node, &errflg );
            bufr_add_datasubset( dts, bsq2, ddo );
            }
         fseek( fp, - strlen(ligne), SEEK_CUR );
         arr_free( &dstrptr );
         return 1;
         }

      if (strncmp( ligne, "DATASUBSET", 10 ) == 0)
         {
         if (bufr_is_verbose())
	    {
            sprintf( msg, _("Loading: %s\n"), ligne );
	    bufr_print_debug( msg );
	    }

         if (bsq2) 
            {
            bufr_mkval_rest_sequence( tbls, bsq2, node, &errflg );
            bufr_add_datasubset( dts, bsq2, ddo );
            }
         bsq2 = bufr_copy_sequence( bsq );
         node = lst_firstnode( bsq2->list );
         if ( ddo ) 
            {
            bufr_free_BufrDDOp( ddo );
            ddo = NULL;
            }
         continue;
         }
      if (ddo == NULL)
         {
         ddo = bufr_create_BufrDDOp( enforce );
         }

      if (kptr != NULL) 
         {
         free( kptr );
         kptr = NULL;
         }
      kptr = strdup( ligne );
      tok = strtok_r( kptr, " \t\n\r,=", &ptr );
      if (tok == NULL) continue;
      icode = atoi(tok);

      if (debug)
         {
         sprintf( errmsg, _("*** Input for: %d\n"), icode );
         bufr_print_debug( errmsg );
         }

      while ( node )
      	{
         cb = (BufrDescriptor *)node->data;
         if ((icode != cb->descriptor)&&(cb->flags & FLAG_SKIPPED))
            node = lst_nextnode( node );
         else
            break;
         }
      if (node == NULL)
         {
         sprintf( errmsg, _("Error: no more descriptors for data\n") );
         bufr_print_debug( errmsg );
         break;
         }
/*
 * current matching descriptor may be skipped but there could
 * be a valid descriptor needing this value further down
 */
      if ((icode == cb->descriptor)&&(cb->flags & FLAG_SKIPPED))
         {
         BufrDescriptor  *cb1 = cb;
         ListNode        *node1;
         node1 = lst_nextnode( node );
         while (node1)
            {
            cb1 = (BufrDescriptor *)node1->data;
            if (cb1->flags & FLAG_SKIPPED)
               node1 = lst_nextnode( node1 );
            else 
               break;
            }
         if (cb1->descriptor != icode)
            {
            node = lst_nextnode( node );
            continue;
            }
         else
            {
            node = node1;
            cb = cb1;
            }
         }

      if (icode != cb->descriptor)
         {
         sprintf( errmsg, _("Error: data descriptor %d mismatch with template %d\n"), 
                  icode, cb->descriptor );
         bufr_print_debug( errmsg );
         arr_free( &dstrptr );
         return -1;
         }

      ddo->current = node;
      bufr_apply_tables2node( ddo, bsq2, dts->tmplte, node, &errcode );
      if (errcode < 0)
	      dts->data_flag |= BUFR_FLAG_INVALID;

      if (cb->value == NULL)
         cb->value = bufr_mkval_for_descriptor( cb );
/*
 * remove spaces
 */
      len = strlen( ptr );
      i = len-1;
      while (isspace(ptr[i]) && (i >= 0)) 
         {
         ptr[i--] = '\0';
         }

      i = 0;
      len = strlen( ptr );
      while (isspace(ptr[i]) && (i < len)) ++i;

      if (ptr[i] == '{')  /* Skip Meta Info */
         {
         int j = len-1;
         if (debug)
            {
            int slen = strlen( ptr );
            if (slen >= arr_count(dstrptr) )
               {
               arr_inc( dstrptr, slen );
               errmsg = arr_get( dstrptr, 0 );
               }
            sprintf( errmsg, _("   *** skipping comment: '%s'"), ptr );
            bufr_print_debug( errmsg );
            }
         while ((ptr[j] != '}') && (j >= i)) --j;
         i = j;
         ptr = ptr+i+1;

         i = 0;
         len = strlen( ptr );
         while (isspace(ptr[i]) && (i < len)) ++i;

         if (debug)
            {
            int slen = strlen( ptr+i );
            if (slen >= arr_count(dstrptr) )
               {
               arr_inc( dstrptr, slen );
               errmsg = arr_get( dstrptr, 0 );
               }
            sprintf( errmsg, "-> '%s'\n", ptr+i );
            bufr_print_debug( errmsg );
            }
         }

      if (ptr[i] == '(')  /* AF bits */
         {
			unsigned long long afbits;
         int j;

         tok = strtok_r( NULL, " \t\n\r():", &ptr );
         sscanf( tok, "%llx", &afbits );
         if (debug)
            {
            sprintf( errmsg, _("   *** has AF: %s -> %llx\n"), tok, afbits );
            bufr_print_debug( errmsg );
            }

         if (cb->value->af != NULL)
				/* FIXME: may overflow; C99 guarantees
				 * sizeof(unsigned long long) >= sizeof(uint64_t), rather than
				 * sizeof(unsigned long long) == sizeof(uint64_t)
				 */
            cb->value->af->bits = afbits;
         else
            {
            sprintf( errmsg, _("Warning: can't set AF at line %d : %s"), lineno-1,  ligne );
            bufr_print_debug( errmsg );
            }

         len = strlen( ptr );
         while ((ptr[i] != ')') && (i < len)) ++i;
         if (ptr[i] == ')') i += 1;
         while (isspace(ptr[i]) && (i < len)) ++i;

         if (debug)
            {
            int slen = strlen( ptr+i );
            if (slen >= arr_count(dstrptr) )
               {
               arr_inc( dstrptr, slen );
               errmsg = arr_get( dstrptr, 0 );
               }
            sprintf( errmsg, _("   *** found value: '%s'\n"), ptr+i );
            bufr_print_debug( errmsg );
            }
         }

      if (ptr[i] == '"') /* QUOTED STRING */
         {
         ptr = ptr+i+1; 
         tok = strtok_r( NULL, "\n\r", &ptr );
         len = strlen( tok );
         for ( i = len-1 ; i > 0 ; i-- )
            {
            if (tok[i] == '"')
               {
               tok[i] = '\0';
               i = 0;
               }
            }
         }
      else
         {
         ptr = ptr+i; 
         tok = strtok_r( NULL, " \t\n\r=", &ptr );
         }

      if (tok == NULL) 
         {
         node = lst_nextnode( node );
         continue;
         }

      if (tok && cb->value)
         {
         switch( cb->value->type )
            {
            case VALTYPE_STRING :
               if (strcmp( tok, "MSNG" ) != 0)
                  {
                  bufr_descriptor_set_svalue( cb, tok );
                  }
               else
                  {
                  int len=cb->encoding.nbits/8;
                  char *tmpbuf;

                  tmpbuf = (char *)malloc( (len+1)*sizeof(char) );
                  bufr_missing_string( tmpbuf, len );
                  bufr_descriptor_set_svalue( cb, tmpbuf );
                  free( tmpbuf );
                  }
               if (debug)
                  {
                  int l;
                  char  *sval;
                  int   slen;

                  sval = bufr_descriptor_get_svalue( cb, &l );
                  slen = strlen( tok ) + 50 + l;
                  if (slen >= arr_count(dstrptr) )
                     {
                     arr_inc( dstrptr, slen );
                     errmsg = arr_get( dstrptr, 0 );
                     }
                  sprintf( errmsg, _("   *** has value: '%s' -> '%s'\n"), 
                        tok, sval );
                  bufr_print_debug( errmsg );
                  }
               break;
            case VALTYPE_INT32 :
            case VALTYPE_INT64 :
               if (strcmp( tok, "MSNG" ) != 0)
                  {
                  if ((cb->encoding.type == TYPE_FLAGTABLE) && bufr_str_is_binary( tok ))
                     ival64 = bufr_binary_to_int( tok );
                  else 
                     {
                     switch (tok[0])
                        {
                        case 'i' :
                           sscanf( tok+1, "%d", &ival );
                           ival64 = ival;
                        break;
                        case 'o' :
                           sscanf( tok+1, "%o", &uival );
                           ival64 = uival;
                        break;
                        case 'x' :
                           sscanf( tok+1, "%x", &uival );
                           ival64 = uival;
                        break;
                        case 'b' :
                           ival64 = bufr_binary_to_int( tok );
                        break;
                        default :
                           ival64 = atol(tok);
                        break;
                        }
                     }
                  }
               else
                  ival64 = bufr_missing_int();

					bufr_descriptor_set_ivalue( cb, ival64 );

               if (debug)
                  {
                  sprintf( errmsg, _("   *** has value: %s -> %lld\n"),
							tok, (long long)ival64 );
                  bufr_print_debug( errmsg );
                  }
               break;
            case VALTYPE_FLT64  :
               if (strcmp( tok, "MSNG" ) != 0)
                  {
                  dval = strtod( tok, NULL );
                  if (!bufr_is_missing_double( dval ))
                     bufr_descriptor_set_dvalue( cb, dval );
                  if (debug)
                     {
                     sprintf( errmsg, _("   *** has value: %s -> %f\n"), tok, dval );
                     bufr_print_debug( errmsg );
                     }
                  }
               break;
            case VALTYPE_FLT32  :
               if (strcmp( tok, "MSNG" ) != 0)
                  {
                  fval = strtof( tok, NULL );
                  if (!bufr_is_missing_float( fval ))
                     bufr_descriptor_set_fvalue( cb, fval );
                  if (debug)
                     {
                     sprintf( errmsg, _("   *** has value: %s -> %f\n"), tok, fval );
                     bufr_print_debug( errmsg );
                     }
                  }
               break;
            default :
               break;
            }
            bufr_init_location( ddo, cb );
            bufr_apply_op_crefval( ddo, cb, dts->tmplte );
         }
    
      if (cb->flags & FLAG_CLASS31)
         {
         ListNode  *nnode;
         BufrDescriptor  *cb2;
         int errcode, skip;

         errcode = 0;
         tmplist = bufr_expand_node_descriptor( bsq2->list, lst_prevnode( node ), 
               OP_EXPAND_DELAY_REPL|OP_ZDRC_IGNORE, tbls, &skip, &errcode, NULL );
         if (errcode != 0)
            dts->data_flag |= BUFR_FLAG_INVALID;                    
         if (tmplist == NULL)
            {
            arr_free( &dstrptr );
            return -1;
            }
         bsq2->list = tmplist;

         nnode = lst_nextnode( node );
         }
      node = lst_nextnode( node );
      }

   if (bsq2)
      {
      bufr_mkval_rest_sequence( tbls, bsq2, node, &errflg );
      bufr_add_datasubset( dts, bsq2, ddo );
      }

   if ( bsq )
      {
      bufr_free_sequence( bsq );
      bsq = NULL;
      }

   bufr_free_BufrDDOp( ddo );

   if (kptr != NULL) 
      {
      free( kptr );
      kptr = NULL;
      }

   arr_free( &dstrptr );

   if (bsq2) 
      return 1;
   else 
      return 0;
   }

/**
 * @english
 * complete the remaining descriptors of a sequence
 * @param   tbls     :  pointer to BUFR Tables
 * @param   bsq2     :  a sequence of descriptors
 * @param   node     :  starting point of the sequence
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void bufr_mkval_rest_sequence(BUFR_Tables   *tbls, BUFR_Sequence *bsq2, ListNode *node, int *errflg )
   {
   BufrDescriptor      *cb;
   int                  skip;

   while ( node )
      {
      cb = (BufrDescriptor *)node->data;
      if (!(cb->flags & FLAG_SKIPPED))
         {
         if (cb->value == NULL)
            {
            cb->value = bufr_mkval_for_descriptor( cb );
            }
         }
      if (cb->flags & FLAG_CLASS31)
         {
         bufr_expand_node_descriptor( bsq2->list, lst_prevnode( node ),
                  OP_EXPAND_DELAY_REPL|OP_ZDRC_IGNORE, tbls, &skip, errflg, NULL );
         }
      node = lst_nextnode( node );
      }
   }

/**
 * @english
 * load the header part of the data stored in a file into a Dataset
 * @param   fp       :  file pointer containing data to load
 * @param   dts      :  destination Dataset
 * @param   ligne    :  string buffer for reading a line of string from the file
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup io dataset internal
 */
static int bufr_load_header( FILE *fp, BUFR_Dataset *dts )
   {
   char    ligne[2048];
   char    errmsg[256];
   char    *tok;
   int     lineno=0;

   while ( fgets(ligne,2048,fp) != NULL )
      {
      ++lineno;
      if ( ligne[0] == '#' ) continue;
      if ( ligne[0] == '*' ) continue;

      if (strncmp( ligne, "BUFR_EDITION", 12 ) == 0)
         {
         tok = strtok( ligne+12, " =\t\n" );
         if (tok)
            {
            int ed = atoi( tok );
            if (dts->tmplte->edition != ed)
               {
               sprintf( errmsg, _("Warning: datafile bufr edition %d differs with template's %d\n"), 
                     ed, dts->tmplte->edition );
               bufr_print_debug( errmsg );
               }
            }
         }
      else if (strncmp( ligne, "BUFR_MASTER_TABLE", 17 ) == 0)
         {
         tok = strtok( ligne+17, " =\t\n" );
         if (tok)
            dts->s1.bufr_master_table = atoi( tok );
         }
      else if (strncmp( ligne, "ORIG_CENTER", 11 ) == 0)
         {
         tok = strtok( ligne+11, " =\t\n" );
         if (tok)
            BUFR_SET_ORIG_CENTRE( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "ORIG_SUB_CENTER", 15 ) == 0)
         {
         tok = strtok( ligne+15, " =\t\n" );
         if (tok)
            BUFR_SET_SUB_CENTRE( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "UPDATE_SEQUENCE", 15 ) == 0)
         {
         tok = strtok( ligne+15, " =\t\n" );
         if (tok)
            BUFR_SET_UPD_SEQUENCE( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "DATA_CATEGORY", 13 ) == 0)
         {
         tok = strtok( ligne+13, " =\t\n" );
         if (tok)
            BUFR_SET_DATA_CATEGORY( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "INTERN_SUB_CATEGORY", 19 ) == 0)
         {
         tok = strtok( ligne+19, " =\t\n" );
         if (tok)
            BUFR_SET_INTERN_SUB_CAT( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "LOCAL_SUB_CATEGORY", 18 ) == 0)
         {
         tok = strtok( ligne+18, " =\t\n" );
         if (tok)
            BUFR_SET_LOCAL_SUB_CAT( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "MASTER_TABLE_VERSION", 20 ) == 0)
         {
         tok = strtok( ligne+20, " =\t\n" );
         if (tok)
            BUFR_SET_MSTR_TBL_VRSN( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "LOCAL_TABLE_VERSION", 19 ) == 0)
         {
         tok = strtok( ligne+19, " =\t\n" );
         if (tok)
            BUFR_SET_LOCAL_TBL_VRSN( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "YEAR", 4 ) == 0)
         {
         tok = strtok( ligne+4, " =\t\n" );
         if (tok)
            BUFR_SET_YEAR( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "MONTH", 5 ) == 0)
         {
         tok = strtok( ligne+5, " =\t\n" );
         if (tok)
            BUFR_SET_MONTH( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "DAY", 3 ) == 0)
         {
         tok = strtok( ligne+3, " =\t\n" );
         if (tok)
            BUFR_SET_DAY( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "HOUR", 4 ) == 0)
         {
         tok = strtok( ligne+4, " =\t\n" );
         if (tok)
            BUFR_SET_HOUR( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "MINUTE", 6 ) == 0)
         {
         tok = strtok( ligne+6, " =\t\n" );
         if (tok)
            BUFR_SET_MINUTE( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "SECOND", 6 ) == 0)
         {
         tok = strtok( ligne+6, " =\t\n" );
         if (tok)
            BUFR_SET_SECOND( dts, atoi( tok ) );
         }
      else if (strncmp( ligne, "DATA_FLAG", 9 ) == 0)
         {
         tok = strtok( ligne+9, " =\t\n" );
         if (tok)
            dts->data_flag = atoi( tok );
         }
      else if (strncmp( ligne, "COMPRESSED", 10 ) == 0)
         {
         int compressed;
         tok = strtok( ligne+10, " =\t\n" );
         if (tok)
            {
            compressed = atoi( tok );
            if (compressed)
               dts->data_flag |= BUFR_FLAG_COMPRESSED;
            }
         }
      else if (strncmp( ligne, "HEADER_STRING", 13 ) == 0)
         {
         int i, len, b, e;
         len = strlen( ligne );
         for (b = 13; b < len ; b++)
            if (ligne[b] == '"')  break;
         if (b < len)
            {
            for (e = len-1; e > b ; e--)
               if (ligne[e] == '"')  break;
            if (e <= b) e = len;
            ligne[e] = '\0';
            dts->header_string = strdup( ligne+b+1 );
            }
         else
            {
            tok = strtok( ligne+13, "=\t\n" );
            if (tok)
               dts->header_string = strdup( tok );
            }
         }
      else
         {
         fseek( fp, - strlen(ligne), SEEK_CUR );
         if (strncmp( ligne, "DATASUBSET", 9 ) == 0 )
            return lineno;
         break;
         }
      }
   return 0;
   }

/**
 * @english
 *    bufr_dump_dataset( dts )
 *    (BUFR_Dataset *dts, char *outfile)
 * It dumps the content (data) that can be read back and re-encoded into a
 * BUFR message dataset. It should be used in conjunction with
 * bufr_save_tenplatein order to do testing.
 * @warning Not thread-safe
 * @param   dts      source Dataset
 * @param   filename file where data will be stored
 * @return int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @deprecated by bufr_fdump_dataset
 * @author Vanh Souvanlasy
 * @ingroup io decode dataset
 */
int bufr_dump_dataset( BUFR_Dataset *dts, const char *filename )
   {
   int            sscount;
   FILE          *fp;

   fp = fopen( filename, "a+" );
   if (fp == NULL) return -1;

   sscount = bufr_fdump_dataset( dts, fp );

   fclose( fp );
   return sscount;
   }

/**
 * @english
 *    bufr_fdump_dataset( dts )
 *    (BUFR_Dataset *dts, char *outfile)
 * It dumps the content (data) that can be read back and re-encoded into a
 * BUFR message dataset. It should be used in conjunction with
 * bufr_save_tenplatein order to do testing.
 * @warning Not thread-safe
 * @param   dts      source Dataset
 * @param   fp       file pointer where data will be stored
 * @return int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup io decode dataset
 */
int bufr_fdump_dataset( BUFR_Dataset *dts, FILE *fp )
   {
   DataSubset    *subset;
   int            sscount, cvcount;
   int            i, j;
   BufrDescriptor      *bcv;
   char           buf[2048];
   int            compressed;
   int            f, x, y;

   fprintf( fp, "BUFR_EDITION=%d\n", dts->tmplte->edition );
   if ( dts->header_string )
      fprintf( fp, "HEADER_STRING=\"%s\"\n", dts->header_string );
   fprintf( fp, "BUFR_MASTER_TABLE=%d\n", dts->s1.bufr_master_table );
   fprintf( fp, "ORIG_CENTER=%d\n", BUFR_GET_ORIG_CENTRE(dts) );
   if (dts->tmplte->edition >= 3)
      fprintf( fp, "ORIG_SUB_CENTER=%d\n", BUFR_GET_SUB_CENTRE(dts) );
   fprintf( fp, "UPDATE_SEQUENCE=%d\n", BUFR_GET_UPD_SEQUENCE(dts) );
   fprintf( fp, "DATA_CATEGORY=%d\n", BUFR_GET_DATA_CATEGORY(dts) );
   fprintf( fp, "INTERN_SUB_CATEGORY=%d\n", BUFR_GET_INTERN_SUB_CAT(dts) );
   fprintf( fp, "LOCAL_SUB_CATEGORY=%d\n", BUFR_GET_LOCAL_SUB_CAT(dts) );
   fprintf( fp, "MASTER_TABLE_VERSION=%d\n", BUFR_GET_MSTR_TBL_VRSN(dts) );
   fprintf( fp, "LOCAL_TABLE_VERSION=%d\n", BUFR_GET_LOCAL_TBL_VRSN(dts) );
   fprintf( fp, "YEAR=%d\n", BUFR_GET_YEAR(dts) );
   fprintf( fp, "MONTH=%d\n", BUFR_GET_MONTH(dts) );
   fprintf( fp, "DAY=%d\n", BUFR_GET_DAY(dts) );
   fprintf( fp, "HOUR=%d\n", BUFR_GET_HOUR(dts) );
   fprintf( fp, "MINUTE=%d\n", BUFR_GET_MINUTE(dts) );
   fprintf( fp, "SECOND=%d\n", BUFR_GET_SECOND(dts) );
   fprintf( fp, "DATA_FLAG=%d\n", dts->data_flag );
   compressed = dts->data_flag & BUFR_FLAG_COMPRESSED;
   fprintf( fp, "COMPRESSED=%d\n", compressed? 1 : 0 );
   
   sscount = bufr_count_datasubset( dts );
   for (i = 0; i < sscount ; i++)
      {
      subset = bufr_get_datasubset( dts, i );
      cvcount = bufr_datasubset_count_descriptor( subset );

      fprintf( fp, "DATASUBSET %d : %d codes\n", i+1, cvcount );

      for (j = 0; j < cvcount ; j++)
         {
         bcv = bufr_datasubset_get_descriptor( subset, j );

         if (bcv->flags & FLAG_SKIPPED)
            {
            if (bcv->flags & FLAG_IGNORED)
               fprintf( fp, "#%.6d ", bcv->descriptor );
            else
               fprintf( fp, "%.6d ", bcv->descriptor );
            }
         else
            {
            fprintf( fp, "%.6d ", bcv->descriptor );

            if ( bcv->s_descriptor != 0 )
               {
               fprintf( fp, "{%.6d} ", bcv->s_descriptor );
               }

            if ( bcv->meta )
               {
               bufr_print_rtmd_data( buf, bcv->meta );
               fprintf( fp, "%s ", buf );
               }

            if (bcv->value)
               {
               if (bcv->value->af)
                  {
                  bufr_print_af( buf, bcv->value->af );
                  fprintf( fp, "%s", buf );
                  }

               if (bufr_print_dscptr_value( buf, bcv ))
                  fprintf( fp, "%s", buf );
               }
            }
         fprintf( fp, "\n" );
         }
      fprintf( fp, "\n" );
      }
   return sscount;
   }

/**
 * @english
 * instantiate a BUFR_Dataset object
 *           from a BUFR_Sequence
 * @param    cl      :  liste of descriptors forming a sequence
 * @param    tbls    :  BUFR Tables in use
 * @param    edition :  BUFR edition to use
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup template dataset
 */
BUFR_Dataset *bufr_create_dataset_from_sequence 
   ( BUFR_Sequence *cl, BUFR_Tables *tbls, int edition )
   {
   BUFR_Dataset    *dts;
   BUFR_Template   *tmplt;
   int              i, count;
   BufrDescValue       *codes;
   ListNode        *node;
   BufrDescriptor        *bc;
   BufrDDOp        *ddo;
   int              errcode;

   count = lst_count( cl->list );

   codes = (BufrDescValue *)malloc( count * sizeof(BufrDescValue) );
   i = 0;
   node = lst_firstnode( cl->list );
   while ( node )
      {
      bc = (BufrDescriptor *)node->data;
      bufr_init_DescValue( &(codes[i]) );
      codes[i].descriptor = bc->descriptor;
      ++i;
      node = lst_nextnode( node );
      }

   tmplt = bufr_create_template( codes, count, tbls, edition );
   if (bufr_finalize_template( tmplt ) < 0) 
      {
      bufr_free_template ( tmplt );
      return NULL;
      }

   dts = bufr_create_dataset( tmplt );
   bufr_free_template( tmplt );

   ddo = bufr_create_BufrDDOp( BUFR_STRICT );
   bufr_apply_Tables( ddo, cl,  dts->tmplte, NULL, &errcode ); 
   bufr_free_BufrDDOp( ddo );
   if (errcode < 0)
	   dts->data_flag |= BUFR_FLAG_INVALID;

   if (errcode >= 0)
      {
      cl = bufr_copy_sequence( cl );
      bufr_add_datasubset( dts, cl, NULL );
      }
   else
      {
      bufr_free_dataset( dts );
      dts = NULL;
      }

   return dts;
   }

/**
 * @english
 *    bufr_genmsgs_from_dump( tmplt, infile, outfile )
 *    ( BUFR_Template *tmplt, const char *infile, const char *outfile )
 * This loads all datasets of a dump text file, as they will be read 1 by 1
 * and the stored in the output BUFR file.
 * @warning The dump file should exactly match the template or there will
 * be a loading error, “dts” will not be valid (but neither set to
 * NULL). 
 * @param   tmplt    template on which datafile messages are based on
 * @param   infile   filename containing data (1 or many bufr messages) to load
 * @param   outfile  output filename of the BUFR messages
 * @param   do_compress  if output message need to be compressed
 * @return int, Upon failure an integer return code will be set less than
 * or equal to zero (zero is where there’s no error but nothing’s
 * loaded).
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup io template encode
 */
int bufr_genmsgs_from_dump
   ( BUFR_Template *tmplt, const char *infile, const char *outfile, int do_compress )
   {
   FILE          *fpi, *fpo;
   int            debug;
   char           errmsg[1024];
   BUFR_Dataset  *dts;
   BUFR_Message  *msg = NULL;

   if (infile == NULL) return -1;
   if (outfile == NULL) return -1;

   debug = bufr_is_debug();

   fpi = fopen ( infile, "rb" ) ;
   if (fpi == NULL) 
      {
      sprintf( errmsg, _("Error: can't open input file %s\n"), infile );
      bufr_print_debug( errmsg );
      return -1;
      }

   fpo = fopen ( outfile, "wb" ) ;
   if (fpo == NULL) 
      {
      sprintf( errmsg, _("Error: can't open output file %s\n"), outfile );
      bufr_print_debug( errmsg );
      fclose( fpi );
      return -1;
      }

   dts = bufr_create_dataset( tmplt );

   while (bufr_read_dataset_dump( dts, fpi ) > 0)
      {
      if (msg != NULL)
         {
         bufr_free_message ( msg );
         fprintf( fpo, "\004" );
         msg = NULL;
         }

      msg = bufr_encode_message ( dts, do_compress );
      if (msg != NULL)
         {
         if (debug)
            {
            sprintf( errmsg, _n("Saving message with %d subset\n", "Saving message with %d subsets\n", bufr_count_datasubset( dts )), bufr_count_datasubset( dts ) );
            bufr_print_debug( errmsg );
            }
         bufr_write_message( fpo, msg );
         }
      }

   fclose ( fpi ) ;
   fclose ( fpo ) ;

   if ( msg != NULL )
      bufr_free_message ( msg );

   bufr_free_dataset( dts );
   return 1;
   }

