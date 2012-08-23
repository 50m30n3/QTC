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

struct tile
{
	int present;
	unsigned short int hash;
	int width, height, size;
	struct pixel *pixels;
};

struct tileindex
{
	int index;
	struct tileindex *next, *prev;
};

struct tilecache
{
	int size;
	int blocksize;

	struct tile *tiles;
	struct tileindex *index;

	struct pixel *data;
};


extern struct tilecache *tilecache_create( int size, int blocksize );
extern void tilecache_free( struct tilecache *cache );

#endif
