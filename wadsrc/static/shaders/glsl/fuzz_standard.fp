//created by Evil Space Tomato

vec4 ProcessTexel()
{
	vec2 texCoord = vTexCoord.st;
	vec4 basicColor = getTexel(texCoord);
	vec2 texSize = vec2(textureSize(tex, 0));

	texCoord.x = float( int(texCoord.x * texSize.x) ) / texSize.x;
	texCoord.y = float( int(texCoord.y * texSize.y) ) / texSize.y;

	float texX = texCoord.x / 3.0 + 0.66;
	float texY = 0.34 - texCoord.y / 3.0;
	float vX = (texX/texY)*21.0;
	float vY = (texY/texX)*13.0;
	float test = mod(timer*2.0+(vX + vY), 0.5);

	basicColor.a = basicColor.a * test;
	basicColor.r = basicColor.g = basicColor.b = 0.0;

	return basicColor;
}
