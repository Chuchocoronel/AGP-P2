///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition.x, aPosition.y, 0.0, 1.0);
}

#elif defined(FRAGMENT) /////////////////////////////////////////////// 

in vec2 vTexCoord;

uniform sampler2D uTexture;

uniform int isDepth;

layout(location = 0) out vec4 oColor;

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * 0.1 * 1000.0) / (1000.0 + 0.1 - z * (1000.0 - 0.1));	
}

void main()
{
	if (isDepth == 1)
	{
		float depth = LinearizeDepth(texture(uTexture, vTexCoord).r) / 1000.0;
		oColor = vec4(vec3(depth), 1);
	}
	else
		oColor = texture(uTexture, vTexCoord);

}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
