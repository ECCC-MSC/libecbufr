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
#include <string.h>
#include "check.h"
#include "bufr_util.h"
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
 char  dest_string[max_length];
 char *dest_null=NULL;
 char *final_string;
 const char *empty_string="";
 const char *spaces="    ";
 const char *no_spaces="1234";
 const char *long_string="abcdefghijklmnopqrstuvwxyz";
 const char *long_string2="abcdefgh                  ";
 const char *space_before="   abc";
 const char *space_both_ends=" abc ";


 //Test if source string is empty
 final_string=strimdup(dest_string, empty_string, max_length);

 fail_unless(strlen(final_string)==0, "Copying empty string failed, bad length: %d", strlen(final_string));
 fail_unless(strcmp(final_string,"")==0, "Copying empty string failed");

 //Test if source string is NULL
 final_string=strimdup(dest_string, NULL, max_length);
 fail_unless(final_string==NULL, "Copying NULL string failed");

 // Test if max_length is zero
 max_length=0;

 final_string=strimdup(dest_string, spaces, max_length);

 fail_unless(final_string==NULL, "Copying string with (max length=0) failed");
 
 //Test if src string is only spaces
 max_length=15;

 final_string=strimdup(dest_string, spaces, max_length);

 fail_unless(strlen(final_string)==0, "Copying empty string failed, bad length: %d", strlen(final_string));
 fail_unless(strcmp(final_string,"")==0, "Copying empty string failed");

 //Test if no trailing spaces
 final_string=strimdup(dest_string, no_spaces, max_length);

 fail_unless(strlen(final_string)==strlen(no_spaces), "Copying string with no spaces failed, bad length: %d", strlen(final_string));
 fail_unless(strcmp(final_string, no_spaces)==0, "Copying string with no spaces failed");

 //Test if source string is longer than maximum length
 final_string=strimdup(dest_string, long_string, max_length);
 fail_unless(strcmp(final_string, "abcdefghijklmn")==0, "Copy of string exceeding maximum length failed, dest_string:%s",final_string);
 
 //Test if source string is longer than maximum length with spaces, but not when the spaces are stripped.
 final_string=strimdup(dest_string, long_string2, max_length);
 fail_unless(strcmp(final_string, "abcdefgh")==0, "Copy of second long string failed, dest_string:%s", final_string);

 //Test if spaces are before string
 final_string=strimdup(dest_string,space_before, max_length);
 fail_unless(strcmp(final_string, space_before)==0, "Copy of string with spaces at the begining failed, dest_string:%s", final_string);

 //Test if spaces are at both ends of the string
 final_string=strimdup(dest_string, space_both_ends, max_length);
 fail_unless(strcmp(final_string, " abc")==0, "Copy of string with spaces at both ends failed, dest_string:%s", final_string); 

 //Test if destination string is NULL
 final_string=strimdup(dest_null, no_spaces, max_length);

 fail_unless(strlen(final_string)==strlen(no_spaces), "Copying string with no spaces failed, bad length: %d", strlen(final_string));
 fail_unless(strcmp(final_string, no_spaces)==0, "Copying string with no spaces failed");

}
END_TEST


START_TEST (test_append_char_to_string_core)
{
 int  size=6;
 char *string;
 int pos;
 unsigned char c='s';


 string=(char *)malloc( (size+1) * sizeof(char) );

 // This should add a letter at the end of the string.
 strcpy(string, "Test");
 pos=strlen(string);

 append_char_to_string(&string, &size, &pos, c);

 fail_unless(strncmp(string, "Tests",5)==0, "The first 5 letters of string: '%s', should be 'Tests'", string);

 // Appending characters to the begining of the line.  This should overwrite the first letter.
 pos=0;
 c='W';
 append_char_to_string(&string, &size, &pos, c);
 fail_unless(strncmp(string, "Wests", 5)==0, "The first 5 letters of string: '%s', should be 'Wests'", string);
 

 // This tests how the function works when appending letters past the original size. It should allocate more memory and
 // change size accordingly.
 pos=strlen(string);
 append_char_to_string(&string, &size, &pos, 'A');
 append_char_to_string(&string, &size, &pos, 'B');
 append_char_to_string(&string, &size, &pos, 'C');
 fail_unless(strncmp(string, "WestsABC", 8)==0, "The first 8 letters of string: '%s', should be 'WestsABC'", string);
 fail_unless(size > 6, "Error: size: %d\nSize should have grown since we appended letters past its original size of 6");

}
END_TEST


START_TEST (test_str_oct2char_core)
{
char *input="A\\102C";
//char *not_octal="A\\888C"; //unspecified when non octal symbols follow the slash (or x for hex, and the rest)
char *two_backslashes="A\\\\102C"; 
int len=0;
char *output;

//Test normal case
len=strlen(input);
output=str_oct2char(input, &len);

fail_unless(strcmp(output,"ABC")==0, "Octal conversion failed, output:%s", output);

/*
//Test when not an octal
len=strlen(not_octal);
output=str_oct2char(not_octal, &len);
fail_unless(strcmp(output,"A888C")==0, "Conversion of normal string failed, output:%s", output);
*/

//Test when two backslashes
len=strlen(two_backslashes);
output=str_oct2char(two_backslashes, &len);

fail_unless(strcmp(output,"A\\102C")==0, "Two backslashes octal conversion failed, output:%s", output);


}
END_TEST

START_TEST (test_str_schar2oct_core)
{
char input[10];
int len=0;
int bsize=10;
char *output=NULL;

strcpy(input, "A B\\t");
len=strlen(input);
output=str_schar2oct(input, &len, &bsize); 
fail_unless(strcmp(output, "A\\040B\\t")==0, "Octal conversion failed, output:%s", output);
}
END_TEST


Suite * bufr_util_suite (void)
{
  Suite *s = suite_create ("bufr_util");

  // strimdup
  TCase *tc_strimdup = tcase_create ("strimdup");
  tcase_add_test (tc_strimdup, test_strimdup_core);
  tcase_add_test (tc_strimdup, test_strimdup_limits);
  suite_add_tcase (s, tc_strimdup);


  // append_char_to_string 
  TCase *tc_append_char_to_string = tcase_create ("append_char_to_string");
  tcase_add_test (tc_append_char_to_string, test_append_char_to_string_core);
  suite_add_tcase (s, tc_append_char_to_string);

  // str_oct2char
  TCase *tc_str_oct2char = tcase_create ("str_oct2char");
  tcase_add_test (tc_str_oct2char, test_str_oct2char_core);
  suite_add_tcase (s, tc_str_oct2char);

  // str_schar2oct
  TCase *tc_str_schar2oct = tcase_create ("str_schar2oct");
  tcase_add_test (tc_str_schar2oct, test_str_schar2oct_core);
  suite_add_tcase (s, tc_str_schar2oct);

  return s;
}


