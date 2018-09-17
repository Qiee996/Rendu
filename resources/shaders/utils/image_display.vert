#version 330

// Output: UV coordinates
out INTERFACE {
	vec2 uv;
} Out ; ///< vec2 uv;

/**
 Generate one triangle covering the whole screen
 2: (-1,3),(0,2)
 *
 | \
 |	 \
 |	   \
 |		 \
 |		   \
 *-----------*  1: (3,-1), (2,0)
 0: (-1,-1), (0,0)
 */
void main(){
	vec2 temp = 2.0 * vec2(gl_VertexID == 1, gl_VertexID == 2);
	gl_Position.xy = 2.0 * temp - 1.0;
	gl_Position.zw = vec2(1.0);
	Out.uv = temp;
}