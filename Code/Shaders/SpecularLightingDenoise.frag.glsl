#version 450

layout (location = 0) out vec4 FragSpecular;
layout (location = 1) out vec3 FragColor;

layout (location = 0) in vec2 TexCoords;
layout (location = 1) in vec3 CameraVector;

layout (set = 0, binding = 0) uniform CameraBuffer
{
	mat4 View;
	mat4 Proj;
	mat4 ProjUnjittered;

	mat4 PrevView;
	mat4 PrevProj;
	
	vec4 CameraPosition; 
    vec4 Viewport; // Width, Height, Near, Far
	vec4 Frustum[6];
    vec4 FrustumCorners[6];
};

layout (set = 0, binding = 1) uniform sampler2D CurrentTexture;
layout (set = 0, binding = 2) uniform sampler2D HistoryTexture;
layout (set = 0, binding = 3) uniform sampler2D VelocityTexture;
layout (set = 0, binding = 4) uniform sampler2D NormalsTexture;
layout (set = 0, binding = 5) uniform sampler2D MaterialTexture;
layout (set = 0, binding = 6) uniform sampler2D DiffuseTexture;
layout (set = 0, binding = 7) uniform sampler2D AlbedoTexture;

void main()
{
	const vec2 Velocity = texture(VelocityTexture, TexCoords).rg;
	const vec3 Normal = texture(NormalsTexture, TexCoords).xyz;
	const vec2 Material = texture(MaterialTexture, TexCoords).rg;

	if (dot(Normal, Normal) == 0.0)
	{
		FragSpecular = vec4(0.0);
	}
	else
	{
    	const vec2 HistoryTexCoords = TexCoords - Velocity;

		float Roughness = Material.g;

		vec4 SpecularNew = texture(CurrentTexture, TexCoords);
		vec4 SpecularOld = texture(HistoryTexture, HistoryTexCoords);

		float BlendFactor = clamp(Roughness, 0.0, 0.8);
		BlendFactor *= clamp((1.0 - 10.0 * abs(SpecularOld.a - SpecularNew.a)), 0.0, 1.0);

		FragSpecular = mix(SpecularNew, SpecularOld, BlendFactor);
	}

	vec3 AlbedoColor = texture(AlbedoTexture, TexCoords).rgb;
	vec3 Diffuse = texture(DiffuseTexture, TexCoords).rgb;

	vec3 Specular = FragSpecular.rgb;
	float Reflectivity = FragSpecular.a;

	float Metallic = 0.8;
	Specular = mix(Specular, Specular * AlbedoColor, Metallic);

	FragColor = mix(Diffuse, Specular, Reflectivity);
}