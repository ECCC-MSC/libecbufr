/***
Copyright Her Majesty The Queen in Right of Canada, Environment Canada, 2010.
Copyright Sa Majesté la Reine du Chef du Canada, Environnement Canada, 2010.

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
#include "check.h"
#include "array.c"
#include "check_libecbufr.h"

#define ADD_TEST_CASE(X) {                                    \
                          TCase *tc_##X = tcase_create (#X);   \
                          tcase_add_test (tc_##X, test_##X##_core); \
                          suite_add_tcase (s, tc_##X);          \
                         }

START_TEST (test_arr_add_core)
{
ArrayPtr arrptr;
Array *arr;
char *elem1="5";
char *elem2="7";
char *elem3="9";
int nb_elem;

// Creating an array with one element
arrptr=arr_create(1,1, 2);
arr = (Array *)arrptr;

//Adding a first element
nb_elem=arr_add(arrptr, (void *)elem1);
fail_unless(memcmp(elem1,arr->eles,1)==0, "Copy of the first element failed: character in array: %.1s", arr->eles);
fail_unless(nb_elem==1, "Bad number of elements: %d", nb_elem);
fail_unless(arr->total==1, "Bad array size: %d", arr->total);

//Adding second element - A growth of 2 in total size of the array should occur
nb_elem=arr_add(arrptr, (void *)elem2);
fail_unless(memcmp(elem2,arr->eles+1,1)==0, "Copy of the first element failed: character in array: %.1s", arr->eles+1);
fail_unless(nb_elem==2, "Bad number of elements: %d", nb_elem);
fail_unless(arr->total==3, "Bad array size: %d", arr->total);

//Adding third element
nb_elem=arr_add(arrptr, (void *)elem3);
fail_unless(memcmp(elem3,arr->eles+2,1)==0, "Copy of the first element failed: character in array: %.1s", arr->eles+2);
fail_unless(nb_elem==3, "Bad number of elements: %d", nb_elem);
fail_unless(arr->total==3, "Bad array size: %d", arr->total);
}
END_TEST

START_TEST (test_arr_count_core)
{
ArrayPtr arrptr=NULL;
char *elem1="5";
char *elem2="7";
char *elem3="9";
int nb_elem;

// Creating an array with one element
arrptr=arr_create(1,1, 2);

// Test new array with no elements
nb_elem=arr_count(arrptr);
fail_unless(nb_elem==0, "Bad number of elements in new empty array: %d", nb_elem);

//Adding a first element
arr_add(arrptr, (void *)elem1);
nb_elem=arr_count(arrptr);
fail_unless(nb_elem==1, "Bad number of elements: %d, should be 1", nb_elem);

//Adding second element - growth of total size of array is 2
arr_add(arrptr, (void *)elem2);
nb_elem=arr_count(arrptr);
fail_unless(nb_elem==2, "Bad number of elements: %d, should be 2", nb_elem);

//Adding third element
arr_add(arrptr, (void *)elem3);
nb_elem=arr_count(arrptr);
fail_unless(nb_elem==3, "Bad number of elements: %d, should be 3", nb_elem);
}
END_TEST

START_TEST (test_arr_get_core)
{
ArrayPtr arrptr=NULL;
ArrayItemPtr itemptr=NULL;
char *elem1="5";
char *elem2="7";
char *elem3="9";
int nb_elem;

// Creating an array with one element
arrptr=arr_create(1,1, 2);

// Test new array with no elements
itemptr=arr_get(arrptr, 0);
fail_unless(itemptr==NULL, "Error: Non-null pointer for empty array");

//Adding a first element
arr_add(arrptr, (void *)elem1);
itemptr=arr_get(arrptr, 0);
fail_unless(itemptr!=NULL, "Error: pointer to item is null");
fail_unless(memcmp(itemptr, elem1, 1)==0, "Error: itemptr points at the wrong place");


//Adding second element - growth of total size of array is 2
// Testing the pointer to the first element
arr_add(arrptr, (void *)elem2);
itemptr=arr_get(arrptr, 0);
fail_unless(itemptr!=NULL, "Error: pointer to item is null");
fail_unless(memcmp(itemptr, elem1, 1)==0, "Error: itemptr points at the wrong place");

// Testing the pointer to the second element
itemptr=arr_get(arrptr, 1);
fail_unless(itemptr!=NULL, "Error: pointer to item is null");
fail_unless(memcmp(itemptr, elem2, 1)==0, "Error: itemptr points at the wrong place");

// Case with a pointer to an allocated memory space in the array but with no elements in it yet.  Should return NULL
itemptr=arr_get(arrptr, 2);
fail_unless(itemptr==NULL, "Error: Non-null pointer for an item that doesn't exist in the array (inside allocated memory)");

// Case with a pointer to an unallocated memory space outside the array.  Should return NULL
itemptr=arr_get(arrptr, 2);
fail_unless(itemptr==NULL, "Error: Non-null pointer for an item that doesn't exist in the array (outside allocated memory)");

// Case with a pointer to a negative place: before the array in memory
itemptr=arr_get(arrptr, -1);
fail_unless(itemptr==NULL, "Error: Non-null pointer for an item that doesn't exist in the array (before the array in memory)");
}
END_TEST

/*
START_TEST (test_arr_share_core)
{
ArrayPtr arrptr=NULL;
ArrayPtr arrptr_copy=NULL;
Array *arr=NULL;
char *elem1="5";
char *elem2="7";
char *elem3="9";

// Creating an array with one element
arrptr=arr_create(1,1, 2);

//Adding three element
arr_add(arrptr, (void *)elem1);
arr_add(arrptr, (void *)elem2);
arr_add(arrptr, (void *)elem3);


arrptr_copy=arr_share(arrptr);

//Testing second element
arr = (Array *)arrptr_copy;
fail_unless(memcmp(arr->eles+1, elem2, 1)==0, "Shared copy doesn't have same second element: %s", arr->eles+1);
}
END_TEST
*/

int comp(const void *s1, const void *s2)
{
return memcmp(s1,s2,1);
}

START_TEST (test_arr_search_core)
{
ArrayPtr arrptr=NULL;
ArrayItemPtr arritem_ptr=NULL;
char *elem1="5";
char *elem2="7";
char *elem3="9";
char *elem4="8";


// Creating an array with one element
arrptr=arr_create(1,1, 2);

//Adding three element
arr_add(arrptr, (void *)elem1);
arr_add(arrptr, (void *)elem2);
arr_add(arrptr, (void *)elem3);

arritem_ptr=arr_search(arrptr, (char *)elem2, comp);
fail_unless(arritem_ptr!=NULL, "Search failed");
fail_unless(memcmp(elem2, arritem_ptr, 1)==0, "Search failed: the content of the pointers isn't the same, elem2=%.1s, arritem_ptr=%.1s", elem2, arritem_ptr);
fail_unless(elem2!=arritem_ptr, "Error: The two pointers are the same");

arritem_ptr=arr_search(arrptr, (char *)elem4, comp);
fail_unless(arritem_ptr==NULL, "Error: Search found an item that does not exist");
}
END_TEST

Suite * array_suite (void)
{
  Suite *s = suite_create ("array");

/* The macro ADD_TEST_CASE(bufr_read_fn) does:
  TCase *tc_bufr_read_fn = tcase_create ("bufr_read_fn");
  tcase_add_test (tc_bufr_read_fn, test_bufr_read_fn_core);
  suite_add_tcase (s, tc_bufr_read_fn);
*/

  // No need to test, too simple: 
  // arr_allocate, arr_create, arr_del, arr_free, arr_inc, arr_reduce

  // arr_add
  ADD_TEST_CASE(arr_add)

  // arr_count
  ADD_TEST_CASE(arr_count)

  // arr_get
  ADD_TEST_CASE(arr_get)

  // arr_share - MVE: Voir l'exemple de Vanh dans le courriel
  //ADD_TEST_CASE(arr_share)

  // arr_search
  ADD_TEST_CASE(arr_search)

  return s;

}

