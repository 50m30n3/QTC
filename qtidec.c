#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "image.h"
#include "qti.h"
#include "qtc.h"
#include "ppm.h"

int main( int argc, char *argv[] )
{
	struct image image;
	struct qti compimage;

	int opt, analyze;
	char *infile, *outfile;

	analyze = 0;
	infile = NULL;
	outfile = NULL;

	while( ( opt = getopt( argc, argv, "a:i:o:" ) ) != -1 )
	{
		switch( opt )
		{
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

	if( ! qti_read( &compimage, infile ) )
		return 0;

	if( analyze == 0 )
	{
		if( ! qtc_decompress( &compimage, NULL, &image, 0, compimage.colordiff >= 2 ) )
			return 0;

		if( compimage.transform == 1 )
			image_transform_fast_rev( &image );
		else if( compimage.transform == 2 )
			image_transform_rev( &image );

		if( compimage.colordiff >= 1 )
			image_color_diff_rev( &image );
	}
	else
	{
		if( ! qtc_decompress_ccode( &compimage, &image, 0, 0, compimage.colordiff >= 2, analyze-1 ) )
			return 0;
	}

	if( ! ppm_write( &image, outfile ) )
		return 0;
	
	image_free( &image );
	qti_free( &compimage );

	free( infile );
	free( outfile );

	return 0;
}

