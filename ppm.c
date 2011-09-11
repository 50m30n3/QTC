#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "image.h"

#include "ppm.h"

int read_ppm( struct image *image, char filename[] )
{
	FILE *ppm;
	char buffer[256];
	int width, height, maxval;

	if( filename == NULL )
	{
		ppm = stdin;
	}
	else
	{
		if( strcmp( filename, "-" ) == 0 )
		{
			ppm = stdin;
		}
		else
		{
			ppm = fopen( filename, "rb" );
		}
	}

	if( ppm != NULL )
	{
		if( fgets( buffer, sizeof( buffer ) - 1, ppm ) == NULL )
		{
			fputs( "read_ppm: Cannot read file\n", stderr );
			if( ppm != stdin )
				fclose( ppm );
			return 0;
		}

		if( strcmp( buffer, "P6\n" ) != 0 )
		{
			fputs( "read_ppm: File is not a ppm file\n", stderr );
			if( ppm != stdin )
				fclose( ppm );
			return 0;
		}

		do
		{
			if( fgets( buffer, sizeof( buffer ) - 1, ppm ) == NULL )
			{
				fputs( "read_ppm: Cannot read file\n", stderr );
				if( ppm != stdin )
					fclose( ppm );
				return 0;
			}
		}
		while( buffer[0] == '#' );

		if( sscanf( buffer, "%i %i\n", &width, &height ) != 2 )
		{
			fputs( "read_ppm: Cannot read image dimension\n", stderr );
			if( ppm != stdin )
				fclose( ppm );
			return 0;
		}

		do
		{
			if( fgets( buffer, sizeof( buffer ) - 1, ppm ) == NULL )
			{
				fputs( "read_ppm: Cannot read file\n", stderr );
				if( ppm != stdin )
					fclose( ppm );
				return 0;
			}
		}
		while( buffer[0] == '#' );

		if( sscanf( buffer, "%i\n", &maxval ) != 1 )
		{
			fputs( "read_ppm: Cannot read maximum pixel value\n", stderr );
			if( ppm != stdin )
				fclose( ppm );
			return 0;
		}

		image->width = width;
		image->height = height;

		image->pixels = malloc( sizeof( struct pixel ) * width * height );

		if( image->pixels != NULL )
		{
			if( fread( image->pixels, sizeof( *(image->pixels) ), width*height, ppm ) != width*height )
			{
				fputs( "read_ppm: Short read on image data\n", stderr );
				if( ppm != stdin )
					fclose( ppm );
				return 0;
			}
		}
		else
		{
			perror( "read_ppm" );
			return 0;
		}

		if( ppm != stdin )
			fclose( ppm );

		return 1;
	}
	else
	{
		perror( "read_ppm" );
		return 0;
	}
}

int write_ppm( struct image *image, char filename[] )
{
	FILE *ppm;

	if( filename == NULL )
	{
		ppm = stdout;
	}
	else
	{
		if( strcmp( filename, "-" ) == 0 )
		{
			ppm = stdout;
		}
		else
		{
			ppm = fopen( filename, "wb" );
		}
	}

	if( ppm != NULL )
	{
		fprintf( ppm, "P6\n%i %i\n255\n", image->width, image->height );

		if( fwrite( image->pixels, sizeof( *(image->pixels) ), image->width*image->height, ppm ) != image->width*image->height )
		{
			fputs( "read_ppm: Short write on image data\n", stderr );

			if( ppm != stdout )
				fclose( ppm );
			return 0;
		}

		if( ppm != stdout )
			fclose( ppm );

		return 1;
	}
	else
	{
		perror( "write_ppm" );
		return 0;
	}
}

