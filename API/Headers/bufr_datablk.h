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
 
 *  file      :  BUFR_DATABLK.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR BUFR DATABLK
 *               THIS IS THE BRIDGE BETWEEN CMC BURP FILES AND BUFR
 *
 *
 */

#ifndef _bufr_datablk_h_
#define	_bufr_datablk_h_

#include "bufr_io.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/

/*
** DEFINITION DES MACROS POUR ACCEDER A LA STRUCTURE D'UN BLOC
*/
#define  DATA_NELE(blk)             ((blk)->nele)
#define  DATA_NVAL(blk)             ((blk)->nval)
#define  DATA_NT(blk)               ((blk)->nt)
#define  DATA_MAX_NELE(blk)         ((blk)->max_nele)
#define  DATA_MAX_NVAL(blk)         ((blk)->max_nval)
#define  DATA_MAX_NT(blk)           ((blk)->max_nt)
#define  DATA_LSTELE(blk)           ((blk)->lstele)
#define  DATA_LSTELEas(blk,e)       ((blk)->lstele[e])
#define  DATA_DLSTELE(blk)          ((blk)->dlstele)
#define  DATA_DLSTELEas(blk,e)      ((blk)->dlstele[e])
#define  DATA_IVAL(blk)             ((blk)->ival)
#define  DATA_RVAL(blk)             ((blk)->rval)
#define  DATA_DVAL(blk)             ((blk)->drval)
#define  DATA_IVALas(blk,e,v,t)     (blk)->ival[bufr_asdata(blk,e,v,t)]
#define  DATA_RVALas(blk,e,v,t)     (blk)->rval[bufr_asdata(blk,e,v,t)]
#define  DATA_DVALas(blk,e,v,t)     (blk)->drval[bufr_asdata(blk,e,v,t)]
#define  DATA_IVALlas(blk,i)        (blk)->ival[i]
#define  DATA_RVALlas(blk,i)        (blk)->rval[i]
#define  DATA_DVALlas(blk,i)        (blk)->drval[i]
#define  DATA_STORE_TYPE(blk)   (blk)->store_type


/*
** DEFINITION DES MACROS POUR AFFECTER DANS LA STRUCTURE D'UN BLOC DE BURP
*/
#define  DATA_SetNELE(blk,val)             (blk)->nele=val
#define  DATA_SetNVAL(blk,val)             (blk)->nval=val
#define  DATA_SetNT(blk,val)               (blk)->nt=val
#define  DATA_SetMAX_NELE(blk,val)         (blk)->max_nele=val
#define  DATA_SetMAX_NVAL(blk,val)         (blk)->max_nval=val
#define  DATA_SetMAX_NT(blk,val)           (blk)->max_nt=val
#define  DATA_SetLSTELEas(blk,i,val)       (blk)->lstele[i]=val
#define  DATA_SetDLSTELEas(blk,i,val)      (blk)->dlstele[i]=val
#define  DATA_SetIVALas(blk,e,v,t,val)     (blk)->ival[bufr_asdata(blk,e,v,t)]=val
#define  DATA_SetRVALas(blk,e,v,t,val)     (blk)->rval[bufr_asdata(blk,e,v,t)]=val
#define  DATA_SetDVALas(blk,e,v,t,val)     (blk)->drval[bufr_asdata(blk,e,v,t)]=val
#define  DATA_SetIVALlas(blk,i,val)        (blk)->ival[i]=val
#define  DATA_SetRVALlas(blk,i,val)        (blk)->rval[i]=val
#define  DATA_SetDVALlas(blk,i,val)        (blk)->drval[i]=val
#define  DATA_SetSTORE_TYPE(blk,val)       (blk)->store_type=val


#define  STORE_INTEGER    'I'
#define  STORE_FLOAT      'F'
#define  STORE_DOUBLE      'D'

/*
** DEFINITON DES STRUCTURES POUR REGROUPER TOUS LES PARAMETRES ET TABLES 
** DE VALEURS POUR UN BLOC DE DONNEES BURP
*/
typedef  struct 
   {
   int    *lstele;
   int    *dlstele;
   int    *ival;
   float  *rval;
   double *drval;
   int    nele;
   int    nval;
   int    nt;
   int    max_nval, max_nele, max_nt;
   char   store_type;
   } DATA_BLK;

extern  DATA_BLK  *bufr_newblk          ( void );
extern  void       bufr_freeblk         ( DATA_BLK *blk );
extern  void       bufr_freedata        ( DATA_BLK *blk );
extern  void       bufr_allocdata       ( DATA_BLK *blk, int  nele, int nval, int nt );
extern  int        bufr_add_dlste       ( int  descriptor, DATA_BLK *blk );
extern  int        bufr_asdata          ( DATA_BLK  *blk, int  ele, int val, int t );
extern  int        bufr_searchdlste     ( int  descriptor, DATA_BLK *blk );

extern int         bufr_write_datablks  ( FILE *,BUFR_Message *,DATA_BLK **,int nblk,int docomp, BUFR_Tables * );
extern void        bufr_cvt_blk         ( DATA_BLK *blk, BUFR_Tables * );

#ifdef __cplusplus
}
#endif

/***************************************************************************
***************************************************************************/
#endif /* _bufr_datablk_h_ */
