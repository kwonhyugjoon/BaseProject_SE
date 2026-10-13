#version 330
in vec2 v_UV;
uniform sampler2D u_Scene;
uniform vec2 u_Texel;
uniform bool u_Enabled;
layout(location = 0) out vec4 FragColor;

float luminance(vec3 c)
{
	return dot(c, vec3(0.299, 0.587, 0.114));
}

void main()
{
	vec3 center = texture(u_Scene, v_UV).rgb;

	if (!u_Enabled)
	{
		FragColor = vec4(center, 1);
		return;
	}

	// Local contrast-aware edge smoothing.
	vec3 n = texture(u_Scene, v_UV + vec2(0, u_Texel.y)).rgb;
	vec3 s = texture(u_Scene, v_UV - vec2(0, u_Texel.y)).rgb;
	vec3 e = texture(u_Scene, v_UV + vec2(u_Texel.x, 0)).rgb;
	vec3 w = texture(u_Scene, v_UV - vec2(u_Texel.x, 0)).rgb;
	float hi = max(luminance(center),
	               max(max(luminance(n), luminance(s)), max(luminance(e), luminance(w))));
	float lo = min(luminance(center),
	               min(min(luminance(n), luminance(s)), min(luminance(e), luminance(w))));
	vec3 c =
	    mix(center, (n + s + e + w + center * 4.0) / 8.0, smoothstep(0.09, 0.35, hi - lo) * 0.6);

	// Sparse, soft bloom from HDR lights. Fixed footprint in pixels.
	vec3 glow = max(center - vec3(0.82), vec3(0)) * 4.0;

	for (int y = -1; y <= 1; ++y)
		for (int x = -1; x <= 1; ++x)
		{
			if (x == 0 && y == 0)
				continue;
			float k = x == 0 || y == 0 ? 2.0 : 1.0;
			vec3 sampleColor = texture(u_Scene, v_UV + vec2(x, y) * u_Texel * 4.5).rgb;
			glow += max(sampleColor - vec3(0.82), vec3(0)) * k;
		}
	c += glow / 16.0 * 0.42;
	c = (c - 0.22) * 1.06 + 0.22;
	c *= vec3(1.045, 1.018, 0.968);
	float vignette = 1.0 - 0.16 * smoothstep(0.2, 0.72, length(v_UV - 0.5));
	FragColor = vec4(clamp(c * vignette, 0.0, 1.0), 1);
}
