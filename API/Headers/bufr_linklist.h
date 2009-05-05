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
 
 *  file      :  LINKEDLIST.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR LINKED LIST
 *
 *
 */

#ifndef _Linked_List_h
#define _Linked_List_h


/***************************************************************************/

#include <sys/types.h>
#include <stdio.h>

/***************************************************************************/
/***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

typedef char *ListNodeDataPtr;

struct List_Node;
typedef struct List_Node
   {
	ListNodeDataPtr   data;
	struct List_Node *prev;
	struct List_Node *next;
   char     *name;
   } ListNode;

/***************************************************************************/

typedef struct 
   {
	ListNode *first;
	ListNode *last; 
	int	nb_node; 
   char     *name;
   } LinkedList;

/***************************************************************************/

/***************************************************************************/

extern LinkedList    *lst_newlist   ( void );
extern void           lst_dellist   ( LinkedList *clst );
extern int            lst_movelist  ( LinkedList *dest, ListNode *, LinkedList *src );

extern ListNode      *lst_newnode   ( void *data );
extern void           lst_delnode   ( ListNode * );

extern void           lst_addfirst  ( LinkedList *clst, ListNode *node );
extern void           lst_addlast   ( LinkedList *clst, ListNode *node );
extern void           lst_addafter  ( LinkedList *clst, ListNode *after, ListNode *node );
extern void           lst_addbefore ( LinkedList *clst, ListNode *after, ListNode *node );
extern void           lst_addpos    ( LinkedList *clst, ListNode *node, int pos );

extern void           lst_deletepos ( LinkedList *clst, int pos );
extern ListNode      *lst_rmpos     ( LinkedList *clst, int   pos );
extern ListNode      *lst_rmfirst   ( LinkedList *clst );
extern ListNode      *lst_rmlast    ( LinkedList *clst );
extern ListNode      *lst_rmnode    ( LinkedList *clst, ListNode * node );
extern ListNode      *lst_rmafter   ( LinkedList *clst, ListNode * node );
extern ListNode      *lst_nodepos   ( LinkedList *clst, int pos );

extern ListNode      *lst_nextnode  ( ListNode *node );
extern ListNode      *lst_prevnode  ( ListNode *node );
extern ListNode      *lst_firstnode ( LinkedList *);
extern ListNode      *lst_lastnode  ( LinkedList *);
extern ListNode      *lst_skipnodes ( ListNode *current, int count );

extern char          *lst_namenode  ( ListNode *node, const char *name );
extern char          *lst_namelist  ( LinkedList *clst, const char *name );
extern int            lst_count     ( LinkedList *clst );

#ifdef __cplusplus
}
#endif

/***************************************************************************/
#endif /* _Linked_List */
