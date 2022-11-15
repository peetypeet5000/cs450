#version 120

uniform float	uVertDistort;		// True if distortion is on
uniform float	uAnimate;			// 0-10 value used to animate 

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

	// Apply our distortion
	vert.x = vert.x + abs(sin(pow(vert.x, 4) + uAnimate));
	vert.y = vert.y + abs(sin(pow(vert.y, 4) + uAnimate));

	vec4 ECposition = gl_ModelViewMatrix * vec4( vert, 1. );
	vN = normalize( gl_NormalMatrix * gl_Normal );	// normal vector
	vL = LightPosition - ECposition.xyz;		// vector from the point
							// to the light position
	vE = vec3( 0., 0., 0. ) - ECposition.xyz;	// vector from the point
							// to the eye position 
	gl_Position = gl_ModelViewProjectionMatrix * vec4( vert, 1. );
}