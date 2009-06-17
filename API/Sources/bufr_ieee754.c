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
#include <math.h> 
#include <limits.h> 
#include <inttypes.h>

#include "bufr_io.h"
#include "bufr_ieee754.h"

#define  USE_C_IEEE754_LAYOUT   0

#define  SIGN_BIT_32     0x80000000
#define  EXPON_BITS_32   0x7f800000
#define  FRACT_BITS_32   0x007fffff
#define  FRACT_NBITS_32  23
#define  EXPON_SHIFT_32  FRACT_NBITS_32
#define  EXPON_BIAS_32   127
#define  SIGN_SHIFT_32   31
#define  MAX_BITS_32     0x7f7fffff

#ifndef __UINT64_C
# if __WORDSIZE == 64
#  define __UINT64_C(c) c ## UL
# else
#  define __UINT64_C(c) c ## ULL
# endif
#endif

#ifndef FP_ZERO
#define FP_ZERO   1
#endif

#define  SIGN_BIT_64     (__UINT64_C(0x8000000000000000))
#define  EXPON_BITS_64   (__UINT64_C(0x7ff0000000000000))
#define  FRACT_BITS_64   (__UINT64_C(0x000fffffffffffff))
#define  MAX_BITS_64     (__UINT64_C(0x7fefffffffffffff))
#define  FRACT_NBITS_64  52
#define  EXPON_SHIFT_64  FRACT_NBITS_64
#define  EXPON_BIAS_64   1023
#define  SIGN_SHIFT_64   63

#define  FRACTION_NBMAX  53
#define  BASE2_NBMAX     64


#define  DEBUG           0

static int      initted_lim=0;

static int      C_use_ieee754=0;
static double   fractions2[FRACTION_NBMAX];
static float    max_float=0.0;
static double   max_double=0.0;


       void    bufr_init_limits(void);

static int     check_C_ieee754_compliance  ( void );
static int     check_type_size             ( void );
static void    check_word_size             ( char *string, int size, int expected, int *failed );
static int     check_sign_bit              ( void );
static int     check_single_mem_layout     ( void );
static int     check_double_mem_layout     ( void );
static int     test_decoding_double        ( double dval );
static int     test_decoding_single        ( float fval );
static int     test_encoding_single        ( float fval );
static int     test_encoding_double        ( double fval );
static void    init_numbers                ( void );

static int32_t bufr_single_get_significand ( float fvalue, int32_t *exponent, int *denormal );
static int64_t bufr_double_get_significand ( double fvalue, int64_t *exponent, int *denormal );
static double  bufr_get_significand_value  ( uint64_t fraction, int nbits, int denormal );

/**
 * @english
 * decode a double IEEE 754 64 bits into a double
 * @param    lval : bits of double prec. value
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
double bufr_ieee_decode_double( uint64_t lval )
   {
   uint64_t signific;
   int64_t  exponent;
   double   signif;
   int      sign;
   double   dval;
   int      denormal=0;

   if (C_use_ieee754)
      {
      double *ptr;

      ptr = (double *)&lval;
      return *ptr;
      }

   sign = (lval & SIGN_BIT_64) ? -1 : 1;
   exponent = ((lval & EXPON_BITS_64) >> EXPON_SHIFT_64);
   signific = lval & FRACT_BITS_64;

   if (exponent == 0)
      {
      if (signific == 0) 
         return sign * 0.0;
      else
         denormal = 1;
      }
   else if (exponent == 0x7ff)
      {
      if (signific == 0) 
         return sign * HUGE_VALL;
      else
         return nan("char-sequence");
      }

   if (denormal)
      exponent = -1022;
   else
      exponent -= EXPON_BIAS_64;

   signif = bufr_get_significand_value( signific, FRACT_NBITS_64, denormal );
   dval = sign * signif * pow( 2, exponent );

#if DEBUG
   if (bufr_is_debug())
      {
      char errmsg[256];

      sprintf( errmsg, "### Decoded double: %llx ->  %E  (sign=%d exponent=%llx signific=%llx)\n", 
               lval, dval, (sign<0)?1:0, exponent, signific );
      bufr_print_debug( errmsg );
      }
#endif
   return dval;
   }

/**
 * @english
 * decode a single IEEE 754 32 bits into a float
 * @param    ival : bits of single prec. value
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
float bufr_ieee_decode_single( uint32_t ival )
   {
   uint32_t signific;
   int32_t  exponent;
   double   signif;
   float    sign;
   float    fval;
   int      denormal=0;

   if (C_use_ieee754)
      {
      float *ptr;

      ptr = (float *)&ival;
      return *ptr;
      }

   sign = (ival & SIGN_BIT_32) ? -1.0 : 1.0;
   exponent = ((ival & EXPON_BITS_32) >> EXPON_SHIFT_32);
   signific = ival & FRACT_BITS_32;

   if (exponent == 0)
      {
      if (signific == 0) 
         return sign * 0.0;
      else
         denormal = 1;
      }
   else if (exponent == 0xff)
      {
      if (signific == 0) 
         return sign * HUGE_VALF;
      else
         return nanf("char-sequence");
      }

   if (denormal)
      exponent = -126;
   else
      exponent -= EXPON_BIAS_32;

   signif = bufr_get_significand_value( signific, FRACT_NBITS_32, denormal );
   fval = sign * signif * powf( 2.0, (float)exponent );

#if DEBUG
   if (bufr_is_debug())
      {
      char errmsg[256];

      sprintf( errmsg, "### Decoded single: %x ->  %E  (sign=%d exponent=%x signific=%x)\n", 
         ival, fval, (sign<0)?1:0, exponent, signific );
      bufr_print_debug( errmsg );
      }
#endif
   return fval;
   }

/**
 * @english
 * encode a single into a IEEE 754 32 bits
 * @param    fvalue : single prec. value
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
uint32_t bufr_ieee_encode_single ( float fvalue )
   {
   int32_t  exponent;
   int      sign;
   uint32_t ival, ifract;
   int      denormal=0;

   if (C_use_ieee754)
      {
      uint32_t *ptr;

      ptr = (uint32_t *)&fvalue;
      return *ptr;
      }
/*
 * special cases

*/
   if (isnan(fvalue))
      {
      ival = EXPON_BITS_32 | (1ULL << (FRACT_NBITS_32-1)) ;
      return ival;
      }
   else if (isinf(fvalue))
      {
      ival = EXPON_BITS_32;
      if (fvalue < 0) ival |= SIGN_BIT_32;
      return ival;
      }
   else if (fpclassify (fvalue) == FP_ZERO)
      {
      ival = 0;
      if (signbit( fvalue ))
         ival |= SIGN_BIT_32;
      return ival;
      }

   if (fvalue < 0)
      {
      sign = 1;
      fvalue = fvalue * -1;
      }
   else
      sign = 0;

   ifract = bufr_single_get_significand( fvalue, &exponent, &denormal );
   if (denormal)
      {
      exponent = exponent + 126;
      ival = ifract;
      }
   else
      {
      exponent = exponent + 127;
      ival = (exponent << EXPON_SHIFT_32) | ifract;
      }
   if (sign) ival |= SIGN_BIT_32;
   return ival;
   }


/**
 * @english
 * encode a double into a IEEE 754 64 bits
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
uint64_t bufr_ieee_encode_double( double fvalue )
   {
   int64_t  exponent;
   int      sign;
   uint64_t ival, ifract;
   int      denormal=0;

   if (C_use_ieee754)
      {
      uint64_t *ptr;

      ptr = (uint64_t *)&fvalue;
      return *ptr;
      }

/*
 * special cases

*/
   if (isnan(fvalue))
      {
      ival = EXPON_BITS_64 | (1ULL << (FRACT_NBITS_64-1)) ;
      return ival;
      }
   else if (isinf(fvalue))
      {
      ival = EXPON_BITS_64;
      if (fvalue < 0) ival |= SIGN_BIT_64;
      return ival;
      }
   else if (fpclassify (fvalue) == FP_ZERO)
      {
      ival = 0;
      if (signbit( fvalue ))
         ival |= SIGN_BIT_64;
      return ival;
      }

   if (fvalue < 0)
      {
      sign = 1;
      fvalue = fvalue * -1;
      }
   else
      sign = 0;

   ifract = bufr_double_get_significand( fvalue, &exponent, &denormal );
   if (denormal)
      {
      exponent = exponent + EXPON_BIAS_64 - 1;
      ival = (exponent << EXPON_SHIFT_64) | ifract;
      }
   else
      {
      exponent = exponent + EXPON_BIAS_64;
      ival = (exponent << EXPON_SHIFT_64) | ifract;
      }
   if (sign) 
      ival |= SIGN_BIT_64;
   return ival;
   }

/**
 * @english
 * return maximum value of a ieee float 32 bits
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
float bufr_get_max_float(void)
   {
   if (initted_lim == 0)
      bufr_init_limits();

   return max_float; 
   }

/**
 * @english
 * return maximum value of a ieee float 64 bits
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
double bufr_get_max_double(void)
   {
   if (initted_lim == 0)
      bufr_init_limits();

   return max_double; 
   }

/**
 * @english
 * initialize a base 2 fractions table
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void init_numbers(void)
   {
   static int initted_nbrs=0;
   int    i;

   if (initted_nbrs) return;

   initted_nbrs = 1;

   for (i = 0; i < FRACTION_NBMAX ; i++)
      {
      fractions2[i] = 1.0 / pow( 2, i );
      }
   }

/**
 * @english
 *    bufr_init_limits()
 *    (void)
 * This call initializes the maximum value of both float and of double
 * precision values.
 * @warning This function may be reviewed.
 * @warning Not thread-safe
 * @return void
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void bufr_init_limits(void)
   {

   if (initted_lim) return;

   initted_lim = 1;

   max_float = bufr_ieee_decode_single ( MAX_BITS_32 );
   max_double = bufr_ieee_decode_double( MAX_BITS_64 );

#if 0
   inf_float = bufr_ieee_decode_single ( EXPON_BITS_32 );
   inf_double = bufr_ieee_decode_double( EXPON_BITS_64 );
#endif
   }

/**
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int check_match_encoding2decoding(void)
   {
   uint32_t   i32;
   uint64_t   i64;
   float      flmax;
   double     dmax;
   int        error=0;
   char       errmsg[256];

   bufr_init_limits();

   flmax = bufr_get_max_float();
   i32 = bufr_ieee_encode_single( flmax );
   if (i32 != MAX_BITS_32)
      {
      sprintf( errmsg, "Warning: IEEE 754 encoding/decoding mismatch F32: %x -> %E -> %x\n",
            MAX_BITS_32, flmax, i32 );
      bufr_print_debug( errmsg );
      error = -1;
      }
#if DEBUG
   else if (bufr_is_debug())
      {
      sprintf( errmsg, "### IEEE 754 encoding/decoding matched F32: %x -> %E -> %x\n",
            MAX_BITS_32, flmax, i32 );
      bufr_print_debug( errmsg );
      }
#endif

   dmax = bufr_get_max_double();
   i64 = bufr_ieee_encode_double( dmax );
   if (i64 != MAX_BITS_64)
      {
      sprintf( errmsg, "Warning: IEEE 754 encoding/decoding mismatch F64: %llx -> %E -> %llx\n",
            MAX_BITS_64, dmax, i64 );
      bufr_print_debug( errmsg );
      error = -1;
      }
#if DEBUG
   else if (bufr_is_debug())
      {
      sprintf( errmsg, "### IEEE 754 encoding/decoding matched F64: %llx -> %E -> %llx\n",
            MAX_BITS_64, dmax, i64 );
      bufr_print_debug( errmsg );
      }
#endif
   return error;
   }

/**
 * check_word_size
 * @english
 * test if the size of a word correspond to expected size
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void check_word_size(char *string, int size, int expected, int *failed)
   {
   char errmsg[128];
   int  debug=bufr_is_debug();

   if (debug)
      {
      sprintf( errmsg, "### Checking: %s size is %d bytes:", string, size );
      bufr_print_debug( errmsg );
      }

   if (size == expected)
      {
      if (debug) 
         bufr_print_debug( "Ok\n" );
      }
   else
      {
      if (debug) 
         {
         sprintf( errmsg, "Failed: should be %d bytes\n", expected );
         bufr_print_debug( errmsg );
         }
      *failed = 1;
      }
   }

/**
 * @english
 * see if the size of base types  float double uint32_t uinit64_t are those of
 *           a 32 bits 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int check_type_size(void)
   {
   int size;
   int failed=0;
   /*
    * see if we are on a 32 bits machines
    */
   size = sizeof(float);
   check_word_size( " FLOAT", size, 4, &failed );
   size = sizeof(uint32_t);
   check_word_size( "   INT", size, 4, &failed );
   size = sizeof(double);
   check_word_size( "DOUBLE", size, 8, &failed );
   size = sizeof(uint64_t);
   check_word_size( "  LONG", size, 8, &failed );
   return (failed ? 0 : 1);
   }


/**
 * @english
 * see if sign bit of float and double are compliant with IEEE 754
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int check_sign_bit(void)
   {
   float fval;
   double  dval;
   uint32_t *ptrInt;
   uint32_t ival;
   uint64_t *ptrLong, lval;

   ptrInt = (uint32_t *) &fval;
   ptrLong = (uint64_t *) &dval;

   fval = -1.0;
   ival = *ptrInt;
   if ((ival & SIGN_BIT_32)==0)
      {
      bufr_print_debug( "Failed: sign bit is not 1 on negative float value\n" );
      return -1;
      }

   fval = 1.0;
   ival = *ptrInt;
   if ((ival & SIGN_BIT_32)!=0)
      {
      bufr_print_debug( "Failed: sign bit is not 0 on positive float value\n" );
      return -1;
      }

   dval = -1.0;
   lval = *ptrLong;
   if ((lval & SIGN_BIT_64)==0)
      {
      bufr_print_debug( "Failed: sign bit is not 1 on double negative value\n" );
      return 0;
      }

   dval = 1.0;
   lval = *ptrLong;
   if ((lval & SIGN_BIT_64)!=0)
      {
      bufr_print_debug( "Failed: sign bit is not 0 on double positive value\n" );
      return 0;
      }

   return 1;
   }

/**
 * @english
 * return fraction value of a significand
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static double bufr_get_significand_value(uint64_t fraction, int nbits, int denormal)
   {
   int  i;
   double  s;
   uint64_t mask;

   init_numbers();

   s = denormal ? 0.0 : fractions2[0];
   for (i = 1; i <= nbits; i++ )
      {
      mask = 1ULL<<(nbits-i);
      if (fraction & mask)
         {
         s += fractions2[i];
         }
      }
   return s;
   }

/**
 * @english
 * extract significand bits of a single (32 bits)
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int32_t bufr_single_get_significand ( float fvalue, int32_t *exponent, int *denormal )
   {
   uint32_t ival;
   int      n, nb;
   int      rem;
   int      ni0;
   int      nbits = FRACT_NBITS_32;
   double   dvalue = fvalue;
   int      expon;

   *denormal = 0;
   expon = logf(fvalue)/logf(2.0);
   if (expon < -126) expon = -126;
   if (expon > 127) expon = 127;
   fvalue = fvalue / pow( 2.0, expon );

   ival = (uint32_t)fvalue;
   nb = (ival > 0 ) ? bufr_leftest_bit( ival ) : 0;

   rem = (nb > 0) ? nbits - nb + 1 : nbits + 1;
   dvalue = fvalue - ival;
   ni0 = n = 0;
   while ((dvalue > 0)&&(rem > 0))
      {
      ++n;
      dvalue = dvalue * 2;
      if (dvalue >= 1.0)
         {
         ival = (ival << 1) | 1;
         dvalue -= 1.0;
         if (ni0 == 0)
            {
            ni0 = n;
            }
         }
      else
         ival = ival << 1;
      if ((ni0 > 0)||(nb > 0)) rem -= 1;
      }
   if (nb > 0)
      {
      *exponent = expon + nb - 1;
      if (rem > 0)
         ival = ( ival << rem ) & FRACT_BITS_32;
      else
         ival = ival & FRACT_BITS_32;
      }
   else
      {
      if (expon == -126)
         {
         *exponent = expon;
         if (rem > 1)
            ival = ( ival << (rem-1) ) & FRACT_BITS_32;
         *denormal = 1;
         }
      else
         {
         *exponent = expon - ni0;
         ival = ( ival << rem ) & FRACT_BITS_32;
         }
      }
   return ival;
   }


/**
 * @english
 * extract significand bits of a double (64 bits)
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int64_t bufr_double_get_significand ( double fvalue, int64_t *exponent, int *denormal )
   {
   uint64_t ival;
   int      n, nb;
   int      rem;
   int      ni0;
   int      nbits = FRACT_NBITS_64;
   double   dvalue = fvalue;
   int      expon;

   *denormal = 0;
   expon = log(fvalue)/log(2.0);
   if (expon < -1022) expon = -1022;
   if (expon > 1023) expon = 1023;
   fvalue = fvalue / pow( 2.0, expon );

   ival = (uint64_t)fvalue;
   nb = (ival > 0 ) ? bufr_leftest_bit( ival ) : 0;

   rem = (nb > 0) ? nbits - nb + 1 : nbits + 1;
   dvalue = fvalue - ival;
   ni0 = n = 0;
   while ((dvalue > 0)&&(rem > 0))
      {
      ++n;
      dvalue = dvalue * 2;
      if (dvalue >= 1.0)
         {
         ival = (ival << 1) | 1;
         dvalue -= 1.0;
         if (ni0 == 0)
            {
            ni0 = n;
            }
         }
      else
         ival = ival << 1;
      if ((ni0 > 0)||(nb > 0)) rem -= 1;
      }
   if (nb > 0)
      {
      *exponent = expon + nb - 1;
      if (rem > 0)
         ival = ( ival << rem ) & FRACT_BITS_64;
      else
         ival = ival & FRACT_BITS_64;
      }
   else
      {
      if (expon == -1022)
         {
         *exponent = expon;
         if (rem > 1)
            ival = ( ival << (rem-1) ) & FRACT_BITS_64;
         *denormal = 1;
         }
      else
         {
         *exponent = expon - ni0;
         ival = ( ival << rem ) & FRACT_BITS_64;
         }
      }
   return ival;
   }

/**
 * @english
 * check if C float and double follows IEEE 754, if not, 
 *           we will need to encode and decode on our own
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int bufr_use_C_ieee754(int use)
   {
   static int checked=0;

   if (checked == 0)
      {
      if ( check_C_ieee754_compliance() ) 
         checked = 1;
      else
         checked = -1;
      }

   C_use_ieee754 = ((checked > 0) && use) ? 1 : 0 ;

   return C_use_ieee754;
   }

/**********************************************************************************************/

/**
 * @english
 * test a few values to see if float are encoded as IEEE 754 
 *           by decoding them
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int check_C_ieee754_compliance(void)
   {
   if (bufr_is_debug())
      bufr_print_debug( "### Checking Memory Layout of float and double for IEEE 754 ...\n" );

   if (!check_type_size()) return 0;
   if (check_sign_bit() < 0) return 0;
   if (check_single_mem_layout() < 0) return 0;
   if (check_double_mem_layout() < 0) return 0;

   if (check_match_encoding2decoding() < 0) return 0;

   if (bufr_is_debug())
      bufr_print_debug( "### Checked: good, C float and double use IEEE 754\n" );

   return 1;
   }

/**
 * @english
 * test a few values to see if float are encoded as IEEE 754 
 *           by decoding them
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int check_single_mem_layout(void)
   {
   int rtrn = 0;
   int  i;
   float  values[8] = { 0.0, 5.9E-39, 3.4E38, 0.0, 1.0, 0.15625, 1.18E-38, -750.15625 };

   values[0] = nanf("char-sequence");

   for (i = 0; i < 8 ; i++)
      {
      if (test_decoding_single( values[i] ) < 0) rtrn = -1;
      if (test_encoding_single( values[i] ) < 0) rtrn = -1;
      }

   return rtrn;
   }

/**
 * @english
 * test a few values to see if double are encoded as IEEE 754 
 *           by decoding them
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int check_double_mem_layout(void)
   {
   int rtrn = 0;
   int  i;
   double values[8] = { 0.0, 5.9E-39, 3.4E38, 0.0, 1.0, 0.15625, 1.18E-38, -750.15625 };

   values[0] = nan("char-sequence");
   for (i = 0; i < 8 ; i++)
      {
      if (test_decoding_double( values[i] ) < 0) rtrn = -1;
      if (test_encoding_double( values[i] ) < 0) rtrn = -1;
      }

   return rtrn;
   }

/**
 * @english
 * test if a value of a double is encoded as IEEE 754 
 *           by decoding it and compare the values
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int test_decoding_double(double dval)
   {
   double    dval2;
   uint64_t *ptrLong;
   uint64_t  lval;
   char      errmsg[128];

   ptrLong = (uint64_t *) &dval;
   lval = *ptrLong;
   dval2 = bufr_ieee_decode_double( lval );
   if ((dval2 == dval)||(isnan(dval2)&&isnan(dval)))
      {
#if DEBUG
      if (bufr_is_debug())
         {
         sprintf( errmsg, "### Decoding double OK for value %E\n", dval );
         bufr_print_debug( errmsg );
         }
#endif
      return 1;
      }
   else
      {
      sprintf( errmsg, "Warning: decoding of double differ with value %E != %E\n", 
               dval, dval2 );
      bufr_print_debug( errmsg );
      return -1;
      }
   }

/**
 * @english
 * test if a value of a double is encoded as IEEE 754 
 *           by decoding it and compare the values
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int test_decoding_single(float fval)
   {
   float     fval2;
   uint32_t *ptrInt;
   uint32_t  ival;
   char      errmsg[128];

   ptrInt = (uint32_t *) &fval;

   ival = *ptrInt;
   fval2 = bufr_ieee_decode_single( ival );
   if ((fval2 == fval)||(isnan(fval2)&&isnan(fval)))
      {
#if DEBUG
      if (bufr_is_debug())
         {
         sprintf( errmsg, "### Decoding float OK for value %E\n", fval );
         bufr_print_debug( errmsg );
         }
#endif
      return 1;
      }
   else
      {
      sprintf( errmsg, "Warning: decoding of float differ with value %E != %E\n", 
               fval, fval2 );
      bufr_print_debug( errmsg );
      return -1;
      }
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int test_encoding_single(float fval)
   {
   float     fval2, *ptr;
   uint32_t  ival;
   char      errmsg[256];

   ival = bufr_ieee_encode_single( fval );
   ptr = (float *) &ival;
   fval2 = *ptr;
   if ((fval2 == fval)||(isnan(fval2) && isnan(fval)))
      {
#if DEBUG
      if (bufr_is_debug())
         {
         sprintf( errmsg, "### Encoding float OK for value %E\n", fval );
         bufr_print_debug( errmsg );
         }
#endif
      return 1;
      }
   else
      {
      sprintf( errmsg, "Warning: encoding of float differ with value %E != %E\n", 
               fval, fval2 );
      bufr_print_debug( errmsg );
      return -1;
      }
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int test_encoding_double(double fval)
   {
   double    fval2, *ptr;
   uint64_t  ival;
   char      errmsg[256];

   ival = bufr_ieee_encode_double( fval );
   ptr = (double *)&ival;
   fval2 = *ptr;
   if ((fval2 == fval)||(isnan(fval2) && isnan(fval)))
      {
#if DEBUG
      if (bufr_is_debug())
         {
         sprintf( errmsg, "### Encoding double OK for value %E\n", fval );
         bufr_print_debug( errmsg );
         }
#endif
      return 1;
      }
   else
      {
      sprintf( errmsg, "Warning: encoding of double differ with value %E != %E\n", 
               fval, fval2 );
      bufr_print_debug( errmsg );
      return -1;
      }
   }
