#version 430            
layout(location = 0) uniform mat4 P;
layout(location = 1) uniform mat4 V;
layout(location = 2) uniform mat4 M;
layout(location = 3) uniform mat4 PVM;
layout(location = 4) uniform float time;

layout(location = 0) in vec3 pos_attrib;
layout(location = 1) in vec2 tex_coord_attrib;
layout(location = 2) in vec3 normal_attrib;		//object space normal
layout(location = 3) in vec3 tangent_attrib;	//object space tangent
layout(location = 4) in vec3 bitangent_attrib;	//object space bitangent

out vec2 tex_coord; 
out vec3 normal; //world
out vec3 tangent; // world
out vec3 bitangent; //world
out vec3 p; //world
out vec3 eye;//world
out mat3 TBN; //world

const vec4 origin = vec4(0.0, 0.0, 0.0, 1.0); //w = 1 becase this is a point

void main(void)
{
   gl_Position = PVM*vec4(pos_attrib, 1.0);
   tex_coord = tex_coord_attrib;
   normal = vec3(normalize(M*vec4(normal_attrib, 0.0))); //send world-space normal
   tangent = vec3(normalize(M*vec4(tangent_attrib, 0.0))); // world-space tengent
   bitangent = vec3 (normalize(M*vec4(bitangent_attrib, 0.0))); // world-space tengent

   TBN = mat3(tangent, bitangent, normal); //TODO, fix this

   //Compute world-space vertex position
   p = vec3(M*vec4(pos_attrib, 1.0));           //w = 1 becase this is a point

   //Compute world-space eye position
   eye = vec3(inverse(V)*origin); //Would be better to pass eye position as a uniform...
}