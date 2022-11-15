#version 120

// From host program
uniform float   uKa, uKd, uKs;		// coefficients of each type of lighting -- make sum to 1.0
uniform vec3    uColor;				// object color
uniform vec3    uSpecularColor;		// light color
uniform float   uShininess;			// specular exponent
uniform float	uOuterRadius;		// Outer radius of the torus
uniform float	uFragAnimate;		// 0-5 value used to animate 

// From vertex shader
varying  vec2  vST;		   			// texture coords
varying  vec3  vN;					// normal vector
varying  vec3  vL;					// vector from point to light
varying  vec3  vE;					// vector from point to eye


void
main( )
{
	vec3 Normal = normalize(vN);
	vec3 Light = normalize(vL);
	vec3 Eye = normalize(vE);

	vec3 myColor = uColor;
	if( mod(floor(vST.y * uOuterRadius * uFragAnimate), 2.) == 0) {
		myColor = vec3(1., .3, .5);
	}

	vec3 ambient = uKa * myColor;

	float d = max( dot(Normal,Light), 0. );       // only do diffuse if the light can see the point
	vec3 diffuse = uKd * d * myColor;

	float s = 0.;
	if( dot(Normal,Light) > 0. )	          // only do specular if the light can see the point
	{
		vec3 ref = normalize(  reflect( -Light, Normal )  );
		s = pow( max( dot(Eye,ref),0. ), uShininess );
	}
	vec3 specular = uKs * s * uSpecularColor;
	gl_FragColor = vec4( ambient + diffuse + specular,  1. );
}
