#version 400

// Attributes
layout(location = 0) in vec3 v;///< Position.
layout(location = 2) in vec2 uv;///< Uv coordinates.

uniform float ratio = 1.0f; ///< The screen aspect ratio.
uniform vec2 position; ///< The position of the anchor point on screen.

out INTERFACE {
	vec2 uv; ///< Texture coordinates.
} Out ;


/** Compute the 2D position of the glyph on screen. */
void main(){
	gl_Position.xy = position + v.xy * vec2(ratio, 1.0);
	gl_Position.zw = vec2(1.0);
	Out.uv = uv ;
}
