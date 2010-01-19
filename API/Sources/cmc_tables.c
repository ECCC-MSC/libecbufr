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
***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bufr_tables.h"

/**
 * @english
 *    bufr_load_cmc_tables( file_tables )
 *    (BUFR_Tables *tables)
 * This will load the CMC master Table B and D into the BUFR tables object
 * that is created by bufr_create_tables. Table B and D should be located
 * in the directory pointed by the environment variable "BUFR_TABLES"
 * and the tables should be in the CMC table format.
 * @warning This filename format is only related to the function
 * bufr_load_cmc_tables. This also uses an environment variable
 * "AFSISIO" which points to an operational directory that contains a
 * subdirectory "datafiles/constants" (caution: will not work in
 * Windows) beneath it where the Table D and Table B files reside. Caution:
 * The names of the tables should be in the format "table_b_bufr" and
 * "table_d_bufr". This does not support the current WinIDE, Met
 * manager, Codecon, NCP practice of TABB003.037 type names that allow for
 * concurrent versions to be in the directory that are versioned and also
 * permit the use of the DOS 8.3 filename format.
 * @return int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 * @todo should AFSISIO be supported as an env variable?
 */
int bufr_load_cmc_tables( BUFR_Tables *tables )
   {
   char *env;
   char  filename[512];
   char  path[512];
   int   rtrnB, rtrnD;

   if (tables == NULL) return -1;

   env = getenv( "AFSISIO" );
   if (env) 
      {
      sprintf( path, "%s/datafiles/constants", env );
      }
   else 
      {
      env = getenv( "BUFR_TABLES" );
      if (env) 
         sprintf( path, "%s", env );
      }

   if (env == NULL) return -1;

   sprintf( filename, "%s/table_b_bufr", path );

   rtrnB = bufr_load_m_tableB( tables, filename );

   sprintf( filename, "%s/table_d_bufr", path );
   rtrnD = bufr_load_m_tableD( tables, filename );

   return ( (rtrnD >= 0) && (rtrnB >= 0 ));
   }

