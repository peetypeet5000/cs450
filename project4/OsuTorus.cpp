#include <stdio.h>
#include <math.h>
#include <OpenGl/gl.h>

void	OsuTorus( GLfloat, GLfloat, GLint, GLint );


void
OsuTorus( GLfloat r, GLfloat R, GLint numSides, GLint numRings )
{
	GLfloat ringDelta = 2.0 * M_PI / (float)numRings;
	GLfloat sideDelta = 2.0 * M_PI / (float)numSides;

	GLfloat theta    = 0.0;
	GLfloat cosTheta = 1.0;
	GLfloat sinTheta = 0.0;

	for( int i = 0; i < numRings; i++ )
	{
		GLfloat theta1 = theta + ringDelta;
		GLfloat cosTheta1 = cos(theta1);
		GLfloat sinTheta1 = sin(theta1);

		GLfloat phi = 0.0;
		float t  = (float)i     / (float)numRings;
		float t1 = (float)(i+1) / (float)numRings;

		glBegin( GL_QUAD_STRIP );

		for( int j = 0; j <= numSides; j++ )
		{
			phi += sideDelta;
			GLfloat cosPhi = cos(phi);
			GLfloat sinPhi = sin(phi);
			GLfloat dist = R + r * cosPhi;

			float s = (float)j / (float)numSides;
			glTexCoord2f( s, t );
			glNormal3f( cosTheta1 * cosPhi, -sinTheta1 * cosPhi, sinPhi );
			glVertex3f( cosTheta1 * dist,   -sinTheta1 * dist,   r * sinPhi );

			glTexCoord2f( s, t1 );
			glNormal3f( cosTheta * cosPhi, -sinTheta * cosPhi, sinPhi );
			glVertex3f( cosTheta * dist,   -sinTheta * dist,   r * sinPhi );
		}

		glEnd( );

		theta = theta1;
		cosTheta = cosTheta1;
		sinTheta = sinTheta1;
	}
}
