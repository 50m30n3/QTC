/*
*    QTC: qtidec.c (c) 2011, 2012 50m30n3
*
*    This file is part of QTC.
*
*    QTC is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    QTC is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with QTC.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "image.h"
#include "qti.h"
#include "qtc.h"
#include "ppm.h"

/*******************************************************************************
* This is the reference qti decoder.                                           *
*                                                                              *
* It implements all the features currently supported by the qti codec.         *
*******************************************************************************/

void print_help( void )
{
	puts( "qtidec (c) 50m30n3 2011, 2012" );
	puts( "USAGE: qtidec [options] -i infile -o outfile" );
	puts( "\t-h\t\t-\tPrint help" );
	puts( "\t-a [0..2]\t-\tAnalysis mode" );
	puts( "\t-i filename\t-\tInput file (-)" );
	puts( "\t-o filename\t-\tOutput file (-)" );
}

int main( int argc, char *argv[] )
{
	struct image image;
	struct qti compimage;

	int opt, analyze;
	char *infile, *outfile;

	analyze = 0;
	infile = NULL;
	outfile = NULL;

	while( ( opt = getopt( argc, argv, "ha:i:o:" ) ) != -1 )
	{
		switch( opt )
		{
			case 'h':
				print_help();
				return 0;
			break;

			case 'a':
				if( sscanf( optarg, "%i", &analyze ) != 1 )
					fputs( "main: Can not parse command line: -a\n", stderr );
			break;

			case 'i':
				infile = strdup( optarg );
			break;

			case 'o':
				outfile = strdup( optarg );
			break;

			default:
			case '?':
				fputs( "main: Can not parse command line: unknown option\n", stderr );
				return 1;
			break;
		}
	}

	if( ( analyze < 0 ) || ( analyze > 2 ) )
	{
		fputs( "main: Analysis mode out of range\n", stderr );
		return 1;
	}

	if( ! qti_read( &compimage, infile ) )		// Read compressed image from file
		return 2;

	if( ! image_create( &image, compimage.width, compimage.height, 0 ) )
		return 2;

	if( analyze == 0 )
	{
		if( ! qtc_decompress( &compimage, NULL, &image ) )		// Decompress image
			return 2;

		if( image.transform == 1 )		// Apply reverse image transforms
			image_transform_fast_rev( &image );
		else if( image.transform == 2 )
			image_transform_rev( &image );

		if( image.colordiff )		// Apply reverse fakeyuv transform
			image_color_diff_rev( &image );
	}
	else
	{
		if( ! qtc_decompress_ccode( &compimage, &image, analyze-1 ) )		// Create analysis image
			return 2;
	}

	if( ! ppm_write( &image, outfile ) )		// Write decompressed image to file
		return 2;
	
	image_free( &image );
	qti_free( &compimage );

	free( infile );
	free( outfile );

	return 0;
}

