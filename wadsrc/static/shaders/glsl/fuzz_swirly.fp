//created by Evil Space Tomato

vec4 ProcessTexel()
{
	vec2 texCoord = vTexCoord.st;
	vec4 basicColor = getTexel(texCoord);

	float texX = sin(texCoord.x * 100.0 + timer*5.0);
	float texY = cos(texCoord.x * 100.0 + timer*5.0);
	float vX = (texX/texY)*21.0;
	float vY = (texY/texX)*13.0;
	float test = mod(timer*2.0+(vX + vY), 0.5);

	basicColor.a = basicColor.a * test;
	basicColor.r = basicColor.g = basicColor.b = 0.0;

	return basicColor;
}
