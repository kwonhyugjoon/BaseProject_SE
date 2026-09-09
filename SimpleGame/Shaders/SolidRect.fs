#version 330
in vec4 v_Color;
in vec2 v_World;
in vec2 v_UV;
flat in int v_Material;
flat in int v_Local;
uniform float u_Time;
uniform sampler2D u_Atlas;
layout(location=0) out vec4 FragColor;
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p)
{
    vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+1.0),f.x),f.y);
}
void main()
{
    vec3 c=v_Color.rgb;float alpha=v_Color.a;
    vec2 p=v_Local==1?v_UV:v_World;
    if(v_Material==7)
    {
        vec4 tex=texture(u_Atlas,v_UV);
        c=tex.r<0.01&&tex.g<0.01?c*tex.b:tex.rgb;
        FragColor=vec4(c,alpha*tex.a);return;
    }
    if(v_Material==1) // Grass, clumps and small dried blades.
    {
        float patch=noise(p*0.7),grain=noise(p*16.0);
        c*=0.78+patch*0.34+grain*0.17;
        c+=vec3(0.09,0.07,0.01)*smoothstep(0.74,0.92,noise(p*vec2(18,4)));
    }
    else if(v_Material==2) // Compacted earth with gravel and cart ruts.
    {
        float gravel=smoothstep(0.68,0.78,noise(p*13.0));
        c*=0.83+noise(p*2.5)*0.25+gravel*0.22;
        c*=1.0-0.10*pow(abs(sin(p.x*5.0+noise(p)*0.3)),20.0);
    }
    else if(v_Material==3) // Staggered, uneven fieldstone paving.
    {
        vec2 q=p*vec2(1.8,2.4);q.x+=mod(floor(q.y),2.0)*0.5;
        vec2 f=fract(q);vec2 edge=min(f,1.0-f);
        float mortar=smoothstep(0.025,0.08,min(edge.x,edge.y));
        float stone=0.72+hash(floor(q))*0.38+noise(p*20.0)*0.10;
        c*=mix(0.44,stone,mortar);
        c+=vec3(0.02,0.038,0.009)*(1.0-mortar)*noise(p*3.0);
    }
    else if(v_Material==4) // Timber grain, plank joints and knots.
    {
        float grain=sin(p.y*42.0+noise(p*2.0)*7.0);
        float joint=smoothstep(0.02,0.065,min(fract(p.x*2.0),1.0-fract(p.x*2.0)));
        c*=mix(0.48,0.93+grain*0.12,joint);
    }
    else if(v_Material==5) // Hand-laid roof shingles.
    {
        vec2 q=p*vec2(2.5,3.5);q.x+=mod(floor(q.y),2.0)*0.5;
        vec2 f=fract(q);float seam=smoothstep(0.025,0.085,min(min(f.x,1-f.x),f.y));
        c*=mix(0.58,0.8+0.25*hash(floor(q))+f.y*0.13,seam);
    }
    else if(v_Material==6)
    {
        vec2 d=(p-vec2(57,28))/vec2(12,10);float shore=length(d);
        vec2 flow=vec2(u_Time*0.12,-u_Time*0.07);
        float swell=noise(p*0.48+flow),ripples=noise(p*2.6-flow*2.0);
        float surface=swell*0.7+ripples*0.3;
        // Approximate light on waves; this is not a scene reflection pass.
        vec3 normal=normalize(vec3(-dFdx(surface)*16.0,-dFdy(surface)*16.0,1.0));
        float glint=pow(max(dot(normal,normalize(vec3(-0.35,-0.45,1.0))),0.0),64.0);
        float shallow=smoothstep(0.60,1.0,shore);
        c=mix(vec3(0.035,0.15,0.21),v_Color.rgb,shallow);
        c+=vec3(0.09,0.16,0.18)*swell;
        c+=vec3(0.09,0.16,0.11)*shallow*pow(ripples,5.0);
        c+=vec3(0.65,0.63,0.44)*glint*(0.25+0.75*noise(p+flow));
        float crest=sin(shore*95.0-u_Time*1.8+swell*8.0)*0.5+0.5;
        float foam=smoothstep(0.94,1.0,shore)*smoothstep(0.78,0.95,crest);
        c=mix(c,vec3(0.58,0.68,0.54),foam*noise(p*3.0));
    }
    else if(v_Material==8) // Lime plaster and weather stains.
    {
        c*=0.88+noise(p*3.0)*0.14+noise(p*35.0)*0.08;
        c=mix(c,c*vec3(0.68,0.73,0.56),exp(-max(p.y,0.0)*3.0)*0.42);
    }
    else if(v_Material==9) // Small leaf clusters on the shaped canopy.
    {
        float leaves=noise(p*0.22),detail=noise(p*0.65);
        c*=0.76+leaves*0.36+detail*0.16;
        c+=vec3(0.055,0.055,0.012)*smoothstep(0.65,0.86,leaves);
    }
    FragColor=vec4(max(c,0.0),alpha);
}
