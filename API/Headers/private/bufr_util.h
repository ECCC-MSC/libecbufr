/*
 *
 *  file      :  BUFR_UTIL.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR BUFR STRINGS UTILITIES
 *
 *
 */


#ifndef _bufr_util_h_
#define _bufr_util_h_

extern char *strimdup              ( char *dest, const char *src, int maxlen );

extern char *str_schar2oct         ( char *str, int *len, int *bsize );
extern char *str_oct2char          ( char *str, int *len );

extern void  append_char_to_string ( char **str, int *size, int *pos, unsigned char c );


#endif  /* _bufr_util_h_ */
