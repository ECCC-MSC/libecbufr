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
#include "bufr_array.h"
#include "bufr_api.h"
#include "bufr_io.h"
#include "bufr_datablk.h"
#include "bufr_tables.h"
#include "bufr_value.h"

static int bufr_zip_sect3(BUFR_Tables *tbls, BUFR_Message *bufr );
static int bufr_tdzip_sect3(BUFR_Tables *, int *zdesc, int *desc, int descnt );

/*
 * nom: bufr_write_datablks
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ecrire dans un fichier DATA les donnees d'une station
 *
 * parametres:  
 *        fp   : pointeur du fichier DATA
 *        bufr : structure de donnees d'un message DATA
 *        blks : les blocs de donnees a ecrire
 *        nblk : le nombre de blocs
 *        x_compress : s'il faut compresser les donnees
 */
int bufr_write_datablks
(FILE *fp, BUFR_Message *bufr, DATA_BLK **blks, int nblk, int x_compress, BUFR_Tables *tbls )
   {
   EntryTableB *e1, e;
   EntryTableBArray desc_list;
   int    blen, klen;
   int    i, j, k, t, it, nt;
   char   buf[128];
   int    ival;
   DATA_BLK *blk;
   int    cnts[20];
   int    nbinc,imin,imax;
   int    descriptor;

   if ((nblk < 0)||(nblk > 200)) 
      {
      sprintf( buf, "Error: illegal number of blocks requested: %d\n", nblk );
      bufr_print_debug( buf );
      return -1;
      }

/*
 * on accepte de malanger les blocs nt=1 avec d'autres
 */
/* trouve 1er NT > 1 */
   blk = blks[0];
   nt = DATA_NT(blk);
   for ( k = 1 ; k < nblk ; k++ ) 
      {
      if (DATA_NT(blks[k]) > nt)
         {
         nt = DATA_NT(blk);
         break;
         }
      }

   for ( k = 1 ; k < nblk ; k++ ) {
      if ((DATA_NT(blks[k]) != nt)&&(DATA_NT(blks[k]) != 1)) {
         sprintf( buf, "Error: cannot mix blocks of different NT, %d against %d\n", 
                  nt, DATA_NT(blks[k]) );
         bufr_print_debug( buf );
         return -1;
         }
      }

/*
 * faire la compression seulement pour les series temporelles
 */
   if (nt == 1) x_compress = 0;

   if (x_compress)
      {
      /* other data, compressed */
      BUFR_SET_COMPRESSED( bufr );
      BUFR_SET_OTHER_DATA( bufr );
      }
   else 
      {
      /* other data, non-compressed */
      BUFR_SET_UNCOMPRESSED( bufr );
      BUFR_SET_OTHER_DATA( bufr );
      }
   bufr_begin_message( bufr );

/*
 * construire la liste descripteurs de la section 3
 */
   desc_list = (EntryTableBArray)arr_create( 300, sizeof(EntryTableB), 100 );
   BUFR_SET_NB_DATASET(bufr, nt);

   it = 0;
   blen = 0;
   nbinc = 0;
   for ( k = 0 ; k < nblk ; k++ ) {
      blk = blks[k];
/*
 * NELE are repeated NVAL times, delayed replication are not supported
 */
      e.descriptor = FXY_TO_DESC(1,DATA_NELE(blk),DATA_NVAL(blk));
      klen = 0;
      arr_add( desc_list, (char *)&e ); 
      ++it;
      descriptor = e.descriptor;
      arr_add( bufr->s3.desc_list, (char *)&descriptor );
      cnts[k] = it;
      for ( i = 0 ; i < DATA_NELE(blk) ; i++ ) {
         e1 = bufr_fetch_tableB( tbls, DATA_DLSTELEas(blk,i) );
         if (e1)
            {
            arr_add( desc_list, (char *)e1 ); ++it;
            descriptor = e1->descriptor;
            arr_add( bufr->s3.desc_list, (char *)&descriptor );
            klen += e1->encoding.nbits;
            }
         }
      blen += klen * DATA_NVAL(blk);
      nbinc += 6 * DATA_NVAL(blk);
      }

   bufr_zip_sect3( tbls, bufr );

/*
 * calculer la longueur maximum de la section4, 
 * en tenant compte de la compression de la pire cas
 */
   blen = ((blen * (bufr->s3.no_data_subsets+1))+nbinc+7)/8;
   bufr_alloc_sect4( bufr, blen );

   if (!BUFR_IS_COMPRESSED(bufr)) {
      for ( t = 0 ; t < nt ; t++ ) 
         {
         for ( k = 0 ; k < nblk ; k++ ) 
            {
            blk = blks[k];
            it = (DATA_NT(blk) > 1) ? t : 0;
            e1 = (EntryTableB *)arr_get( desc_list, cnts[k] );
            for ( j = 0 ; j < DATA_NVAL(blk) ; j++ ) 
               {
               for ( i = 0 ; i < DATA_NELE(blk) ; i++ ) 
                  {
                  ival = DATA_IVALas(blk,i,j,it);
                  bufr_putbits( bufr, ival, e1[i].encoding.nbits );
                  }
               }
            }
         }
      } else {
/*
 * mode compression
 */
      for ( k = 0 ; k < nblk ; k++ ) 
         {
         blk = blks[k];
         e1 = (EntryTableB *)arr_get( desc_list, cnts[k] );
         for ( j = 0 ; j < DATA_NVAL(blk) ; j++ ) 
            {
            for ( i = 0 ; i < DATA_NELE(blk) ; i++ ) 
               {
               imin = DATA_IVALas(blk,i,j,0);
               imax = DATA_IVALas(blk,i,j,0);
               for ( t = 1 ; t < DATA_NT(blk) ; t++ ) 
                  {
                  ival = DATA_IVALas(blk,i,j,t);
                  if (ival < imin) imin = ival;
                  if (ival > imax) imax = ival;
                  }
               if (imin == imax) {
                  bufr_putbits( bufr, imin, e1[i].encoding.nbits );  /* Ref */
                  bufr_putbits( bufr, 0, 6 );                        /* NBINC */
                  } else {
                  bufr_putbits( bufr, imin, e1[i].encoding.nbits );  /* Ref */
                  imax -= imin;
                  nbinc = bufr_value_nbits( imax );
                  bufr_putbits( bufr, nbinc, 6 );          /* NBINC */
                  for ( t = 0 ; t < nt ; t++ ) {
                     ival = DATA_IVALas(blk,i,j,t) - imin;
                     bufr_putbits( bufr, ival, nbinc );     /* Inc 's */
                     }
                  }
               }
            }
         }
      }

   arr_free( &desc_list );
   bufr_end_message( bufr );
   bufr_write_message( fp, bufr );
   return 0;
   }

/*
 * nom: bufr_cvt_blk
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: convertir les valeurs de pt. floatant en entier en utilisant
 *           les tables B chargees localement
 *
 * parametres:  
 *        blk      : bloc de donnees
 *
 */
void bufr_cvt_blk( DATA_BLK *blk, BUFR_Tables *tbls )
   {
   int  i, j, k, code;
   int  ival;
   float rval;
   EntryTableB *e;

   if (DATA_STORE_TYPE(blk) == STORE_INTEGER) return;

   for ( i = 0 ; i < DATA_NELE(blk) ; i++ ) 
      {
      code = DATA_DLSTELEas(blk,i);
      e = bufr_fetch_tableB( tbls, code );
      if ( e != NULL ) {
         for ( j = 0 ; j < DATA_NVAL(blk) ; j++ ) 
            {
            for ( k = 0 ; k < DATA_NT(blk) ; k++ ) 
               {
               if (DATA_STORE_TYPE(blk) == STORE_DOUBLE)
                  rval = DATA_DVALas( blk,i,j,k );
               else
                  rval = DATA_RVALas( blk,i,j,k );
               ival = bufr_cvt_fval_to_i32(e->descriptor, &(e->encoding), rval);
               DATA_SetIVALas( blk, i, j, k, ival );
               }
            }
         }
      }
   }

/*****************************************************************************
 ***NAME:  bufr_add_dlste()
 *
 *AUTHOR: Souvanlasy Viengsavanh
 *
 *PURPOSE:  add a new item code to the block if is not present
 *
 *PARAMETERS:
 *
 *   code    : the decoded DATA code to be add in DLSTELE
 *
 *   blk     : pointer to block data structure
 *
 *OUTPUT:
 *
 *    > 0 et < NELE   :  le code est insere dans DLSTELE, et indique
 *                       la position dans le tableau ou ce code a ete insere
 *
 *    -1   :   ce code n'existe pas dans DLSTELE et ne peut etre y insere
 **
-----------------------------------------------------------------------*/
int  bufr_add_dlste( int  code, DATA_BLK *blk )
   {
   int  i; /** compteur de boucle **/

   for ( i = 0 ; i < DATA_NELE(blk) ; i++ )
      {
      if ( code == DATA_DLSTELEas(blk,i) )
         return( i );
      }
   if ( i < DATA_MAX_NELE(blk) )
      {
      DATA_SetDLSTELEas(blk,i,code);
      DATA_SetNELE(blk,DATA_NELE(blk)+1);
      return( i );
      }
   else
      return( -1 );
   }

/*****************************************************************************
 ***NAME:  bufr_allocdata()
 *
 *AUTHOR: Souvanlasy Viengsavanh
 *
 *PURPOSE:  allouer l'espace memoire necessaire pour contenir un 
 *        bloc bufr, l'espace allouer est alors contenu dans une
 *        structure contenant les parametres d'un bloc, dont la taille
 *        du LSTELE en NELE, NVAL et NT
 *        
 *LANGAGE:  C
 *
 *PARAMETERS:
 *
 *  blk   : pointeur vers une structure pouvant contenir les informations
 *          d'un bloc de bufr
 *
 *  nele  : le nombre d'element du bloc pour LSTELE ( 1er dimension )
 *
 *  nval  : le nombre de niveau du bloc ( 2e dimension )
 *
 *  nt    : la profondeur du bloc ( 3e dimension )
 *
 *NOTES:   arrangements des tableaux
 *
 *   LSTELE  :  un tableau d'entier dont la taille est NELE
 *
 *   DLSTELE  :  un tableau d'entier dont la taille est NELE
 *
 *   RVAL  :  un tableau de reelles de 1 dimension mais represente 3
 *
 *   IVAL :  un tableau de d'entier de 1 dimension  mais represente 3
 *            
 *NOTES: acces des tableau  de rval et ival
 *
 *   i = (t * NVAL * NELE ) + ( val * NELE ) + ele
 *
 *   rval( ele , val , t )   =  rval[i]
 *
 *   ival( ele , val , t ) =  ival[i]
 *
 **
----------------------------------------------------------------------------*/
void bufr_allocdata( DATA_BLK *blk, int  nele, int nval, int nt )
   {
   int max_len, len;
/*
** liberer l'espace memoire de ces elements s'il ne sont pas deja faits
*/
   DATA_SetNELE(blk, nele );
   DATA_SetNVAL(blk, nval );
   DATA_SetNT(blk, nt );

   len = nele * nval * nt;
   max_len = DATA_MAX_NELE(blk)*DATA_MAX_NVAL(blk)*DATA_MAX_NT(blk);
   if (DATA_MAX_NELE(blk) < nele) 
      {
      DATA_SetMAX_NELE(blk, nele );
      if (blk->lstele == NULL)
         blk->lstele = (int *) malloc( nele * sizeof(int) );
      else
         blk->lstele = (int *) realloc( blk->lstele, nele * sizeof(int) );
      if (blk->dlstele == NULL)
         blk->dlstele = (int *) malloc( nele * sizeof(int) );
      else
         blk->dlstele = (int *) realloc( blk->dlstele, nele * sizeof(int) );
      }

   if (len <= max_len) {
      nval = max_len / (nele * nt);
      DATA_SetMAX_NVAL(blk, nval );
      DATA_SetMAX_NT(blk, nt );
      return;
      }

   DATA_SetMAX_NVAL(blk, nval );
   DATA_SetMAX_NT(blk, len/(DATA_MAX_NELE(blk)*nval) );
/*
** tous les tableaux sont lineaires, meme les tableau a plusieurs dimensions
** l'acces aux element de ce tableau doit etre fait de telle sorte a referer
** aux bons elements selon l'ordre fortran
*/
   if (blk->ival == NULL)
      blk->ival = (int *) malloc ( len * sizeof(int) );
   else
      blk->ival = (int *) realloc ( blk->ival, len * sizeof(int) );
   if (blk->rval == NULL)
      blk->rval = (float *) malloc ( len * sizeof(float) );
   else
      blk->rval = (float *) realloc ( blk->rval, len * sizeof(float) );
   if (blk->drval == NULL)
      blk->drval = (double *) malloc ( len * sizeof(double) );
   else
      blk->drval = (double *) realloc ( blk->drval, (len+1)/2 * sizeof(double) );
   }

/*****************************************************************************
 ***NAME:  bufr_asdata()
 *
 *AUTHOR: Souvanlasy Viengsavanh
 *
 *PURPOSE:  retourne l'indice des tableaux de RVAL ou de IVAL selon le
 *        ( ele , val , t ) et de ( NELE , NVAL , NT ) des tableaux alloues
 *
 *LANGAGE:  C
 *
 *PARAMETERS:
 *
 *  ele   : element du bloc ( 1er dimension )
 *
 *  val   : le  niveau du bloc ( 2e dimension )
 *
 *  t     : la profondeur du bloc ( 3e dimension )
 *
 *NOTES: l'indice du tableau lineaire pour representer un tableau en 3D
 *
 *  l'indice des elements du tableau sont conforme au format fortran
 *  et de taille NELE * NVAL * NT
 *  pour que le tableau puisse etre passe en argument aux fonctions 
 *  fortran de BURP
 *
 **
----------------------------------------------------------------------------*/
int  bufr_asdata( DATA_BLK  *blk, int  ele, int val, int t )
   {
   int    indice;

   indice =  ele +                       /** element varie le plus vite **/
      val * DATA_NELE(blk) +
      ( t ? (t * DATA_NVAL(blk)*DATA_NELE(blk)) : 0 );

   return( indice );
   }

/******************************************************************************
 ***NAME:  bufr_freeblk()
 *
 *AUTHOR: Souvanlasy Viengsavanh 
 *
 *PURPOSE:  liberer l'espace memoire occupe par la structure de donnees d'un
 *        bloc alloue avec bufr_newblk()
 *
 *LANGAGE: C
 *
 *PARAMETERS:
 *
 *
 *MODULES:
 *
 *  
 **
---------------------------------------------------------------------------*/
void  bufr_freeblk( DATA_BLK *blk )
   {
   if ( blk == NULL ) return;

   bufr_freedata( blk );

   free( blk );
   }

/*****************************************************************************
 ***NAME:  bufr_freedata()
 *
 *AUTHOR: Souvanlasy Viengsavanh
 *
 *PURPOSE:  Liberer l'espace occupe par le LSTELE, DLSTELE, RVAL et IVAL
 *        contenue dans une structure definie par DATA_BLK
 *       
 *PARAMETERS:
 *
 *  blk  : un pointeur vers une variable de type DATA_BLK contenant les 
 *         elements a liberer
 *        
 **
----------------------------------------------------------------------------*/
void  bufr_freedata( DATA_BLK *blk )
   {
   if ( blk == NULL ) return;
 
   free( DATA_LSTELE(blk) );
   free( DATA_DLSTELE(blk) );
   free( DATA_IVAL(blk) );
   free( DATA_RVAL(blk) );
   free( DATA_DVAL(blk) );

   DATA_SetMAX_NELE(blk, 0);
   DATA_SetMAX_NVAL(blk, 0);
   DATA_SetMAX_NT(blk, 0);
   DATA_SetNELE(blk, 0);
   DATA_SetNVAL(blk, 0);
   DATA_SetNT(blk, 0);
   blk->dlstele = NULL;
   blk->lstele = NULL;
   blk->ival = NULL;
   blk->rval = NULL;
   blk->drval = NULL;
   }

/*****************************************************************************
 ***NAME:  bufr_newblk()
 *
 *AUTHOR: Souvanlasy Viengsavanh
 *
 *PURPOSE:  creer une structure de donnees pouvant contenir un bloc
 *        puis retourne le pointeur vers cette structure
 *
 *LANGAGE:  C
 *
 *PARAMETERS:
 *
 *  nele  : le nombre d'element du bloc pour LSTELE ( 1er dimension )
 *
 *  nval  : le nombre de niveau du bloc ( 2e dimension )
 *
 *  nt    : la profondeur du bloc ( 3e dimension )
 *
 *MODULES:
 *
 *  bufr_allocdata :  allouer l'espace pour contenir les valeurs d'un bloc
 *
 *  bufr_resetblkhdr : initialise les valeurs de l'entete d'un bloc a zero
 *
 **
----------------------------------------------------------------------------*/
DATA_BLK *bufr_newblk (void)
   {
   DATA_BLK *blk;

   blk = (DATA_BLK *) malloc ( sizeof(DATA_BLK) );

   DATA_SetMAX_NELE(blk, 0);
   DATA_SetMAX_NVAL(blk, 0);
   DATA_SetMAX_NT(blk, 0);
   blk->dlstele = NULL;
   blk->lstele = NULL;
   blk->ival = NULL;
   blk->rval = NULL;
   blk->drval = NULL;
   blk->store_type = STORE_FLOAT;

   return( blk );
   }

/*****************************************************************************
 ***NAME:  bufr_searchdlste()
 *
 *AUTHOR: Souvanlasy Viengsavanh
 *
 *PURPOSE:  retourner l'indice d'un element dans un vecteur d'elements non codes
 *        
 *PARAMETERS:
 *
 *   code    : le code de DATA decode recherche dans  DLSTELE
 *
 *   blk     : pointeur vers une structure contenant les informations sur
 *             un bloc de donnees, 
 *             dont DLSTELE qui contient la liste des elements decodes
 *             et NELE le nombre d'element dans DLSTELE
 *
 *VALEUR RETOURNEE:
 *
 *    > 0 et < NELE   :  le code est trouve dans DLSTELE
 *
 *    -1   :   ce code n'existe pas dans DLSTELE
 **
-----------------------------------------------------------------------*/
int  bufr_searchdlste( int  code, DATA_BLK *blk )
   {
   int  i; /** compteur de boucle **/

   for ( i = 0 ; i < DATA_NELE(blk) ; i++ )
      {
      if ( code == DATA_DLSTELEas(blk,i) )
         return( i );
      }
   return( -1 );
   }

/*
 * nom: bufr_zip_sect3
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: compression a multi-passe de la section 3
 *           avec la table D
 *
 * parametres: 
 *      bufr : la structure de donnees BUFR
 */
static int bufr_zip_sect3(BUFR_Tables *tbls, BUFR_Message *bufr )
   {
   int i, zcnt;
   int *tdesc, *kdesc;
   int *zdesc, *desc; 
   int *zdesc0, *desc0; 
   int descnt;
   int  *e1;
   char *desc_list;

   descnt = arr_count( bufr->s3.desc_list );
   zdesc0 = zdesc = (int *)malloc(descnt *sizeof(int));
   desc0 = desc = (int *)malloc(descnt *sizeof(int));
   e1 = (int *)arr_get( bufr->s3.desc_list, 0 );
   for ( i = 0 ; i < descnt ; i++ ) 
      {
      desc[i] = e1[i];
      }

   kdesc = zdesc;
   zcnt = descnt;
   do {
      descnt = zcnt;
      zcnt = bufr_tdzip_sect3( tbls, zdesc, desc, descnt );
      tdesc = zdesc;
      zdesc = desc;
      desc = tdesc;
      } while ( zcnt < descnt );

   if (zdesc == kdesc) zdesc = desc;
   
   desc_list = bufr->s3.desc_list;
   arr_del( desc_list, arr_count(desc_list) );

   for ( i = 0 ; i < zcnt ; i++ ) 
      {
      arr_add( desc_list, (char *)(zdesc+i) );
      }

   free( desc0 );
   free( zdesc0 );
   return zcnt;
   }

/*
 * nom: bufr_tdzip_sect3
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: faire 1 passe de compress avec une table D
 *
 * parametres: 
 *      zdesc  : descripteurs compresses
 *      desc   : descripteurs en entree
 *      descnt : nombre de descripteurs
 */
static int bufr_tdzip_sect3(BUFR_Tables *tbls, int *zdesc, int *desc, int descnt )
   {
   int i, k, l;
   int count, zcnt;
   EntryTableD *d;
   int  f,x,y;
   int  *codes;
  
   zcnt = 0;
   for ( i = 0 ; i < descnt ; ) 
      {
      bufr_descriptor_to_fxy( desc[i], &f, &x, &y );
      if ((f == 1)&&((x+i)<descnt)) 
         {
         int tot;
         int *zdesc1;
         zdesc1 = (int *)malloc(x * sizeof(int));
         if (y == 0) 
            {
            tot = bufr_tdzip_sect3( tbls, zdesc1, desc+i+2, x );
            zdesc[zcnt++] = FXY_TO_DESC(f,tot,y);
            zdesc[zcnt++] = desc[i+1];
            i += x + 2;
            } 
         else 
            {
            tot = bufr_tdzip_sect3( tbls, zdesc1, desc+i+1, x );
            zdesc[zcnt++] = FXY_TO_DESC(f,tot,y);
            i += x + 1;
            }
         for (l = 0; l < tot; l++) zdesc[zcnt++] = zdesc1[l];
         free( zdesc1 );
         } 
      else 
         {
         d = bufr_fetch_tableD ( tbls, desc[i] );
         if (d)
            {
            codes = d->descriptors;
            count = d->count;
            for ( k = 1 ; (k < count)&&((i+k)<descnt) ; k++ )
               if (codes[k] != desc[i+k]) break;
            if (k >= count) 
               {
               zdesc[zcnt++] = d->descriptor;
               i += count;
               break; /* j */
               }
            }
         else
            {
            zdesc[zcnt++] = desc[i];
            ++i;
            }
         }
      }
   return zcnt;
   }

