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

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	vec3 position;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

in vec2 vTexCoord;

layout(location = 0) out vec4 oColor;

layout(location = 0) uniform sampler2D colColor;
layout(location = 1) uniform sampler2D posColor;
layout(location = 2) uniform sampler2D norColor;

void main()
{
	vec3 position = texture(posColor, vTexCoord).rgb;
	vec3 norm = texture(norColor, vTexCoord).rgb;

	vec3 result = vec3(0.0);

	for(int i = 0; i < uLightCount; i++)
	{
		if(uLight[i].type == 0)
		{
			float diff = max(dot(norm, uLight[i].direction), 0.0);
			vec3 diffuse = diff * uLight[i].color;
			result += diffuse;
		}
		if(uLight[i].type == 1)
		{
			vec3 lightDir = normalize(uLight[i].position - position);
			float diff = max(dot(norm, lightDir), 0.0);
			vec3 diffuse = diff * uLight[i].color;
			float distance = length(uLight[i].position - position);
			float attenuation = 1.0 / (distance * distance);
			attenuation *= 2;
			diffuse *= attenuation;
			result += diffuse;
		}
	}

	oColor = vec4(result * texture(colColor, vTexCoord).rgb, 1.0);;
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
