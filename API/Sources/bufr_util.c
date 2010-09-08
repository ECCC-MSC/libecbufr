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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "private/bufr_util.h"

/**
 * @english
 * @brief make copy of string without trailing space
 *
 * copy a string a crop it in order to remove any trailing space 
 *
 * @param dest destination string 
 * @param src source string
 * @param maxlen max size of dest
 * @return a copy of the string
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @ingroup internal
 */
char *strimdup ( char *dest, const char *src, int maxlen )
   {
   int   k, len;

   if (src == NULL) return NULL;
   if (maxlen <= 0) return NULL;

   len = strlen( src );
   for (k = len-1; k >= 0; k-- )
      if (!isspace( src[k] )) break;
   len = k+1;
   if (dest == NULL)
      {
      dest = (char *)malloc( (len+1) * sizeof(char) );
      maxlen = len+1;
      }

   if (len >= maxlen)
      len = maxlen - 1;

   strncpy( dest, src, len );
   dest[len] = '\0';
   return dest;
   }

/**
 * @english
 * @brief append a character to a string
 *
 * append a character to a string, memory allocation is resized whenever necessary
 *
 * @param str destination string 
 * @param size current buffer size
 * @param pos current position of last character in buffer
 * @param c character to append at the end of buffer
 * @return none
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @ingroup internal
 */
void append_char_to_string
   ( char **str, int *size, int *pos, unsigned char c )
   {
   int  i;

   i = *pos;
   if (i >= *size)
      {
      int newsize = *size + 64;
      *str = (char *)realloc( *str, newsize * sizeof(char) );
      *size = newsize;
      }
   (*str)[i] = (char)c;
   *pos = i + 1;
   }

/**
 * @english
 * @brief remove printed octal char from string
 *
 * Turn all printed octal form characters into single character in a string
 *
 * @param str0 original source string 
 * @param len reference to number of bytes in input string and also resulting string length
 * @return a copy of converted string 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @ingroup internal
 */
char *str_oct2char( char *str0, int *len )
   {
   char  *str;
   int   i, k;
   char buf[4]; 
   unsigned int c;

   str = (char *)malloc( (*len+1) * sizeof(char) );
   strncpy( str, str0, *len );

   i = 0;
   for ( k = 0; k < *len ; k++)
      {
      if (str[k] == '\\')
         {
         if (str[k+1] == '\\') 
            {
            str[i++] = str[k];
            k += 1;
            }
         else if (str[k+1] == 'n') 
            {
            str[i++] = '\012';
            k += 1;
            }
         else
            {
            strncpy( buf, str+k+1, 3 );
            buf[3] = '\0';
            sscanf( buf, "%o", &c );
            str[i++] = c;

            k += 3;
            }
         }
      else
         {
         str[i++] = str[k];
         }
      }
   str[i] = '\0';
   *len = i;
   return str;
   }

/**
 * @english
 * @brief convert special characters into printed octal char in string
 *
 * Turn special characters like spaces and control characters to printed octal form \xxx in a string
 *
 * @param str original source string 
 * @param len reference to number of bytes in input string and also resulting string length
 * @param bsize buffer size to hold string
 * @return a copy of converted string 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @ingroup internal
 */
char *str_schar2oct( char *str, int *len, int *bsize )
   {
   int    i, j, k, l;
   char   buf[4];
   char  *str2, *tagstr;

   i = *len;
   str2 = (char *)malloc( (*bsize+1) * sizeof(char) );
   str[i] = '\0';
   l = 0;

   for (j = 0; j < i ; j++ )
      {
      if (isspace(str[j])||iscntrl(str[j])||(str[j]=='\0'))
         {
         sprintf( buf, "%.3o", str[j] );
         append_char_to_string( &str2, bsize, &l, '\\' );
         append_char_to_string( &str2, bsize, &l, buf[0] );
         append_char_to_string( &str2, bsize, &l, buf[1] );
         append_char_to_string( &str2, bsize, &l, buf[2] );
         }
      else
         append_char_to_string( &str2, bsize, &l, str[j] );
      }
   tagstr = (char *)malloc( (l+1)*sizeof(char) );
   strncpy( tagstr, str2, l );
   tagstr[l] = '\0';
   free( str2 );
   *len = l;
   return tagstr;
   }

/**
 * @english
 * @brief trim trailing characters from string of the same char
 *
 * cut away all characters that match character c starting from end of string
 *
 * @param str original source string 
 * @param c character to remove
 * @return none
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @ingroup internal
 */
void str_trimchar( char *str, char c )
   {
   int  len;

   len = strlen( str ) - 1;
   while (str[len] == c)
      --len;
   if ( str[len] == '.')
      str[len] = '\0';
   else
      str[len+1] = '\0';
   }

