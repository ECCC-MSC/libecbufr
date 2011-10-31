/**
@example decode_net.c
@english
Repeat example 2 decoding BUFR message using sockets

@verbatim
nc -l -p 1234 -c 'cat ../Test/BUFR/iusd40_okli.bufr' &
BUFR_TABLES=../Tables ./decode_net
@endverbatim
@endenglish
@francais
@todo decode_net.c description should be translated
@endfrancais
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

#include "bufr_api.h"

/*
 * define these string and point them to a valid BUFR TABLE B file and TABLE D file
 * in order to load other local tables
 */
static char *str_ltableb=NULL;
static char *str_ltabled=NULL;

int main(int argc, char *argv[])
   {
   BUFR_Dataset  *dts;
   FILE          *fp;
   BUFR_Tables   *tables=NULL;
   char           buf[256];
   BUFR_Message  *msg;
   int            rtrn;
   int            count;
   char           filename[256];
   struct protoent     *proto;
   struct sockaddr_in   addr;
   struct hostent *host;
   int             fd;
   int             status;

/*
 * load CMC Table B and D
 * includes local descriptors
 */
   tables = bufr_create_tables();
   bufr_load_cmc_tables( tables );  
/*
 * load local tables if any 
 */
   if (str_ltableb)
      bufr_load_l_tableB( tables, str_ltableb );
   if (str_ltabled)
      bufr_load_l_tableD( tables, str_ltabled );

/*
 * use the tcp protocol
 */
   proto = getprotobyname("tcp");
/*
 * open a socket
 */
   fd = socket(AF_INET, SOCK_STREAM, proto->p_proto );
/*
 * define address where to connect to
 * then connect to it
 */
   addr.sin_family = AF_INET;
/* 
 * substitute with valid port number and hostname 
 */
   addr.sin_port = htons(1234); 
   if (host = gethostbyname( "localhost" ))
      {
      memcpy(&(addr.sin_addr), host->h_addr, host->h_length);
      }

   status = connect( fd, (struct sockaddr *)&addr, sizeof(addr)) ;
   if (status < 0)
      {
      fprintf( stderr, "Error: unable to connect\n" );
      exit(-1);
      }

   fp = fdopen( fd, "r" );
/*
 * open a file for reading
 */
   if (fp == NULL)
      {
      bufr_free_tables( tables );
      fprintf( stderr, "Error: can't get a file pointer \"%s\"\n", argv[1] );
      exit(-1);
      }
/*
 * read all messages from the input file
 */
   count = 0;
/*
 * a new instance of BUFR_Message is assigned to msg at each invocation
 */
   while ( (rtrn = bufr_read_message( fp, &msg )) > 0 )
      {
      ++count;
/* 
 * BUFR_Message ==> BUFR_Dataset 
 * decode the message using the BUFR Tables
 * the decoded message is stored in BUFR_Dataset *dts structure
 * a new instance of BUFR_Dataset is assigned to dts at each invocation
 */
      dts = bufr_decode_message( msg, tables ); 
      if (dts == NULL) 
         {
         fprintf( stderr, "Error: can't decode messages #%d\n", count );
         continue;
         }
/*
 * dump the content of the Message into a file
 */
      sprintf( filename, "./OUTPUT-%d.TXT", count );
      bufr_dump_dataset( dts, filename );
/*
 * discard the message
 */
      bufr_free_message( msg );
/*
 * discard an instance of the Decoded message pointed to by dts
 */
      bufr_free_dataset( dts );
      }
/*
 * close all file and cleanup
 */
   fclose( fp );
   close( fd );

   bufr_free_tables( tables );
   }

