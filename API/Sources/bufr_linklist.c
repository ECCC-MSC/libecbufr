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
***/    

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bufr_linklist.h"
#include "private/gcmemory.h"
#include "config.h"

static void *  ListNode_gcmemory=NULL;


/**************************************************************************
 ***NOM: *lst_newlist()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: Creer une structure d'une liste chainee 
 *
 *LIBRAIRIES: 
 *
 *ARGUMENTS: Aucun
 *
 **
--------------------------------------------------------------------------*/
LinkedList *lst_newlist(void)
   {
   LinkedList *tmp;
   
   tmp = (LinkedList *) malloc ( sizeof(LinkedList));
   assert( tmp );
   tmp->last = tmp->first = NULL;
   tmp->nb_node = 0; /* indique le nombre de noeud dans la liste */
   tmp->name = NULL;
   return( tmp );
   }

/**************************************************************************
 ***NOM: *lst_newnode()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: Creer une structure d'un noeud d'une liste chainee, dans laquelle
 *       se trouve egalement la donnee de cette noeud
 *
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *   void *data  :   donnee que contient ce noeud
 *
 **
--------------------------------------------------------------------------*/
ListNode * lst_newnode(void *data)
   {
   ListNode * tmp;

#if USE_GCMEMORY
   if (ListNode_gcmemory == NULL)
      {
      ListNode_gcmemory = gcmem_new(GCMEM_LISTNODE_SIZE, sizeof(ListNode) );
      }

   tmp = gcmem_alloc(ListNode_gcmemory);
#else
   if ((tmp = (ListNode *) malloc(sizeof(ListNode))) == NULL) 
      {
      perror( "malloc" );
      exit(1);
      }
#endif

   tmp->data = data;
   tmp->next = NULL;
   tmp->prev = NULL;
   tmp->name = NULL;
   return ( tmp );
   }


/**************************************************************************
 ***NOM: *lst_addfirst()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: Ajouter un nouveau noeud au debut de la liste chainee
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList  *clst  :  pointeur sur la liste chainee 
 *    ListNode    * node :  le pointeur sur la noeud 
 **
--------------------------------------------------------------------------*/
void lst_addfirst( LinkedList *clst, ListNode *node)
   {
   if (( node == NULL )||( clst == NULL ))
      return;

   if ( clst->first == NULL )
      {
      clst->last = clst->first = node;
      node->prev = NULL;
      node->next = NULL;
      clst->nb_node = 1;
      }
   else 
      {
      node->next = clst->first;
      node->prev = NULL;
      clst->first->prev = node;
      clst->first = node;
      clst->nb_node += 1;
      }
   }

/**************************************************************************
 ***NOM: *lst_addlast()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: Ajouter un nouveau noeud a la fin de la liste chainee
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList  *clst : pointeur sur la liste chainee 
 *    ListNode    *node : le pointeur sur la noeud 
 *
 **
--------------------------------------------------------------------------*/
void lst_addlast( LinkedList *clst, ListNode *node)
   {
   if (( node == NULL )||( clst == NULL ))
      return;
   /*
    * si la liste est vide, alors ajouter au debut de la liste
    */
   if ( clst->first == NULL )
      {
      lst_addfirst( clst , node );
      }
   else 
      {
      node->next = NULL;
      clst->last->next = node;
      node->prev = clst->last;
      clst->last = node;
      clst->nb_node += 1;
      }
   }

/**************************************************************************
 ***NOM: *lst_addpos()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: Ajouter un nouveau noeud a une position de la liste chainee
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList  *clst :  pointeur sur la liste chainee 
 *    ListNode    *node :  le pointeur sur la noeud 
 *    int          pos  :  la position dans la liste a ajouter
 **
--------------------------------------------------------------------------*/
void lst_addpos( LinkedList *clst, ListNode *node, int pos)
   {
   ListNode * current;

   if (( node == NULL )||( clst == NULL ))
      return;
   
   /*
    * Ajouter a la fin si la position est 0 
    */
   if ( pos == 0 )
      lst_addlast( clst , node );
   else if ( pos == 1)
      lst_addfirst( clst , node );
   else 
      {
      /*
       * Localiser le pointeur sur un noeud precedent a la position a 
       * laquelle on veut inserer
       */
      current = (ListNode *) lst_nodepos( clst,  pos-1 );
      if ( current != NULL )
         lst_addafter( clst, current, node );
      else
         lst_addlast( clst , node );
      }
   }
   
/**************************************************************************
 ***NOM: *lst_deletepos()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: Enlever un noeud a une position de la liste chainee
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList  *clst : pointeur sur la liste chainee 
 *    int          pos  : la position dans la liste a ajouter
 **
--------------------------------------------------------------------------*/
void lst_deletepos( LinkedList *clst, int pos)
   {
   ListNode * node;

   node = lst_rmpos( clst, pos );

   if ( node != NULL ) lst_delnode( node );
   }

/**************************************************************************
 ***NOM: *lst_rmpos()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: Enlever un noeud a une position de la liste chainee
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList  *clst : pointeur sur la liste chainee 
 *    int          pos  : la position dans la liste a ajouter
 **
--------------------------------------------------------------------------*/
ListNode *lst_rmpos( LinkedList *clst, int   pos)
   {
   ListNode *current, *node;

   if ( ( pos <= 0) || (pos > clst->nb_node) || ( clst == NULL ))
      return ( NULL );
   
   /*
    * Enlever le premier
    */
   if ( pos == 1 )
      node = lst_rmfirst( clst );
   else 
      {
      /*
       * Localiser le pointeur sur un noeud precedent a la position a 
       * laquelle on veut enlever
       */
      current = (ListNode *) lst_nodepos( clst,  pos-1 );
      if ( current != NULL )
         node = lst_rmafter( clst, current );
      }
   return( node );
   }
   
/**************************************************************************
 ***NOM: *lst_addafter()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: Ajouter un nouveau noeud apres un noeud dans la liste chainee
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList  *clst  :  pointeur sur la liste chainee 
 *    ListNode    *after :  le pointeur sur la noeud qui precede
 *                          le noeud inserer
 *    ListNode    *node  :  le pointeur sur la noeud 
 *
 **
--------------------------------------------------------------------------*/
void lst_addafter( LinkedList *clst, ListNode *after, ListNode *node)
   {
   if (( node == NULL )||( clst == NULL )||( after == NULL))
      return;
   if ( clst->first == NULL )
      {
      lst_addfirst( clst , node );
      } 
   else if ( clst->last == after ) 
      {
      lst_addlast( clst, node );
      } 
   else 
      {
      node->next = after->next;
      node->prev = after;
      after->next = node;
      clst->nb_node += 1;
      }
   }

/**************************************************************************
 ***NOM: *lst_addbefore()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: Ajouter un nouveau noeud devant un noeud dans la liste chainee
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList    *clst :  pointeur sur la liste chainee 
 *    ListNode      *b4   :  le pointeur sur la noeud qui precede
 *                           le noeud inserer
 *    ListNode      *node :  le pointeur sur la noeud 
 *
 **
--------------------------------------------------------------------------*/
void lst_addbefore( LinkedList *clst, ListNode *b4, ListNode *node)
   {
   if (( node == NULL )||( clst == NULL ))
      return;
   if (( clst->first == NULL )||(clst->first == b4))
      {
      lst_addfirst( clst , node );
      } 
   else if ( b4 == NULL ) 
      {
      lst_addlast( clst, node );
      } 
   else 
      {
      node->next = b4;
      node->prev = b4->prev;
      b4->prev->next = node;
      b4->prev = node;
      clst->nb_node += 1;
      }
   }
   
/**************************************************************************
 ***NOM: *lst_rmfirst()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: Enlever un noeud au debut de la liste chainee
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList    *clst : pointeur sur la liste chainee 
 **
--------------------------------------------------------------------------*/
ListNode *lst_rmfirst(LinkedList *clst)
   {
   ListNode * tmp;


   if ( clst == NULL ) return( NULL );
   if ( clst->first == NULL ) return( NULL );

   tmp = clst->first;
   clst->first = tmp->next;

   if ( tmp == clst->last )
      clst->last = tmp->prev;

   if ( clst->first )
      clst->first->prev = NULL;

   tmp->next = NULL;
   tmp->prev = NULL;

   clst->nb_node -= 1;
   return( tmp );
   }

/**************************************************************************
 ***NOM: *lst_rmafter()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: Enlever un noeud apres celui pointe par le pointeur node de la lisT
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList    *clst  : pointeur sur la liste chainee 
 *    ListNode      *after :  le pointeur sur la noeud a qui precede
 *                          le noeud enlever
 *
 **
--------------------------------------------------------------------------*/
ListNode * lst_rmafter(LinkedList *clst, ListNode * node)
   {
   ListNode * tmp;

   /*
    * liste et noeud illegale
    */
   if (( node == NULL )||( clst == NULL ) || ( node->next == NULL ))
      return( NULL );

   if ( node->next == clst->last ) clst->last = node;

   tmp = node->next;
   node->next = tmp->next;
   if (node->next)
      node->next->prev = node;
   tmp->next = NULL;
   tmp->prev = NULL;
   clst->nb_node -= 1;
   return( tmp );
   }

/**************************************************************************
 ***NOM: *lst_rmnode()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: Enlever un noeud 
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList    *clst : pointeur sur la liste chainee 
 *    ListNode      *node  :  le pointeur sur la noeud a qui sera enleve
 *
 **
--------------------------------------------------------------------------*/
ListNode * lst_rmnode(LinkedList *clst, ListNode * node)
   {
   if (clst == NULL) return NULL;
   if (node == NULL) return NULL;
   if (clst->first == NULL) return NULL;

   if (clst->first == node)
      return lst_rmfirst( clst );
   else if (clst->last == node)
      return lst_rmlast( clst );
   else
      {
      node->prev->next = node->next;
      node->next->prev = node->prev;
      node->next = NULL;
      node->prev = NULL;
      clst->nb_node -= 1;
      return node;
      }
   }

/**************************************************************************
 ***NOM: *lst_rmlast()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: Enlever un noeud a la fin de la liste chainee
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList    *clst : pointeur sur la liste chainee 
 **
--------------------------------------------------------------------------*/
ListNode * lst_rmlast( LinkedList *clst)
   {
   ListNode * tmp;

   if (clst == NULL) return NULL;
   if (clst->first == NULL) return NULL;
   /*
    * un seul element a enlever de la liste 
    */
   if ( clst->first == clst->last ) 
      {
      tmp = lst_rmfirst( clst );
      return( tmp );
      }
   /*
    * deplacer jusqu'a l'avant dernier de la liste 
    */
   for ( tmp = clst->first ; tmp->next->next != NULL ; tmp = tmp->next )
      ;

   clst->last = tmp;
   tmp = tmp->next;
   clst->last->next = NULL;
   clst->nb_node -= 1;
   tmp->next = NULL;
   tmp->prev = NULL;
   return ( tmp );
   }

/**************************************************************************
 ***NOM: lst_nodepos()
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: retourner un pointeur sur le noeud a la position
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList    *clst : pointeur sur la liste chainee 
 *    int           pos  : la position du noeud a partir de 1
 *
 *VALEUR RETOURNEE: 
 *
 *    ListNode * : un pointeur sur le noeud a la position
 **
--------------------------------------------------------------------------*/
ListNode * lst_nodepos(LinkedList *clst, int pos)
   {
   ListNode *    current;
   int          i;

   if ( clst == NULL ) return NULL;

   if (( clst->first == NULL ) || (pos <= 0) || (pos > clst->nb_node))
      {
      return ( NULL );
      }
   else 
      {
      /*
       * Parcourir jusqu'a la position desiree
       */
      for ( current = clst->first, i = 1; (i<pos)&&(current->next != NULL ) ;
            i++ , current = current->next );
      return ( current );
      }
   }

/**************************************************************************
 ***NOM: lst_dellist
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: detruire la liste et tous ses noeuds 
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList    *clst : pointeur sur la liste chainee 
 **
--------------------------------------------------------------------------*/
void lst_dellist( LinkedList *clst )
   {
   ListNode *current, *tmp;

   if (clst == NULL) return;

   for ( current = clst->first ; current != NULL ; ) 
      {
      tmp = current;
      current = current->next;
      lst_delnode( tmp );
      }

   if (clst->name)
      free( clst->name );

   free( clst );
   }

/**************************************************************************
 ***NOM: lst_delnode
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: detruire un noeud 
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    ListNode  *tmp : pointeur sur un noeud
 **
--------------------------------------------------------------------------*/
void lst_delnode(ListNode *tmp)
   {
   if (tmp->name)
      {
      free( tmp->name );
      tmp->name = NULL;
      }
#if USE_GCMEMORY
   gcmem_dealloc( ListNode_gcmemory,  (caddr_t)tmp );
#else
   free( tmp );
#endif
   }

/**************************************************************************
 ***NOM: lst_movelist
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: deplacer tous les noeuds d'une liste vers une autre liste
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList *dest :   
 *    LinkedList *src  :  
 *    ListNode   *clst : pointeur sur un noeud a inserer apres
 *                      si NULL, alors insere au debut de la liste
 **
--------------------------------------------------------------------------*/
int lst_movelist(LinkedList *dest, ListNode *after, LinkedList *src )
   {
   if ((src->last == NULL)&&(src->first == NULL)) return 0;

   if ((dest->last == NULL)&&(dest->first == NULL)) 
      {                              /* dest is empty */
      dest->last = src->last;
      dest->first = src->first;
      dest->nb_node = src->nb_node;
      } 
   else 
      {
      if (after == NULL)   /* insert at beginning */
         {
         src->last->next = dest->first;
         dest->first->prev = src->last;
         dest->first = src->first;
         }
      else                 /* insert after a node */
         {
         src->last->next = after->next;
         if (after->next)
            after->next->prev = src->last;
         after->next = src->first;
         src->first->prev = after;
         if (dest->last == after)
            dest->last = src->last;
         }
      dest->nb_node += src->nb_node;
      }

   src->last = src->first = NULL;
   src->nb_node = 0;
   return dest->nb_node;
   }

/**************************************************************************
 ***NOM: lst_namelist
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: associer un nom a une liste
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList  *clst : pointeur sur la liste a nommer
 *    char        *name :
 **
--------------------------------------------------------------------------*/
char *lst_namelist( LinkedList *clst, const char *name )
   {
   if (name)
      {
      if (clst->name) free( clst->name );
      clst->name = strdup( name );
      }
   return clst->name;
   }

/**************************************************************************
 ***NOM: lst_namenode
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: donner un nom a un noeud
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    ListNode    *node : pointeur sur un noeud
 *    char        *name :
 **
--------------------------------------------------------------------------*/
char *lst_namenode( ListNode *node, const char *name )
   {
   if (name)
      {
      if (node->name) free( node->name );
      node->name = strdup( name );
      }
   return node->name;
   }

/**************************************************************************
 ***NOM: lst_firstnode
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: retourne le 1er noeud de la liste
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList  *clst : pointeur sur la liste
 **
--------------------------------------------------------------------------*/
ListNode * lst_firstnode( LinkedList *clst )
   {
   if (clst == NULL) return NULL;
   return clst->first;
   }

/**************************************************************************
 ***NOM: lst_lastnode
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: retourne le dernier noeud de la liste
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList  *clst : pointeur sur la liste
 **
--------------------------------------------------------------------------*/
ListNode * lst_lastnode( LinkedList *clst )
   {
   if (clst == NULL) return NULL;
   return clst->last;
   }

/**************************************************************************
 ***NOM: lst_nextnode
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: retourne le prochain noeud de la liste
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    ListNode  *node : pointeur sur un noeud
 **
--------------------------------------------------------------------------*/
#ifndef  lst_nextnode
ListNode * lst_nextnode( ListNode *node )
   {
   if (node == NULL) return NULL;
   return node->next;
   }
#endif

/**************************************************************************
 ***NOM: lst_prevnode
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: retourne le noeud precedant de la liste
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    ListNode  *node : pointeur sur un noeud
 **
--------------------------------------------------------------------------*/
ListNode * lst_prevnode( ListNode *node )
   {
   if (node == NULL) return NULL;
   return node->prev;
   }

/**************************************************************************
 ***NOM: lst_skipnodes
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: sauver au nieme prochain noeud
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    ListNode  *current : pointeur sur un noeud
 *    int        count   : nombre de noeud a sauter
 **
--------------------------------------------------------------------------*/
ListNode *lst_skipnodes ( ListNode *current, int count )
   {
   int skip=count;
   ListNode *node=current;

   while ( node  && (skip > 0) )
      {
      --skip;
      node = lst_nextnode( node );
      }
   return node;
   }

/**************************************************************************
 ***NOM: lst_count
 *
 *AUTEUR: Souvanlasy Viengsavanh
 *
 *REVISION: AUCUN
 *
 *LANGAGE: C
 *
 *OBJET: compter le nombre de noeuds
 *      
 *LIBRAIRIES: 
 *
 *ARGUMENTS: 
 *
 *    LinkedList  *clst : pointeur sur la liste
 **
--------------------------------------------------------------------------*/
int lst_count ( LinkedList *clst )
   {
   if ( clst == NULL ) return 0;
   return  clst->nb_node ;
   }

/**
 * @english
 * @brief free internal garbage collector
 *   free memory allocation used to create garbage collector of linked list
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 * @warning should be called by bufr_end_api
 */
void bufr_linklist_end(void)
   {
#if USE_GCMEMORY
   int size, isize;
   char errmsg[256];

   isize = gcmem_blk_size( ListNode_gcmemory );
   size = gcmem_delete( ListNode_gcmemory );
   ListNode_gcmemory = NULL;
   if (gcmem_is_verbose()) 
      {
      sprintf( errmsg, "GCMEM used %d ListNode, blocs size=%d\n", size, isize );
      bufr_print_output( errmsg );
      }
#endif
   }

