#version 330

/// Input: texture coordinates.
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D texture0; ///< Color texture.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Texture each face. */
void main(){
	fragColor = texture(texture0, In.uv);
}