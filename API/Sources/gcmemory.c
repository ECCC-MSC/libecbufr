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

 * file : gcalloc.c
 *
 * author:  Vanh Souvanlasy 
 *
 * function: 
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "private/gcmemory.h"


#define ABORT_OOM      \
{                      \
   perror("malloc");   \
   exit(1);            \
}

/*
 * link list block of free cells 
 */
struct _FreeBlock
{
  char   **cells;
  long     free_cnt;
  struct _FreeBlock *next;
};
typedef struct _FreeBlock FreeBlock;

/*
 * link list block of memory
 */
struct _MemBlock
{
  char    *memory;
  long     cells_cnt;
  struct _MemBlock *next;
};

typedef struct _MemBlock MemBlock;

typedef struct
{
   MemBlock  *blocks;
   FreeBlock *freeblks;
   long       blk_size;
   long       cell_size;
   long       free_size;
} MemBlkList;

static int  gcmem_verbose=0;
/*
 * this is for internal use only
 */
static void gcmem_addnewblk(MemBlkList *ml);
static void gcmem_addnewfree(MemBlkList *ml);

/**
 * @english
 * return gcmemory verbose mode, which can be enabled with env. GCMEMORY_VERBOSE
 * @param   MemBlkList *ml
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int gcmem_is_verbose(void)
   {
   static char *env = NULL;

   if (env == NULL) 
      {
      env = getenv( "GCMEMORY_VERBOSE" );
      if ( env && ((strcmp( env, "yes" )==0)||(strcmp( env, "1" )==0)) )
            gcmem_verbose = 1;
      }
   return gcmem_verbose;
   }

/**
 * @english
 * return gcmemory initial allocation size
 * @param   MemBlkList *ml
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int gcmem_blk_size(char *m)
{
    MemBlkList *ml=(MemBlkList *)m;

    return ml->blk_size ;
}

/**
 * @english
 * add a new memory block to stack when no more room left in allocated blocks
 * @param   MemBlkList *ml
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void gcmem_addnewblk(MemBlkList *ml)
{
    MemBlock *blk;

    blk = (MemBlock *)malloc( sizeof(MemBlock) );
    if ( blk == NULL ) ABORT_OOM;
    blk->memory = (char *)malloc((ml->blk_size)*(ml->cell_size));
    if ( blk->memory == NULL ) ABORT_OOM;
    blk->cells_cnt = 0;
/*
 * push new block
 */
    blk->next = ml->blocks;
    ml->blocks = blk;
}

/**
 * @english
 * add a new freed memory block to stack when no more room left in allocated blocks
 * @param   MemBlkList *ml
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void gcmem_addnewfree(MemBlkList *ml)
{
    FreeBlock *blk;

    blk = (FreeBlock *)malloc( sizeof(FreeBlock) );
    if ( blk == NULL ) ABORT_OOM;
    blk->cells = (char **)malloc((ml->free_size)*sizeof(char *));
    if ( blk->cells == NULL ) ABORT_OOM;
    blk->free_cnt = 0;
/*
 * push new block
 */
    blk->next = ml->freeblks;
    ml->freeblks = blk;
}

/**
 * @english
 * obtain a cell address from allocated memory blocks list
 * @param   MemBlkList *ml
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
char *gcmem_alloc(char *m)
{
    MemBlock *blk;
    FreeBlock *fblk;
    MemBlkList *ml=(MemBlkList *)m;
    char     *addr;

    if (ml == NULL) return NULL;
/*
 * look in free blocks first
 */
    fblk = ml->freeblks;
    if ( fblk->free_cnt > 0 )
    {
      fblk->free_cnt -= 1;
      addr = fblk->cells[fblk->free_cnt];
      if ((fblk->free_cnt <= 0)&&(fblk->next))
      {
         ml->freeblks = fblk->next;
         free( fblk->cells );
         free( fblk );
      }
      return (addr);
    }

    blk = ml->blocks;
    if ( blk->cells_cnt >= ml->blk_size )
    {
       gcmem_addnewblk( ml );
       blk = ml->blocks;
    }
    addr =  blk->memory + ((blk->cells_cnt) * (ml->cell_size)) ;
    blk->cells_cnt += 1;
    return( addr );
}

/**
 * @english
 * free a cell address of allocated memory blocks list,
 * memory are not freed but moved to freed list
 * until gcmem_delete is called
 * @param   (MemBlkList *ml) casted as (char *)
 * @param   addr   address of memory to be freed (allocated by gcmem_alloc)
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void gcmem_dealloc(char *m, void *addr)
{
    FreeBlock *fblk;
    MemBlkList *ml=(MemBlkList *)m;

    if (ml == NULL) return;
    fblk = ml->freeblks;
    if ( fblk->free_cnt >= ml->free_size )
    {
       gcmem_addnewfree( ml );
       fblk = ml->freeblks;
    }
    fblk->cells[fblk->free_cnt] = addr;
    fblk->free_cnt += 1;
}

/**
 * @english
 * free a memory blocks list
 * @param   (MemBlkList *ml) casted as (char *)
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int gcmem_delete(char *m)
{
  MemBlkList *ml=(MemBlkList *)m;
  MemBlock  *blk;
  FreeBlock  *fblk;
  int        totalused=0;

  if (ml == NULL) return 0;
  blk=ml->blocks;
  fblk=ml->freeblks;
  while ( blk )
  {
     totalused += blk->cells_cnt;
     ml->blocks = blk->next;
     free( blk->memory );
     free( blk );
     blk = ml->blocks;
  }

  while( fblk )
  {
     ml->freeblks = fblk->next;
     free( fblk->cells );
     free( fblk );
     fblk = ml->freeblks;
  }

  free( ml );
  return totalused;
}

/**
 * @english
 * free and reduce memory blocks list
 * @param   (MemBlkList *ml) casted as (char *)
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void gcmem_free(char *m)
{
  MemBlkList *ml=(MemBlkList *)m;
  MemBlock  *blk;
  FreeBlock  *fblk;

  if (ml == NULL) return;
  blk=ml->blocks;
  fblk=ml->freeblks;
  while ( blk->next )
  {
     ml->blocks = blk->next;
     free( blk->memory );
     free( blk );
     blk = ml->blocks;
  }
  blk->cells_cnt = 0;

  while( fblk->next )
  {
     ml->freeblks = fblk->next;
     free( fblk->cells );
     free( fblk );
     fblk = ml->freeblks;
  }
  fblk->free_cnt = 0;
}

/**
 * @english
 * create a new memory blocks list
 * @param   blk_size   size of each block
 * @param   cell_size  size of each item in a block
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
char *gcmem_new(long blk_size, long cell_size)
{
    MemBlkList *ml;

    ml = (MemBlkList *)malloc( sizeof(MemBlkList) );
    if ( ml == NULL ) ABORT_OOM;
    ml->cell_size = cell_size;
    ml->free_size = blk_size/4;
    if ( ml->free_size < 100 ) ml->free_size = 100;
    ml->blk_size = blk_size;
    ml->blocks = NULL;
    ml->freeblks = NULL;
    gcmem_addnewfree( ml );
    gcmem_addnewblk( ml );
    return ((char *)ml);
}
