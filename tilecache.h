/*
*    QTC: tilecache.h (c) 2011, 2012 50m30n3
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

#ifndef TILECACHE_H
#define TILECACHE_H

/*******************************************************************************
* Structure to hold a single cached tile                                       *
*                                                                              *
* present indicates wether the current tile is used or not                     *
* size is the size of the cached tile in pixels                                *
* hash ist the fletcher16 hash of the tile data                                *
* next ist the index of the next tile with the same hash, -1 if there is none  *
* data is the cached data                                                      *
*******************************************************************************/
struct tile
{
	int present;
	int size;
	unsigned short int hash;
	int next;
	unsigned int *data;
};

/*******************************************************************************
* Structure to hold all the data associated with a tile cache                  *
*                                                                              *
* size is the number of tiles in the cache                                     *
* blocksize is the width/height of a single tile                               *
* tilesize is the size of one tile in bytes                                    *
* indexbits is the number of bits needed to represent a cache index            *
* index is the index of the last cached tile                                   *
* numblocks is the total number of blocks written to the cache                 *
* hits is the total number of cache hits                                       *
* tiles contains the cached tiles                                              *
* tileindex is a hash table containing tile indices                            *
* data is the cache data used by the tiles                                     *
* tempdata is a temporary buffer used for internal operations                  *
*******************************************************************************/
struct tilecache
{
	int size;
	int blocksize;
	int tilesize;
	int indexbits;

	int index;

	unsigned long int numblocks;
	unsigned long int hits;

	struct tile *tiles;
	int *tileindex;

	unsigned int *data;
	unsigned int *tempdata;
};


extern struct tilecache *tilecache_create( int size, int blocksize );
extern void tilecache_free( struct tilecache *cache );
extern void tilecache_reset( struct tilecache *cache );
extern int tilecache_write( struct tilecache *cache, unsigned int *pixels, int x1, int x2, int y1, int y2, int width, unsigned int mask );
extern void tilecache_read( struct tilecache *cache, unsigned int *pixels, int index, int x1, int x2, int y1, int y2, int width, unsigned int mask );
extern void tilecache_add( struct tilecache *cache, unsigned int *pixels, int x1, int x2, int y1, int y2, int width, unsigned int mask );

#endif
