/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Interface for CH375 **
**  VC5.0@PC                          **
****************************************
*/

#define PC_DEBUG_DRIVE			"\\\\.\\PHYSICALDRIVE1"

#include	<windows.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<conio.h>

main( ULONG argc, UCHAR *argv[] ) {
	UCHAR	c;
	ULONG	i, j;
	ULONG	lba;
	HANDLE	mhDevice;
	UCHAR	buffer[512];
	printf( "Copyright (C) W.ch 1998-2005, http://wch.cn\n" );
	printf( "Show sector data in second physical disk\n" );
	if ( argc <= 1 ) {
		printf( "This program only work under WINDOWS 2000/XP\n" );
		printf( "Command line:  CH375UD  sector_number\n" );
		printf( "Example:\n" );
		printf( "  to show    0# (0000H) sector: CH375UD  0\n" );
		printf( "  to show   63# (003FH) sector: CH375UD  63\n" );
		printf( "  to show 1758# (06DEH) sector: CH375UD  1758\n" );
		printf( "to show sector data and save to TXT file:\n" );
		printf( "  CH375UD  1758  > file_name_for_saving_text\n" );
		printf( "warning !!! to clear 1758# (06DEH) sector: CH375UD  1758 /C\n" );
		exit( 1 );
	}
	lba = atol( argv[1] );
	mhDevice = CreateFile( PC_DEBUG_DRIVE, GENERIC_READ | GENERIC_WRITE,
							FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( mhDevice == INVALID_HANDLE_VALUE ) {
		printf( "Please insert USB disk and retry\n" );
		exit( 2 );
	}
	i = lba * 512;
	j = SetFilePointer( mhDevice, i, NULL, FILE_BEGIN );  // 指定扇区号
	if ( i != j ) {
		CloseHandle( mhDevice );
		printf( "Error on set disk sector address\n" );
		exit( 3 );
	}
	i = 512;
	if ( ! ReadFile( mhDevice, buffer, 512, &i, NULL ) ) {  // 读一个扇区512字节
		CloseHandle( mhDevice );
		printf( "Error on reading disk sector\n" );
		exit( 4 );
	}
	if ( i != 512 ) {
		CloseHandle( mhDevice );
		printf( "Error on reading disk sector, return length = %d\n", i );
		exit( 5 );
	}
	for ( i = 0; i < 512; i += 16 ) {
		printf( "@%03X:", i );
		for ( j = 0; j < 16; j ++ ) {
			if ( j == 8 ) printf( " -" );
			printf( " %02X", buffer[i+j] );
		}
		printf( "   " );
		for ( j = 0; j < 16; j ++ ) {
			c = buffer[i+j];
			if ( c < 0x20 ) c = '.';
			if ( c == 0x7f ) c = '.';
			printf( "%c", c );
		}
		printf( "\n" );
	}
	if ( argc >= 3 ) {
		if ( argv[2][0] == '/' && ( argv[2][1] == 'C' || argv[2][1] == 'c' ) ) {
			printf( "Press TAB to confirm clear %ld# sector in second disk\n", lba );
			if ( getch( ) == 9 ) {
				i = lba * 512;
				SetFilePointer( mhDevice, i, NULL, FILE_BEGIN );  // 指定扇区号
				for ( i = 0; i < 512; i ++ ) buffer[i] = 0;
				if ( ! WriteFile( mhDevice, buffer, 512, &i, NULL ) ) {  // 写一个扇区512字节
					CloseHandle( mhDevice );
					printf( "Error on writing disk sector\n" );
					exit( 6 );
				}
				if ( i == 512 ) printf( "Clear successfully\n" );
				else printf( "Error return length = %d\n", i );
			}
			else printf( "Canceled\n" );
		}
	}
	CloseHandle( mhDevice );
	printf( "  CH375UD  %ld | more   or  CH375UD  %ld > SECTOR.TXT  ", lba, lba );
	return( 1 );
}
