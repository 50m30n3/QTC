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

#include "image.h"

#include "tilecache.h"

const int indexsize = 0x10000;

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

	cache->tiles = malloc( sizeof( *cache->tiles ) * size );
	if( cache->tiles == NULL )
	{
		perror( "tilecache_create: malloc" );
		return NULL;
	}

	cache->index = malloc( sizeof( *cache->index ) * indexsize );
	if( cache->index == NULL )
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

	for( i=0; i<size; i++ )
	{
		cache->tiles[i].present = 0;
		cache->tiles[i].pixels = &cache->data[i*blocksize*blocksize];
	}

	for( i=0; i<indexsize; i++ )
		cache->index[i].present = NULL;

	return cache;
}

void tilecache_free( struct tilecache *cache )
{
	free( cache->tiles );
	free( cache->index );
	free( cache->data );
	free( cache );
}

