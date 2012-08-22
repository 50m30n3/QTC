/*
*    QTC: qtc.h (c) 2011, 2012 50m30n3
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

#ifndef QTC_H
#define QTC_H

extern int qtc_compress( struct image *input, struct image *refimage, struct qti *output, int lazyness, int bgra, int colordiff );
extern int qtc_decompress( struct qti *input, struct image *refimage, struct image *output, int bgra );
extern int qtc_decompress_ccode( struct qti *input, struct image *output, int bgra, int channel );

#endif
