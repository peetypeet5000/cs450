#version 120

uniform float	uA;		// A
uniform float	uB;		// B

varying  vec3  vN;		// normal vector
varying  vec2  vST;	    // texture coords
varying  vec3  vL;		// vector from point to light
varying  vec3  vE;		// vector from point to eye

vec3 LightPosition = vec3(  0., 5., 5. );

void
main( )
{ 
	vST = gl_MultiTexCoord0.st;
	vec3 vert = gl_Vertex.xyz;
	//<< change vert to perform vertex distortion >>
	vec4 ECposition = gl_ModelViewMatrix * vec4( vert, 1. );
	vN = normalize( gl_NormalMatrix * gl_Normal );	// normal vector
	vL = LightPosition - ECposition.xyz;		// vector from the point
							// to the light position
	vE = vec3( 0., 0., 0. ) - ECposition.xyz;	// vector from the point
							// to the eye position 
	gl_Position = gl_ModelViewProjectionMatrix * vec4( vert, 1. );
}