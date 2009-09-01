#include <stdlib.h>
#include <stdio.h>
#include "check.h"
#include "bufr_io.c"
#include "check_libecbufr.h"


START_TEST (test_bufr_read_fn_core)
{
 FILE  *f;                 //File to read
 size_t tb_len=10;         //Length of test bytes
 char   test_bytes[tb_len];//Input bytes
 char   result[tb_len];    //bytes read
 int    return_code;
 char  *test_file="test_bufr_read_fn.txt";
 int    nb=6;             //number of cases
 size_t nb_cases[nb];     //Number of bytes to read
 int    fail_index=-1;    // When -1, it means it didn't fail.
 int    error=0;

 //String initializing
 strcpy(test_bytes, "String");
 test_bytes[6]='\0';
 test_bytes[7]='j';
 test_bytes[8]='Y';
 test_bytes[9]='#';
 
 //File creation
 f=fopen(test_file,"w");
 fwrite(test_bytes, 1, tb_len, f);
 fclose(f);

 //Testing

 nb_cases[0]=1; // Reading one character 
 nb_cases[1]=4; // Reading part of a File (4 bytes) 
 nb_cases[2]=tb_len;   // Reading the whole file 
 nb_cases[3]=tb_len+1; // Reading more than the whole file 
 nb_cases[4]=0; // Reading nothing 
 nb_cases[5]=-5;// Reading a negative quantity 

 for (int i=0; i<nb; i++)
    {
    f=fopen(test_file,"r");
    return_code = bufr_read_fn(f, nb_cases[i], result);

    error=ferror(f); //Checking if there was an error with the read.

    if (nb_cases[i] > tb_len)//When reading more than the whole file, 
       {                     //consider only the file length to verify
       nb_cases[i]=tb_len;   //if it was correctly read.
       }

    if (error)          // When there is an error, nothing was read.
       {
       nb_cases[i]=0;
       }

    fail_unless (return_code == nb_cases[i],"Read failed, return code is: %d, should be: %d", return_code, nb_cases[i]);

    // Comparing each bytes 
    for (int j=0; j<nb_cases[i]; j++)
       {
       if (result[j]!=test_bytes[j])
          {
          fail_index=j;
          break;
          }
       }

    fail_unless (fail_index == -1,"Byte read was: \"%c\", is supposed to be: \"%c\"", result[fail_index], test_bytes[fail_index]);
    fclose(f);
    }

 remove(test_file);
 
 
}
END_TEST

START_TEST (test_bufr_memread_fn_core)
{
 struct bufr_mem data[1];  //memory structure to read
 size_t tb_len=10;         //Length of test bytes
 char   test_bytes[tb_len];//Input bytes
 char   result[tb_len];    //bytes read
 int    return_code;
 int    nb=6;             //number of cases
 size_t nb_cases[nb];     //Number of bytes to read
 int    fail_index=-1;    // When -1, it means it didn't fail.
 int    error=0;

 //memory structure creation and initializing
 data[0].mem = (char *)malloc(sizeof(char)*tb_len);
 strcpy(data[0].mem, "String");
 data[0].mem[6]='\0';
 data[0].mem[7]='j';
 data[0].mem[8]='Y';
 data[0].mem[9]='#';

 data[0].max_len=tb_len;
 
 //Testing

 nb_cases[0]=1; // Reading one character 
 nb_cases[1]=4; // Reading part of a File (4 bytes) 
 nb_cases[2]=tb_len;   // Reading the whole file 
 nb_cases[3]=tb_len+1; // Reading more than the whole file 
 nb_cases[4]=0; // Reading nothing 
 nb_cases[5]=-5;// Reading a negative quantity 
 
 for (int i=0; i<nb; i++)
    {
    // Initializing data position in memory structure
    data[0].pos=0;

    return_code = bufr_memread_fn(data, nb_cases[i], result);

    if (nb_cases[i] > tb_len)//When reading more than the whole file, 
       {                     //consider only the file length to verify
       nb_cases[i]=tb_len;   //if it was correctly read.
       }

    fail_unless (return_code == nb_cases[i],"Read failed, return code is: %d, should be: %d", return_code, nb_cases[i]);

    // Comparing each bytes 
    for (int j=0; j<nb_cases[i]; j++)
       {
       if (result[j]!=data[0].mem[j])
          {
          fail_index=j;
          break;
          }
       }

    fail_unless (fail_index == -1,"Byte read was: \"%c\", is supposed to be: \"%c\", fail index: %d", result[fail_index], data[0].mem[fail_index], fail_index);
    }
 free(data[0].mem);
}
END_TEST

ssize_t mock_bufr_read_octet_callback(void *cd, size_t len, char *buffer)
   {
   ssize_t * value = (ssize_t *)cd;
   return value[0];
   }

START_TEST (test_bufr_read_octet_core)
{
int nb_cases=2;
ssize_t return_code[nb_cases];
unsigned char * nothing=NULL;
int real_return_code;

return_code[0]=2;  // Positive return code
return_code[1]=-2; // Negative return code
return_code[2]=0;  // Zero


// Testing that the return value of a mock function is propagated properlly
for (int i=0; i<nb_cases; i++)
   {
   real_return_code=bufr_read_octet(mock_bufr_read_octet_callback, (void *)(&(return_code[i])), nothing);
   fail_unless(return_code[i] == real_return_code, "return_code: %d, real_return_code:%d", return_code[0], real_return_code);
   }

}
END_TEST

/*START_TEST (test_bufr_seek_msg_start_core)
{

}
END_TEST
*/

Suite * bufr_io_suite (void)
{
  Suite *s = suite_create ("bufr_io");

  // bufr_read_fn
  TCase *tc_bufr_read_fn = tcase_create ("bufr_read_fn");
  tcase_add_test (tc_bufr_read_fn, test_bufr_read_fn_core);
  suite_add_tcase (s, tc_bufr_read_fn);

  // bufr_memread_fn
  TCase *tc_bufr_memread_fn = tcase_create ("bufr_memread_fn");
  tcase_add_test (tc_bufr_memread_fn, test_bufr_memread_fn_core);
  suite_add_tcase (s, tc_bufr_memread_fn);

  // bufr_read_octet
  TCase *tc_bufr_read_octet = tcase_create ("bufr_read_octet");
  tcase_add_test (tc_bufr_read_octet, test_bufr_read_octet_core);
  suite_add_tcase (s, tc_bufr_read_octet);

  return s;

}
