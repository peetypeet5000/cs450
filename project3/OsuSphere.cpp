#include <stdio.h>
#include <math.h>
#include <OpenGL/gl.h>


#ifndef POINT_H
#define POINT_H
struct point
{
	float x, y, z;		// coordinates
	float nx, ny, nz;	// surface normal
	float s, t;		// texture coords
};

inline
void
DrawPoint( struct point *p )
{
	glNormal3fv( &p->nx );
	glTexCoord2fv( &p->s );
	glVertex3fv( &p->x );
}
#endif

int		SphNumLngs, SphNumLats;
struct point *	SphPts;

inline
struct point *
SphPtsPointer( int lat, int lng )
{
	if( lat < 0 )			lat += (SphNumLats-1);
	if( lng < 0 )			lng += (SphNumLngs-0);
	if( lat > SphNumLats-1 )	lat -= (SphNumLats-1);
	if( lng > SphNumLngs-1 )	lng -= (SphNumLngs-0);
	return &SphPts[ SphNumLngs*lat + lng ];
}

void
OsuSphere( float radius, int slices, int stacks )
{
	// set the globals:

	SphNumLngs = slices;
	SphNumLats = stacks;
	if( SphNumLngs < 3 )
		SphNumLngs = 3;
	if( SphNumLats < 3 )
		SphNumLats = 3;

	// allocate the point data structure:

	SphPts = new struct point[ SphNumLngs * SphNumLats ];

	// fill the SphPts structure:

	for( int ilat = 0; ilat < SphNumLats; ilat++ )
	{
		float lat = -M_PI/2.  +  M_PI * (float)ilat / (float)(SphNumLats-1);	// ilat=0/lat=0. is the south pole
											// ilat=SphNumLats-1, lat=+M_PI/2. is the north pole
		float xz = cosf( lat );
		float  y = sinf( lat );
		for( int ilng = 0; ilng < SphNumLngs; ilng++ )				// ilng=0, lng=-M_PI and
											// ilng=SphNumLngs-1, lng=+M_PI are the same meridian
		{
			float lng = -M_PI  +  2. * M_PI * (float)ilng / (float)(SphNumLngs-1);
			float x =  xz * cosf( lng );
			float z = -xz * sinf( lng );
			struct point* p = SphPtsPointer( ilat, ilng );
			p->x  = radius * x;
			p->y  = radius * y;
			p->z  = radius * z;
			p->nx = x;
			p->ny = y;
			p->nz = z;
			p->s = ( lng + M_PI    ) / ( 2.*M_PI );
			p->t = ( lat + M_PI/2. ) / M_PI;
		}
	}

	struct point top, bot;		// top, bottom points

	top.x =  0.;		top.y  = radius;	top.z = 0.;
	top.nx = 0.;		top.ny = 1.;		top.nz = 0.;
	top.s  = 0.;		top.t  = 1.;

	bot.x =  0.;		bot.y  = -radius;	bot.z = 0.;
	bot.nx = 0.;		bot.ny = -1.;		bot.nz = 0.;
	bot.s  = 0.;		bot.t  =  0.;

	// connect the north pole to the latitude SphNumLats-2:

	glBegin(GL_TRIANGLE_STRIP);
	for (int ilng = 0; ilng < SphNumLngs; ilng++)
	{
		float lng = -M_PI + 2. * M_PI * (float)ilng / (float)(SphNumLngs - 1);
		top.s = (lng + M_PI) / (2. * M_PI);
		DrawPoint(&top);
		struct point* p = SphPtsPointer(SphNumLats - 2, ilng);	// ilat=SphNumLats-1 is the north pole
		DrawPoint(p);
	}
	glEnd();

	// connect the south pole to the latitude 1:

	glBegin( GL_TRIANGLE_STRIP );
	for (int ilng = SphNumLngs - 1; ilng >= 0; ilng--)
	{
		float lng = -M_PI + 2. * M_PI * (float)ilng / (float)(SphNumLngs - 1);
		bot.s = (lng + M_PI) / (2. * M_PI);
		DrawPoint(&bot);
		struct point* p = SphPtsPointer(1, ilng);					// ilat=0 is the south pole
		DrawPoint(p);
	}
	glEnd();

	// connect the horizontal strips:

	for( int ilat = 2; ilat < SphNumLats-1; ilat++ )
	{
		struct point* p;
		glBegin(GL_TRIANGLE_STRIP);
		for( int ilng = 0; ilng < SphNumLngs; ilng++ )
		{
			p = SphPtsPointer( ilat, ilng );
			DrawPoint( p );
			p = SphPtsPointer( ilat-1, ilng );
			DrawPoint( p );
		}
		glEnd();
	}

	// clean-up:

	delete [ ] SphPts;
	SphPts = NULL;
}
