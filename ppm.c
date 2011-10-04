#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "image.h"

#include "ppm.h"

int ppm_read( struct image *image, char filename[] )
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
			fputs( "ppm_read: Cannot read file\n", stderr );
			if( ppm != stdin )
				fclose( ppm );
			return 0;
		}

		if( strcmp( buffer, "P6\n" ) != 0 )
		{
			fputs( "ppm_read: File is not a ppm file\n", stderr );
			if( ppm != stdin )
				fclose( ppm );
			return 0;
		}

		do
		{
			if( fgets( buffer, sizeof( buffer ) - 1, ppm ) == NULL )
			{
				fputs( "ppm_read: Cannot read file\n", stderr );
				if( ppm != stdin )
					fclose( ppm );
				return 0;
			}
		}
		while( buffer[0] == '#' );

		if( sscanf( buffer, "%i %i\n", &width, &height ) != 2 )
		{
			fputs( "ppm_read: Cannot read image dimension\n", stderr );
			if( ppm != stdin )
				fclose( ppm );
			return 0;
		}

		do
		{
			if( fgets( buffer, sizeof( buffer ) - 1, ppm ) == NULL )
			{
				fputs( "ppm_read: Cannot read file\n", stderr );
				if( ppm != stdin )
					fclose( ppm );
				return 0;
			}
		}
		while( buffer[0] == '#' );

		if( sscanf( buffer, "%i\n", &maxval ) != 1 )
		{
			fputs( "ppm_read: Cannot read maximum pixel value\n", stderr );
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
				fputs( "ppm_read: Short read on image data\n", stderr );
				if( ppm != stdin )
					fclose( ppm );
				return 0;
			}
		}
		else
		{
			perror( "ppm_read" );
			return 0;
		}

		if( ppm != stdin )
			fclose( ppm );

		return 1;
	}
	else
	{
		perror( "ppm_read" );
		return 0;
	}
}

int ppm_write( struct image *image, char filename[] )
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
			fputs( "ppm_read: Short write on image data\n", stderr );

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
		perror( "ppm_write" );
		return 0;
	}
}

