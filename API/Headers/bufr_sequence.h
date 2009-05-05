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
 
 *  file      :  BUFR_SEQUENCE.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR BUFR DESCRIPTOR LIST
 *
 *
 */


#ifndef _bufr_sequence_h
#define _bufr_sequence_h

#include  <stdio.h>
#include  "bufr_linklist.h"
#include  "bufr_array.h"
#include  "bufr_desc.h"
#include  "bufr_template.h"
#include  "bufr_ddo.h"

/**
 * @english
 * Flags used by functions for expanding descriptors or Sequences:
 *
 *    OP_EXPAND_DELAY_REPL : Specify if unexpanded delayed replication should expand during this call
 *                           If not specified, they will remain unexpanded.
 *                           This is useful when expanding a BUFR_Sequence for use as a template, 
 *                           or for encoding, where replication counts have not yet been properly defined.
 *  
 *    OP_ZDRC_SKIP         : Specify if Delayed Replication with Count equal Zero should be mark
 *                           as skipped and not processed during this expansion call. 
 *                           This is usefull when encoding a message and progressively setting 
 *                           delayed replication count.
 *                         
 *    OP_ZDRC_IGNORE       : Specify if Delayed Replication with Count equal Zero should be mark
 *                           as skipped and not processed during this expansion call
 *                           for the first level only. Embedded replications will not be marked as skipped.
 *
 *    OP_RM_XPNDBL_DESC    : Specify if all expansion descriptors should be removed from the resulting 
 *                           BUFR_Sequence. If not specified, they will stay in the sequence and flagged
 *                           with FLAG_SKIPPED to notify that it has been processed.
 * @warning 
 *    OP_RM_XPNDBL_DESC should not be used while encoding a message with delayed replication 
 *    because this would remove the ability to set replication count and expand a BUFR_Sequence.
 *
 * @endenglish
 * @francais
 * @a faire: traduction en francais
 * @endfrancais
 * @see bufr_expand_descriptor(), bufr_expand_sequence(), bufr_expand_node_descriptor()
 */

#define  OP_EXPAND_DELAY_REPL   0x1
#define  OP_RM_XPNDBL_DESC      0x2
#define  OP_ZDRC_SKIP           0x4
#define  OP_ZDRC_IGNORE         0x8

typedef ArrayPtr     ListNodeArray;

typedef struct desc_list
   {
   LinkedList      *list;   /* list of BufrDescriptor */
   ListNodeArray    index;  /* index of ListNode */
   } BUFR_Sequence;


extern BUFR_Sequence      *bufr_create_sequence            ( LinkedList * );
extern void                bufr_free_sequence              ( BUFR_Sequence * );

extern void                bufr_reindex_sequence           ( BUFR_Sequence * );
extern ListNode           *bufr_getnode_sequence           ( BUFR_Sequence *, int pos );
extern void                bufr_add_descriptor_to_sequence ( BUFR_Sequence *, BufrDescriptor * );

extern BUFR_Sequence      *bufr_copy_sequence              ( BUFR_Sequence * );

extern int                 bufr_expand_node_descriptor     ( LinkedList *, ListNode *, int, BUFR_Tables * );
extern void                bufr_expand_sequence            ( BUFR_Sequence *lst, int flag, BUFR_Tables * );
extern BUFR_Sequence      *bufr_expand_descriptor          ( int desc, int flag, BUFR_Tables * );

extern BufrDDOp           *bufr_apply_Tables               ( BufrDDOp *ddo, BUFR_Sequence *bcl, BUFR_Template *tmplt, ListNode *, int *err );
extern int                 bufr_init_location              ( BufrDDOp *ddo, BufrDescriptor *bdsc );
extern int                 bufr_apply_op_crefval           
                             ( BufrDDOp *ddo, BufrDescriptor *bdsc, BUFR_Template *tmplt );

extern int                 bufr_check_sequence             ( BUFR_Sequence *descriptors, int version, int *flags, BUFR_Tables *, int );
extern BufrDescriptorArray bufr_sequence_to_array          ( BUFR_Sequence *descriptors, int dovalue );
extern BufrDPBM           *bufr_index_dpbm                 ( BufrDDOp *ddo, BUFR_Sequence *bcl );

#endif
