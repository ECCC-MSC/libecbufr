#include <stdlib.h>
#include <stdio.h>
#include "check.h"
#include "private/bufr_util.h"
#include "check_libecbufr.h"


START_TEST (test_strimdup_core)
{
 const char *src_string="Test string   ";
 int   max_length=15;
 char  dest_string[max_length];
   
 strimdup(dest_string, src_string, max_length);

 fail_unless(strlen(dest_string)==strlen("Test string"), "Incorrect string length, dest_string: %d, src_string: %d", strlen(dest_string), strlen("Test string"));
 fail_unless(strcmp(dest_string, "Test string")==0, "copy wasn\'t permormed correctly");

}
END_TEST


START_TEST (test_strimdup_limits)
{
 // Test if src string is null
 int   max_length=10;
 char  src_string[max_length];
 char  dest_string[max_length];
 const char *empty_string="";
 const char *spaces="    ";
 const char *no_spaces="1234";


 strimdup(dest_string, empty_string, max_length);

 fail_unless(strlen(dest_string)==0, "Copying empty string failed, bad length: %d", strlen(dest_string));
 fail_unless(strcmp(dest_string,"")==0, "Copying empty string failed");

 // Test if max_length is zero
 max_length=0;

 strimdup(dest_string, spaces, max_length);

 fail_unless(strlen(dest_string)==0, "Copying empty string failed, bad length: %d", strlen(dest_string));
 fail_unless(strcmp(dest_string,"")==0, "Copying empty string failed");
 
 //Test if src string is only spaces
 max_length=15;

 strimdup(dest_string, spaces, max_length);

 fail_unless(strlen(dest_string)==0, "Copying empty string failed, bad length: %d", strlen(dest_string));
 fail_unless(strcmp(dest_string,"")==0, "Copying empty string failed");

 //Test if no trailing spaces
 strimdup(dest_string, no_spaces, max_length);

 fail_unless(strlen(dest_string)==strlen(no_spaces), "Copying string with no spaces failed, bad length: %d", strlen(dest_string));
 fail_unless(strcmp(dest_string, no_spaces)==0, "Copying string with no spaces failed");

}
END_TEST

/*START_TEST (test_append_char_to_string_core)
{

}
END_TEST

START_TEST (test_append_char_to_string_limits)
{

}
END_TEST

START_TEST (test_str_oct2char_core)
{

}
END_TEST

START_TEST (test_str_schar2oct_core)
{

}
END_TEST
*/

Suite * str_util_suite (void)
{
  Suite *s = suite_create ("str_util");

  // strimdup
  TCase *tc_strimdup = tcase_create ("strimdup");
  tcase_add_test (tc_strimdup, test_strimdup_core);
  tcase_add_test (tc_strimdup, test_strimdup_limits);
  suite_add_tcase (s, tc_strimdup);

  // append_char_to_string 
/*  TCase *tc_append_char_to_string = tcase_create ("append_char_to_string");
  tcase_add_test (tc_append_char_to_string, test_append_char_to_string_core);
  tcase_add_test (tc_append_char_to_string, test_append_char_to_string_limits);
  suite_add_tcase (s, tc_append_char_to_string);
*/
  // str_oct2char
/*  TCase *tc_str_oct2char = tcase_create ("str_oct2char");
  tcase_add_test (tc_str_oct2char, test_str_oct2char_core);
  suite_add_tcase (s, tc_str_oct2char);
*/
  // str_schar2oct
/*  TCase *tc_str_schar2oct = tcase_create ("str_schar2oct");
  tcase_add_test (tc_str_schar2oct, test_str_schar2oct_core);
  suite_add_tcase (s, tc_str_schar2oct);
*/
  return s;
}


