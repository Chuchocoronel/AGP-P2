///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
//layout(location = 3) in vec3 aTangent;
//layout(location = 4) in vec3 aBitangent;

//uniform mat4 proj;
//uniform mat4 view;

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

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition; // In worldSpace
out vec3 vNormal; // In worldSpace
out vec3 vViewDir;

void main()
{
	//vTexCoord = aTexCoord;
	//gl_Position = vec4(aPosition, 1.0);

	//float clippingScale = 10.0;

	//gl_Position = proj * view * vec4(aPosition, clippingScale);

	//gl_Position.z = -gl_Position.z;

	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir = uCameraPosition - vPosition;
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
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

// TODO: Write your fragment shader here
in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 posColor;
layout(location = 2) out vec4 norColor;
layout(location = 3) out vec4 depColor;

void main()
{
	vec3 result = vec3(0.0);

	vec3 norm = normalize(vNormal);

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
			vec3 lightDir = normalize(uLight[i].position - vPosition);
			float diff = max(dot(norm, lightDir), 0.0);
			vec3 diffuse = diff * uLight[i].color;
			float distance = length(uLight[i].position - vPosition);
			float attenuation = 1.0 / (distance * distance);
			attenuation *= 2;
			diffuse *= attenuation;
			result += diffuse;
		}
	}

	oColor = vec4(result * texture(uTexture, vTexCoord).rgb, 1.0);
	posColor = vec4(vPosition, 1.0);
	norColor = vec4(norm, 1.0);
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
