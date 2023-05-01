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

 * fichier : bufr_sequence.c
 *
 * author:  Vanh Souvanlasy 
 *
 * function: 
 *
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bufr_linklist.h"
#include "bufr_array.h"
#include "bufr_desc.h"
#include "bufr_sequence.h"
#include "bufr_tables.h"
#include "bufr_ddo.h"
#include "bufr_io.h"
#include "bufr_template.h"
#include "bufr_i18n.h"

#define DEBUG         0
#define TESTINDEX     0

static int         bufr_is_dd_for_dpbm( BufrDescriptor *bcv );
static void        bufr_change_af_sig( BufrDDOp *ddo , char *sig );
static void        bufr_reassign_table2code( BufrDescriptor *bc, int f, EntryTableB *tb );
static void        bufr_assign_descriptors( ListNode *node, int nbdesc, int flags, BUFR_Tables * );
static LinkedList *bufr_expand_list( LinkedList *lst, int, BUFR_Tables *, int *errflg, BUFR_DecodeInfo *s4 );
static LinkedList *bufr_expand_desc( int desc, int, BUFR_Tables *, int *errflg, BUFR_DecodeInfo *s4 );
static void        bufr_free_descriptorNode(ListNode *node);
static void        bufr_free_descriptorList(LinkedList *list);
static LinkedList *bufr_repl_descriptors    ( ListNode *first, int nbdesc, int count, int, BUFR_Tables *, int *errflg, BUFR_DecodeInfo *s4 );
static void        cumulate_code2CodeArray( BufrDescriptor *cb, void *client_data );
static void        cumulate_code2CodeArrayValues( BufrDescriptor *cb, void *client_data );
static int         decrease_repeat_counters( int, LinkedList *stack, int *skip1 );

static int         bufr_solve_replication( int value, int y2, int descriptor );

static void        bufr_walk_sequence
                      ( BUFR_Sequence *bsq, void (*proc)(BufrDescriptor *, void * ), 
                        void *client_data  );
static void        free_rep_cnt_stack ( LinkedList *stack );

static int         bufr_simple_check_seq( BUFR_Sequence *bsq, ListNode *node, int depth );
static void        bufr_transfer_rtmd ( LinkedList *sublist, BufrRTMD *rtmd );

extern int         bufr_debugmode;
extern int         bufr_meta_enabled;

/**
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup template internal
 */
void bufr_free_sequence( BUFR_Sequence *bsq )
   {
   ListNode *node;
   LinkedList *list;

   if (bsq == NULL) return;

   if ( bsq->list )
      {
      bufr_free_descriptorList( bsq->list );
      bsq->list = NULL;
      }

   if ( bsq->index )
      {
      arr_free( &(bsq->index) );
      bsq->index = NULL;
      }

   free( bsq );
   }

/**
 * @english
 *    bsq = bufr_create_sequence(NULL)
 *    (LinkedList *)
 * This is a utility call to create a BUFR code list structure which is a
 * linked list of type BufrDescriptor. The index allows for random access.
 * @warning This is a building block and only an example of other building
 * blocks in the library. Not all of these building block calls are
 * currently documented. We will defer documentation of these type of API
 * calls until review by the group. If the list is changed in subsequent
 * calls, we need to call bufr_reindex_sequenceto refresh it.
 * @return BUFR_Sequence
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @see bufr_free_sequence
 * @author Vanh Souvanlasy
 * @ingroup template internal
 */
BUFR_Sequence *bufr_create_sequence(LinkedList *list)
   {
   BUFR_Sequence *bsq;

   bsq = (BUFR_Sequence *)malloc( sizeof(BUFR_Sequence) );
   assert( bsq );
   if (list == NULL)
      list = lst_newlist();
   bsq->list = list;
   bsq->index = NULL;
   return bsq;
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup template descriptor internal
 */
void bufr_add_descriptor_to_sequence( BUFR_Sequence *bsq, BufrDescriptor *cb )
   {
   LinkedList  *list;

   list = (bsq && bsq->list) ? bsq->list : NULL;
   if (list == NULL)
      {
      bufr_print_debug( _("Error in bufr_add_descriptor_to_sequence(): list is NULL\n") );
      return;
      }
   lst_addlast( list, lst_newnode( cb ) );
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup template internal
 */
int bufr_expand_sequence( BUFR_Sequence *bsq, int flags, BUFR_Tables *tbls )
   {
   LinkedList *lst1;
   int         errflg=0;

   if (bsq == NULL) return 0;

   lst1 = bufr_expand_list( bsq->list, flags, tbls, &errflg, NULL );
   if (lst1 == NULL)
      {
      bufr_free_descriptorList( bsq->list );
      bufr_print_debug( _("Error: cannot expand template sequence\n") );
      }
   bsq->list = lst1;
   return ((lst1 == NULL)||errflg) ? -1 : 1;
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
static LinkedList *bufr_expand_list( LinkedList *lst, int flags, BUFR_Tables *tbls, int *errflg, BUFR_DecodeInfo *s4 )
   {
   ListNode *node;
   int       skip;
   LinkedList *lst1;

   node = lst_firstnode( lst );
   while ( node )
      {
      lst1 = bufr_expand_node_descriptor( lst, node, flags, tbls, &skip, errflg, s4 );
      if (lst1 == NULL) return NULL;
      lst = lst1;
      if (skip > 0)
         {
         node = lst_skipnodes( node, skip );
         }
      else if (skip < 0)
         {
         ListNode *nnode;

         while ((skip < 0)&& node)
            {
            nnode = lst_nextnode( node );
            bufr_free_descriptorNode( lst_rmnode( lst, node ) );
            node = nnode;
            skip += 1;
            }
         }
      else
         {
         node = lst_nextnode( node );
         }
      }
   return lst;
   }

/**
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup template internal
 */
BUFR_Sequence *bufr_expand_descriptor( int desc, int flags, BUFR_Tables *tbls, int *errflg )
   {
   BUFR_Sequence  *bsq;
   LinkedList  *list;

   list = bufr_expand_desc ( desc, flags, tbls, errflg, NULL );
   if (list == NULL) return NULL;
   bsq = bufr_create_sequence( list );
   return bsq;
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
static LinkedList *bufr_expand_desc( int desc, int flags, BUFR_Tables *tbls, int *errflg, BUFR_DecodeInfo *s4 )
   {
   EntryTableD   *etblD;
   LinkedList    *lst, *lst1;
   int           i, count;
   int           code;
   BufrDescriptor  *bcd;
   int  f;

   f = DESC_TO_F( desc );
   if (f != 3) return NULL;

   etblD = bufr_fetch_tableD( tbls, desc );
   if (etblD == NULL) 
      {
      return NULL;
      }

   lst = lst_newlist();

   count = etblD->count;
   for (i = 0; i < count ; i++ )
      {
      code = etblD->descriptors[i];
      f = DESC_TO_F( code );
      bcd = bufr_create_descriptor( tbls, code );
      if (f == 0)
         {
         if (bcd->encoding.nbits == -1)
            {
            EntryTableB *tb = bcd->etb ? bcd->etb : bufr_fetch_tableB( tbls, code );
            if (tb)
               bcd->encoding = tb->encoding;
            else if ((i > 0)&& bufr_is_sig_datawidth( etblD->descriptors[i-1] ))
               {
               /* known size from Table C operator 206 */
               }
            else
               {
               char errmsg[256];
               bufr_free_descriptorList( lst );

               if (errflg) *errflg = 1;
               sprintf( errmsg, _("Error: unknown descriptor in bufr_expand_desc %d\n") , 
                     code );
               bufr_print_debug( errmsg );
               return NULL;
               }
            }
         }
      lst_addlast( lst, lst_newnode( bcd ) );
      }

   lst1 = bufr_expand_list( lst, flags, tbls, errflg, s4 );
   if (lst1 == NULL)
      bufr_free_descriptorList( lst );
   return lst1;
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
LinkedList *bufr_expand_node_descriptor( LinkedList *list, ListNode *node, int flags, BUFR_Tables *tbls, int *skip, int *errflg, BUFR_DecodeInfo *s4 )
   {
   int         f, x, y;
   BufrDescriptor   *cb;
   LinkedList *sublist;
   int   err;
   int depth, tmpflags;
   BUFR_Sequence *bsq;
   char errmsg[256];
   int  edition;

   if (node == NULL) return list;
   cb = (BufrDescriptor *)node->data;

   *skip = 0;
   if (cb->flags & FLAG_SKIPPED) return list;
   if (cb->flags & FLAG_EXPANDED) return list;

   sublist = NULL;

   f = DESC_TO_F( cb->descriptor );
   x = DESC_TO_X( cb->descriptor );
   y = DESC_TO_Y( cb->descriptor );
   if (f == 1)
      {
      if (y > 0)
         {
         cb->flags |= FLAG_EXPANDED | FLAG_SKIPPED;
         sublist = bufr_repl_descriptors( lst_nextnode( node ), x, y, flags, tbls, errflg, s4 );
         *skip = -(x + 1);
         if (sublist == NULL)
            {
            sprintf( errmsg, _("Error: invalid replication: code=%d\n"), cb->descriptor );
            bufr_print_debug( errmsg );
            if (errflg) *errflg = 1;
            return NULL;
            }
         }
      else
         {
         int  f2, x2, y2;
         ListNode  *nnode;
         BufrDescriptor *cb31;

         nnode = lst_nextnode( node );
         if (nnode == NULL) 
            {
            if (errflg) *errflg = 1;
            cb->flags |= FLAG_EXPANDED | FLAG_SKIPPED;
            return list;
            }

         cb31 = (BufrDescriptor *)nnode->data;
         f2 = DESC_TO_F( cb31->descriptor );
         x2 = DESC_TO_X( cb31->descriptor );
         y2 = DESC_TO_Y( cb31->descriptor );
         if ((f2 == 0)&&(x2 == 31))
            {
            int rep = 0;
            int value = bufr_value_get_int32( cb31->value );
/*
 * this value should never be less than 0, this will corrupt everything
 * it is safer to set it to 0,
 */
            if (value < 0)
               {
               if (cb31->value == NULL)
                  cb31->value = bufr_mkval_for_descriptor( cb31 );
               bufr_value_set_int32( cb31->value, 0 );
               value = 0;
               }
            rep = bufr_solve_replication( value, y2, cb31->descriptor );
            if (rep < 0) rep = 0;
            cb31->flags |= FLAG_CLASS31;
            if ( (rep > 0) && (flags&OP_EXPAND_DELAY_REPL) ) /* delayed replication */
               { 
               sublist = bufr_repl_descriptors( lst_nextnode( nnode ), x, rep, flags, tbls, errflg, s4 );
               if (sublist == NULL)
                  {
                  sprintf( errmsg, _("Error: invalid delayed replication code=%d  count=%d\n"), cb->descriptor, rep );
                  bufr_print_debug( errmsg );
                  if (errflg) *errflg = 1;
                  return NULL;
                  }
               nnode = lst_rmnode( list, nnode );
               lst_addfirst( sublist, nnode );
               *skip = -(x + 1);
               cb->flags |= FLAG_EXPANDED | FLAG_SKIPPED;
               cb31->flags |= FLAG_EXPANDED;
               }
            else /* delayed replication to be resolved later, or never */
               {
               bufr_assign_descriptors(  lst_nextnode( nnode ), x, flags, tbls );
               sublist = NULL;
               *skip = x + 2;
               }
            }
         else
            {
            sprintf( errmsg, _("Error: delayed replication not followed by class 31 code=%d\n") , 
                     cb31->descriptor );
            bufr_print_debug( errmsg );
            if (errflg) *errflg = 1;
            *skip = -1;
            return list;
            }
         }
      }
   else if (f == 3)
      {

      cb->flags |= FLAG_EXPANDED | FLAG_SKIPPED;
      sublist = bufr_expand_desc( cb->descriptor, flags, tbls, errflg, s4 );
      if (sublist == NULL) 
         {
         if (errflg) *errflg = 1;
         return NULL;
         }

      if (cb->meta)
         {
         bufr_transfer_rtmd( sublist, cb->meta );
         }

//      if (bufr_meta_enabled && (cb->meta == NULL))
//         cb->meta = bufr_create_rtmd( 0 );

      if ( 1 )
         {
         depth = cb->meta ? cb->meta->nb_nesting : 0 ;
         bsq = bufr_create_sequence( list );
         tmpflags = 0;
/*       err = bufr_check_sequence( bsq, node, &tmpflags, tbls, depth ); */
         err = bufr_simple_check_seq( bsq, node, depth );

         bsq->list = NULL;
         bufr_free_sequence( bsq );

         if (err < 0) 
            {
            if (errflg) *errflg = 1;
            return NULL;
            }

         bsq = bufr_create_sequence( sublist );
         err = bufr_simple_check_seq( bsq, NULL, depth );
         bsq->list = NULL;
         bufr_free_sequence( bsq );
         }
      *skip = -1;
      }
   else if (f == 0)
      {
      if (x == 31)
         cb->flags |= FLAG_CLASS31;
      }

   if (*skip < 0)
      {
      ListNode   *node1;
      ListNode   *nnode;

      *skip += 1;
      node1 = lst_nextnode( node );
      while ((*skip < 0)&& node1)
         {
         nnode = lst_nextnode( node1 );
         bufr_free_descriptorNode( lst_rmnode( list, node1 ) );
         node1 = nnode;
         *skip += 1;
         }
      if (flags & OP_RM_XPNDBL_DESC)
         *skip = -1;
      }

   if (sublist)
      {
      if (flags & OP_RM_XPNDBL_DESC)
         {
         ListNode   *prev;
      /* 
       * insert new items before expand code
		 */
         prev = lst_prevnode(node);
         if (lst_count( sublist ) > 0)
            {
            lst_movelist( list, prev, sublist ); 
            }
         }
      else
         {
      /* 
       * insert new items after expand code, then skip over them
       */
         *skip = lst_count( sublist );
         if (*skip > 0)
            {
            lst_movelist( list, node, sublist );
            *skip += 1;
            }
         }
      lst_dellist( sublist );
      }

   if (cb->meta)
      {
      cb->meta->len_expansion = *skip;
      }

   return list;
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
static LinkedList *bufr_repl_descriptors
   ( ListNode *first, int nbdesc, int count, int flags, BUFR_Tables *tbls, int *errflg, BUFR_DecodeInfo *s4 )
   {
   int  i, j;
   LinkedList   *lst, *lst1;
   BufrDescriptor *cb, *desc;
   ListNode *node, *prev;
   int       n;
   int       extraflags=0;

/*
 * add this flag for processing of Data Present Bitmap of Table C Operator 2 22 000
 */
   if (nbdesc == 1)
      {
      int  f2, x2, y2;

      cb = (BufrDescriptor *)first->data;
      f2 = DESC_TO_F( cb->descriptor );
      x2 = DESC_TO_X( cb->descriptor );
      y2 = DESC_TO_Y( cb->descriptor );
      if ((f2 == 0)&&(x2 == 33))
         extraflags = FLAG_CLASS33;
      }

   lst = lst_newlist();
   prev = NULL;
   for (j = 0 ; j < count ; j++ )
      {
      node = first;
      for (i = 0 ; i < nbdesc ; i++ )
         {
         if (node == NULL)
            {
            bufr_abort( _("Error in bufr_repl_descriptors(): node is null\n") );
            break;
            }
         cb = (BufrDescriptor *)node->data;
         if (cb->encoding.nbits == -1)
            {
            cb->encoding.nbits = 0;
            if (bufr_is_table_b( cb->descriptor ))
               {
               char errmsg[128];
               EntryTableB *tb = cb->etb ? cb->etb : bufr_fetch_tableB( tbls, cb->descriptor );
               if (tb)
                  cb->encoding = tb->encoding;
               if (cb->etb == NULL) cb->etb = tb;
               }
            }
         if (bufr_meta_enabled)
            if (cb->meta == NULL) cb->meta = bufr_create_rtmd( 1 );
         desc = bufr_dupl_descriptor ( cb );
         if ( desc == NULL )
            {
            char errmsg[128];

            sprintf( errmsg, _("ERROR, can't replicate codes: %d %d while duplicating: %d\n"), 
                  nbdesc, count, cb->descriptor );
            if (errflg) *errflg = 1;
            bufr_abort( errmsg );
            }
/*
 * removed the skipped flag if it was previously expanded with 0 replication

*/
         desc->flags &= ~FLAG_SKIPPED;
/*
 * add extra flags to each expanded item

*/
         desc->flags |= extraflags;
/*
 * establish replication nesting position
*/
         desc->repl_rank = j+1;
         if (desc->meta)
            {
            for (n = 0; n < desc->meta->nb_nesting ; n++ )
               {
               if ( desc->meta->nesting[n] == 0)
                  {
                  desc->meta->nesting[n] = j+1;
                  break;
                  }
               }
            } 
         lst_addlast( lst, lst_newnode( desc ) );
         prev = node;
         node = lst_nextnode( node );
         }
/* 
 * Here, we are looking for some badly formed BUFR message that is short in size 
 * but has a very high delayed replication count of a long Table D sequence
 * we will try to halt some nasty replication before it is even started 
 */
/* check first occurence length, then extrapolate */
      if (( j == 0 )&& s4)
         {
         BUFR_Sequence *bsq;
         int len;

         bsq = bufr_create_sequence( lst );
         len = bufr_estimate_seq_length( bsq, tbls );
         len = len * count / 8;
         bsq->list = NULL;
         bufr_free_sequence( bsq );
         if (len > (s4->max_len*3))
            {
            char   errmsg[256];

            bufr_free_descriptorList( lst );
            sprintf( errmsg, _("Error: BUFR Message is too short: %d > maxlen=%d\n"), len, s4->max_len );
            bufr_print_debug( errmsg );
            return NULL;
            }
         }
      }
/*
 * skipping of inner zero delay replication flag is disabled after first level
*/
   if (flags & OP_ZDRC_IGNORE)
      flags = flags & ~OP_ZDRC_IGNORE;

   lst1 = bufr_expand_list( lst, flags, tbls, errflg, s4 );
   if (lst1 == NULL)
      bufr_free_descriptorList( lst );
   return lst1;
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
static void bufr_assign_descriptors( ListNode *node, int nbdesc, int flags, BUFR_Tables *tbls )
   {
   int       i;
   BufrDescriptor *cb;
   int       skipped;

   i = 0;
   while ( node )
      {
      skipped = 0;
      cb = (BufrDescriptor *)node->data;
      if (flags & OP_EXPAND_DELAY_REPL)
         {
         if ((flags & OP_ZDRC_SKIP)||(flags & OP_ZDRC_IGNORE))
            {
            cb->flags |= FLAG_SKIPPED;
            cb->flags |= FLAG_IGNORED;
            skipped = 1;
            }
         }

      if (!skipped && (cb->encoding.nbits == -1))
         {
         cb->encoding.nbits = 0;
         if (bufr_is_table_b( cb->descriptor ))
            {
            EntryTableB *tb = cb->etb ? cb->etb : bufr_fetch_tableB( tbls, cb->descriptor );
            if (tb)
               cb->encoding = tb->encoding;
            if (cb->etb == NULL) cb->etb = tb;
            }
         }
      node = lst_nextnode( node );
      i++;
      if (i >= nbdesc) break;
      }
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
static void bufr_walk_sequence
   ( BUFR_Sequence *bsq, void (*proc)(BufrDescriptor *, void * ), 
     void *client_data  )
   {
   ListNode *node = lst_firstnode( bsq->list );

   while ( node )
      {
      proc( (BufrDescriptor *)node->data, client_data );
      node = lst_nextnode( node );
      }
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
int bufr_check_sequence 
   ( BUFR_Sequence *bsq, ListNode *node, int *flags, BUFR_Tables *tbls, int depth )
   {
   BufrDescriptor *cb;
   LinkedList  *stack, *codes;
   int count;

   int  repl_active;
   int  f, x, y;
   int  next_class_31=0;
   int  next_31021=0;
   int  next_local_desc=0;
   int  skip1;
   int  *repl;
   char  errmsg[256];
 
/*
 * make sure replication F==1 descriptors count are properly set
 * i.e. inner repl desc count is contained within outer repl.
 */
   codes = bsq->list;
   stack = lst_newlist();
   repl_active = 0;
   skip1 = 0;
   if (node == NULL)
      node = lst_firstnode( codes );
   while ( node )
      {
      cb = (BufrDescriptor *)node->data;
      f = DESC_TO_F( cb->descriptor );
      x = DESC_TO_X( cb->descriptor );
      y = DESC_TO_Y( cb->descriptor );

/*
 * make sure that Table D code are defined
 */
      if (f == 3)
         {
         EntryTableD   *td;

         td = bufr_fetch_tableD( tbls, cb->descriptor );
         if (td == NULL) 
            {
            free_rep_cnt_stack( stack );
            return -1;
            }
         }        

      if (next_class_31)
         {
         if ( (f != 0)||(x != 31)||
              ((y != 0)&&(y != 1)&&(y != 2)&&(y != 11)&&(y != 12)) )
            {
            next_class_31 = cb->descriptor;
            node = NULL;
            break;
            }
         else
            {
            next_class_31 = 0;
            skip1 = 1;
            }
         }
      else if (next_31021)
         {
         if (cb->descriptor == 31021)
            {
            next_31021 = 0;
            }
         else
            {
            next_31021 = cb->descriptor;
            node = NULL;
            break;
            }
         }
      else if (next_local_desc)
         {
         if (bufr_is_local_descriptor( cb->descriptor ))
            {
            next_local_desc = 0;
            }
         else
            {
            sprintf( errmsg, _("Warning: %d is followed by %d (not a local descriptor)\n"), 
                  next_local_desc, cb->descriptor );
            bufr_print_debug( errmsg );
            next_local_desc = 0;
            }
         }

      if (f == 2)
         {
         if ((x == 4)&&(y != 0)) 
            {
            next_31021 = cb->descriptor; /* make sure 204YYY is followed by 31021 */
            }
         if (x == 6) 
            {
            next_local_desc = cb->descriptor; /* make sure 206YYY is followed by a local descriptor */
            }
         }
      else if (f == 1) 
         {
         if (y == 0)
            {
            next_class_31 = 1; /* make sure delayed replication is followed by class 31 element */
            *flags |= HAS_DELAYED_REPLICATION;
            }

         repl = (int *)malloc( sizeof(int) );
         *repl = x;
         lst_addfirst( stack, lst_newnode((void *)repl) );
         skip1 = 1;
         }

/*
 * determine nesting level of code in replication

*/
      count = lst_count( stack ) + depth;
      if (bufr_meta_enabled)
         {
         if ((count > 0)&&(cb->meta == NULL))
            cb->meta = bufr_create_rtmd( count );
         }

      repl_active = decrease_repeat_counters( cb->descriptor, stack, &skip1 );
      if (repl_active < 0)
         {
         node = NULL;
         break;
         }

      node = lst_nextnode( node );
      }

   free_rep_cnt_stack( stack );
   stack = NULL;
   if (next_31021 != 0)
      {
      sprintf( errmsg, _("Error: expecting Class 31021 after 204YYY but found: %d\n"), next_31021 );
      bufr_print_debug( errmsg );
      return -1;
      }
   if (next_class_31 != 0)
      {
      sprintf( errmsg, _("Error: expecting Class 31 after delayed replication but found: %d\n"), 
            next_class_31 );
      bufr_print_debug( errmsg );
      return -1;
      }
   if (repl_active > 1)
      {
      sprintf( errmsg, _("Error: bad replication code count in dataset definition\n") );
      bufr_print_debug( errmsg );
      return -1;
      }
   if (next_local_desc > 0)
      {
      sprintf( errmsg, _("Warning: expecting local descriptor after 206YYY but found: %d\n"), 
            next_local_desc );
      bufr_print_debug( errmsg );
      /* 
       * will still be able to function without it 
       */
      }

   return 1;
   }

/**
 * @english
 *   deallocate element stored in the replication counter stack
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void free_rep_cnt_stack( LinkedList *stack )
   {
   ListNode *node;

   node = lst_rmfirst( stack );
   while ( node )
      {
      free( node->data );
      lst_delnode( node );
      node = lst_rmfirst( stack );
      }
   lst_dellist( stack );
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
static int decrease_repeat_counters( int descriptor, LinkedList *stack, int *skip1 )
   {
   ListNode *node;
   ListNode *first;
   int      *repl=NULL;
   int       rtrn=0;
   int       isdebug=bufr_debugmode;
   char      errmsg[128];

#if DEBUG
   if (isdebug) 
      {
      sprintf( errmsg,_("### Checking #Code Replication >> %.6d : {"), descriptor );
      bufr_print_debug( errmsg );
      }

   if (lst_count( stack) <= 0) 
      {
      if (isdebug) 
         bufr_print_debug( " 0 }\n" );
      return 0;
      }

   if (isdebug) 
      {
      first = lst_firstnode( stack );
      node = lst_lastnode( stack );
      while (node)
         {
         repl = (int *)node->data;
         if (*skip1 && (first == node))
            {
            bufr_print_debug( " * " );
            }
         else if (*repl > 0)
            {
            sprintf( errmsg," %d ", *repl );
            bufr_print_debug( errmsg );
            }
         node = lst_prevnode(node);
         }
      bufr_print_debug( "}\n" );
      }
#endif
/*
 * decrease by 1 every replication (embedded) counters
 */
   node = lst_firstnode( stack );
   if (node == NULL) return 0;
   while (node)
      {
      repl = (int *)node->data;
      if (*skip1)
         {
         *skip1 = 0;
         }
      else
         *repl -= 1;
      if (*repl < 0) rtrn = -1;
      node = lst_nextnode(node);
      }

   rtrn = (repl) ? *repl : 0;
/*
 * remove all last counters if zero

*/
   node = lst_firstnode( stack );
   repl = (int *)node->data;
   while (node && repl && (*repl == 0))
      {
      node = lst_rmfirst( stack );
      free( node->data );
      lst_delnode( node );
      node = lst_firstnode( stack );
      if (node)
         repl = (int *)node->data;
      else
         repl = NULL;
      }

   return rtrn;
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
BufrDescriptorArray bufr_sequence_to_array( BUFR_Sequence *codes, int dovalues )
   {
   BufrDescriptorArray arr;
   int   count;

   count = lst_count( codes->list );
   arr = (BufrDescriptorArray)arr_create( count, 
         sizeof(BufrDescriptor *), 100 );
   if (dovalues)
      bufr_walk_sequence( codes, cumulate_code2CodeArrayValues, (void *)arr );
   else
      bufr_walk_sequence( codes, cumulate_code2CodeArray, (void *)arr );
   return (BufrDescriptorArray)arr;
   }

static void cumulate_code2CodeArrayValues( BufrDescriptor *cb, void *client_data )
   {
   char *arr = (char *)client_data;

   if (cb)
      {
      if (cb->value == NULL)
         cb->value = bufr_mkval_for_descriptor( cb );
      }
   arr_add( arr, (char *)&cb );
   }

static void cumulate_code2CodeArray( BufrDescriptor *cb, void *client_data )
   {
   char *arr;

   arr = (char *)client_data;
   if (cb)
      {
      arr_add( arr, (char *)&cb );
      }
   }

/**
 * @english
 *    bsq2 = bufr_copy_sequence( bsq )
 *    (BUFR_Sequence *)
 * This creates a duplicate of the sequence (see
 * bufr_create_sequence below). This would be created in turn for each data
 * subset.
 * @warning An example will be added or referred to in a later version.
 * @return BUFR_Sequence
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup template internal
 */
BUFR_Sequence *bufr_copy_sequence( BUFR_Sequence *bsq )
   {
   BUFR_Sequence *list;
   BufrDescriptor *cb, *cb1;
   ListNode *node;

   list = bufr_create_sequence( NULL );

   node = lst_firstnode( bsq->list );
   while ( node )
      {
      cb = (BufrDescriptor *)node->data;

      if (cb) 
         cb1 = bufr_dupl_descriptor( cb );
      else
         cb1 = NULL;
      bufr_add_descriptor_to_sequence( list, cb1 );
      node = lst_nextnode( node );
      }
   return list;
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
static void bufr_free_descriptorNode(ListNode *node)
   {
   BufrDescriptor    *cb;

   cb = (BufrDescriptor *)node->data;
   node->data = NULL;
   lst_delnode( node );
   bufr_free_descriptor( cb );
   }

/**
 * @english
 *    bufr_apply_Tables( NULL, bsq,  dts->tmplte, NULL, &errcode )
 *    (BufrDDOp *ddo, BUFR_Sequence *bsq, BUFR_Template *tmplt, ListNode *)
 * This is a call to apply Table C operators on each code value that is in
 * the sequence. It may be used sequentially for processing the whole sequence
 * by a single call. The current “ListNode” structure parameters
 * should be known at the time of the call. When the first parameter
 * (BufrDDOp) is NULL, an internal structure (that will allow processing of
 * elements one at a time) will be created and returned which should be
 * freed after this call and its subsequent use. If the users do not want
 * to create their own structures, then NULL can be coded in its place. 
 * @return BufrDDOp
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @bug shouldn't it be bufr_apply_tables()?
 * @ingroup template internal
 */

BufrDDOp  *bufr_apply_Tables
   ( BufrDDOp *ddoi, BUFR_Sequence *bsq, BUFR_Template *tmplt, ListNode *current, int *errcode )
   {
   ListNode         *node;
   BufrDDOp         *ddo;

   *errcode = 0;
   if (bsq == NULL) return NULL;

   if (ddoi ==NULL)
      ddo = bufr_create_BufrDDOp( BUFR_STRICT ); 
   else
      ddo = ddoi;

   node = ddo->current;
   if (node == NULL)
      node = lst_firstnode( bsq->list );

   while ( node )
      {
      bufr_apply_tables2node( ddo, bsq, tmplt, node, errcode );
      if (node == current) break;
      node = lst_nextnode( node );
      }

   return ddo;
   }


/**
 * @english
 *    bufr_apply_tables2node( ddo, bsq,  dts->tmplte, NULL, &errcode, debug )
 *    (BufrDDOp *ddo, BUFR_Sequence *bsq, BUFR_Template *tmplt, ListNode *)
 * This is a call to apply Table C operators on a code value 
 * @return  int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup template internal
 */
int bufr_apply_tables2node
   ( BufrDDOp *ddo, BUFR_Sequence *bsq, BUFR_Template *tmplt, ListNode *node, int *errcode )
   {
   BufrDescriptor   *cb;
   int               f, x, y;
   int               res;
   EntryTableB      *otb;
   char              errmsg[256];
   int               debug=bufr_debugmode;

   cb = (BufrDescriptor *)node->data;
   f = DESC_TO_F( cb->descriptor );
   x = DESC_TO_X( cb->descriptor );
   y = DESC_TO_Y( cb->descriptor );
   if (x == 31) cb->flags |= FLAG_CLASS31;

   if (ddo->dpbm && bufr_is_marker_dpbm(cb->descriptor))
      {
      ListNode  *node1;
      BufrDescriptor  *cbm;
      int        ndx;
      int idp = cb->repl_rank;
      if (idp <= 0)
         {
         if (debug)
            {
            sprintf( errmsg, _("Error obtaining Repl Rank of %d = %d\n"),
               cb->descriptor, idp );
            bufr_print_debug( errmsg );
            }
         }
      else
         {
         ndx = ddo->dpbm->dp[idp-1];
         if (ndx >= 0)
            {
            ndx = ddo->dpbm->index[ndx];
            node1 = bufr_getnode_sequence( bsq, ndx );
            cbm = (BufrDescriptor *)node1->data;
            if (cb->value)
               bufr_free_value( cb->value );
            cb->value = bufr_mkval_for_descriptor( cbm );
            cb->encoding = cbm->encoding;
            cb->s_descriptor = cbm->descriptor;
            if (debug)
               {
               sprintf( errmsg, _("NDX=%d DESC=%d\n"), ndx, cbm->descriptor );
               bufr_print_debug( errmsg );
               }
            }
         }
      return 0;
      }
   else if (cb->flags & FLAG_CLASS33)
      {
      ListNode  *node1;
      BufrDescriptor  *cbm;
      int        ndx;
      int        idx;
      int idp = cb->repl_rank;

      if (ddo->dpbm == NULL)
         ddo->dpbm = bufr_index_dpbm( ddo, bsq );
      idx = ddo->dpbm->dp[idp-1];
      if (idx >= 0)
         {
         ndx = ddo->dpbm->index[ddo->dpbm->dp[idp-1]];
         node1 = bufr_getnode_sequence( bsq, ndx );
         cbm = (BufrDescriptor *)node1->data;
         cb->s_descriptor = cbm->descriptor;
         }
      }
   else if (cb->descriptor == 236000)
      {
      if (ddo->dpbm == NULL)
         ddo->dpbm = bufr_index_dpbm( ddo, bsq );
      }
   else if ((ddo->flags & DDO_BIT_MAP_FOLLOW) && (cb->descriptor == 31031))
      {
      if (ddo->remain_dpi > 0)
         {
         ddo->remain_dpi -= 1;
         }
      if (ddo->remain_dpi == 0)
         {
         if (ddo->dpbm == NULL)
            ddo->dpbm = bufr_index_dpbm( ddo, bsq );
         bufr_init_dpbm( ddo->dpbm, ddo->start_dpi );
         ddo->remain_dpi = -1;
         }
      }
   else if ((ddo->flags & DDO_BIT_MAP_FOLLOW) && (cb->descriptor != 31031))
      {
      if (ddo->dpbm == NULL)
         ddo->dpbm = bufr_index_dpbm( ddo, bsq );
      if ((ddo->remain_dpi > 0)&&(ddo->remain_dpi < ddo->dpbm->nb_codes))
         {
         sprintf( errmsg, _("Warning: bitmap size %d != %d data present descriptors\n"),
            ddo->dpbm->nb_codes - ddo->remain_dpi, ddo->dpbm->nb_codes );
         bufr_print_debug( errmsg );
         bufr_init_dpbm( ddo->dpbm, ddo->start_dpi );
         ddo->remain_dpi = -1;
         ddo->flags &= ~DDO_BIT_MAP_FOLLOW;
         }
      }

/*
 * overriding table B entry

*/
   otb = NULL;
   if (f == 0)
      {
      if ( ddo->override_tableb )
         otb = bufr_tableb_fetch_entry( ddo->override_tableb, cb->descriptor );
      if (otb == NULL)
         otb = cb->etb ? cb->etb : bufr_fetch_tableB( tmplt->tables, cb->descriptor );
      }

   bufr_reassign_table2code( cb, f, otb );
/*
 * applying time or location descriptor followed by replication or separate by table C descriptor
 */
/* a 205YYY would turn a TABLE C into CCITT_IA5 */
   if (f == 2)
      {
      int version = tmplt->edition;
      if (BUFR_STRICT==ddo->enforce)
         {
         switch( version )
            {
            case 5 :
               res = bufr_resolve_tableC_v5( cb, ddo, x, y, version, node );
               break;
            case 4 :
               res = bufr_resolve_tableC_v4( cb, ddo, x, y, version, node );
               break;
            case 3 :
               res = bufr_resolve_tableC_v3( cb, ddo, x, y, version, node );
               break;
            default :
               res = bufr_resolve_tableC_v2( cb, ddo, x, y, version, node );
               break;
            }
         }
      else
         {
         /* some use edition 4 operators but encode as edition 3 */
         /* when BUFR rules are laxed, calling version 5 C operators handler for any version will open the gate for all */
         res = bufr_resolve_tableC_v5( cb, ddo, x, y, version, node );
         }
      if (res < 0) 
         *errcode = res;
      }
   else if ((ddo->flags & DDO_DEFINE_EVENT)==0)
      {
      if (bufr_init_location( ddo, cb )==0)
         {
         if (ddo->flags & DDO_HAS_LOCATION)
            {
            if (f == 1)
               {
               bufr_assoc_location( cb, ddo );
               bufr_clear_location( ddo );
               }
            ddo->flags &= ~DDO_HAS_LOCATION;
            }
         }
      }

   if (cb->meta)
      {
      if (cb->meta->tlc) free( cb->meta->tlc );
      cb->meta->tlc = bufr_current_location( ddo, cb->meta, &(cb->meta->nb_tlc) );
      }

   switch ( cb->encoding.type )
      {
      case TYPE_CCITT_IA5 :
      case TYPE_NUMERIC :
      case TYPE_CODETABLE :
      case TYPE_FLAGTABLE :
         if (cb->flags & FLAG_CLASS31) /* data description operators don't apply to class 31 */
            {
            if ((ddo->add_af_nbits > 0) && (cb->descriptor == 31021))
               bufr_change_af_sig( ddo, node->data );
            break;
            }

         if (ddo->add_af_nbits > 0) /* assign YYY bits of Associated Fied to each data element */
            {
            cb->encoding.af_nbits = ddo->add_af_nbits;
            bufr_set_descriptor_afd( cb, ddo->af_list );
            }
         break;
      default :
         break;
      }

   switch ( cb->encoding.type )
      {
      case TYPE_CCITT_IA5 :
         if (ddo->redefine_ccitt_ia5 > 0)
            cb->encoding.nbits = ddo->redefine_ccitt_ia5 * 8;
         break;
      case TYPE_NUMERIC :
         /* 
          * Data Descriptor Operator shall not apply to TableB class 31 
          */
         if (cb->flags & FLAG_CLASS31)  
            break;

         if (ddo->use_ieee_fp)
            {
            cb->encoding.type = TYPE_IEEE_FP;
            cb->encoding.nbits = ddo->use_ieee_fp;
            tmplt->flags |= HAS_IEEE_FP ;
            break;
            }
        
         if (ddo->change_ref_val_op > 0)
            {
            bufr_apply_op_crefval( ddo, cb, tmplt );
            }
         else
            {
            if (ddo->local_nbits_follows > 0)
               {
               if (bufr_is_local_descriptor( cb->descriptor ))
                  {
                  if (cb->encoding.nbits > 0)
                     {
                     if (cb->encoding.nbits != ddo->local_nbits_follows)
                        {
#if 0
                        BufrDescriptor *cb1;
                        ListNode *prev;
                        int       new206;

                        prev = lst_prevnode( node );
                        cb1 = (BufrDescriptor *)prev->data;
                        new206 = 206000 + ddo->local_nbits_follows;
                        if (debug)
                           {
                           sprintf( errmsg, _n("Warning: local descriptor %.6d (%d bit) doesn't match %d, should have been %d\n", 
                                               "Warning: local descriptor %.6d (%d bits) doesn't match %d, should have been %d\n", 
                                               cb->encoding.nbits), 
                                    cb->descriptor, cb->encoding.nbits, cb1->descriptor, new206 );
                           bufr_print_debug( errmsg );
                           }
#endif
                        cb->encoding.nbits = ddo->local_nbits_follows;
                        }
                     }
                  else
                     {
                     if (debug)
                        {
                        sprintf( errmsg, _n("### Setting local descriptor %.6d to %d bit\n", 
                                            "### Setting local descriptor %.6d to %d bits\n", 
                                            ddo->local_nbits_follows), 
                                 cb->descriptor, ddo->local_nbits_follows );
                        bufr_print_debug( errmsg );
                        }
                     cb->encoding.nbits = ddo->local_nbits_follows;
                     }
                  }
               ddo->local_nbits_follows = 0;
               }
            else if (ddo->add_nbits != 0)
               {
               cb->encoding.nbits += ddo->add_nbits;
               if (debug)
                  {
                  sprintf( errmsg, _("### 201 %d datawidth=%d\n"), 
                           cb->descriptor, cb->encoding.nbits );
                  bufr_print_debug( errmsg );
                  }
               }

            if (ddo->multiply_scale != 0)
               {
               cb->encoding.scale += ddo->multiply_scale;
               if (debug)
                  {
                  sprintf( errmsg, _("### 202 %d scale=%d\n"), 
                           cb->descriptor, cb->encoding.scale );
                  bufr_print_debug( errmsg );
                  }
               }

            if (ddo->change_ref_value != 0)
               {
               cb->encoding.reference *= 10^ddo->change_ref_value;
               cb->encoding.ref_nbits = bufr_value_nbits( cb->encoding.reference );
               if (debug)
                  {
                  sprintf( errmsg, _("### 203 %d reference=%d\n"), 
                      cb->descriptor, cb->encoding.reference );
                  bufr_print_debug( errmsg );
                  }
               }
            }
         break;
      default :
         break;
      }

   return 0;
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
int bufr_init_location( BufrDDOp *ddo, BufrDescriptor *cb )
   {
   if (bufr_is_location( cb->descriptor ) && (cb->value != NULL) )
      {
      float value = bufr_value_get_float(cb->value);

      if (!bufr_is_missing_float( value ))
         {
         bufr_keep_location( ddo, cb->descriptor, value );
         ddo->flags |= DDO_HAS_LOCATION;
         return 1;
         }
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
int bufr_apply_op_crefval( BufrDDOp *ddo, BufrDescriptor *cb, BUFR_Template *tmplt )
   {
   char   errmsg[256];
   int    debug=bufr_debugmode;

   switch ( cb->encoding.type )
      {
      case TYPE_NUMERIC :
         if (ddo->change_ref_val_op > 0)
            {
            
            if (cb->value == NULL)
               {
               cb->encoding.type = TYPE_CHNG_REF_VAL_OP;
               cb->encoding.reference = 0;
               cb->encoding.scale = 0;
               cb->encoding.af_nbits = 0;
               cb->encoding.ref_nbits = 0;
               cb->encoding.nbits = ddo->change_ref_val_op;
               if (debug)
                  {
                  sprintf( errmsg, _n("### Changing REF: %d to %d bit:", 
                                   "### Changing REF: %d to %d bits:", 
                                   ddo->change_ref_val_op), 
                        cb->descriptor, ddo->change_ref_val_op ); 
                  bufr_print_debug( errmsg );
                  }

               return 0;
               }
            }
      break;
      case TYPE_CHNG_REF_VAL_OP :
         if (ddo->change_ref_val_op > 0)
            {
            EntryTableB *tb1, *tb2;
            float        value;

            value = bufr_value_get_float( cb->value );

            if ( !bufr_is_missing_float(value) )
               {
               if (debug)
                  {
                  sprintf( errmsg, "%f ", value );
                  bufr_print_debug( errmsg );
                  }
               if (bufr_is_table_b( cb->descriptor ))
                  tb2 = cb->etb ? cb->etb : bufr_fetch_tableB( tmplt->tables, cb->descriptor );
               else 
                  tb2 = NULL;
               if (tb2)
                  {
                  tb1 = bufr_new_EntryTableB();
                  bufr_copy_EntryTableB( tb1, tb2 );
                  tb1->encoding.reference = value; 
                  tb1->encoding.ref_nbits = bufr_value_nbits( tb1->encoding.reference );
                  /* 
                   * storage for temporary Table Entry allocation here 
                   */
                  arr_add( tmplt->ddo_tbe, (char *)&tb1 );
                  /* 
                   * adding new override to table b 
                   */
                  arr_add( ddo->override_tableb, (char *)&tb1 ); 
                  if (debug)
                     bufr_print_debug( _("descriptor reference overrided\n") );
                  }
               else
                  {
                  if (debug)
                     bufr_print_debug( _("descriptor not found\n") );
                  }
               }
            else
               {
               if (debug)
                  bufr_print_debug( _("No value\n") );
               }
            }
            return 1;
      break;
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
static void bufr_reassign_table2code( BufrDescriptor *bc, int f, EntryTableB *tb )
   {
   if (tb)
      {
      memcpy( &(bc->encoding), &(tb->encoding), sizeof(BufrValueEncoding) );
      }
   else
      {
      switch (f)
         {
         case 2 : 
            bc->encoding.type = TYPE_OPERATOR;
            break;
         case 3 :
            bc->encoding.type = TYPE_SEQUENCE;
            break;
         case 1 :
            bc->encoding.type = TYPE_REPLICATOR;
            break;
         default :
            if (bufr_is_local_descriptor( bc->descriptor ))
               bc->encoding.type = TYPE_NUMERIC;
            else
               bc->encoding.type = TYPE_UNDEFINED;
            break;
         }
      bc->encoding.scale = 0;
      bc->encoding.reference = 0;
      bc->encoding.nbits = 0;
      bc->encoding.ref_nbits = 0;
      }
   bc->encoding.af_nbits = 0;
   }

/**
 * @english
 * change current associated field meaning
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void bufr_change_af_sig( BufrDDOp *ddo , char *sig )
   {
   ListNode *node;
   BufrDescriptor *bc;

   bc = (BufrDescriptor *)sig;
   if (bc->descriptor != 31021) return;

   node = lst_lastnode( ddo->af_list );
   if (node)
      {
      AF_Definition *afd = (AF_Definition *)node->data;
      afd->sig = sig;
      }
   }

/**
 * @english
 * change current associated field meaning
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int bufr_solve_replication( int value, int y2, int descriptor )
   {
   int rep = 0;

   switch (y2)
      {
      case 0 :
      if (value != -1)
         {
         rep = value;
         if (rep != 0) rep = 1;
         }
         break;
      case 1 :
      case 2 :
         if (value != -1)
            rep = value;
         break;
      case 11 :
      case 12 :
         if (value != -1)
            rep = 1;
         break;
      default :
         {
         char errmsg[256];

         sprintf( errmsg, _("Error: invalid delayed replication count code : %.6d\n") , descriptor );
         bufr_print_debug( errmsg );
         return 0;
         }
      }

   return rep;
   }

/**
 * @english
 * makes an data only index to access code 
 * @param  ddo : pointer to data descriptors operators
 * @param  bsq : pointer to a BUFR_Sequence
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal

 */
BufrDPBM *bufr_index_dpbm ( BufrDDOp *ddo, BUFR_Sequence *bsq )
   {
   ListNode         *node;
   BufrDescriptor   *bcv;
   int               i, nb;
   char              buffer[256];
   BufrDPBM         *dpbm;

   nb = 0;
   node = lst_firstnode( bsq->list );
   while ( node )
      {
      bcv = (BufrDescriptor *)node->data;
#if TESTINDEX
      sprintf( buffer, "%.6d", bcv->descriptor );
      bufr_print_debug( buffer );
#endif
      node = lst_nextnode( node );

      if (bufr_is_start_dpbm( bcv->descriptor )) break;

      if ( bufr_is_dd_for_dpbm(bcv) )
         {
#if TESTINDEX
         bufr_print_debug( "\n" );
#endif
         continue;
         }
#if TESTINDEX
      sprintf( buffer, " #%d\n", nb+1 );
      bufr_print_debug( buffer );
#endif
      ++nb;
      }

   dpbm = bufr_create_BufrDPBM( nb );
/*
 * count DPI down to zero, then we will know when we can safely call bufr_init_dpbm()

*/

   if (bufr_debugmode)
      {
      sprintf( buffer, _("### Code Count: %d\n"), nb );
      bufr_print_debug( buffer );
      }

   nb = 0;
   node = lst_firstnode( bsq->list );
   i = 0;
   while ( node )
      {
      ++i;
      bcv = (BufrDescriptor *)node->data;

#if TESTINDEX
      sprintf( buffer, "%.6d", bcv->descriptor );
      bufr_print_debug( buffer );
#endif

      node = lst_nextnode( node );

      if (bufr_is_start_dpbm( bcv->descriptor )) 
         {
         if (ddo) ddo->start_dpi = node;
         break;
         }

      if ( bufr_is_dd_for_dpbm(bcv) )
         {
#if TESTINDEX
         bufr_print_debug( "\n" );
#endif
         continue;
         }

#if TESTINDEX
      sprintf( buffer, " #%d\n", nb+1 );
      bufr_print_debug( buffer );
#endif
      dpbm->index[nb++] = i;
      }

#if TESTINDEX
      sprintf( buffer, _("### Indexed DPBM Data Descriptor count: %d\n"), dpbm->nb_codes );
      bufr_print_debug( buffer );
#endif

   if (ddo != NULL) 
      {
      ddo->dpbm       = dpbm;
      ddo->remain_dpi = dpbm->nb_codes;
      }
   return dpbm;
   }

/**
 * @english
 * determine if a data descriptor should not a counted for indexing 
 * of Data Present Bitmap
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int bufr_is_dd_for_dpbm( BufrDescriptor *bcv )
   {
   int  f, x;

   if (bcv->flags & FLAG_SKIPPED) /* f=1 and f=3 are flagged */
      return 1;
/*
 * question is: What should be counted, what shoul not?
 * currently, everything are except expanded descriptors F=1 F=3
 * should F=2 be skipped too?

*/
   f = DESC_TO_F( bcv->descriptor );
   x = DESC_TO_X( bcv->descriptor );
   switch(f)
      {
      case 0 :
         return 0;
         break;
      case 2 :
         if (x == 5) return 0;  /* a string define */
      case 1 :
      case 3 :
      default :
         return 1;
         break;
      }
   return 0;
   }

/*
 * name: bufr_free_descriptorList
 *
 * author:  Vanh Souvanlasy
 *
 * function: free a list of descriptors
 *
 * parametres:   list
 *
 */
static void bufr_free_descriptorList(LinkedList *list)
   {
   ListNode *node;
   BufrDescriptor *code;

   node = lst_rmfirst( list );
   while ( node )
      {
      code = (BufrDescriptor *)node->data;
      bufr_free_descriptor ( code );
      node->data = NULL;
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
 */
int bufr_apply_dpbm ( BufrDPBM *dpbm, BUFR_Sequence *bsq, ListNode *svnode )
   {
   int  i, idp, ndx;
   BufrDescriptor *cb, *cbm;
   ListNode *node;

   i = 0;
   idp = 0;
   ndx = dpbm->index[dpbm->dp[idp]];
   node = lst_firstnode( bsq->list );
   while ( node && (idp < dpbm->nb_dp) && svnode )
      {
      ++i;
      if (i != ndx)
         {
         node = lst_nextnode( node );
         continue;
         }
      cbm = (BufrDescriptor *)node->data;
      cb = (BufrDescriptor *)svnode->data;
      cb->encoding = cbm->encoding;
      cb->s_descriptor = cbm->descriptor;
      ++idp;
      ndx = dpbm->index[dpbm->dp[idp]];
      svnode = lst_nextnode( svnode );
      node = lst_nextnode( node );
      }
   return 0;
   }

/**
 * @english
 *    bufr_reindex_sequence(bsq)
 *    (BUFR_Sequence *)
 * Regenerates an index that refers to each node in the list after
 * insertion, changes, and removal of items that have occurred since the
 * original call of bufr_create_sequence.
 * @return Void
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup template internal
 */
void bufr_reindex_sequence ( BUFR_Sequence *cl )
   {
   ListNode *node;

   if (cl->index == NULL)
      {
      cl->index = (ListNodeArray)arr_create ( lst_count(cl->list)+100, 
            sizeof(ListNode *), 100 );
      }
   else
      {
      arr_del ( cl->index, arr_count( cl->index ) );
      }

   node = lst_firstnode( cl->list );
   while ( node )
      {
      arr_add( cl->index, (char *) &node );
      node = node->next; /* lst_nextnode( node ); */
      }
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
ListNode *bufr_getnode_sequence ( BUFR_Sequence *cl, int pos )
   {
   if (cl->index == NULL)
      {
      ListNode **pnode;

      bufr_reindex_sequence( cl );
      pnode = (ListNode **)arr_get( cl->index, pos-1 );
      if (pnode) return *pnode;
      return NULL;
      }

   if (cl->index)
      {
      ListNode **pnode;
      int        count;

      count = lst_count( cl->list );
      if ( arr_count(cl->index) != count )
         {
         bufr_reindex_sequence( cl );
         }
      pnode = (ListNode **)arr_get( cl->index, pos-1 );
      if (pnode) return *pnode;
      return NULL;
      }
   else
      return lst_nodepos( cl->list , pos );
   }

/**
 * @english
 * do a coarse estimate of current length of sequence in bits
 * @endenglish
 * @francais
 * estimation grossiere de la longueur de la sequence en bits
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup debug
 */
int bufr_estimate_seq_length( BUFR_Sequence *seq, BUFR_Tables *tbls )
   {
   ListNode *node;
   int       nbits;
   BufrDescriptor *cb;
   int       errflg;
   BUFR_Sequence  *bsq;
   int       last_desc=0, last_nbits=0;
   int       f, x, y;
   int       rep_desc=0, rep_cnt=0;
   char      errmsg[1024];

   nbits = 0;
   node = lst_firstnode( seq->list );
   while ( node )
      {
      cb = (BufrDescriptor *)node->data;
      f = DESC_TO_F( cb->descriptor );
      x = DESC_TO_X( cb->descriptor );
      y = DESC_TO_Y( cb->descriptor );
#if DEBUG
      sprintf ( errmsg, "Desc=%d   flag=%d rep_desc=%d rep_cnt=%d\n", 
                  cb->descriptor , cb->flags, rep_desc, rep_cnt );
      bufr_print_debug( errmsg );
#endif
      if (cb->encoding.af_nbits > 0)
         {
         nbits += cb->encoding.af_nbits;
	 sprintf( errmsg, "descriptor= %d  af  nbits=%d\n", cb->descriptor, cb->encoding.af_nbits );
         bufr_print_debug( errmsg );
	 }

      if (cb->encoding.nbits > 0 )
         {
         nbits += cb->encoding.nbits;
	 sprintf( errmsg, "descriptor= %d   nbits=%d\n", cb->descriptor, cb->encoding.nbits );
         bufr_print_debug( errmsg );
         }
      else
         {
         if ((cb->flags & FLAG_SKIPPED)
               ||(cb->flags & FLAG_EXPANDED)
               ||(cb->flags & FLAG_IGNORED))
            {
#if DEBUG
            sprintf ( errmsg, "Skipping desc=%d\n", cb->descriptor );
            bufr_print_debug( errmsg );
#endif
            }
         else if (last_desc == cb->descriptor )
            {
            nbits += last_nbits;
	    sprintf( errmsg, "descriptor= %d   nbits=%d\n", last_desc, last_nbits );
            bufr_print_debug( errmsg );
            }
         else
            {
            if ((f == 3) && ((rep_desc==0)||((rep_desc > 0)&&(rep_cnt > 0))))
               {
#if DEBUG
               sprintf ( errmsg, "Processing desc=%d\n", cb->descriptor );
               bufr_print_debug( errmsg );
#endif
               bsq = bufr_expand_descriptor( cb->descriptor, OP_RM_XPNDBL_DESC, tbls, &errflg );
               if (bsq)
                  {
                  last_desc = cb->descriptor;
                  last_nbits = bufr_estimate_seq_length( bsq, tbls );
                  bufr_free_sequence( bsq );
#if DEBUG
                  sprintf ( errmsg, "Processed desc=%d   flag=%d bits=%d\n", cb->descriptor , cb->flags, last_nbits );
                  bufr_print_debug( errmsg );
#endif
                  nbits += last_nbits;
	          sprintf( errmsg, "descriptor= %d   nbits=%d\n", last_desc, last_nbits );
                  bufr_print_debug( errmsg );
                  }
               }
            }
         }

      if (rep_desc > 0) rep_desc -= 1;
      if (f == 1)
         {
         rep_desc = x;
         if (y == 0) rep_desc += 1;
         }
      else if ((f == 0)&&(x == 31))
         {
         rep_cnt = bufr_value_get_int32( cb->value );
         }
      node = node->next; /* lst_nextnode( node ); */
      }

   return nbits;
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
static int bufr_simple_check_seq
   ( BUFR_Sequence *bsq, ListNode *node, int depth )
   {
   BufrDescriptor *cb;
   LinkedList  *stack, *codes;
   int count;

   int  repl_active;
   int  f, x, y;
   int  next_class_31=0;
   int  next_31021=0;
   int  next_local_desc=0;
   int  skip1;
   int  *repl;
   char  errmsg[256];
 
/*
 * make sure replication F==1 descriptors count are properly set
 * i.e. inner repl desc count is contained within outer repl.
 */
   codes = bsq->list;
   stack = lst_newlist();
   repl_active = 0;
   skip1 = 0;
   if (node == NULL)
      node = lst_firstnode( codes );
   while ( node )
      {
      cb = (BufrDescriptor *)node->data;
      f = DESC_TO_F( cb->descriptor );
      x = DESC_TO_X( cb->descriptor );
      y = DESC_TO_Y( cb->descriptor );

      if (next_class_31)
         {
         if ( (f != 0)||(x != 31)||
              ((y != 0)&&(y != 1)&&(y != 2)&&(y != 11)&&(y != 12)) )
            {
            next_class_31 = cb->descriptor;
            node = NULL;
            break;
            }
         else
            {
            next_class_31 = 0;
            skip1 = 1;
            }
         }
      else if (next_31021)
         {
         if (cb->descriptor == 31021)
            {
            next_31021 = 0;
            }
         else
            {
            next_31021 = cb->descriptor;
            node = NULL;
            break;
            }
         }
      else if (next_local_desc)
         {
         if (bufr_is_local_descriptor( cb->descriptor ))
            {
            next_local_desc = 0;
            }
         else
            {
            next_local_desc = 0;
            }
         }

      if (f == 2)
         {
         if ((x == 4)&&(y != 0)) 
            {
            next_31021 = cb->descriptor; /* make sure 204YYY is followed by 31021 */
            }
         if (x == 6) 
            {
            next_local_desc = cb->descriptor; /* make sure 206YYY is followed by a local descriptor */
            }
         }
      else if (f == 1) 
         {
         if (y == 0)
            {
            next_class_31 = 1; /* make sure delayed replication is followed by class 31 element */
            }

         repl = (int *)malloc( sizeof(int) );
         *repl = x;
         lst_addfirst( stack, lst_newnode((void *)repl) );
         skip1 = 1;
         }
/*
 * determine nesting level of code in replication
 */
      if (bufr_meta_enabled)
         {
         count = lst_count( stack ) + depth;
         if ((count > 0)&&(cb->meta == NULL))
            cb->meta = bufr_create_rtmd( count );
         }

      repl_active = decrease_repeat_counters( cb->descriptor, stack, &skip1 );
      if (repl_active < 0)
         {
         node = NULL;
         break;
         }

      node = lst_nextnode( node );
      }

   free_rep_cnt_stack( stack );

   return 1;
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
static void bufr_transfer_rtmd
   ( LinkedList *codes, BufrRTMD *rtmd )
   {
   ListNode *node;
   BufrDescriptor *cb;

   node = lst_firstnode( codes );
   while ( node )
      {
      cb = (BufrDescriptor *)node->data;
      cb->meta = bufr_duplicate_rtmd( rtmd );
      node = lst_nextnode( node );
      }
   }
