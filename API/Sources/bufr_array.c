/***
Copyright Her Majesty The Queen in Right of Canada, Environment Canada, 2009-2010.
Copyright Sa Majesté la Reine du Chef du Canada, Environnement Canada, 2009-2010.

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
#include <string.h>
#include <search.h>

#include "bufr_array.h"
#include "bufr_i18n.h"
#include "bufr_io.h"

 typedef struct { char *eles;  /* pointer to the elements               */
                  int   size;  /* size of each element in the array     */
                  int   grow;  /* increment of the array                */
                  int   count; /* number of elements added in the array */
                  int   total; /* current size of the array             */
                  int   msize;  /* size of each element in the array     */
                } Array ;

 static  void   arr_allocate( Array *arr, int len );



/**
 * @english
 *    add a new element to the dynamic array
 * @param char *array : pointer to array
 * @param char *elem  : pointer to element to add
 * @return   int :  new array size (current)
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */

 int
 arr_add( ArrayPtr obj, const void *elem )
    {
    Array *arr;
  
    if( obj == NULL ) return(0);

    arr = (Array *) obj;
  
    if (arr->count == arr->total) {
       if (arr->grow > 0)
         arr_allocate( arr, arr->total+arr->grow );
       else {
         char errmsg[512];

         sprintf( errmsg, _("Warning: initial array size too small: %d\n"), 
              arr->total );
	 bufr_print_debug( errmsg );
         sprintf( errmsg, _("Warning: item can't be added\n") );
	 bufr_print_debug( errmsg );
         return arr->count;
       }
    }
   
    memcpy( arr->eles+(arr->count*arr->size), elem, arr->size );

    return( ++(arr->count) );
    }


/**
 * @english
 *    allocates or reallocates the memory of the dynamic array
 * @param   char *array  :  pointer to array
 * @param   int   len    :  new array size
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
 static void
 arr_allocate( Array *arr, int len )
    {
    if( arr->eles == NULL )
      arr->eles = (char *)malloc((arr->size)*len);
    else
      arr->eles = (char *)realloc(arr->eles, (arr->size)*len);

    if( arr->eles == NULL )
      {
      printf(_(" error arr_allocate no more memory: %d\n"), len);
      exit(1);
      }

   arr->total = len;
   }


/**
 * @english
 *   return the number of elements inside the dynamic array
 * @param   char *array  :  pointer to array
 * @return  int  : array size
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
 int
 arr_count( ArrayPtr obj )
    {
    Array *arr = (Array *)obj;
    return( arr == NULL ? 0 : arr->count );
    }


/**
 * @english
 *    create a dynamic array
 * @param   int   len   :  initial size of the array
 * @param   int   size  :  size of each element in bytes
 * @param   int   grow  :  size increment when resizing
 * @return  char *array  :  pointer to array
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
 ArrayPtr
 arr_create(int len, int size, int grow)
    {
    Array *arr;

    if (grow > 0) {
      arr = (Array *)malloc(sizeof(Array));

      if( arr == NULL )
      {
      printf(_(" error arr_create no more memory\n"));
      exit(1);
      }

      arr->eles  = NULL;
      arr->count = 0;
      arr->total = 0;
      arr->size  = size;
      arr->grow  = grow;
      arr->msize  = sizeof(Array);
      if( len > 0 ) arr_allocate( arr, len );
      return((ArrayPtr)arr);
    } else {
      int msize = sizeof(Array)+len*size;
      arr = (Array *)malloc(msize);
      arr->msize  = msize;
      arr->eles = (char *)arr + sizeof(Array);
      arr->total = len;
      arr->count = 0;
      arr->size  = size;
      arr->grow  = 0;
      return((ArrayPtr)arr);
    }
  }

/**
 * @english
 *    delete the last n elements of a dynamic array
 * @param   int   nele   :  number of element to remove from array
 * @param   char *array  :  pointer to array
 * @return  int  : array size
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
 int
 arr_del( ArrayPtr obj, int nele )
    {
    Array *arr = (Array *)obj;

    if( arr        == NULL ) return(0);
    if( nele       == 0    ) return(0);
    if( arr->count == 0    ) return(0);

    arr->count -= nele;
    if( arr->count < 0 ) arr->count = 0;

    return( arr->count );
    }


/**
 * @english
 *   free a dynamic array
 * @param   char *array  :  pointer to array
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
 void
 arr_free( ArrayPtr *obj )
    {
    ArrayPtr *pobj=obj;
    Array *arr;

    if ( pobj == NULL ) return;
    arr=(Array *)(*pobj);
    if( arr == NULL ) return;

    if (arr->grow > 0)
       if( arr->eles != NULL ) free( arr->eles );
    free( arr );

    *pobj = NULL;
    }


/**
 * @english
 *     gets into a dynamic array the address of the element at position pos
 * @param   char *array  :  pointer to array
 * @param   int   pos    :  position of element to return >= 0, < n
 * @return  ArrayItemPtr :  a pointer to the address of the element
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
 ArrayItemPtr
 arr_get( ArrayPtr obj, int pos )
    {
    Array *arr=(Array *)obj;

    if( arr == NULL ) return(NULL);

    if( pos < 0 || pos >= arr->count ) return(NULL);

    return( arr->eles+((arr->size)*pos) );
    }


/**
 * @english
 *    increment the count and size of an array by NELE 
 * @param   ArrayPtr array  :  pointer to array
 * @param   int   nele      :  number of new element slot to allocate
 * @return  int             :  the new size of array
 * @warning  this does not add any new element, their value remain uninitialized
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
 int
 arr_inc( ArrayPtr obj, int nele )
    {
    Array *arr = (Array *)obj;

    if( arr        == NULL ) return(0);
    if( nele       == 0    ) return(0);

    if ((arr->count+nele) >= arr->total )
    {
       if (arr->grow > 0) {
         arr_allocate( arr, arr->total+((nele < arr->grow)?(arr->grow):nele) );
         arr->count += nele;
       }
    } else {
       arr->count += nele;
    }

    return( arr->count );
    }


/**
 * @english
 *    reduce a dynamic array to the exact number of elements it contains
 *    to free unused memory
 * @param   ArrayPtr array  :  pointer to array
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
 extern void
 arr_reduce( ArrayPtr obj )
    {
    Array *arr=(Array *)obj;

    if( arr        == NULL       ) return;
    if( arr->eles  == NULL       ) return;
    if( arr->count == arr->total ) return;
    if (arr->grow > 0) arr_allocate( arr,arr->count );
    }

/**
 * @english
 *    create a shared copy of an existing array without actually allocating it.
 *    for use with array allocated within shared memory
 * @param   ArrayPtr array  :  pointer to array
 * @return  ArrayPtr
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */

 ArrayPtr
 arr_share( ArrayPtr obj )
    {
    Array *arr = (Array *)obj;
    Array *rtrn;
    char  *parr;

    rtrn = (Array *)arr_create ( 0, arr->size, 100 );
    rtrn->size = arr->size;
    rtrn->grow = arr->grow;
    rtrn->count = arr->count;
    rtrn->total = arr->total;
    rtrn->msize = arr->msize;
    parr = (char *)obj;
    rtrn->eles = parr + sizeof(Array);
    return (char *)rtrn;
    }


/**
 * @english
 *  search an elements in the sorted dynamic array
 *  and return the address of the element if it's found
 *  compar returns -1, 0, 1 depending if its first argument
 *  is smaller, equal to or bigger than its second.
 * Note that the array must have been sorted via the same
 * comparison function or results will be undefined.
 * @param   ArrayPtr array  :  pointer to array
 * @param  void *vaddr: element we are searching
 * @param  int (*compar)(void *, void *)
 * @return  ArrayItemPtr
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 * @see arr_find
 */
 ArrayItemPtr
 arr_search( ArrayPtr obj, const void *vaddr, int (*compar)( const void *, const void * ) )
    {
    Array *arr=(Array *)obj;

    if( arr == NULL ) return NULL;
    if ((arr->eles == NULL)||(arr->count==0)) return NULL;

    return( (char *)bsearch( (void *)vaddr, (void *)arr->eles, 
	     (int)arr->count, arr->size, compar ) );
    }

/**
 * @english
 *  search an elements in the dynamic array
 *  and return the address of the element if it's found
 *  compar returns -1, 0, 1 depending if its first argument
 *  is smaller, equal to or bigger than its second.
 * Note that this function performs a linear search. While
 * slower than arr_search, it doesn't require the array
 * to have been sorted with any specific comparison function.
 * @param   ArrayPtr array  :  pointer to array
 * @param  void *vaddr: element we are searching
 * @param  int (*compar)(void *, void *)
 * @return  ArrayItemPtr
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 * @see arr_search
 */
 ArrayItemPtr
 arr_find( ArrayPtr obj, const void *vaddr, int (*compar)( const void *, const void * ) )
    {
    Array *arr=(Array *)obj;
	 int c;

    if( arr == NULL ) return NULL;
    if ((arr->eles == NULL)||(arr->count==0)) return NULL;

	 c = (int)arr->count;
    return( (char *)lfind( (void *)vaddr, (void *)arr->eles, 
	     &c, arr->size, compar ) );
    }


/**
 * @english
 *     set the value of an element to the dynamic array
 * @param   ArrayPtr array  :  pointer to array
 * @param   int  pos        :  position of element in the array
 * @param  void *elem       :  pointer to element
 * @return  int  : position of the element added
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
 int
 arr_set( ArrayPtr obj, int pos, const void *elem )
    {
    Array *arr;
  
    if( obj == NULL ) return(0);

    arr = (Array *) obj;
    if ((pos < 0)||(pos >=arr->count)) return(0);
  
    if( arr->count == arr->total ) arr_allocate( arr, arr->total+arr->grow );
  
    if (elem) 
       memcpy( arr->eles+(pos*arr->size), elem, arr->size );
    else
       memset( arr->eles+(pos*arr->size), 0, arr->size );

    return( pos );
    }

/**
 *
 * @english
 *     return the elements container's size of the dynamic array
 * @param   ArrayPtr array  :  pointer to array
 * @return  int  : size of container
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
 int
 arr_size( ArrayPtr obj )
    {
    return( obj == NULL ? 0 : ((Array *)obj)->msize );
    }


/**
 * @english
 *    sorts the elements in the dynamic array
 *    compar returns -1, 0, 1 depending if its first argument
 *    is smaller, equal to or bigger than its second
 * @param   ArrayPtr array  :  pointer to array
 * @param   int (*compar)(void *, void *) : compare function 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */

 void
 arr_sort( ArrayPtr obj, int (*compar)( const void *, const void * ) )
    {
    Array *arr=(Array *)obj;

    if( arr == NULL ) return;
    if ( arr->count <= 1 ) return;

    qsort( (void *)arr->eles, (int)arr->count, arr->size, compar );
    }



/**
 * @english
 *    free an array of string
 * @param   ArrayPtr array  :  pointer to array
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
void arr_free_string(ArrayPtr *parray)
{
    char **ps;
    int count, i;

    ArrayPtr array = *parray;
    
    count = arr_count( array );
    for (i = 0; i < count ; i++ ) 
    {
        ps = (char **)arr_get( array, i );
        if ( ps && *ps )
        {
            free( *ps );
            *ps = NULL;
        }
    }
    arr_free( parray );
}

/**
 * @english
 *    return the position of the matching string in the array
 * @param   ArrayPtr array  :  pointer to array
 * @param   const char *string : string to look for
 * @return  int   :  position in the array
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
int arr_find_string( ArrayPtr array, const char *string  )
   {
   int i, cnt;
   char **ptr, *cstring ;

   cnt = arr_count( array );
   for (i = 0; i < cnt ; i++)
      {
      ptr = (char **)arr_get( array, i );
      if (ptr)
         {
         cstring  = *ptr;
         if (cstring )
            {
            if (strcmp( string , cstring  ) == 0 )
               return i;
            }
         }
      }
   return -1;
   }


/**
 * @english
 *    add a new string to the array
 * @param   ArrayPtr array  :  pointer to array
 * @param   const char *string : new string to add (copied)
 * @return  int   :  position in the array
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
void  arr_add_string( ArrayPtr array, const char *string )
   {
   char *str;

   if (arr_find_string( array, string ))
      return;
   str = strdup( string );
   arr_add( array, (char *)&str );
   }



/**
 * @english
 *    compare function  for (float) array 
 * @param   const void *c1  :  first pointer to element to compare
 * @param   const void *c2  :  second pointer to element to compare
 * @return  int   :    -1, 0, 1
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
int arr_floatcmp(const void *c1, const void *c2)
    {
    float *f1 = (float *)c1;
    float *f2 = (float *)c2;

    if (*f1 < *f2) return -1;
    if (*f1 > *f2) return 1;
    return 0;
    }

/**
 * @english
 *    compare function  for (float) array (reversed order)
 * @param   const void *c1  :  first pointer to element to compare
 * @param   const void *c2  :  second pointer to element to compare
 * @return  int   :    -1, 0, 1
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
int arr_floatdcmp(const void *c1, const void *c2)
    {
    float *f1 = (float *)c1;
    float *f2 = (float *)c2;

    if (*f1 > *f2) return -1;
    if (*f1 < *f2) return 1;
    return 0;
    }

/**
 * @english
 *    compare function  for (int) array
 * @param   const void *c1  :  first pointer to element to compare
 * @param   const void *c2  :  second pointer to element to compare
 * @return  int   :    -1, 0, 1
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
int arr_intcmp(const void *c1, const void *c2)
    {
    int *f1 = (int *)c1;
    int *f2 = (int *)c2;

    if (*f1 < *f2) return -1;
    if (*f1 > *f2) return 1;
    return 0;
    }

/**
 * @english
 *    compare function  for (int) array (reversed order)
 * @param   const void *c1  :  first pointer to element to compare
 * @param   const void *c2  :  second pointer to element to compare
 * @return  int   :    -1, 0, 1
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup data_structures
 */
int arr_intDescCmp
   (const void *c1, const void *c2)
   {
   int *f1 = (int *)c1;
   int *f2 = (int *)c2;

   if (*f1 > *f2) return -1;
   if (*f1 < *f2) return 1;

   return 0;
   }

#ifdef TEST

/*
 * THE FOLLOWING MODULES ARE USED FOR TESTING PURPOSES ONLY
 */
 typedef struct { int pos;
                  float  v;
                } dummy;

 int
 arrcmp(const void *c1, const void *c2)
    {
    dummy *f1 = (dummy *)c1;
    dummy *f2 = (dummy *)c2;

    if ( f1->v < f2->v )
       return -1;
    else if ( f1->v > f2->v )
       return 1;
    else
       return 0;
    }

 test_array()
    {
    char  *arr;
    int    i;
    dummy  ele;
    dummy *pele;

    arr = arr_create( 10, sizeof(dummy), 10 );
    for( i = 255 ; i >= 0 ; i-- )
       {
       ele.pos = i;
       ele.v = (float)i*10;
       arr_add( arr, (char *)&ele );
       }

    arr_sort( arr, arrcmp );
    arr_del( arr, 128 );
    if( arr_count(arr) != 128 ) printf(_("Error in array count\n"));
    for( i = 0 ; i < 128 ; i++ )
       {
       pele = (dummy *)arr_get( arr, i ); 
       if((pele->pos != i)||(pele->v!=i*10))
         printf(_("Error in array\n"));
       }

    arr_inc( arr, 128 );
    if( arr_count(arr) != 256) printf(_("Error in array count\n"));

    arr_free( &arr );
    }



/*
 * ------------- MAN FIRST -----------
 * .TH ARRAY 1 "CMC" "Source documentation"
 * .ad b
 * .sh NAME
 * arr_create, arr_free, arr_add, arr_del, arr_inc
 * arr_count, arr_reduce, arr_get, arr_sort
 * .SH SYNOPSIS
 * 
 * #include  "array.h"
 * 
 * .nf
 * 
 * char   *arr_create(int len, int size, int grow);
 * 
 * void    arr_free( char **arr );
 * 
 * int     arr_add( char *arr, char *vaddr );
 * 
 * int     arr_del( char *arr, int nele );
 * 
 * int     arr_inc( char *arr, int nele );
 * 
 * int     arr_count( char *arr );
 * 
 * int     arr_reduce( char *arr );
 * 
 * char   *arr_get( char *arr, int pos );
 * 
 * char   *arr_search( char *arr, char *vaddr, int (*compar)(void *, void *) );
 * 
 * void    arr_sort( char *arr, int (*compar)(void *, void *) );
 * 
 * .ni
 * 
 * .br
 * .SH ARGUMENTS
 * 
 * len          Specify initial length of the array
 * 
 * size         Specify the size of each element in the array
 * 
 * grow         Specify how much the array will grow 
 *              if it is full
 * 
 * arr          Specify the array 
 * 
 * vaddr        Specify the address of the value to be add or set
 *  
 * pos          Specify the position in the array
 * 
 * compar       Specify the comparison function to be 
 *              used for the sort
 * 
 * nele         Specify the number of element
 * 
 * .SH DESCRIPTION
 * 
 * Dynamic Array can be use for storing data in a one dimensional array.
 * These data can be structures or just any pointers. 
 * At first, an array must be instantiated using \fIarr_create\fR.
 * Data can then be added in the array using \fIarr_add\fR which 
 * add the new element at the end of the array. 
 * If the array becomes too small to hold new data, 
 * it will resize itself to hold \fIgrow\fR
 * more items in the array. Use \fIarr_del\fR to remove nele items from
 * the end of the array, or \fIarr_inc\fR to incremente and reserve space
 * for nele items in the array without actually adding it.
 * \fIarr_count\fR will return the number of items in 
 * the array. \fIarr_get\fR will return the address of the
 * element in the array which must be cast to a pointer to the element
 * type in order to be used. 
 * At the creation or after some elements have been added to the array,
 * the size of an array may be bigger than the size needed to hold 
 * its contents. It can then be resized to free up memory space using
 * \fIarr_reduce\fR. 
 * The array can be sorted using \fIarr_sort\fR by supplying a 
 * comparison function which return -1, 0, 1 for smaller, equal or bigger.
 * this comparison function must accept two pointers to the compared
 * element. It is recommended to have array of pointers to structure instead
 * of array of structure for performance if sorting is needed.
 * A sorted array can also be searched for an element using \fIarr_search\fR.
 * Supply the same comparison function for sorting and a pointer to an
 * item in the array.
 *  
 * .SH EXAMPLES
 * 
 * .nf
 * 
 *  struct
 *  {
 *    ...
 *    value_t v;
 *    ...
 *  } dummy;
 * 
 *  {
 *      char  *arr;
 *      struct dummy ele;
 *      struct dummy *pele;
 *      int    nb_elem;
 * 
 *      ...
 *      arr = arr_create( 50, sizeof(struct dummy), 10 );
 *      for ( i = 0 ; i < nb_elem ; i++ )
 *      {
 *      ...
 *      arr_add( arr, (char *)ele );
 *      ...
 *      }
 *  
 *      arr_sort( arr, dcompar );
 *      nb_elem = arr_count( arr );
 *      for ( i = 0 ; i < nb_elem ; i++ )
 *      {
 *          ...
 *          pele = (struct dummy *)arr_get( arr, i ));
 *          ...
 *      }
 *      arr_free( &arr );
 *      ...
 *  }
 * 
 *  int dcompar(void *c1, void *c2)
 *  {
 *      dummy *f1 = *((dummy **)c1);
 *      dummy *f2 = *((dummy **)c2);
 * 
 *      if ( f1->v < f2->v )
 *         return -1;
 *      else if ( f1->v > f2->v )
 *         return 1;
 *      else
 *         return 0;
 *  }
 * 
 * .fi
 *
 * .SH FILES
 * 
 * ~afsmlib/include/array.h
 * ~afsmlib/lib/afsmlib.a
 * 
 * ------------- MAN LAST  -----------

*/
#endif /* TEST */
