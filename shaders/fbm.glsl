float fade(float t) { return t*t*t*(t*(t*6.0 - 15.0) + 10.0); }
float lerp1(float a, float b, float t) { return a + t*(b - a); }

uint hash_u32(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

uint hash_2d(ivec2 p, uint seed)
{
    uint x = uint(p.x) * 0x9e3779b9u;
    uint y = uint(p.y) * 0x85ebca6bu;
    return hash_u32(x ^ (y + seed * 0x27d4eb2du));
}

vec2 grad2(uint h)
{
    uint g = h & 7u;
        vec2 dirs[8] = vec2[8](
        vec2( 1, 0), vec2(-1, 0), vec2(0,  1), vec2(0, -1),
        vec2( 1, 1), vec2(-1, 1), vec2(1, -1), vec2(-1,-1)
    );
    return normalize(dirs[g]);
}

float perlin2(vec2 p, uint seed)
{
    ivec2 pi = ivec2(floor(p));
    vec2  pf = fract(p);

    float u = fade(pf.x);
    float v = fade(pf.y);

    uint h00 = hash_2d(pi + ivec2(0,0), seed);
    uint h10 = hash_2d(pi + ivec2(1,0), seed);
    uint h01 = hash_2d(pi + ivec2(0,1), seed);
    uint h11 = hash_2d(pi + ivec2(1,1), seed);

    float d00 = dot(grad2(h00), pf - vec2(0,0));
    float d10 = dot(grad2(h10), pf - vec2(1,0));
    float d01 = dot(grad2(h01), pf - vec2(0,1));
    float d11 = dot(grad2(h11), pf - vec2(1,1));

    float x0 = lerp1(d00, d10, u);
    float x1 = lerp1(d01, d11, u);
    return lerp1(x0, x1, v);
}

float fbm(
    in vec2 p,
    float freq,
    in uint num_layers,
    in float lacunarity,
    in float persistence,
    in float domain_warp,
    in uint seed)
{
    float amp = 1.0;
    float sum = 0.0;
    float norm = 0.0;

    vec2 warp = vec2(0.0);

    for (uint o = 0; o < num_layers; ++o)
    {
        vec2 pp = (p + warp) * freq;
        float n = perlin2(pp, seed + o * 101u);

        sum += n * amp;
        norm += amp;

        if (domain_warp != 0.0)
        {
            float wx = perlin2(pp + vec2(31.7, 12.4), seed + 1000u + o * 17u);
            float wy = perlin2(pp + vec2(9.2,  47.1), seed + 2000u + o * 19u);
            warp += domain_warp * vec2(wx, wy) * amp;
        }

        amp *= persistence;
        freq *= lacunarity;
    }

    return (norm > 0.0) ? (sum / norm) : 0.0;
}