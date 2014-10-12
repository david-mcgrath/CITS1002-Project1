/*
   CITS1002 Project 1 2013
   Name(s): David Blake McGrath
   Student number(s): 21301821
   Date: 20/9/2013
*/

// Should have used a hash table... would have made it a lot easier.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

// An ifdef should be here. Required on Linux, not OS X (test machines)
#include <sys/wait.h>
// endif

#define MAX_MACS 500
#define MAX_OUIS 20000
#define MAX_NAME_LENGTH 50

#define MAC_LENGTH 17
#define OUI_LENGTH 8

#define BROADCAST_MAC "ff:ff:ff:ff:ff:ff"
#define UNKNOWN_OUI "??:??:??"
#define UNKNOWN_VENDOR "UNKOWN-VENDOR"

#define PACKET_FORMATTING "%*f\t%*s\t%s\t%i"
#define OUI_FORMATTING "%s\t%[^\n]s"

// Holds information related to the statistics
// ... why did I do it like this? Should have been an array of structs.
struct wifiStats {
	char macAddress[MAX_MACS][MAC_LENGTH + 1];
	int bytesTransferred[MAX_MACS];
	char vendorName[MAX_MACS][MAX_NAME_LENGTH];
	int addressCount;
	bool oui;
} wifiStats;

// Holds vendor names and OUIs
// Same again
struct vendors {
	char vendorOUI[MAX_OUIS][OUI_LENGTH + 1];
	char vendorName[MAX_OUIS][MAX_NAME_LENGTH];
	int vendorCount;
} vendors;


/**
  Adds information from a packet to wifiStats.
  Probably too big. Should have been split up into multiple functions.
**/
void addPacket( char* mac , int size )
{
	// Converts uppercase characters to lowercase
	for( int i = 0 ; i < MAC_LENGTH ; i++ ) {
		mac[i] = tolower( mac[i] );
	}

	// Checks that the given MAC address is not the broadcast MAC address
	if( strcmp( mac , BROADCAST_MAC ) ) {

		char* vendorName;

		// If OUIs are being used, find vendor name. If none can be found, use UNKNOWN-VENDOR and replace OUI with ??:??:??
		if( wifiStats.oui ) {
			bool foundOUI = false;
			
			// Truncates MAC address, leaving only the OUI
			mac[OUI_LENGTH] = '\0';
			
			// Compares OUI with known ones
			for( int i = 0 ; i < vendors.vendorCount ; i++ ) {
				if( !strcmp( mac, vendors.vendorOUI[i] ) ) {
					foundOUI = true;
					vendorName = vendors.vendorName[i];
					break;
				}
			}

			if( !foundOUI ) {
				mac = UNKNOWN_OUI;
				vendorName = UNKNOWN_VENDOR;
			}
		}

        // Compares MAC address / OUI with already added ones. If one has already been added, increases the stored amount downloaded from that source and returns
		for( int i = 0 ; i < wifiStats.addressCount ; i++ ) {
			if( !strcmp( mac , wifiStats.macAddress[i] ) ) {
				wifiStats.bytesTransferred[i] += size;
				return;
			}
        }

		// Adds new MAC address / OUI
		int i = wifiStats.addressCount;
		wifiStats.addressCount += 1;	// Why not just increment? Same thing, but it would be more consistent.

		int length = strlen( mac );
		for( int j = 0 ; j < length ; j++ ) {
			wifiStats.macAddress[i][j] = mac[j];
		}
		wifiStats.macAddress[i][length] = '\0';

		wifiStats.bytesTransferred[i] = size;
		if( wifiStats.oui ) {
			int length = strlen( vendorName );
			for( int j = 0 ; j < length ; j++ ) {
				wifiStats.vendorName[i][j] = vendorName[j];
			}
			wifiStats.vendorName[i][length] = '\0';
		}
	}
}


/**
  Reads the packet file and adds all packets found to wifiStats using addPacket.
**/
void readPackets( FILE* fp )
{
	if( fp == NULL ) {
		perror( "Error: file not found" );
		exit( EXIT_FAILURE );
	} else {
		char* mac;
		int size;

		while( wifiStats.addressCount < MAX_MACS && fscanf( fp , PACKET_FORMATTING , mac , &size ) == 2 ) {
			addPacket( mac , size );
		}
	}
}


/**
  Adds an OUI and its vendor name to vendors.
**/
void addOUI( char* oui , char* vendor ) {
	int i = vendors.vendorCount;
	vendors.vendorCount++;

	// Converts uppercase characters to lowercase and replaces - with : in OUI and copies it to vendors.vendorOUI
	for( int j = 0 ; j < OUI_LENGTH ; j++ ) {
		if( oui[j] == '-' ) {
			oui[j] = ':';
		} else {
			oui[j] = tolower( oui[j] );
		}
		vendors.vendorOUI[i][j] = oui[j];
	}
	vendors.vendorOUI[i][OUI_LENGTH] = '\0';

	// Copies vendor name to vendors.vendorName
	int length = strlen( vendor );
	if( length > MAX_NAME_LENGTH ) {
		length = MAX_NAME_LENGTH;
	}
	
	for( int j = 0 ; j < length ; j++ ) {
		vendors.vendorName[i][j] = vendor[j];
	}
	vendors.vendorName[i][length] = '\0';
}


/**
  Reads OUI file and adds each OUI found to vendors using addOUI.
**/
void readOUIs( FILE* fp )
{
	if( fp == NULL ) {
		perror( "Error: File not found" );
		exit( EXIT_FAILURE );
	} else {
		char oui[OUI_LENGTH];
		char name[BUFSIZ];

		while(vendors.vendorCount < MAX_OUIS && fscanf( fp , OUI_FORMATTING , oui , name ) == 2 ) {
			addOUI( oui , name );
		}
	}
}


/**
  Writes information to the temporary file in preparation for sorting.
**/
void writeFile( char* filename )
{
	FILE* fp = fopen( filename , "w" );

	// Checks if file was opened correctly
	if( fp == NULL ) {
		perror( "Error: File could not be opened" );
		exit( EXIT_FAILURE );
	}

	// Writes information to file
	if( wifiStats.oui ) {
		for( int i = 0 ; i < wifiStats.addressCount ; i++ ) {
			fprintf( fp , "%s\t%s\t%i\n" , wifiStats.macAddress[i] , wifiStats.vendorName[i] , wifiStats.bytesTransferred[i] );
		}
	} else {
        	for( int i = 0 ; i < wifiStats.addressCount ; i++ ) {
			fprintf( fp , "%s\t%i\n" , wifiStats.macAddress[i] , wifiStats.bytesTransferred[i] );
		}
	}

	fclose( fp );
}


/**
  Removes the temporary file.
**/
void removeFile( char* filename )
{
	if( remove( filename ) ) {
		perror( "Error: Could not remove temporary file" );
		exit( EXIT_FAILURE );
	}
}


/**
  Sorts the temporary file and prints the results to stdout using /usr/bin/sort.
**/
void sort( char* filename )
{
	int status;

	char* path = "/usr/bin/sort";
	char* argv[] = { path , "-k2,2rn" , "-k1,1" , "-t\t" , filename , NULL };

	if( wifiStats.oui ) {
		argv[1] = "-k3,3rn";
		argv[2] = "-k2,2";
	}

	switch( fork() ) {
		case -1:
			perror( "Error" );
			exit( EXIT_FAILURE );
			break;
		case 0:
			// Executed by child: runs /usr/bin/sort with the required arguments on the temporary file
			execv( path ,  argv );
			break;
		default:
			// Wait for child to finish
			wait( &status );	// I don't think I used that right. Not sure.
			break;
	}
}



int main( int argc , char** argv )
{
	switch( argc ) {
		case 2:
			wifiStats.oui = false;
			break;
		case 3:
			wifiStats.oui = true;
			vendors.vendorCount = 0;
			break;
		default:
			printf( "Error: incorrect number of arguments. Correct format is:\n%s packets [ouis]\n" , argv[0] );
			exit( EXIT_FAILURE );
	}

	wifiStats.addressCount = 0;

	FILE* fp;

	if( wifiStats.oui ) {
		fp = fopen( argv[2] , "r" );
		readOUIs( fp );
		fclose( fp );
	}
	
	fp = fopen( argv[1] , "r" );
	readPackets( fp );
	fclose( fp );

	char* filename = "temp";	// What if a file named temp exists already? Although I guess with a name like that, they obviously don't care what happens to it.

	writeFile( filename );
	sort( filename );
	removeFile( filename );

	exit( EXIT_SUCCESS );
}
