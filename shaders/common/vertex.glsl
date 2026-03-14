struct Vertex
{
    vec4 pos_xyz_norm_x;
    vec4 norm_yz_uv;
};

void set_vert_pos(inout Vertex v, in vec3 pos)
{
    v.pos_xyz_norm_x.xyz = pos;
}

void set_vert_norm(inout Vertex v, in vec3 norm)
{
    v.pos_xyz_norm_x.w = norm.x;
    v.norm_yz_uv.xy = norm.yz;
}

void set_vert_uv(inout Vertex v, in vec2 uv)
{
    v.norm_yz_uv.zw = uv;
}

vec3 get_vert_pos(in Vertex v)
{
    return v.pos_xyz_norm_x.xyz;
}

vec3 get_vert_norm(in Vertex v)
{
    return vec3(v.pos_xyz_norm_x.w, v.norm_yz_uv.xy);
}

vec2 get_vert_uv(in Vertex v)
{
    return v.norm_yz_uv.zw;
}