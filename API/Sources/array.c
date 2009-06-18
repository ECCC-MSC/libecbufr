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
 *  file      :  ARRAY.C
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  THIS FILE CONTAINS ALL THE MODULES
 *               MANAGING DYNAMIC ARRAYS
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bufr_array.h"

 typedef struct { char *eles;  /* pointer to the elements               */
                  int   size;  /* size of each element in the array     */
                  int   grow;  /* increment of the array                */
                  int   count; /* number of elements added in the array */
                  int   total; /* current size of the array             */
                  int   msize;  /* size of each element in the array     */
                } Array ;

 static  void   arr_allocate( Array *arr, int len );


/*
 *
 *  module    :  ARR_ADD
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  char *array;
 *               char *elem;
 *               int   count;
 *               count = arr_add( array, elem );
 *
 *  object    :  THIS MODULE ADDS A NEW ELEMENT TO THE DYNAMIC ARRAY
 *

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
         fprintf( stderr, "Warning: initial array size too small: %d\n", 
              arr->total );
         fprintf( stderr, "Warning: item can't be added\n" );
         return arr->count;
       }
    }
   
    memcpy( arr->eles+(arr->count*arr->size), elem, arr->size );

    return( ++(arr->count) );
    }


/*
 *
 *  module    :  ARR_ALLOCATE
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  char *array;
 *               int   len;
 *               arr_allocate( array, len );
 *
 *  object    :  THIS MODULE ALLOCATES OR REALLOCATES THE MEMORY
 *               OF THE DYNAMIC ARRAY
 *

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
      printf(" error arr_allocate no more memory: %d\n", len);
      exit(1);
      }

   arr->total = len;
   }


/*
 *
 *  module    :  ARR_COUNT
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  char *array;
 *               int   count;
 *               count = arr_count( array );
 *
 *  object    :  THIS MODULE RETURNS THE NUMBER OF ELEMENTS INTO
 *               THE DYNAMIC ARRAY
 *

*/

 int
 arr_count( ArrayPtr obj )
    {
    Array *arr = (Array *)obj;
    return( arr == NULL ? 0 : arr->count );
    }


/*
 *
 *  module    :  ARR_CREATE
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  char *array;
 *               int   len,size,grow;
 *               array = arr_create( len,size,grow );
 *
 *  object    :  THIS MODULE CREATES A DYNAMIC ARRAY.
 *

*/

 ArrayPtr
 arr_create(int len, int size, int grow)
    {
    Array *arr;

    if (grow > 0) {
      arr = (Array *)malloc(sizeof(Array));

      if( arr == NULL )
      {
      printf(" error arr_create no more memory\n");
      exit(1);
      }

      arr->eles  = NULL;
      arr->count = 0;
      arr->total = 0;
      arr->size  = size;
      arr->grow  = grow;
      arr->msize  = sizeof(Array);
      if( len > 0 ) arr_allocate( arr, len );
      return((char *)arr);
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

/*
 *
 *  module    :  ARR_DEL
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  char *array;
 *               int   n;
 *               int   count;
 *               count = arr_del( array, n );
 *
 *  object    :  THIS MODULE DELETES THE LAST N ELEMENTS OF A DYNAMIC ARRAY
 *

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


/*
 *
 *  module    :  ARR_FREE
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  char *array;
 *               arr_free( &array );
 *
 *  object    :  THIS MODULE FREES A DYNAMIC ARRAY.
 *

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


/*
 *
 *  module    :  ARR_GET
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  char *array;
 *               char *elem;
 *               int   pos;
 *               elem = arr_get( array, pos );
 *
 *  object    :  THIS MODULE GETS INTO A DYNAMIC ARRAY THE ADDRESS
 *               OF THE ELEMENT AT POSITION pos
 *

*/

 ArrayItemPtr
 arr_get( ArrayPtr obj, int pos )
    {
    Array *arr=(Array *)obj;

    if( arr == NULL ) return(NULL);

    if( pos < 0 || pos >= arr->count ) return(NULL);

    return( arr->eles+((arr->size)*pos) );
    }


/*
 *
 *  module    :  ARR_INC
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  char *array;
 *               int   n;
 *               int   count;
 *               count = arr_inc( array, n );
 *
 *  object    :  THIS MODULE INCREMENT THE COUNT AND SIZE OF AN ARRAY
 *               BY nele UNIT
 *

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


/*
 *
 *  module    :  ARR_REDUCE
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  char *array;
 *               arr_reduce( &array );
 *
 *  object    :  THIS MODULE REDUCE A DYNAMIC ARRAY TO THE EXACT
 *               NUMBER OF ELEMENTS IT CONTAINS
 *

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

/*
 *
 *  module    :  ARR_SHARE
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  char *array;
 *               newarray = arr_share( array );
 *
 *  object    :  
 *

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


/*
 *
 *  module    :  ARR_SEARCH
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  void *array;
 *               void *vaddr;
 *               int (*compar)(void *, void *);
 *               arr_search( array, vaddr, compar );
 *
 *  object    :  THIS MODULE SEARCH AN ELEMENTS IN THE DYNAMIC ARRAY
 *               AND RETURN THE ADDRESS OF THE ELEMENT IF IT'S FOUND
 *               COMPAR RETURNS -1, 0, 1 DEPENDING IF ITS FIRST ARGUMENT
 *               IS SMALLER, EQUAL TO OR BIGGER THAN ITS SECOND.
 *

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


/*
 *
 *  module    :  ARR_SET
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  void *array;
 *               void *elem;
 *               int   count;
 *               count = arr_add( array, elem );
 *
 *  object    :  THIS MODULE SET A NEW ELEMENT TO THE DYNAMIC ARRAY
 *

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

/*
 *
 *  module    :  ARR_SIZE
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  char *array;
 *               int   count;
 *               count = arr_count( array );
 *
 *  object    :  THIS MODULE RETURNS THE NUMBER OF ELEMENTS INTO
 *               THE DYNAMIC ARRAY
 *

*/

 int
 arr_size( ArrayPtr obj )
    {
    return( obj == NULL ? 0 : ((Array *)obj)->msize );
    }


/*
 *
 *  module    :  ARR_SORT
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  call      :  char *array;
 *               int (*compar)(void *, void *);
 *               arr_sort( array, compar );
 *
 *  object    :  THIS MODULE SORTS THE ELEMENTS IN THE DYNAMIC ARRAY
 *               COMPAR RETURNS -1, 0, 1 DEPENDING IF ITS FIRST ARGUMENT
 *               IS SMALLER, EQUAL TO OR BIGGER THAN ITS SECOND.
 *

*/

 void
 arr_sort( ArrayPtr obj, int (*compar)( const void *, const void * ) )
    {
    Array *arr=(Array *)obj;

    if( arr == NULL ) return;

    qsort( (void *)arr->eles, (int)arr->count, arr->size, compar );
    }



/*
 ****************************************************************************
 ***NAME: arr_free_string
 *
 *AUTHOR: Souvanlasy Viengsavanh 18/10/2000
 *
 *PURPOSE: free an array of string
 *
 *LANGAGE:  C
 *
 *NOTES:
 *
 *PARAMETERS:
 *
 *  parray : array of string
 **
 *---------------------------------------------------------------------------

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
               return 1;
            }
         }
      }
   return 0;
   }


void  arr_add_string( ArrayPtr array, const char *string )
   {
   char *str;

   if (arr_find_string( array, string ))
      return;
   str = strdup( string );
   arr_add( array, (char *)&str );
   }



int arr_floatcmp(const void *c1, const void *c2)
    {
    float *f1 = (float *)c1;
    float *f2 = (float *)c2;

    if (*f1 < *f2) return -1;
    if (*f1 > *f2) return 1;
    return 0;
    }

int arr_floatdcmp(const void *c1, const void *c2)
    {
    float *f1 = (float *)c1;
    float *f2 = (float *)c2;

    if (*f1 > *f2) return -1;
    if (*f1 < *f2) return 1;
    return 0;
    }

int arr_intcmp(const void *c1, const void *c2)
    {
    int *f1 = (int *)c1;
    int *f2 = (int *)c2;

    if (*f1 < *f2) return -1;
    if (*f1 > *f2) return 1;
    return 0;
    }

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
 *
 *  module    :  TEST_ARRAY
 *
 *  author    :  Souvanlasy Viengsvanh
 *
 *  revision  :  V0.1   Michel Grenier
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  THE FOLLOWING MODULES ARE USED FOR TESTING PURPOSES ONLY
 *

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
    if( arr_count(arr) != 128 ) printf("Error in array count\n");
    for( i = 0 ; i < 128 ; i++ )
       {
       pele = (dummy *)arr_get( arr, i ); 
       if((pele->pos != i)||(pele->v!=i*10))
         printf("Error in array\n");
       }

    arr_inc( arr, 128 );
    if( arr_count(arr) != 256) printf("Error in array count\n");

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
