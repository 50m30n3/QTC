/*
*    QTC: qtvplay.c (c) 2011, 2012 50m30n3
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
#include <signal.h>

#include <SDL/SDL.h>

#include "utils.h"
#include "image.h"
#include "databuffer.h"
#include "qti.h"
#include "qtc.h"
#include "qtv.h"
#include "ppm.h"

/*******************************************************************************
* This is a simple qtv player using SDL.                                       *
*******************************************************************************/

void print_help( void )
{
	puts( "qtvplay (c) 50m30n3 2011, 2012" );
	puts( "USAGE: qtvplay [options] -i infile" );
	puts( "\t-h\t\t-\tPrint help" );
	puts( "\t-v\t\t-\tBe verbose" );
	puts( "\t-r [1..]\t-\tOverride frame rate" );
	puts( "\t-w\t\t-\tRead QTW file" );
	puts( "\t-i filename\t-\tInput file (-)" );
	puts( "Keys:" );
	puts( "\t[space]\t\t-\tPlay/Pause" );
	puts( "\t[left]\t\t-\tSeek backwards 10sec" );
	puts( "\t[right]\t\t-\tSeek forwards 10sec" );
	puts( "\t[down]\t\t-\tSeek backwards 1min" );
	puts( "\t[up]\t\t-\tSeek forwards 1min" );
	puts( "\t[a]\t\t-\tToggle analysis mode" );
	puts( "\t[o]\t\t-\tToggle overlay mode\t" );
	puts( "\t[t]\t\t-\tToggle Peath transform" );
	puts( "\t[y]\t\t-\tToggle fakeyuv" );
	puts( "\t[s]\t\t-\tPrint stats" );

}

int main( int argc, char *argv[] )
{
	struct image image, ccimage, refimage;
	struct qti compimage;
	struct qtv video;

	SDL_Surface *screen;
	SDL_Event event;

	int opt, analyze, overlay, transform, colordiff, printstats, qtw;
	int done, framenum, playing, step;
	int framerate;
	long int delay, start, frame_start;
	double fps, load;
	char *infile;

	int i;
	unsigned int *pixels, *ccpixels;

	framerate = -1;
	analyze = 0;
	overlay = 0;
	transform = 1;
	colordiff = 1;
	printstats = 0;
	qtw = 0;
	infile = NULL;

	while( ( opt = getopt( argc, argv, "hvwi:r:" ) ) != -1 )
	{
		switch( opt )
		{
			case 'h':
				print_help();
				return 0;
			break;

			case 'v':
				printstats = 1;
			break;

			case 'r':
				if( sscanf( optarg, "%i", &framerate ) != 1 )
					fputs( "main: Can not parse command line: -r\n", stderr );
			break;

			case 'w':
				qtw = 1;
			break;

			case 'i':
				infile = strdup( optarg );
			break;

			default:
			case '?':
				fputs( "main: Can not parse command line: unknown option\n", stderr );
				return 1;
			break;
		}
	}

	if( framerate < -1 )
	{
		fputs( "main: Frame rate out of range\n", stderr );
		return 1;
	}

	done = 0;
	framenum = 0;
	playing = 1;
	step = 0;
	fps = 0.0;
	load = 0.0;

	if( ! qtv_read_header( &video, qtw, infile ) )
		return 0;

	if( framerate == -1 )
		framerate = video.framerate;

	fprintf( stderr, "Video: %ix%i, %i FPS\n", video.width, video.height, framerate );

	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		fputs( "main: SDL_Init: Can not initialize SDL\n", stderr );
		return 1;
	}

	screen = SDL_SetVideoMode( video.width, video.height, 32, SDL_SWSURFACE );

	if( ! screen )
	{
		fputs( "main: SDL_SetVideoMode: Can not set video mode\n", stderr );
		SDL_Quit();
		return 1;
	}

	SDL_WM_SetCaption( "QTV Play", "QTV Play" );

	image_create( &refimage, video.width, video.height );

	start = get_time();

	do
	{
		frame_start = get_time();

		if( playing || step )
		{
			if( ! qtv_read_frame( &video, &compimage ) )
				return 2;

			if( ! qtc_decompress( &compimage, &refimage, &image, 1 ) )
				return 2;

			image_copy( &image, &refimage );

			if( !analyze || overlay )
			{
				if( transform )
				{
					if( image.transform == 1 )
						image_transform_fast_rev( &image );
					else if( image.transform == 2 )
						image_transform_rev( &image );
				}

				if( colordiff )
				{
					if( image.colordiff )
						image_color_diff_rev( &image );
				}
			}
			
			if( analyze )
			{
				compimage.imagedata->pos = 0;
				compimage.imagedata->bitpos = 8;
				compimage.commanddata->pos = 0;
				compimage.commanddata->bitpos = 8;
				
				if( overlay )
				{
					if( ! qtc_decompress_ccode( &compimage, &ccimage, 1, analyze-1 ) )
						return 2;

					pixels = (unsigned int *)image.pixels;
					ccpixels = (unsigned int *)ccimage.pixels;

					for( i=0; i<image.width*image.height; i++ )
						pixels[i] = ((pixels[i]&0xfefefefe)>>1)+((ccpixels[i]&0xfefefefe)>>1);
					
					image_free( &ccimage );
				}
				else
				{
					image_free( &image );

					if( ! qtc_decompress_ccode( &compimage, &image, 1, analyze-1 ) )
						return 2;
				}
			}

			memcpy( screen->pixels, image.pixels, video.width*video.height*4 );

			SDL_Flip( screen );

			image_free( &image );
			qti_free( &compimage );

			step = 0;
		}

		if( ! qtv_can_read_frame( &video ) )
			done = 1;


		while( SDL_PollEvent( &event ) )
		{
			switch( event.type )
			{
				case SDL_QUIT:
					done = 1;
				break;

				case SDL_KEYDOWN:
					switch( event.key.keysym.sym )
					{
						case 'q':
						case SDLK_ESCAPE:
							fputs( "Quit\n", stderr );
							done = 1;
						break;

						case 's':
							printstats = !printstats;
						break;

						case 'a':
							if( analyze == 0 )
							{
								fputs( "Analyze ON (Luma)\n", stderr );
								analyze = 1;
							}
							else if( analyze == 1 )
							{
								fputs( "Analyze ON (Chroma)\n", stderr );
								analyze = 2;
							}
							else
							{
								fputs( "Analyze OFF\n", stderr );
								analyze = 0;
							}
						break;

						case 'o':
							if( overlay )
							{
								fputs( "Overlay OFF\n", stderr );
								overlay = 0;
							}
							else
							{
								fputs( "Overlay ON\n", stderr );
								overlay = 1;
							}
						break;

						case 't':
							if( transform )
							{
								fputs( "Transform OFF\n", stderr );
								transform = 0;
							}
							else
							{
								fputs( "Transform ON\n", stderr );
								transform = 1;
							}
						break;

						case 'y':
							if( colordiff )
							{
								fputs( "Fakeyuv OFF\n", stderr );
								colordiff = 0;
							}
							else
							{
								fputs( "Fakeyuv ON\n", stderr );
								colordiff = 1;
							}
						break;

						case SDLK_SPACE:
							if( playing )
							{
								fputs( "PAUSE\n", stderr );
								playing = 0;
							}
							else
							{
								fputs( "PLAYING\n", stderr );
								playing = 1;
							}
						break;

						case '.':
							fputs( "Step\n", stderr );
							playing = 0;
							step = 1;
						break;

						case SDLK_LEFT:
							if( video.has_index )
							{
								qtv_seek( &video, video.framenum - 10*framerate );
								fprintf( stderr, "Seek to: %i \n", video.framenum );
							}
							else
							{
								fputs( "Cannot seek without index\n", stderr );
							}
						break;

						case SDLK_RIGHT:
							if( video.has_index )
							{
								qtv_seek( &video, video.framenum + 10*framerate );
								fprintf( stderr, "Seek to: %i \n", video.framenum );
							}
							else
							{
								fputs( "Cannot seek without index\n", stderr );
							}
						break;

						case SDLK_DOWN:
							if( video.has_index )
							{
								qtv_seek( &video, video.framenum - 60*framerate );
								fprintf( stderr, "Seek to: %i \n", video.framenum );
							}
							else
							{
								fputs( "Cannot seek without index\n", stderr );
							}
						break;

						case SDLK_UP:
							if( video.has_index )
							{
								qtv_seek( &video, video.framenum + 60*framerate );
								fprintf( stderr, "Seek to: %i \n", video.framenum );
							}
							else
							{
								fputs( "Cannot seek without index\n", stderr );
							}
						break;

						default:
						break;
					}
				break;

				default:
				break;
			}
		}

		load = load*0.9 + 0.1*((double)(get_time()-frame_start)*(double)framerate/1000000.0*100.0);

		if( printstats )
		{
			if( qtw )
			{
				fprintf( stderr, "Frame:%i/%i Block:%i/%i FPS:%.2f Load:%.2f%% Type:(K:%i,T:%i,Y:%i,S:%i,M:%i)\n",
				         video.framenum-1, video.numframes-1, video.blocknum, video.numblocks-1, fps, load,
				         compimage.keyframe, compimage.transform, compimage.colordiff, compimage.minsize, compimage.maxdepth );
			}
			else
			{
				fprintf( stderr, "Frame:%i/%i FPS:%.2f Load:%.2f%% Type:(K:%i,T:%i,Y:%i,S:%i,M:%i)\n",
				         video.framenum-1, video.numframes-1, fps, load,
				         compimage.keyframe, compimage.transform, compimage.colordiff, compimage.minsize, compimage.maxdepth );
			}
		}

		framenum++;

		if( framerate != 0 )
		{
			delay = (framenum*(1000000l/(long int)framerate))-(get_time()-start);
		
			if( delay > 0 )
				usleep( delay );
		}

		fps = fps*0.75 + 0.25*(1000000.0/(get_time()-frame_start));
	}
	while( ! done );

	fps = 1000000.0/((get_time()-start)/framenum);

	image_free( &refimage );
	qtv_free( &video );

	if( printstats )
	{
		fprintf( stderr, "FPS:%.2f\n", fps );
	}

	free( infile );

	SDL_Quit();

	return 0;
}

