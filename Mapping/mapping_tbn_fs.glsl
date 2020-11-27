#version 430

layout(location = 5) uniform sampler2D diffuse_map;
layout(location = 6) uniform sampler2D normal_map;
layout(location = 7) uniform sampler2D bump_map;
layout(location = 8) uniform samplerCube env_map;
layout(location = 9) uniform float slider;
layout(location = 10) uniform int mode;
layout(location = 11) uniform float tex_scale = 1.0f;

out vec4 fragcolor;           
in vec2 tex_coord;
in vec3 normal;		//world-space normal
in vec3 tangent;	//world-space tangent
in vec3 bitangent;	//world-space bitangent
in vec3 p;			//World-space fragment position
in vec3 eye;		//World-space eye position
in mat3 TBN;

//light and material colors
const vec4 La = vec4(0.3, 0.3, 0.3, 1.0);
const vec4 kd = vec4(0.8);
const vec4 Ld = vec4(1.0);
const vec4 ks = vec4(0.25);
const vec4 Ls = vec4(1.0);

const vec3 l = vec3(0.0, 0.707, 0.707); //world space directional light

vec4 lighting();
vec4 environment_mapping();
vec4 environment_mapped_bump_mapping();

vec3 UnpackUnitNormal(vec3 n)
{
   return normalize(2.0*n - vec3(1.0, 1.0, 1.0));
}

void main(void)
{   
	if(mode == 0)	
	{
		fragcolor = vec4(abs(normalize(normal)), 1.0);
	}
	else if (mode == 1)
	{
		fragcolor = vec4(abs(normalize(tangent)), 1.0);
	}
	else if (mode == 2)
	{
		fragcolor = vec4(abs(normalize(bitangent)), 1.0);
	}
	else if (mode == 3)
	{
		fragcolor = environment_mapping();
	}
	else if (mode == 4)
	{
		fragcolor = environment_mapped_bump_mapping();
	}
	else if (mode == 5)
	{
		fragcolor = lighting();
	}
}

vec4 environment_mapping()
{
	vec3 n = normalize(normal);
	vec3 v = normalize(eye-p); // unit view vector
	vec3 rv = reflect(-v, n); // unit reflection view
	return texture(env_map, rv, 10.0*slider); //be sure to use world-space reflection vectors, since the cube map represents the surroundings of the world
}

vec4 environment_mapped_bump_mapping()
{
	//TODO: fix me
	//vec3 n = normalize(TBN*UnpackUnitNormal(texture(normal_map, tex_coord).xyz));
	vec3 n = normalize(UnpackUnitNormal(texture(normal_map, tex_coord).xyz));// T space
	//vec3 v = normalize(eye-p); // unit view vector
	vec3 v = normalize(inverse(TBN)*(eye-p)); //T space
	//vec3 rv = reflect(-v, n); // unit reflection view
	vec3 rv = reflect(-v, n); //  v, n are both in T, therefore rv is in T
	return texture(env_map, rv, 10.0*slider);
}

//**
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
	float height_scale = 0.1;

	const float minLayers = 8.0;
	const float maxLayers = 32.0;

	// number of depth layers
    //const float numLayers = 10;
	
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));

    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy * height_scale; 
    vec2 deltaTexCoords = P / numLayers;

//	float height =  texture(bump_map, texCoords).r;    
//    vec2 p = viewDir.xy / viewDir.z * (height * height_scale);
//    return texCoords - p;    

	vec2  currentTexCoords     = texCoords;
	float currentDepthMapValue = 1.0-texture(bump_map, currentTexCoords).r;
  
while(currentLayerDepth < currentDepthMapValue)
{
    // shift texture coordinates along direction of P
    currentTexCoords -= deltaTexCoords;
    // get depthmap value at current texture coordinates
    currentDepthMapValue = 1.0-texture(bump_map, currentTexCoords).r;  
    // get depth of next layer
    currentLayerDepth += layerDepth;  
}

vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
float afterDepth  = currentDepthMapValue - currentLayerDepth;
float beforeDepth =  1.0-texture(bump_map, prevTexCoords).r - currentLayerDepth + layerDepth;

float weight = afterDepth / (afterDepth - beforeDepth);
vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 -weight);

return finalTexCoords;  

//return currentTexCoords;

} 

//TODO compute lighting and parallax mapping in tangent-space here
vec4 lighting()
{

   //vec3 n = normalize(normal); // unit normal vector
   vec3 v = normalize(inverse(TBN)*(eye-p)); //T space

   
	//**offset texture coordinates with parallax mapping
	vec2 texCoordPar = ParallaxMapping(tex_coord,  v);
	if(texCoordPar.x > 1.0 || texCoordPar.y > 1.0 || texCoordPar.x < 0.0 || texCoordPar.y < 0.0)
    discard;

	vec3 n = normalize(UnpackUnitNormal(texture(normal_map, texCoordPar).xyz));// T space
	
   //vec3 v = normalize(eye-p); // unit view vector
   
   vec3 r = reflect(inverse(TBN)*(-l), n); // T space


   //compute phong lighting in world space

   //diffuse term
   //vec4 diff = Ld*max(dot(n,l) , 0.0);
   vec4 diff = Ld*max(dot(n,inverse(TBN)*l) , 0.0);
   vec4 amb = La;

	vec4 tex_color = texture(diffuse_map, tex_scale*texCoordPar);
	diff = tex_color*diff;
	amb = tex_color*amb;


   //specular term from env map
   vec3 rv = reflect(-v, n); // T space, unit reflection view
   vec4 spec = ks*Ls*texture(env_map, rv, 10.0*slider);

   return amb+diff+spec;
}

