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
fail_unless(memcmp(elem1,arr->eles,1)==0, "Copy of the first element failed: character in array: %s", arr->eles);
fail_unless(nb_elem==1, "Bad number of elements: %d", nb_elem);
fail_unless(arr->total==1, "Bad array size: %d", arr->total);

//Adding second element - A growth of 2 in total size of the array should occur
nb_elem=arr_add(arrptr, (void *)elem2);
fail_unless(memcmp(elem2,arr->eles+1,1)==0, "Copy of the first element failed: character in array: %s", arr->eles+1);
fail_unless(nb_elem==2, "Bad number of elements: %d", nb_elem);
fail_unless(arr->total==3, "Bad array size: %d", arr->total);

//Adding third element
nb_elem=arr_add(arrptr, (void *)elem3);
fail_unless(memcmp(elem3,arr->eles+2,1)==0, "Copy of the first element failed: character in array: %s", arr->eles+2);
fail_unless(nb_elem==3, "Bad number of elements: %d", nb_elem);
fail_unless(arr->total==3, "Bad array size: %d", arr->total);
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
  // arr_allocate

  // arr_add
  ADD_TEST_CASE(arr_add)

  return s;

}
