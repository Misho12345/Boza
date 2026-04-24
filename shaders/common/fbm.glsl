float fade(float t) { return t*t*t*(t*(t*6.0 - 15.0) + 10.0); }

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

uint hash_3d(ivec3 p, uint seed)
{
    uint x = uint(p.x) * 0x9e3779b9u;
    uint y = uint(p.y) * 0x85ebca6bu;
    uint z = uint(p.z) * 0xc2b2ae35u;
    return hash_u32(x ^ (y + seed * 0x27d4eb2du) ^ (z * 0x165667b1u));
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

vec3 grad3(uint h)
{
    uint g = h % 12u;
    vec3 dirs[12] = vec3[12](
        vec3( 1, 1, 0), vec3(-1, 1, 0), vec3( 1,-1, 0), vec3(-1,-1, 0),
        vec3( 1, 0, 1), vec3(-1, 0, 1), vec3( 1, 0,-1), vec3(-1, 0,-1),
        vec3( 0, 1, 1), vec3( 0,-1, 1), vec3( 0, 1,-1), vec3( 0,-1,-1)
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

    float x0 = mix(d00, d10, u);
    float x1 = mix(d01, d11, u);
    return mix(x0, x1, v);
}

float perlin3(vec3 p, uint seed)
{
    ivec3 pi = ivec3(floor(p));
    vec3 pf = fract(p);

    float u = fade(pf.x);
    float v = fade(pf.y);
    float w = fade(pf.z);

    uint h000 = hash_3d(pi + ivec3(0,0,0), seed);
    uint h100 = hash_3d(pi + ivec3(1,0,0), seed);
    uint h010 = hash_3d(pi + ivec3(0,1,0), seed);
    uint h110 = hash_3d(pi + ivec3(1,1,0), seed);
    uint h001 = hash_3d(pi + ivec3(0,0,1), seed);
    uint h101 = hash_3d(pi + ivec3(1,0,1), seed);
    uint h011 = hash_3d(pi + ivec3(0,1,1), seed);
    uint h111 = hash_3d(pi + ivec3(1,1,1), seed);

    float d000 = dot(grad3(h000), pf - vec3(0,0,0));
    float d100 = dot(grad3(h100), pf - vec3(1,0,0));
    float d010 = dot(grad3(h010), pf - vec3(0,1,0));
    float d110 = dot(grad3(h110), pf - vec3(1,1,0));
    float d001 = dot(grad3(h001), pf - vec3(0,0,1));
    float d101 = dot(grad3(h101), pf - vec3(1,0,1));
    float d011 = dot(grad3(h011), pf - vec3(0,1,1));
    float d111 = dot(grad3(h111), pf - vec3(1,1,1));

    float x00 = mix(d000, d100, u);
    float x10 = mix(d010, d110, u);
    float x01 = mix(d001, d101, u);
    float x11 = mix(d011, d111, u);

    float y0 = mix(x00, x10, v);
    float y1 = mix(x01, x11, v);

    return mix(y0, y1, w);
}

float fbm2(
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

float fbm3(
    in vec3 p,
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

    vec3 warp = vec3(0.0);

    for (uint o = 0u; o < num_layers; ++o)
    {
        vec3 pp = (p + warp) * freq;
        float n = perlin3(pp, seed + o * 101u);

        sum += n * amp;
        norm += amp;

        if (domain_warp != 0.0)
        {
            float wx = perlin3(pp + vec3(31.7, 12.4, 19.1), seed + 1000u + o * 17u);
            float wy = perlin3(pp + vec3(9.2, 47.1, 73.5),  seed + 2000u + o * 19u);
            float wz = perlin3(pp + vec3(58.3,  4.8, 26.7), seed + 3000u + o * 23u);
            warp += domain_warp * vec3(wx, wy, wz) * amp;
        }

        amp *= persistence;
        freq *= lacunarity;
    }

    return (norm > 0.0) ? (sum / norm) : 0.0;
}