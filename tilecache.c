/*
*    QTC: tilecache.c (c) 2011, 2012 50m30n3
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
#include <string.h>

#include "tilecache.h"

const int indexsize = 0x10000;

/*******************************************************************************
* Function to compute the fletcher16 checksum of a block of data               *
*                                                                              *
* data is the data the checksum should be computed of                          *
* length is the length aof the data in bytes                                   *
*                                                                              *
* Returns the fletcher16 checksum of the data                                  *
*******************************************************************************/
static unsigned short int fletcher16( unsigned char *data, int length )
{
	unsigned char s1, s2;
	int i;

	s1 = s2 = 0;

	for( i=0; i<length; i++ )
	{
		s1 += data[i];
		s2 += s1;
	}

	return (s2<<8)|s1;
}

/*******************************************************************************
* Function to create a new tile cache                                          *
*                                                                              *
* size is the number of tiles to cache                                         *
* blocksize is the width and height of the quadratic tiles                     *
*                                                                              *
* Returns a new tile cache or NULL on failure                                  *
*******************************************************************************/
struct tilecache *tilecache_create( int size, int blocksize )
{
	struct tilecache *cache;
	int i;
	
	cache = malloc( sizeof( *cache ) );
	if( cache == NULL )
	{
		perror( "tilecache_create: malloc" );
		return NULL;
	}
	
	cache->size = size;
	cache->blocksize = blocksize;
	cache->tilesize = sizeof( *cache->data )*blocksize*blocksize;

	if( size <= 0x1<<16 )
		cache->indexbits = 16;
	else if( size <= 0x1<<24 )
		cache->indexbits = 24;
	else
		cache->indexbits = 32;

	cache->index = 0;

	cache->numblocks = 0;
	cache->hits = 0;

	cache->tiles = malloc( sizeof( *cache->tiles ) * size );
	if( cache->tiles == NULL )
	{
		perror( "tilecache_create: malloc" );
		return NULL;
	}

	cache->tileindex = malloc( sizeof( *cache->tileindex ) * indexsize );
	if( cache->tileindex == NULL )
	{
		perror( "tilecache_create: malloc" );
		return NULL;
	}

	cache->data = malloc( sizeof( *cache->data ) * size * blocksize * blocksize );
	if( cache->data == NULL )
	{
		perror( "tilecache_create: malloc" );
		return NULL;
	}

	cache->tempdata = malloc( sizeof( *cache->tempdata ) * blocksize * blocksize );
	if( cache->tempdata == NULL )
	{
		perror( "tilecache_create: malloc" );
		return NULL;
	}

	for( i=0; i<size; i++ )
	{
		cache->tiles[i].present = 0;
		cache->tiles[i].next = -1;
		cache->tiles[i].data = &cache->data[i*blocksize*blocksize];
	}

	for( i=0; i<indexsize; i++ )
		cache->tileindex[i] = -1;

	return cache;
}

/*******************************************************************************
* Function to free a tile buffer structure                                     *
*                                                                              *
* cache is the tile cache to free                                              *
*                                                                              *
* Modifies tile cache                                                          *
*******************************************************************************/
void tilecache_free( struct tilecache *cache )
{
	free( cache->tiles );
	free( cache->tileindex );
	free( cache->data );
	free( cache->tempdata );
	free( cache );
}

/*******************************************************************************
* Function to reset a tile buffer to its initial state                         *
*                                                                              *
* cache is the tile cache to be reset                                          *
*                                                                              *
* Modifies tile cache                                                          *
*******************************************************************************/
void tilecache_reset( struct tilecache *cache )
{
	int i;

	cache->index = 0;

	for( i=0; i<cache->size; i++ )
	{
		cache->tiles[i].present = 0;
		cache->tiles[i].next = -1;
	}
	
	for( i=0; i<indexsize; i++ )
		cache->tileindex[i] = -1;
}

/*******************************************************************************
* Function to write a new tile to the cache                                    *
*                                                                              *
* cache is the tile cache to use                                               *
* pixels is a pointer to the pixel array containing the tile                   *
* x1, x2, y1, y2 describe the tile position                                    *
* with is the width of the complete image                                      *
* mask is the channel mask used during write                                   *
*                                                                              *
* Modifies tile cache, returns index of tile if already cached, -1 otherwise   *
*******************************************************************************/
int tilecache_write( struct tilecache *cache, unsigned int *pixels, int x1, int x2, int y1, int y2, int width, unsigned int mask )
{
	int size;
	int x, y, i, j;
	int found;
	unsigned short int hash;

	cache->numblocks++;

	memset( (unsigned char *)cache->tempdata, 0, cache->tilesize );

	j = 0;
	for( y=y1; y<y2; y++ )
	{
		i = x1 + y*width;
		for( x=x1; x<x2; x++ )
			cache->tempdata[j++] = pixels[i++]&mask;
	}

	size = (x2-x1)*(y2-y1);
	
	hash = fletcher16( (unsigned char *)cache->tempdata, cache->tilesize );
	i = cache->tileindex[hash];

	found = 0;

	while( ( i != -1 ) && ( ! found ) )
	{
		if( (  cache->tiles[i].size == size ) && ( memcmp( cache->tempdata, cache->tiles[i].data, cache->tilesize ) == 0 ) )
		{
			found = 1;
			cache->hits++;
		}
		else
		{
			i = cache->tiles[i].next;
		}
	}

	if( ! found )
	{
		cache->index++;
		cache->index %= cache->size;

		if( cache->tiles[cache->index].present )
		{
			i = cache->tileindex[cache->tiles[cache->index].hash];

			if( i == cache->index )
			{
				cache->tileindex[cache->tiles[cache->index].hash] = cache->tiles[i].next;
			}
			else
			{
				j = i;
				i = cache->tiles[i].next;
				
				while( i != -1 )
				{
					if( i == cache->index )
					{
						cache->tiles[j].next = cache->tiles[i].next;
						break;
					}
					
					j = i;
					i = cache->tiles[i].next;
				}
			}
		}

		cache->tiles[cache->index].present = 1;
		cache->tiles[cache->index].size = size;
		cache->tiles[cache->index].hash = hash;
		cache->tiles[cache->index].next = cache->tileindex[hash];
		cache->tileindex[hash] = cache->index;
		memcpy( cache->tiles[cache->index].data, cache->tempdata, cache->tilesize );
		
		return -1;
	}
	else
	{
		return i;
	}
}

/*******************************************************************************
* Function to read a tile from a tile cache                                    *
*                                                                              *
* cache is the tile cache to use                                               *
* pixels is a pointer to the pixel array the tile should be written to         *
* index is the index of the tile to retreive                                   *
* x1, x2, y1, y2 describe the tile position                                    *
* with is the width of the complete image                                      *
* mask is the channel mask used during write                                   *
*                                                                              *
* Modifies tile pixels                                                         *
*******************************************************************************/
void tilecache_read( struct tilecache *cache, unsigned int *pixels, int index, int x1, int x2, int y1, int y2, int width, unsigned int mask )
{
	int x, y, i, j;
	unsigned int invmask;
	unsigned int *data;

	invmask = ~mask;
	data = cache->tiles[index].data;

	j = 0;
	for( y=y1; y<y2; y++ )
	{
		i = x1 + y*width;
		for( x=x1; x<x2; x++ )
		{
			pixels[i] &= invmask;
			pixels[i++] |= data[j++]&mask;
		}
	}
}

/*******************************************************************************
* Function to add a tile to a tile cache                                       *
*                                                                              *
* cache is the tile cache to use                                               *
* pixels is a pointer to the pixel array containing the tile                   *
* x1, x2, y1, y2 describe the tile position                                    *
* with is the width of the complete image                                      *
* mask is the channel mask used during write                                   *
*                                                                              *
* Modifies tile cache                                                          *
*******************************************************************************/
void tilecache_add( struct tilecache *cache, unsigned int *pixels, int x1, int x2, int y1, int y2, int width, unsigned int mask )
{
	int x, y, i, j;

	cache->numblocks++;

	memset( (unsigned char *)cache->tempdata, 0, cache->tilesize );

	j = 0;
	for( y=y1; y<y2; y++ )
	{
		i = x1 + y*width;
		for( x=x1; x<x2; x++ )
			cache->tempdata[j++] = pixels[i++]&mask;
	}

	cache->index++;
	cache->index %= cache->size;

	memcpy( cache->tiles[cache->index].data, cache->tempdata, cache->tilesize );
}

