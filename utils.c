/*
*    QTC: utils.c (c) 2011, 2012 50m30n3
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
#include <string.h>
#include <sys/time.h>

/*******************************************************************************
* Function to get the current time                                             *
*                                                                              *
* Returns the current time in microsecond accuracy                             *
*******************************************************************************/
unsigned long int get_time( void )
{
	struct timeval tv;

	gettimeofday( &tv, NULL );

	return tv.tv_sec * 1000000lu + tv.tv_usec;
}

/*******************************************************************************
* Function to increase a numerical field in a filename                         *
*                                                                              *
* name is a pointer to the file name                                           *
*                                                                              *
* Modifies name                                                                *
*                                                                              *
* Returns 1 on success, 0 on overflow                                          *
*******************************************************************************/
int inc_filename( char *name )
{
	int i, carry, done;
	
	carry = 1;
	done = 0;

	for( i=strlen( name )-1; i>=0; i-- )
	{
		if( ( name[i] >= '0' ) && ( name[i] <= '9' ) )
		{
			done = 1;
			if( carry )
			{
				if( name[i] < '9' )
				{
					name[i]++;
					carry = 0;
				}
				else
				{
					name[i] = '0';
					carry = 1;
				}
			}
			else
			{
				break;
			}
		}
		else
		{
			if( done )
				break;
		}
	}
	
	return !carry;
}


