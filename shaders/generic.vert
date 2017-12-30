#version 150

in vec2 Texcoord;
in vec3 position;
in vec4 weight;
in vec4 boneID;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 boneTransformation[64];
uniform mat4 modelTransformation;

out vec2 texcoord;

void main() {
	int b1 = int(boneID.x); int b2 = int(boneID.y); int b3 = int(boneID.z); int b4 = int(boneID.w);
	mat4 bTrans = boneTransformation[b1] * weight.x;
	if (b2 != -1)
	{bTrans += boneTransformation[b2] * weight.y;}
	if (b3 != -1)
	{bTrans += boneTransformation[b3] * weight.z;}
	if (b4 != -1)
	{bTrans += boneTransformation[b4] * weight.w;}

	texcoord = Texcoord;
	gl_Position = projection * view * model * modelTransformation * bTrans * vec4(position, 1.0);
}
