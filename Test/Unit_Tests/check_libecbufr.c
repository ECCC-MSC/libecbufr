#include <stdlib.h>
#include <stdio.h>
#include "check.h"
#include "check_libecbufr.h"


int main (void)
{
  int number_failed;

  SRunner *sr = srunner_create (str_util_suite ());
  srunner_add_suite(sr, bufr_io_suite ());
  srunner_add_suite(sr, array_suite ());

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
