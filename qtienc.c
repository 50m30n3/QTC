/*
*    QTC: qtienc.c (c) 2011, 2012 50m30n3
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
* This is the reference qti encoder.                                           *
*                                                                              *
* It implements all the features currently supported by the qti codec.         *
*******************************************************************************/

int main( int argc, char *argv[] )
{
	struct image image;
	struct qti compimage;

	int opt, verbose;
	unsigned long int insize, bsize, outsize;
	int transform, colordiff;
	int rangecomp;
	int minsize;
	int maxdepth;
	int lazyness;
	char *infile, *outfile;

	verbose = 0;
	transform = 0;
	colordiff = 0;
	rangecomp = 0;
	minsize = 2;
	maxdepth = 16;
	lazyness = 0;
	infile = NULL;
	outfile = NULL;

	while( ( opt = getopt( argc, argv, "cvy:t:s:d:l:i:o:" ) ) != -1 )
	{
		switch( opt )
		{
			case 't':
				if( sscanf( optarg, "%i", &transform ) != 1 )
					fputs( "main: Can not parse command line: -t\n", stderr );
			break;

			case 'c':
				rangecomp = 1;
			break;

			case 'y':
				if( sscanf( optarg, "%i", &colordiff ) != 1 )
					fputs( "main: Can not parse command line: -y\n", stderr );
			break;

			case 'v':
				verbose = 1;
			break;

			case 's':
				if( sscanf( optarg, "%i", &minsize ) != 1 )
					fputs( "main: Can not parse command line: -s\n", stderr );
			break;

			case 'd':
				if( sscanf( optarg, "%i", &maxdepth ) != 1 )
					fputs( "main: Can not parse command line: -d\n", stderr );
			break;

			case 'l':
				if( sscanf( optarg, "%i", &lazyness ) != 1 )
					fputs( "main: Can not parse command line: -l\n", stderr );
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

	if( ( transform < 0 ) || ( transform > 2 ) )
	{
		fputs( "main: Transform mode out of range\n", stderr );
		return 1;
	}

	if( ( colordiff < 0 ) || ( colordiff > 2 ) )
	{
		fputs( "main: Fakeyuv mode out of range\n", stderr );
		return 1;
	}

	if( minsize < 0 )
	{
		fputs( "main: Mininmal block size out of range\n", stderr );
		return 1;
	}

	if( maxdepth < 0 )
	{
		fputs( "main: Maximum recursion depth out of range\n", stderr );
		return 1;
	}

	if( lazyness < 0 )
	{
		fputs( "main: Lazyness recursion depth out of range\n", stderr );
		return 1;
	}

	if( ! ppm_read( &image, infile ) )		// Read the input image
		return 0;

	insize = image.width * image.height * 3;

	if( colordiff >= 1 )		// Apply fakeyuv transform
		image_color_diff( &image );

	if( transform == 1 )		// Apply image transforms
		image_transform_fast( &image );
	else if( transform == 2 )
		image_transform( &image );

	if( ! qtc_compress( &image, NULL, &compimage, minsize, maxdepth, lazyness, 0, colordiff >= 2 ) )		// Compress the image
		return 0;

	bsize = qti_getsize( &compimage );

	compimage.transform = transform;
	compimage.colordiff = colordiff;
	
	if( ! ( outsize = qti_write( &compimage, rangecomp, outfile ) ) )		// Write image to file
		return 0;
	
	image_free( &image );
	qti_free( &compimage );
	
	if( verbose )
		fprintf( stderr, "In:%luB Buff:%luB,%f%% Out:%luB,%f%%\n", insize, bsize/8, (bsize/8)*100.0/insize, outsize, outsize*100.0/insize );

	free( infile );
	free( outfile );

	return 0;
}

