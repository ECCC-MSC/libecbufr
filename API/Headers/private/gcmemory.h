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

 *
 *  file      :  gcmemory.h
 *
 *  author    :  
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  THIS IS THE HEADER FILE
 *               FOR MEMORY GARBAGE COLLECTOR
 *               IN ORDER TO IMPROVE EFFICIENCY
 *
 *
 */

#ifndef _MYGCALLOC_H
#define _MYGCALLOC_H

/*****************************************************************************/
/*****************************************************************************/
extern char    *gcmem_alloc(char *m);
extern void     gcmem_dealloc(char *m, void *cell);
extern int      gcmem_delete(char *m);
extern void     gcmem_free(char *m);
extern char    *gcmem_new(long blk_size, long cell_size);
extern int      gcmem_is_verbose(void);
extern int      gcmem_blk_size(char *m);

#endif
