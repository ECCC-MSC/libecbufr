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
***/

#include <stdlib.h>
#include <stdio.h>
#include "bufr_array.h"
#include "bufr_linklist.h"
#include "bufr_io.h"
#include "bufr_af.h"
#include "bufr_tables.h"
#include "bufr_ddo.h"
#include "bufr_meta.h"
#include "bufr_i18n.h"

static int  bufr_match_increment ( int desc );
static void free_af_list         ( LinkedList *list );

static void print_msg_bad_ed_tco( BufrDDOp *ddo, char *errmsg );

/**
 * @english
 * @param   nb number of element code
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
BufrDPBM  *bufr_create_BufrDPBM    ( int nb )
   {
   BufrDPBM  *dpbm;
   int        i;

   dpbm           = (BufrDPBM *)malloc ( sizeof(BufrDPBM) );
   dpbm->index    = (int *)malloc( sizeof(int) * nb );
   dpbm->bit_map  = (int8_t *)malloc( sizeof(int8_t) * nb );
   dpbm->dp       = (int *)malloc( sizeof(int) * nb );
   dpbm->nb_dp    = 0;
   dpbm->nb_codes = nb;

   for (i = 0; i < nb ; i++ )
      {
      dpbm->index[i] = -1;
      dpbm->bit_map[i] = -1;
      dpbm->dp[i] = -1;
      }

   return dpbm;
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void  bufr_free_BufrDPBM ( BufrDPBM *dpbm )
   {
   if (dpbm == NULL) return;

   if (dpbm->index != NULL)
      {
      free( dpbm->index );
      dpbm->index = NULL;
      }
   if (dpbm->bit_map != NULL)
      {
      free( dpbm->bit_map );
      dpbm->bit_map = NULL;
      }
   if (dpbm->dp != NULL)
      {
      free( dpbm->dp );
      dpbm->dp = NULL;
      }
   dpbm->nb_codes = 0;
   dpbm->nb_dp    = 0;
   free( dpbm );
   }

/**
 * bufr_init_dpbm
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void bufr_init_dpbm( BufrDPBM *dpbm, ListNode *node )
   {
   int       i;
   int       val;
   BufrDescriptor *cb;
   char      buf[256];
   int       isdebug = bufr_is_debug();

/*
 * skip till we encounter the first 31031

*/
   while ( node )
      {
      cb = (BufrDescriptor *)node->data;
      if (cb->descriptor == 31031) break;
      node = lst_nextnode( node );
      }

   for ( i = 0; (i < dpbm->nb_codes) && (node != NULL) ; i++ )
      {
      cb = (BufrDescriptor *)node->data;
      val = bufr_descriptor_get_ivalue( cb );
      if (val == 0)
         {
         dpbm->bit_map[i] = val;
         dpbm->dp[dpbm->nb_dp] = i;
         ++dpbm->nb_dp;
         if (isdebug)
            {
            sprintf( buf, _("DPBM Pos=%d ON\n"), i );
            bufr_print_debug( buf );
            }
         }
      node = lst_nextnode( node );
      }
   if (isdebug)
      {
      sprintf( buf, _("DPBM DPCount=%d\n"), dpbm->nb_dp );
      bufr_print_debug( buf );
      }
   }

/**
 * @english
 *    ddo = *bufr_create_BufrDDOp(enforce)
 * This function creates the BufrDDOp (BUFR Data Descriptors Operator
 * structure), which is required for storing information for applying table
 * C operators.
 * @param  enforce : BUFR rules enforcement level
 * @warning Once it becomes used in “bufr_apply_Tables”, do not reuse;
 * the data will become corrupted; thus use only once for all codes at
 * once, or use sequentially according to the list codes. The structure,
 * when finished with, should be freed using bufr_free_BufrDDOp.
 * @return BufrDDOp
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
BufrDDOp *bufr_create_BufrDDOp( BUFR_Enforcement enforce )
   {
   BufrDDOp *ddo;

   ddo = (BufrDDOp *)malloc( sizeof(BufrDDOp) );
   ddo->af_list             = lst_newlist();

   ddo->current             = NULL;
   ddo->add_nbits           =  0;
   ddo->multiply_scale      =  0;
   ddo->add_af_nbits        =  0;
   ddo->local_nbits_follows =  0;
   ddo->use_ieee_fp         =  0;
   ddo->change_ref_val_op   =  0;
   ddo->change_ref_value    =  0;
   ddo->redefine_ccitt_ia5  =  0;

   ddo->flags               =  0;
   ddo->cnt_msg_bad_ed_tco  =  0;

   ddo->tlc_arr              = (LocationEncodingArray)arr_create( 20, sizeof(LocationEncoding), 10 );
   ddo->override_tableb      = (EntryTableBArray)arr_create( 32, sizeof(EntryTableB *), 128 );
   ddo->current_location = (LocationValueArray)arr_create( 4, sizeof(LocationValue), 4 );

   ddo->dpbm                = NULL;
   ddo->start_dpi           = NULL;
   ddo->remain_dpi          = -1;
   ddo->enforce             = enforce;
   return ddo;
   }

/**
 * @english
 *    bufr_free_BufrDDOp( ddo )
 *    (BufrDDOp *)
 * Frees BUFRDDOp structure when work is complete
 * @warning Ensure that all processing of nodes in the sequence has been
 * completed before this call.
 * @return void
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void bufr_free_BufrDDOp( BufrDDOp *ddo )
   {
   if (ddo == NULL) return;

   if (ddo->af_list)
      {
      free_af_list( ddo->af_list);
      ddo->af_list = NULL;
      }

   if ( ddo->dpbm )
      {
      bufr_free_BufrDPBM( ddo->dpbm );
      ddo->dpbm = NULL;
      }

   if ( ddo->current_location )
      {
      arr_free( &(ddo->current_location) );
      }

   if ( ddo->override_tableb )
      arr_free( &(ddo->override_tableb) );


   if ( ddo->tlc_arr )
      arr_free( &(ddo->tlc_arr) );

   free( ddo );
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void free_af_list (LinkedList *list)
   {
   ListNode      *node;
   AF_Definition  *af;

   node = lst_rmfirst( list );
   while (node)
      {
      af = (AF_Definition *)node->data;
      af->sig = NULL;
      free( af );
      lst_delnode( node );
      node = lst_rmfirst( list );
      }

   lst_dellist( list );
   }

/**
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 * @todo unused, should remove?
 */
int bufr_substitute_svmo
   ( ListNode *node, int desc_8083  )
   {
   return 0;
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int bufr_resolve_tableC_v2
   ( BufrDescriptor *cb, BufrDDOp *ddo, int x, int y, int version, ListNode *node )
   {
   switch( x )
      {
      case 1 :
         if (y == 0)
            ddo->add_nbits = 0;
         else
            ddo->add_nbits = y - 128;
         break;
      case 2 :
         if (y == 0)
            ddo->multiply_scale = 0;
         else
            ddo->multiply_scale = y - 128;
         break;
      case 3 :
         if (y == 255)   /* termination */
            {
            ddo->change_ref_val_op = 0;
            }
         else if (y == 0)
            {
/*
 * clearing override of table B 

*/
            ddo->change_ref_val_op = y;
            arr_del ( ddo->override_tableb, arr_count(ddo->override_tableb) );
            }
         else
            {
            ddo->change_ref_val_op = y;
            }
         break;
      case 4 :
         if (y > 0) /* add a new definition associated field */
            {
            AF_Definition *afr;
            
            afr = (AF_Definition *)malloc( sizeof(AF_Definition) );
            afr->nbits = y;
            node = lst_nextnode( node );
            afr->sig = node->data;
            lst_addlast ( ddo->af_list, lst_newnode( afr ) );
            ddo->add_af_nbits += y;
            }
         else /* cancel the most recently defined associated field */
            {
            AF_Definition  *afr;

            node = lst_rmlast( ddo->af_list );
            if (node)
               {
               afr = (AF_Definition *)node->data;
               ddo->add_af_nbits -= afr->nbits;
               free( afr );
               lst_delnode( node );
               }
            }
         break;
      case 5 :
         cb->encoding.type = TYPE_CCITT_IA5;
         cb->encoding.nbits = y * 8;
         break;
      case 6 : /* define next descriptor (local) bitlength */
         ddo->local_nbits_follows = y;
         break;
      default :
         {
         char errmsg[256];

         sprintf( errmsg, _("Warning: unsupported Table C operator %d in BUFR version %d\n"), 
               cb->descriptor, version );
         print_msg_bad_ed_tco( ddo, errmsg );
         return -1;
         }
      }
   return x;
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int bufr_resolve_tableC_v3
   ( BufrDescriptor *cb, BufrDDOp *ddo, int x, int y, int version, ListNode *node )
   {
   int   res;
   char  errmsg[256];
   int   op_version=3;
   int   bad_version=0;

   switch( x )
      {
      case 23 :
         if (version < op_version) bad_version = 1;
         if (y == 0)
            ddo->flags |= DDO_SUBST_VAL_FOLLOW;
         break;
      case 24 :
         if (version < op_version) bad_version = 1;
         if (y == 0)
            ddo->flags |= DDO_FO_STATS_VAL_FOLLOW;
         break;
      case 36 : 
         if (version < op_version) bad_version = 1;
         ddo->flags |= DDO_BIT_MAP_FOLLOW;
         break;
      case 37 : 
         if (version < op_version) bad_version = 1;
         ddo->flags |= DDO_USE_PREV_BIT_MAP;
         break;
      case 22 :
         if (version < op_version) bad_version = 1;
         if (y == 0)
            ddo->flags |= DDO_QUAL_INFO_FOLLOW;
         break;
      case 21 :
      case 25 :
      case 32 : 
      case 35 : 
         sprintf( errmsg, _("Warning: Table C operator %d has not been implemented\n"), x );
         bufr_print_debug( errmsg );
         return -1;
      default :
         res = bufr_resolve_tableC_v2 ( cb, ddo, x, y, version, node );
         if (res < 0)
            return -1;
         break;
      }

   if (bad_version && (ddo->enforce!=BUFR_LAX))
      {
      sprintf( errmsg, _("Warning: Illegal use of BUFR version %d Table C operator %d in BUFR version %d\n"), 
               op_version, cb->descriptor, version );
      print_msg_bad_ed_tco( ddo, errmsg );
      if (ddo->enforce == BUFR_STRICT) return -1; /* this will mark the message as invalid */
      }
   return x;
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int bufr_resolve_tableC_v4
   ( BufrDescriptor *cb, BufrDDOp *ddo, int x, int y, int version, ListNode *node )
   {
   int   res;
   char  errmsg[256];
   int   op_version=4;
   int   bad_version=0;

   switch( x )
      {
      case 7  :
         if (version < op_version) bad_version = 1;
         if ( y == 0 )
            {
            ddo->add_nbits = 0;
            ddo->multiply_scale = 0;
            ddo->change_ref_value = 0;
            }
         else
            {
            ddo->add_nbits = ((10 * y) + 2) / 3 ;
            ddo->multiply_scale = y;
            ddo->change_ref_value = y;
            }
         break;
      case 41 : /* define event operator, nothing to do */
         if (version < op_version) bad_version = 1;
         if (y == 255)
            ddo->flags &= ~DDO_DEFINE_EVENT;
         else
            ddo->flags |= DDO_DEFINE_EVENT;
         break;
      case 8  :
         if (version < op_version) bad_version = 1;
         ddo->redefine_ccitt_ia5 = y;
         break;
      case 42 : 
      case 43 : 
         {
         char errmsg[256];
         sprintf( errmsg, _("Warning: Table C operator %d has not been implemented\n"), x );
         bufr_print_debug( errmsg );
         return -1;
         }
      default :
         res = bufr_resolve_tableC_v3 ( cb, ddo, x, y, version, node );
         if (res < 0)
            return -1;
         break;
      }

   if (bad_version && (ddo->enforce!=BUFR_LAX))
      {
      sprintf( errmsg, _("Warning: Illegal use of BUFR version %d Table C operator %d in BUFR version %d\n"), 
               op_version, cb->descriptor, version );
      print_msg_bad_ed_tco( ddo, errmsg );
      if (ddo->enforce == BUFR_STRICT) return -1; /* this will mark the message as invalid */
      }
   return x;
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int bufr_resolve_tableC_v5
   ( BufrDescriptor *cb, BufrDDOp *ddo, int x, int y, int version, ListNode *node )
   {
   int   res;
   int   op_version=5;
   int   bad_version=0;

   switch( x )
      {
      case 9 :   /* IEEE 754 */
         if (version < op_version) bad_version = 1;
         if ((y == 32)||(y == 64))
            {
            ddo->use_ieee_fp = y;
            }
         else if (y == 0)
            {
            ddo->use_ieee_fp = 0;
            }
         else 
            {
            char errmsg[256];

            sprintf( errmsg, _("Warning: unsupported IEEE floating point width=%d, ignored\n"), y );
            bufr_print_debug( errmsg );
            }
         break;
      default :
         res = bufr_resolve_tableC_v4 ( cb, ddo, x, y, version, node );
         if (res < 0)
            return -1;
         break;
      }

   if (bad_version && (ddo->enforce!=BUFR_LAX))
      {
      char  errmsg[256];

      sprintf( errmsg, _("Warning: Illegal use of BUFR version %d Table C operator %d in BUFR version %d\n"), 
               op_version, cb->descriptor, version );
      print_msg_bad_ed_tco( ddo, errmsg );
      if (ddo->enforce == BUFR_STRICT) return -1; /* this will mark the message as invalid */
      }
   return x;
   }


/**
 * @english
 * print warning message on bad edition Table C operator
 * @endenglish
 * @francais
 * @todo 
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup encode descriptor
 */
static void print_msg_bad_ed_tco( BufrDDOp *ddo, char *errmsg )
   {
   if (ddo->cnt_msg_bad_ed_tco < BUFR_ERR_MSGS_LIMIT)
      {
      bufr_print_debug( errmsg );
      ddo->cnt_msg_bad_ed_tco += 1;
      if (BUFR_ERR_MSGS_LIMIT == ddo->cnt_msg_bad_ed_tco)
         {
         sprintf( errmsg, _("Warning: similar messages count limit reached. not printing anymore\n") );
         bufr_print_debug( errmsg );
         }
      }
   }

/**
 * @english
 * set associated field defintions
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup encode descriptor
 */
void bufr_set_descriptor_afd( BufrDescriptor *bc, LinkedList *af_list )
   {
   ListNode *node;
   AF_Definition  *afd;
   int       count, i;
   int       blens[256];

   i = 0;
   count = lst_count( af_list );
   node = lst_firstnode( af_list );
   while ( node )
      {
      afd = (AF_Definition *)node->data;
      blens[i++] = afd->nbits;
      node = lst_nextnode( node );
      }

   if (bc->afd)
      {
      bufr_free_afd( bc->afd );
      }
   bc->afd = bufr_create_afd( blens, count );

   i = 0;
   node = lst_firstnode( af_list );
   while ( node )
      {
      afd = (AF_Definition *)node->data;
      bc->afd->defs[i].sig = afd->sig;
      bc->afd->defs[i].nbits = afd->nbits;
      node = lst_nextnode( node );
      }

   if (bc->value)
      {
      bufr_set_value_af( bc->value, bc );
      }
   }

/**
 * @english
 * set associated field defintions
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void bufr_set_current_location( BufrDDOp *ddo, int code, float value, int npos )
   {
   int                i, count;
   LocationEncoding   *tlc, tlci;
   int                res;
   int                code2;

   res = bufr_is_location( code );
   code2 = bufr_match_increment( code );
   if ( res % 2 )
      {
      char  buf[256];

      if (bufr_is_debug())
         {
         sprintf( buf, _("Matching increment %.6d to %.6d incr=%f\n"), code, code2, value );
         bufr_print_debug( buf );
         }
       }

   count = arr_count( ddo->tlc_arr );

   for ( i = 0; i < count ; i++ )
      {
      tlc = (LocationEncoding *)arr_get( ddo->tlc_arr, i );
      if ( res % 2 )
         {
         if ((tlc->idescriptor == code)||(tlc->descriptor == code2))
            {
            tlc->increment = value;
            tlc->npos = npos;
            return;
            }
         }
      else
         {
         if ((tlc->descriptor == code)||(tlc->idescriptor == code2))
            {
            tlc->value += value;
            tlc->value0 = tlc->value;
            tlc->npos = npos;
            return;
            }
         }
      }

   if ( res % 2 )
      {
      tlci.idescriptor = code;
      tlci.descriptor = 0;
      tlci.increment = value;
      tlci.value  = 0;
      tlci.value0 = 0;
      }
   else
      {
      tlci.descriptor = code;
      tlci.idescriptor = 0;
      tlci.increment = 0;
      tlci.value  = value;
      tlci.value0 = value;
      }
   tlci.npos = npos;

   arr_add( ddo->tlc_arr, (char *)&tlci );
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void bufr_keep_location   ( BufrDDOp *ddo, int desc,  float value )
   {
   int           i, count;
   LocationValue  *tlc, tlci;

   count = arr_count( ddo->current_location );
   for (i = 0; i < count ; i++)
      {
      tlc = (LocationValue *)arr_get( ddo->current_location, i );
      if (tlc->descriptor == desc)
         {
         tlc->value = value;
         return;
         }
      }

   tlci.descriptor = desc;
   tlci.value = value;

   arr_add( ddo->current_location, (char *)&tlci );
   }

/**
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void bufr_clear_location ( BufrDDOp *ddo )
   {
   if (ddo->current_location)
      arr_del( ddo->current_location, arr_count( ddo->current_location ) );
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void bufr_assoc_location  ( BufrDescriptor *bc, BufrDDOp *ddo )
   {
   int           i, count;
   LocationValue  *tlc;
   char          buf[256];
   int           npos;
   int           isdebug=bufr_is_debug();
   
   count = arr_count( ddo->current_location );

   npos = -1;
   if (bc->meta == NULL)
      {
      bc->meta = bufr_create_rtmd( 0 );
      }
   else
      {
      if (bc->meta->tlc != NULL)
         free( bc->meta->tlc );
      bc->meta->nb_tlc = 0;
      npos = bc->meta->nb_nesting - 1;
      }
   if (count == 0) return;

   if (isdebug)
      {
      sprintf( buf, _("Associating %d\n"), bc->descriptor );
      bufr_print_debug( buf );
      }

   bc->meta->tlc = (LocationValue *)malloc( sizeof(LocationValue) * count );
   for (i = 0; i < count ; i++)
      {
      tlc = (LocationValue *)arr_get( ddo->current_location, i );
      bc->meta->tlc[i] = *tlc;
      bufr_set_current_location( ddo, tlc->descriptor, tlc->value, npos );

      if (isdebug)
         {
         sprintf( buf, _("   %d=%f npos=%d\n"), tlc->descriptor, tlc->value, npos );
         bufr_print_debug( buf );
         }
      }
   bc->meta->nb_tlc = count;
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
LocationValue  *bufr_current_location( BufrDDOp *ddo, BufrRTMD *meta, int *nbtlc )
   {
   int           i;
   int           count, cnt;
   LocationValue  *tlc;
   LocationEncoding  *tle;

   count = arr_count( ddo->tlc_arr );
   if (count == 0)
      {
      *nbtlc = 0;
      return NULL;
      }


   tlc = (LocationValue *)malloc ( sizeof(LocationValue) * count );

   cnt = 0;
   for (i = 0; i < count ; i++ )
      {
      tle = (LocationEncoding *)arr_get( ddo->tlc_arr, i );
      if (tle->descriptor == 0) continue;
      tlc[cnt].descriptor = tle->descriptor;
      if ((tle->npos >= 0)&&(tle->npos < meta->nb_nesting))
         {
         tlc[cnt].value = tle->value0 + meta->nesting[tle->npos] * tle->increment;
         tle->value = tlc[cnt].value;
         ++cnt;
         }
      }
   *nbtlc = cnt;
   return tlc;
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int bufr_is_location     ( int desc )
   {
   switch ( desc )
      {
      case 4011 : /* INCREMENT TEMPOREL ANNEE */
      case 4012 : /* INCREMENT TEMPOREL MOIS */
      case 4013 : /* INCREMENT TEMPOREL JOUR */
      case 4014 : /* INCREMENT TEMPOREL HEURE */
      case 4015 : /* INCREMENT TEMPOREL MIN */
      case 4016 : /* INCREMENT TEMPOREL SEC */
      case 4065 : /* INCREMENT TEMPOREL COURT MINUTES */
         return 1;
      case 5052 : /* INCREMENT DE NO DE CANAL */
      case 5011 : /* INCREMENT DE LATITUDE (HAUTE PRECISION) */
      case 5012 : /* INCREMENT DE LATITUDE (BASSE PRECISION) */
      case 6011 : /* INCREMENT LONGITUDE (HAUTE PRECISION) */
      case 6012 : /* INCREMENT LONGITUDE (BASSE PRECISION) */
      case 7005 : /* INCREMENT DE HAUTEUR */
         return 3;
      case 4021 : /* PERIODE OU DEPLACEMENT TEMPOREL ANNEE */
      case 4022 : /* PERIODE OU DEPLACEMENT TEMPOREL MOIS */
      case 4023 : /* PERIODE OU DEPLACEMENT TEMPOREL JOUR */
      case 4024 : /* PERIODE OU DEPLACEMENT TEMPOREL HEURE */
      case 4025 : /* PERIODE OU DEPLACEMENT TEMPOREL MIN */
      case 4026 : /* PERIODE OU DEPLACEMENT TEMPOREL SEC */
      case 4073 : /* PERIODE OU DEPLACEMENT TEMPOREL COURT JOUR */
      case 4074 : /* PERIODE OU DEPLACEMENT TEMPOREL COURT HEURE */
      case 4075 : /* PERIODE OU DEPLACEMENT TEMPOREL COURT MIN */
      case 4086 : /* PERIODE OU DEPLACEMENT TEMPOREL LONG SEC */
         return 2;
      case 5042 : /* NO DE CANAL */
      case 5015 : /* DEPLACEMENT DE LATITUDE (HAUTE PRECISION) */
      case 5002 : /* LATITUDE (BASSE PRECISION) */
      case 6015 : /* DEPLACEMENT LONGITUDE (HAUTE PRECISION) */
      case 6002 : /* LONGITUDE (BASSE PRECISION) */
      case 7002 : /* HAUTEUR */
         return 4;
      default :
         break;
      }
   return 0;
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int bufr_match_increment ( int desc )
   {
   switch ( desc )
      {
      case 4011 : /* INCREMENT TEMPOREL ANNEE */
         return 4021;
      case 4012 : /* INCREMENT TEMPOREL MOIS */
         return 4022;
      case 4013 : /* INCREMENT TEMPOREL JOUR */
         return 4023;
      case 4014 : /* INCREMENT TEMPOREL HEURE */
         return 4024;
      case 4015 : /* INCREMENT TEMPOREL MIN */
         return 4025;
      case 4016 : /* INCREMENT TEMPOREL SEC */
         return 4026;
      case 4065 : /* INCREMENT TEMPOREL COURT MINUTES */
         return 4075;
      case 5052 : /* INCREMENT DE NO DE CANAL */
         return 5042;
      case 5011 : /* INCREMENT DE LATITUDE (HAUTE PRECISION) */
         return 5015;
      case 5012 : /* INCREMENT DE LATITUDE (BASSE PRECISION) */
         return 5002;
      case 6011 : /* INCREMENT LONGITUDE (HAUTE PRECISION) */
         return 6015;
      case 6012 : /* INCREMENT LONGITUDE (BASSE PRECISION) */
         return 6002;
      case 7005 : /* INCREMENT DE HAUTEUR */
         return 7002;
      default :
         break;
      }
   return -1;
   }

/**
 * @english
 * check if descriptor is the starting code using data present bitmap
 *           which ends normal data descriptor sequence.
 * @param  descriptor  : data data descriptor
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int bufr_is_start_dpbm(int descriptor)
   {
   switch( descriptor )
      {
      case 222000 :
      case 223000 :
      case 224000 :
      case 225000 :
      case 232000 :
         return 1;
      default :
         break;
      }

   return  0;
   }

/**
 * @english
 * check if descriptor is the marker operator code of data present bitmap
 * @param  descriptor  : data data descriptor
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int bufr_is_marker_dpbm(int descriptor)
   {
   switch( descriptor )
      {
      case 223255 :
      case 224255 :
      case 225255 :
      case 232255 :
         return 1;
      default :
         break;
      }

   return  0;
   }

/**
 * @english
 * check if descriptor is a signify data width operator 
 * @param  descriptor  : data data descriptor
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int bufr_is_sig_datawidth(int descriptor)
   {
   int op = descriptor / 1000;
   return (op == 206);
   }
