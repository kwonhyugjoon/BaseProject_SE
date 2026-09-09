#version 330
layout(location=0) in vec2 a_Position;
layout(location=1) in vec4 a_Color;
layout(location=2) in float a_Material;
layout(location=3) in vec2 a_UV;
uniform vec2 u_Viewport;
uniform vec2 u_Origin;
out vec4 v_Color;
out vec2 v_World;
out vec2 v_UV;
flat out int v_Material;
flat out int v_Local;
void main()
{
    gl_Position=vec4(a_Position.x/u_Viewport.x*2.0-1.0,1.0-a_Position.y/u_Viewport.y*2.0,0,1);
    vec2 ground=(a_Position-u_Origin)/vec2(23.0,11.5);
    v_World=vec2(ground.x+ground.y,ground.y-ground.x)*0.5;
    v_Local=a_Material<0.0?1:0;
    v_Color=a_Color;v_Material=int((v_Local==1?-a_Material-1.0:a_Material)+0.5);v_UV=a_UV;
}
