#version 400

#include "atmosphere.glsl"

in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

uniform mat4 clipToWorld; ///< Clip-to-world space transformation matrix.
uniform vec3 viewPos; ///< The position in view space.
uniform vec3 lightDirection; ///< The light direction in world space.

layout(binding = 0) uniform sampler2D precomputedScattering; ///< Secondary scattering lookup table.

layout(location = 0) out vec3 fragColor; ///< Atmosphere color.

/** Simulate sky color based on an atmospheric scattering approximate model. */
void main(){
	// Move to -1,1
	vec4 clipVertex = vec4(-1.0+2.0*In.uv, 0.0, 1.0);
	// Then to world space.
	vec3 viewRay = normalize((clipToWorld * clipVertex).xyz);
	// We then move to the planet model space, where its center is in (0,0,0).
	vec3 planetSpaceViewPos = viewPos + vec3(0,atmosphereGroundRadius,0) + vec3(0.0,1.0,0.0);
	vec3 atmosphereColor = computeAtmosphereRadiance(planetSpaceViewPos, viewRay, lightDirection, defaultSunColor, precomputedScattering);
	fragColor = atmosphereColor;
}

