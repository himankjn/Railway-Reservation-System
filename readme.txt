
This contains the steps to run/use the Railway ticket booking system project.


Steps to run (Unix environment):

1) reset database as       > ./db_reset.sh
2) compile server.c as     > gcc -o server server.c
3) compile client.c as     > gcc -o client client.c	
4) run server as 	   > ./server
5) run concurrent clients as > ./client

Directory Structure:

'conf.h' : contains default values and structures used.
'common.h': contains the necessary dependency header files for both client.c and server.c
'client.c': client side C code
'server.c': server side C code
'db_reset.sh': shell script to reset database files and directory.
'database': directory containing database files 'train','booking','user'

To reset the database run db_reset.sh

*** The default values like port,ip of server, etc can be changed in conf.h file ***


