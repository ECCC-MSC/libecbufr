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
