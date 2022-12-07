#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <cstdlib>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#endif

#include "glew.h"
#include <OpenGl/gl.h>
#include <OpenGl/glu.h>
#include "glut.h"

// Include OSU Sphere
#include "OsuSphere.cpp"

// Include OBJ loader function
#include "loadobj.cpp"


//  CS 450 Final
//	Author:			Peter LaMontagne

// title of these windows:
const char *WINDOWTITLE = "Herd Immunity Simulation -- Peter LaMontagne";
const char *GLUITITLE   = "User Interface Window";

// what the glui package defines as true and false:
const int GLUITRUE  = true;
const int GLUIFALSE = false;

// the escape key:
const int ESCAPE = 0x1b;

// initial window size:
const int INIT_WINDOW_SIZE = 800;

// multiplication factors for input interaction:
//  (these are known from previous experience)
const float ANGFACT = 1.f;
const float SCLFACT = 0.005f;

// minimum allowable scale factor:
const float MINSCALE = 0.05f;

// scroll wheel button values:
const int SCROLL_WHEEL_UP   = 3;
const int SCROLL_WHEEL_DOWN = 4;

// equivalent mouse movement when we click the scroll wheel:
const float SCROLL_WHEEL_CLICK_FACTOR = 5.f;

// active mouse buttons (or them together):
const int LEFT   = 4;
const int MIDDLE = 2;
const int RIGHT  = 1;

// Simulation Parameters
const int MAX_HUMANS = 100;
const int INITIAL_IMMUNITY = 100;
const int INITIAL_INFECTED = 20;

// which projection:

enum Projections
{
	ORTHO,
	PERSP
};

// which button:

enum ButtonVals
{
	RESET,
	QUIT
};

enum HumanStates
{
	NONE,
	INFECTED,
	IMMUNE,
	DEAD
};

// window background color (rgba):

const GLfloat BACKCOLOR[ ] = { 0., 0., 0., 1. };
float White[3] = { 1., 1., 1. };

// Struct for storing sim data

struct SimHuman {
	int model;
	float rotation;
	HumanStates state;
};

// non-constant global variables:

int		ActiveButton;			// current button that is down
int		FreezeOn;				// != 0 means to freeze the scene
int		DebugOn;				// != 0 means to print debugging info
GLuint	SphereList;				// Display List for OSU Sphere
GLuint	Human1;					// Display List for Human obj
GLuint	Human2;					// Display List for Human 2 obj
GLuint	Human3;					// Display List for Human 3 obj
int		MainWindow;				// window id for main graphics window
float	Scale;					// scaling factor
int		WhichProjection;		// ORTHO or PERSP
int		Xmouse, Ymouse;			// mouse values
float	Xrot, Yrot;				// rotation angles in degrees
int   	Ticks;					// time in animation
int		TickTime;				// time for one tick
int		LastTickTime;			// glut Time value when last tick occured
int 	SimSize;				// Width & Height of array for simulation
int 	Transmissibility;		// How easily the virus spreads, lower the faster
int 	LostImmunity;			// Chance a person will lose immunity
int 	MortailityRate;			// Likelyhood of dying after being infected
int		InfectedTime;			// Time someone is infected (slightly randomized)
SimHuman SimState[100][100];	// 2D Array of simulation state	

// function prototypes:

void	Simulate( );
void	TickSimulation();
void	Display( );
void	DrawHuman( SimHuman );
void	DoFloatMenu( int );
void	DoColorMenu( int );
void	DoDepthMenu( int );
void	DoDebugMenu( int );
void	DoMainMenu( int );
void	DoProjectMenu( int );
void	DoRasterString( float, float, float, char * );
void	DoStrokeString( float, float, float, float, char * );
float	ElapsedSeconds( );
void	InitGraphics( );
void	InitSimulation( );
void	InitLists( );
void	InitMenus( );
void	Keyboard( unsigned char, int, int );
void	MouseButton( int, int, int, int );
void	MouseMotion( int, int );
void	Reset( );
void	Resize( int, int );
void	Visibility( int );

unsigned char *	BmpToTexture( char *, int *, int * );
int				ReadInt( FILE * );
short			ReadShort( FILE * );

void			HsvRgb( float[3], float [3] );
void			SetMaterial( float, float, float, float );
void			SetPointLight( int, float, float, float,  float, float, float );
void			SetSpotLight( int, float, float, float, float, float, float, float, float, float );
float *			Array3( float, float, float );
float *			MulArray3( float, float [3] );
void			Cross(float[3], float[3], float[3]);
float			Dot(float [3], float [3]);
float			Unit(float [3], float [3]);


// main program:

int
main( int argc, char *argv[ ] )
{
	// turn on the glut package:
	// (do this before checking argc and argv since it might
	// pull some command line arguments out)

	glutInit( &argc, argv );

	// setup all the graphics stuff:

	InitGraphics( );

	// Make display lists

	InitLists();

	// init all the global variables used by Display( ):
	// this will also post a redisplay

	Reset( );

	// setup all the user interface stuff:

	InitMenus( );

	// draw the scene once and wait for some interaction:
	// (this will never return)

	glutSetWindow( MainWindow );
	glutMainLoop( );

	// glutMainLoop( ) never actually returns
	// the following line is here to make the compiler happy:

	return 0;
}


// this is where one would put code that is to be called
// everytime the glut main loop has nothing to do
//
// Every TickTime, compute new infected persons.
// Spend the time between cycles animating the old ones

void
Simulate( )
{	
	int ms = glutGet( GLUT_ELAPSED_TIME );	// milliseconds
	ms = ms - LastTickTime; 	// Remove time before most recent tick

	if(ms > TickTime) {
		Ticks++;
		LastTickTime = glutGet( GLUT_ELAPSED_TIME );

		// Still allow ticks to procced even if sim is frozen
		if(!FreezeOn) {
			TickSimulation();
		}
	}

	// force a call to Display( ) next time it is convenient:
	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// Every MS_IN_SIMULATION_CYCLE, tick the underlying simulation
// to a new state
void
TickSimulation() {
	for(int i = 0; i < SimSize; i++) {
		for(int j = 0; j < SimSize; j++) {

			// Each infected person has a chance to infect the people touching them
			// UNLESS that person is dead or immune
			// Also, they either die or become immune after infection
			if(SimState[i][j].state == INFECTED) {
				// Up
				if(j > 0) {
					if(rand() % Transmissibility == 0) {
						if(SimState[i][j - 1].state == NONE) {
							SimState[i][j - 1].state = INFECTED;
						}
					}
				}
						
				// Down
				if(j < SimSize - 1) {
					if(rand() % Transmissibility == 0) {
						if(SimState[i][j + 1].state == NONE) {
							SimState[i][j + 1].state = INFECTED;
						}
					}
				}

				// Left
				if(i > 0) {
					if(rand() % Transmissibility == 0) {
						if(SimState[i - 1][j].state == NONE) {
							SimState[i - 1][j].state = INFECTED;
						}
					}
				}

				// Right
				if(i < SimSize - 1) {
					if(rand() % Transmissibility == 0) {
						if(SimState[i + 1][j].state == NONE) {
							SimState[i + 1][j].state = INFECTED;
						}
					}
				}

				// Small chance of dying, medium chance of becoming immune
				if(rand() % MortailityRate == 0) {
					SimState[i][j].state = DEAD;
				} else {
					if(rand() % InfectedTime == 0)
					SimState[i][j].state = IMMUNE;
				}

			// Have a chance of loosing immunity	
			} else if(SimState[i][j].state == IMMUNE) {
				if(rand() % LostImmunity == 0) {
					SimState[i][j].state = NONE;
				}
			} 
		}
	}
}

// draw the complete scene:

void
Display( )
{
	// set which window we want to do the graphics into:
	glutSetWindow( MainWindow );


	// erase the background:
	glDrawBuffer( GL_BACK );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glEnable( GL_DEPTH_TEST );

	// specify shading to be flat:
	glShadeModel( GL_FLAT );


	// set the viewport to a square centered in the window:
	GLsizei vx = glutGet( GLUT_WINDOW_WIDTH );
	GLsizei vy = glutGet( GLUT_WINDOW_HEIGHT );
	GLsizei v = vx < vy ? vx : vy;			// minimum dimension
	GLint xl = ( vx - v ) / 2;
	GLint yb = ( vy - v ) / 2;
	glViewport( xl, yb,  v, v );


	// set the viewing volume:
	// remember that the Z clipping  values are actually
	// given as DISTANCES IN FRONT OF THE EYE
	// USE gluOrtho2D( ) IF YOU ARE DOING 2D !
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	if( WhichProjection == ORTHO )
		glOrtho( -2.f, 2.f,     -2.f, 2.f,     0.1f, 1000.f );
	else
		gluPerspective( 70.f, 1.f,	0.1f, 1000.f );


	// place the objects into the scene:
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );


	// set the eye position, look-at position, and up-vector:
	gluLookAt( 100.f, 50.f, -50.f,     0.f, 0.f, 0.f,     0.f, 1.f, 0.f );

	// rotate the scene:
	glRotatef( (GLfloat)Yrot, 0.f, 1.f, 0.f );
	glRotatef( (GLfloat)Xrot, 1.f, 0.f, 0.f );


	// uniformly scale the scene:
	if( Scale < MINSCALE )
		Scale = MINSCALE;
	glScalef( (GLfloat)Scale, (GLfloat)Scale, (GLfloat)Scale );

	// No Fog
	glDisable( GL_FOG );

	// since we are using glScalef( ), be sure the normals get unitized:
	glEnable( GL_NORMALIZE );

	// Set ambient lighting
	glLightModelfv( GL_LIGHT_MODEL_AMBIENT, MulArray3( .3f, White ) );
	glLightModeli ( GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE );
	//glEnable( GL_LIGHTING );
	glShadeModel( GL_FLAT );

	// Create a point light above the scene
	SetPointLight( GL_LIGHT0,  0.f, 200.f, 0.f,  1., 1., 1. );

	glPushMatrix();
		glScalef(.03, .03, .03);	// Make humans small
		
		// Adjust so that the center of the mass of humans is at 0,0,0
		glTranslatef((float)SimSize / 2 * - 150.f, 0.f, (float)SimSize / 2 * - 150.f);
		
		// Draw 2D Display of humans
		for(int i = 0; i < SimSize; i++) {
			glPushMatrix();
				for(int j = 0; j < SimSize; j++) {
					glTranslatef(0.f, 0.f, 150.0); // Translate 100 for each y
					DrawHuman(SimState[i][j]);
				}
			glPopMatrix();
			glTranslatef(150.f, 0.f, 0.f);	// Translate 100 for each x
		}				
	glPopMatrix();

	// swap the double-buffered framebuffers:
	glutSwapBuffers( );

	// Disable lighting
	glDisable(GL_LIGHTING);

	// be sure the graphics buffer has been sent:
	// note: be sure to use glFlush( ) here, not glFinish( ) !
	glFlush( );
}


// Draw the correcct human model based on the simulation paramter
void 
DrawHuman(SimHuman human) {
	glPushMatrix();

	// Apply rotation variance
	glRotatef(human.rotation, 0.f, 1.f, 0.f);

	// Change color based on infected status
	if(human.state == DEAD) {
		glColor3f(.3f, 0.f, 0.f);
	} else if(human.state == INFECTED) {
		glColor3f(1.f, .2f, .2f);
	} else if(human.state == IMMUNE) {
		glColor3f(.6f, .7f, .4f);
	} else {
		glColor3f(1.f, 1.f, 1.f);
	}

	switch (human.model) {
		case 0:
			glCallList(Human1);
			break;
		case 1:	
			glCallList(Human2);
			break;

		case 2:
			glCallList(Human3);
			break;
	}

	glPopMatrix();
}


void
DoFreezeMenu( int id )
{
	FreezeOn = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// main menu callback:

void
DoMainMenu( int id )
{
	switch( id )
	{
		case RESET:
			Reset( );
			break;

		case QUIT:
			// gracefully close out the graphics:
			// gracefully close the graphics window:
			// gracefully exit the program:
			glutSetWindow( MainWindow );
			glFinish( );
			glutDestroyWindow( MainWindow );
			exit( 0 );
			break;

		default:
			fprintf( stderr, "Don't know what to do with Main Menu ID %d\n", id );
	}

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoProjectMenu( int id )
{
	WhichProjection = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}

void
DoSizeMenu( int id )
{
	SimSize = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}

void
DoTransmisMenu( int id )
{
	Transmissibility = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoTimeMenu( int id )
{
	InfectedTime = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoMortailityMenu( int id )
{
	MortailityRate = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoRetransmissionMenu( int id )
{
	LostImmunity = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoSpeedMenu( int id )
{
	TickTime = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// use glut to display a string of characters using a raster font:

void
DoRasterString( float x, float y, float z, char *s )
{
	glRasterPos3f( (GLfloat)x, (GLfloat)y, (GLfloat)z );

	char c;			// one character to print
	for( ; ( c = *s ) != '\0'; s++ )
	{
		glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, c );
	}
}


// use glut to display a string of characters using a stroke font:

void
DoStrokeString( float x, float y, float z, float ht, char *s )
{
	glPushMatrix( );
		glTranslatef( (GLfloat)x, (GLfloat)y, (GLfloat)z );
		float sf = ht / ( 119.05f + 33.33f );
		glScalef( (GLfloat)sf, (GLfloat)sf, (GLfloat)sf );
		char c;			// one character to print
		for( ; ( c = *s ) != '\0'; s++ )
		{
			glutStrokeCharacter( GLUT_STROKE_ROMAN, c );
		}
	glPopMatrix( );
}


// return the number of seconds since the start of the program:

float
ElapsedSeconds( )
{
	// get # of milliseconds since the start of the program:

	int ms = glutGet( GLUT_ELAPSED_TIME );

	// convert it to seconds:

	return (float)ms / 1000.f;
}


// initialize the glui window:

void
InitMenus( )
{
	glutSetWindow( MainWindow );

	int freezemenu = glutCreateMenu( DoFreezeMenu );
	glutAddMenuEntry( "Off",  0 );
	glutAddMenuEntry( "On",   1 );

	int projmenu = glutCreateMenu( DoProjectMenu );
	glutAddMenuEntry( "Orthographic",  ORTHO );
	glutAddMenuEntry( "Perspective",   PERSP );

	int sizemenu = glutCreateMenu( DoSizeMenu );
	glutAddMenuEntry( "25",  25 );
	glutAddMenuEntry( "50",   50 );
	glutAddMenuEntry( "100",   100 );

	int mortalitymenu = glutCreateMenu( DoMortailityMenu );
	glutAddMenuEntry( "Less", 6);
	glutAddMenuEntry( "Standard", 5);
	glutAddMenuEntry( "More", 4);
	glutAddMenuEntry( "Max", 3);

	int transmismenu = glutCreateMenu( DoTransmisMenu );
	glutAddMenuEntry( "Less", 4);
	glutAddMenuEntry( "Standard", 3);
	glutAddMenuEntry( "More", 2);
	glutAddMenuEntry( "Max", 1);

	int retransmissionmenu = glutCreateMenu( DoRetransmissionMenu );
	glutAddMenuEntry( "Low", 25);
	glutAddMenuEntry( "Standard", 20);
	glutAddMenuEntry( "High", 15);
	glutAddMenuEntry( "Max", 10);

	int timemenu = glutCreateMenu( DoTimeMenu );
	glutAddMenuEntry( "Short", 1);
	glutAddMenuEntry( "Standard", 3);
	glutAddMenuEntry( "Long", 5);
	glutAddMenuEntry( "Longest", 10);

	int speedmenu = glutCreateMenu( DoSpeedMenu );
	glutAddMenuEntry( "Faster", 1000);
	glutAddMenuEntry( "Standard", 3000);
	glutAddMenuEntry( "Slower", 5000);

	int mainmenu = glutCreateMenu( DoMainMenu );
	glutAddSubMenu(   "Freeze",        freezemenu);
	glutAddSubMenu(   "Projection",    projmenu );
	glutAddSubMenu(	  "Size",		   sizemenu);
	glutAddSubMenu(	  "Transmissability",  transmismenu);
	glutAddSubMenu(   "Infected Time",  timemenu );
	glutAddSubMenu(	  "Waning Immunity",  retransmissionmenu);
	glutAddSubMenu(	  "Mortaility",		   mortalitymenu);
	glutAddSubMenu(   "Simulation Speed",  speedmenu );
	glutAddMenuEntry( "Reset",		   RESET);
	glutAddMenuEntry( "Quit",          QUIT );

// attach the pop-up menu to the right mouse button:

	glutAttachMenu( GLUT_RIGHT_BUTTON );
}



// initialize the glut and OpenGL libraries:
//	also setup callback functions

void
InitGraphics( )
{
	// request the display modes:
	// ask for red-green-blue-alpha color, double-buffering, and z-buffering:

	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );

	// set the initial window configuration:

	glutInitWindowPosition( 0, 0 );
	glutInitWindowSize( INIT_WINDOW_SIZE, INIT_WINDOW_SIZE );

	// open the window and set its title:

	MainWindow = glutCreateWindow( WINDOWTITLE );
	glutSetWindowTitle( WINDOWTITLE );

	// set the framebuffer clear values:

	glClearColor( BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3] );

	// setup the callback functions:
	// DisplayFunc -- redraw the window
	// ReshapeFunc -- handle the user resizing the window
	// KeyboardFunc -- handle a keyboard input
	// MouseFunc -- handle the mouse button going down or up
	// MotionFunc -- handle the mouse moving with a button down
	// PassiveMotionFunc -- handle the mouse moving with a button up
	// VisibilityFunc -- handle a change in window visibility
	// EntryFunc	-- handle the cursor entering or leaving the window
	// SpecialFunc -- handle special keys on the keyboard
	// SpaceballMotionFunc -- handle spaceball translation
	// SpaceballRotateFunc -- handle spaceball rotation
	// SpaceballButtonFunc -- handle spaceball button hits
	// ButtonBoxFunc -- handle button box hits
	// DialsFunc -- handle dial rotations
	// TabletMotionFunc -- handle digitizing tablet motion
	// TabletButtonFunc -- handle digitizing tablet button hits
	// MenuStateFunc -- declare when a pop-up menu is in use
	// TimerFunc -- trigger something to happen a certain time from now
	// IdleFunc -- what to do when nothing else is going on

	glutSetWindow( MainWindow );
	glutDisplayFunc( Display );
	glutReshapeFunc( Resize );
	glutKeyboardFunc( Keyboard );
	glutMouseFunc( MouseButton );
	glutMotionFunc( MouseMotion );
	glutPassiveMotionFunc(MouseMotion);
	//glutPassiveMotionFunc( NULL );
	glutVisibilityFunc( Visibility );
	glutEntryFunc( NULL );
	glutSpecialFunc( NULL );
	glutSpaceballMotionFunc( NULL );
	glutSpaceballRotateFunc( NULL );
	glutSpaceballButtonFunc( NULL );
	glutButtonBoxFunc( NULL );
	glutDialsFunc( NULL );
	glutTabletMotionFunc( NULL );
	glutTabletButtonFunc( NULL );
	glutMenuStateFunc( NULL );
	glutTimerFunc( -1, NULL, 0 );

	// setup glut to call Simulate( ) every time it has
	// 	nothing it needs to respond to (which is most of the time)
	// we don't need to do this for this program, and really should set the argument to NULL
	// but, this sets us up nicely for doing animation

	glutIdleFunc( Simulate );

	// init the glew package (a window must be open to do this):

#ifdef WIN32
	GLenum err = glewInit( );
	if( err != GLEW_OK )
	{
		fprintf( stderr, "glewInit Error\n" );
	}
	else
		fprintf( stderr, "GLEW initialized OK\n" );
	fprintf( stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

	// Seed rand function
	srand(time(NULL));

	// Set some initial values
	ActiveButton = 0;
	Scale  = 1.0;
	WhichProjection = PERSP;
	Xrot = Yrot = 0.;
	FreezeOn = 0;
	SimSize = 25;
	Transmissibility = 3;
	MortailityRate = 5;
	LostImmunity = 20;
	InfectedTime = 3;
	TickTime = 3000;
	InfectedTime = 3;

	InitSimulation();

}


// Set initial model and rotation for all simulated humans
void
InitSimulation( ) {
	for(int i = 0; i < MAX_HUMANS; i++) {
		for(int j = 0; j < MAX_HUMANS; j++) {
			SimState[i][j].model = rand() % 3;			// Random number 0 - 2
			SimState[i][j].rotation = rand() % 361;		// Random number 0 - 90

			// Random chance some humans into infection and immunity
			if(rand() % INITIAL_INFECTED == 0) {
				SimState[i][j].state = INFECTED;
			} else if (rand() % INITIAL_IMMUNITY == 0) {
				SimState[i][j].state = IMMUNE;
			} else {
				SimState[i][j].state = NONE;
			}
		}
	}
}


// Create display lists for things that do not change
void
InitLists( )
{
	glutSetWindow( MainWindow );

	// Create the display list for the sphere
	SphereList = glGenLists(1);
	glNewList( SphereList, GL_COMPILE );

	glPushMatrix();
	glNewList(SphereList, GL_COMPILE);
	glColor3f(1., 1., 1.);
	OsuSphere(.5f, 20.f, 10.f);
	glPopMatrix();

	glEndList();

	// Create the display list for the humans from the obj files
	Human1 = glGenLists( 1 );
	glNewList( Human1, GL_COMPILE );
	LoadObjFile( "low_poly_male.obj" );
	glEndList( );

	Human2 = glGenLists( 1 );
	glNewList( Human2, GL_COMPILE );
	glPushMatrix();
	glScalef(35.f, 35.f, 35.f);
	LoadObjFile( "low_poly_male_2.obj" );
	glPopMatrix();
	glEndList( );

	Human3 = glGenLists( 1 );
	glNewList( Human3, GL_COMPILE );
	glPushMatrix();
	glScalef(1.5f, 1.5f, 1.5f);
	LoadObjFile( "low_poly_male_3.obj" );
	glPopMatrix();
	glEndList( );
}


// the keyboard callback:

void
Keyboard( unsigned char c, int x, int y )
{
	if( DebugOn != 0 )
		fprintf( stderr, "Keyboard: '%c' (0x%0x)\n", c, c );

	switch( c )
	{
		case 'o':
		case 'O':
			WhichProjection = ORTHO;
			break;

		case 'q':
		case 'Q':
		case ESCAPE:
			DoMainMenu( QUIT );	// will not return here
			break;				// happy compiler

		case 'f':
		case 'F':
			FreezeOn = !FreezeOn;
			break;

		case 'r':
		case 'R':
			Reset();
			break;

		default:
			fprintf( stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c );
	}

	// force a call to Display( ):

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// called when the mouse button transitions down or up:

void
MouseButton( int button, int state, int x, int y )
{
	int b = 0;			// LEFT, MIDDLE, or RIGHT

	if( DebugOn != 0 )
		fprintf( stderr, "MouseButton: %d, %d, %d, %d\n", button, state, x, y );

	
	// get the proper button bit mask:

	switch( button )
	{
		case GLUT_LEFT_BUTTON:
			b = LEFT;		break;

		case GLUT_MIDDLE_BUTTON:
			b = MIDDLE;		break;

		case GLUT_RIGHT_BUTTON:
			b = RIGHT;		break;

		case SCROLL_WHEEL_UP:
			Scale += SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
			// keep object from turning inside-out or disappearing:
			if (Scale < MINSCALE)
				Scale = MINSCALE;
			break;

		case SCROLL_WHEEL_DOWN:
			Scale -= SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
			// keep object from turning inside-out or disappearing:
			if (Scale < MINSCALE)
				Scale = MINSCALE;
			break;

		default:
			b = 0;
			fprintf( stderr, "Unknown mouse button: %d\n", button );
	}

	// button down sets the bit, up clears the bit:

	if( state == GLUT_DOWN )
	{
		Xmouse = x;
		Ymouse = y;
		ActiveButton |= b;		// set the proper bit
	}
	else
	{
		ActiveButton &= ~b;		// clear the proper bit
	}

	glutSetWindow(MainWindow);
	glutPostRedisplay();

}


// called when the mouse moves while a button is down:

void
MouseMotion( int x, int y )
{
	int dx = x - Xmouse;		// change in mouse coords
	int dy = y - Ymouse;

	if( ( ActiveButton & LEFT ) != 0 )
	{
		Xrot += ( ANGFACT*dy );
		Yrot += ( ANGFACT*dx );
	}

	if( ( ActiveButton & MIDDLE ) != 0 )
	{
		Scale += SCLFACT * (float) ( dx - dy );

		// keep object from turning inside-out or disappearing:

		if( Scale < MINSCALE )
			Scale = MINSCALE;
	}

	Xmouse = x;			// new current position
	Ymouse = y;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// reset the transformations and the colors:
// this only sets the global variables --
// the glut main loop is responsible for redrawing the scene

void
Reset( )
{
	// Reset the simulation
	InitSimulation();
}


// called when user resizes the window:

void
Resize( int width, int height )
{
	// don't really need to do anything since window size is
	// checked each time in Display( ):

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// handle a change to the window's visibility:

void
Visibility ( int state )
{
	if( DebugOn != 0 )
		fprintf( stderr, "Visibility: %d\n", state );

	if( state == GLUT_VISIBLE )
	{
		glutSetWindow( MainWindow );
		glutPostRedisplay( );
	}
	else
	{
		// could optimize by keeping track of the fact
		// that the window is not visible and avoid
		// animating or redrawing it ...
	}
}

///////////////////////////////////////   HANDY UTILITIES:  //////////////////////////


// the stroke characters 'X' 'Y' 'Z' :

static float xx[ ] = { 0.f, 1.f, 0.f, 1.f };

static float xy[ ] = { -.5f, .5f, .5f, -.5f };

static int xorder[ ] = { 1, 2, -3, 4 };

static float yx[ ] = { 0.f, 0.f, -.5f, .5f };

static float yy[ ] = { 0.f, .6f, 1.f, 1.f };

static int yorder[ ] = { 1, 2, 3, -2, 4 };

static float zx[ ] = { 1.f, 0.f, 1.f, 0.f, .25f, .75f };

static float zy[ ] = { .5f, .5f, -.5f, -.5f, 0.f, 0.f };

static int zorder[ ] = { 1, 2, 3, 4, -5, 6 };

// fraction of the length to use as height of the characters:
const float LENFRAC = 0.10f;

// fraction of length to use as start location of the characters:
const float BASEFRAC = 1.10f;

// read a BMP file into a Texture:

#define VERBOSE				false
#define BMP_MAGIC_NUMBER	0x4d42
#ifndef BI_RGB
#define BI_RGB				0
#define BI_RLE8				1
#define BI_RLE4				2
#endif


// bmp file header:
struct bmfh
{
	short bfType;		// BMP_MAGIC_NUMBER = "BM"
	int bfSize;		// size of this file in bytes
	short bfReserved1;
	short bfReserved2;
	int bfOffBytes;		// # bytes to get to the start of the per-pixel data
} FileHeader;

// bmp info header:
struct bmih
{
	int biSize;		// info header size, should be 40
	int biWidth;		// image width
	int biHeight;		// image height
	short biPlanes;		// #color planes, should be 1
	short biBitCount;	// #bits/pixel, should be 1, 4, 8, 16, 24, 32
	int biCompression;	// BI_RGB, BI_RLE4, BI_RLE8
	int biSizeImage;
	int biXPixelsPerMeter;
	int biYPixelsPerMeter;
	int biClrUsed;		// # colors in the palette
	int biClrImportant;
} InfoHeader;



// read a BMP file into a Texture:

unsigned char *
BmpToTexture( char *filename, int *width, int *height )
{
	FILE *fp;
#ifdef _WIN32
        errno_t err = fopen_s( &fp, filename, "rb" );
        if( err != 0 )
        {
		fprintf( stderr, "Cannot open Bmp file '%s'\n", filename );
		return NULL;
        }
#else
		fp = fopen( filename, "rb" );
		if( fp == NULL )
		{
			fprintf( stderr, "Cannot open Bmp file '%s'\n", filename );
			return NULL;
		}
#endif

	FileHeader.bfType = ReadShort( fp );


	// if bfType is not BMP_MAGIC_NUMBER, the file is not a bmp:

	if( VERBOSE ) fprintf( stderr, "FileHeader.bfType = 0x%0x = \"%c%c\"\n",
			FileHeader.bfType, FileHeader.bfType&0xff, (FileHeader.bfType>>8)&0xff );
	if( FileHeader.bfType != BMP_MAGIC_NUMBER )
	{
		fprintf( stderr, "Wrong type of file: 0x%0x\n", FileHeader.bfType );
		fclose( fp );
		return NULL;
	}


	FileHeader.bfSize = ReadInt( fp );
	if( VERBOSE )	fprintf( stderr, "FileHeader.bfSize = %d\n", FileHeader.bfSize );

	FileHeader.bfReserved1 = ReadShort( fp );
	FileHeader.bfReserved2 = ReadShort( fp );

	FileHeader.bfOffBytes = ReadInt( fp );


	InfoHeader.biSize = ReadInt( fp );
	InfoHeader.biWidth = ReadInt( fp );
	InfoHeader.biHeight = ReadInt( fp );

	const int nums = InfoHeader.biWidth;
	const int numt = InfoHeader.biHeight;

	InfoHeader.biPlanes = ReadShort( fp );

	InfoHeader.biBitCount = ReadShort( fp );
	if( VERBOSE )	fprintf( stderr, "InfoHeader.biBitCount = %d\n", InfoHeader.biBitCount );

	InfoHeader.biCompression = ReadInt( fp );
	if( VERBOSE )	fprintf( stderr, "InfoHeader.biCompression = %d\n", InfoHeader.biCompression );

	InfoHeader.biSizeImage = ReadInt( fp );
	if( VERBOSE )	fprintf( stderr, "InfoHeader.biSizeImage = %d\n", InfoHeader.biSizeImage );

	InfoHeader.biXPixelsPerMeter = ReadInt( fp );
	InfoHeader.biYPixelsPerMeter = ReadInt( fp );

	InfoHeader.biClrUsed = ReadInt( fp );
	if( VERBOSE )	fprintf( stderr, "InfoHeader.biClrUsed = %d\n", InfoHeader.biClrUsed );

	InfoHeader.biClrImportant = ReadInt( fp );

	// fprintf( stderr, "Image size found: %d x %d\n", ImageWidth, ImageHeight );

	// pixels will be stored bottom-to-top, left-to-right:
	unsigned char *texture = new unsigned char[ 3 * nums * numt ];
	if( texture == NULL )
	{
		fprintf( stderr, "Cannot allocate the texture array!\n" );
		return NULL;
	}

	// extra padding bytes:

	int requiredRowSizeInBytes = 4 * ( ( InfoHeader.biBitCount*InfoHeader.biWidth + 31 ) / 32 );
	if( VERBOSE )	fprintf( stderr, "requiredRowSizeInBytes = %d\n", requiredRowSizeInBytes );

	int myRowSizeInBytes = ( InfoHeader.biBitCount*InfoHeader.biWidth + 7 ) / 8;
	if( VERBOSE )	fprintf( stderr, "myRowSizeInBytes = %d\n", myRowSizeInBytes );

	int numExtra = requiredRowSizeInBytes - myRowSizeInBytes;
	if( VERBOSE )	fprintf( stderr, "New NumExtra padding = %d\n", numExtra );


	// this function does not support compression:

	if( InfoHeader.biCompression != 0 )
	{
		fprintf( stderr, "Wrong type of image compression: %d\n", InfoHeader.biCompression );
		fclose( fp );
		return NULL;
	}
	
	// we can handle 24 bits of direct color:
	if( InfoHeader.biBitCount == 24 )
	{
		rewind( fp );
		fseek( fp, FileHeader.bfOffBytes, SEEK_SET );
		int t;
		unsigned char *tp;
		for( t = 0, tp = texture; t < numt; t++ )
		{
			for( int s = 0; s < nums; s++, tp += 3 )
			{
				*(tp+2) = fgetc( fp );		// b
				*(tp+1) = fgetc( fp );		// g
				*(tp+0) = fgetc( fp );		// r
			}

			for( int e = 0; e < numExtra; e++ )
			{
				fgetc( fp );
			}
		}
	}

	// we can also handle 8 bits of indirect color:
	if( InfoHeader.biBitCount == 8 && InfoHeader.biClrUsed == 256 )
	{
		struct rgba32
		{
			unsigned char r, g, b, a;
		};
		struct rgba32 *colorTable = new struct rgba32[ InfoHeader.biClrUsed ];

		rewind( fp );
		fseek( fp, sizeof(struct bmfh) + InfoHeader.biSize - 2, SEEK_SET );
		for( int c = 0; c < InfoHeader.biClrUsed; c++ )
		{
			colorTable[c].r = fgetc( fp );
			colorTable[c].g = fgetc( fp );
			colorTable[c].b = fgetc( fp );
			colorTable[c].a = fgetc( fp );
			if( VERBOSE )	fprintf( stderr, "%4d:\t0x%02x\t0x%02x\t0x%02x\t0x%02x\n",
				c, colorTable[c].r, colorTable[c].g, colorTable[c].b, colorTable[c].a );
		}

		rewind( fp );
		fseek( fp, FileHeader.bfOffBytes, SEEK_SET );
		int t;
		unsigned char *tp;
		for( t = 0, tp = texture; t < numt; t++ )
		{
			for( int s = 0; s < nums; s++, tp += 3 )
			{
				int index = fgetc( fp );
				*(tp+0) = colorTable[index].r;	// r
				*(tp+1) = colorTable[index].g;	// g
				*(tp+2) = colorTable[index].b;	// b
			}

			for( int e = 0; e < numExtra; e++ )
			{
				fgetc( fp );
			}
		}

		delete[ ] colorTable;
	}

	fclose( fp );

	*width = nums;
	*height = numt;
	return texture;
}

int
ReadInt( FILE *fp )
{
	const unsigned char b0 = fgetc( fp );
	const unsigned char b1 = fgetc( fp );
	const unsigned char b2 = fgetc( fp );
	const unsigned char b3 = fgetc( fp );
	return ( b3 << 24 )  |  ( b2 << 16 )  |  ( b1 << 8 )  |  b0;
}

short
ReadShort( FILE *fp )
{
	const unsigned char b0 = fgetc( fp );
	const unsigned char b1 = fgetc( fp );
	return ( b1 << 8 )  |  b0;
}


// function to convert HSV to RGB
// 0.  <=  s, v, r, g, b  <=  1.
// 0.  <= h  <=  360.
// when this returns, call:
//		glColor3fv( rgb );

void
HsvRgb( float hsv[3], float rgb[3] )
{
	// guarantee valid input:

	float h = hsv[0] / 60.f;
	while( h >= 6. )	h -= 6.;
	while( h <  0. ) 	h += 6.;

	float s = hsv[1];
	if( s < 0. )
		s = 0.;
	if( s > 1. )
		s = 1.;

	float v = hsv[2];
	if( v < 0. )
		v = 0.;
	if( v > 1. )
		v = 1.;

	// if sat==0, then is a gray:

	if( s == 0.0 )
	{
		rgb[0] = rgb[1] = rgb[2] = v;
		return;
	}

	// get an rgb from the hue itself:
	
	float i = (float)floor( h );
	float f = h - i;
	float p = v * ( 1.f - s );
	float q = v * ( 1.f - s*f );
	float t = v * ( 1.f - ( s * (1.f-f) ) );

	float r=0., g=0., b=0.;			// red, green, blue
	switch( (int) i )
	{
		case 0:
			r = v;	g = t;	b = p;
			break;
	
		case 1:
			r = q;	g = v;	b = p;
			break;
	
		case 2:
			r = p;	g = v;	b = t;
			break;
	
		case 3:
			r = p;	g = q;	b = v;
			break;
	
		case 4:
			r = t;	g = p;	b = v;
			break;
	
		case 5:
			r = v;	g = p;	b = q;
			break;
	}


	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

void
SetMaterial( float r, float g, float b,  float shininess )
{
	glMaterialfv( GL_BACK, GL_EMISSION, Array3( 0., 0., 0. ) );
	glMaterialfv( GL_BACK, GL_AMBIENT, MulArray3( .4f, White ) );
	glMaterialfv( GL_BACK, GL_DIFFUSE, MulArray3( 1., White ) );
	glMaterialfv( GL_BACK, GL_SPECULAR, Array3( 0., 0., 0. ) );
	glMaterialf ( GL_BACK, GL_SHININESS, 5.f );

	glMaterialfv( GL_FRONT, GL_EMISSION, Array3( 0., 0., 0. ) );
	glMaterialfv( GL_FRONT, GL_AMBIENT, Array3( r, g, b ) );
	glMaterialfv( GL_FRONT, GL_DIFFUSE, Array3( r, g, b ) );
	glMaterialfv( GL_FRONT, GL_SPECULAR, MulArray3( .8f, White ) );
	glMaterialf ( GL_FRONT, GL_SHININESS, shininess );
}


void
SetPointLight( int ilight, float x, float y, float z,  float r, float g, float b )
{
	glLightfv( ilight, GL_POSITION,  Array3( x, y, z ) );
	glLightfv( ilight, GL_AMBIENT,   Array3( 0., 0., 0. ) );
	glLightfv( ilight, GL_DIFFUSE,   Array3( r, g, b ) );
	glLightfv( ilight, GL_SPECULAR,  Array3( r, g, b ) );
	glLightf ( ilight, GL_CONSTANT_ATTENUATION, 1.f );
	glLightf ( ilight, GL_LINEAR_ATTENUATION, 0. );
	glLightf ( ilight, GL_QUADRATIC_ATTENUATION, 0. );
	glEnable( ilight );
}


void
SetSpotLight( int ilight, float x, float y, float z, float xdir, float ydir, float zdir, float r, float g, float b )
{
	glLightfv( ilight, GL_POSITION,  Array3( x, y, z ) );
	glLightfv( ilight, GL_SPOT_DIRECTION,  Array3(xdir,ydir,zdir) );
	glLightf(  ilight, GL_SPOT_EXPONENT, 1. );
	glLightf(  ilight, GL_SPOT_CUTOFF, 45. );
	glLightfv( ilight, GL_AMBIENT,   Array3( 0., 0., 0. ) );
	glLightfv( ilight, GL_DIFFUSE,   Array3( r, g, b ) );
	glLightfv( ilight, GL_SPECULAR,  Array3( r, g, b ) );
	glLightf ( ilight, GL_CONSTANT_ATTENUATION, 1. );
	glLightf ( ilight, GL_LINEAR_ATTENUATION, 0. );
	glLightf ( ilight, GL_QUADRATIC_ATTENUATION, 0. );
	glEnable( ilight );
}


float *
Array3( float a, float b, float c )
{
	static float array[4];
	
	array[0] = a;
	array[1] = b;
	array[2] = c;
	array[3] = 1.;
	return array;
}

float *
MulArray3( float factor, float array0[3] )
{
        static float array[4];

        array[0] = factor * array0[0];
        array[1] = factor * array0[1];
        array[2] = factor * array0[2];
        array[3] = 1.;
        return array;
}
