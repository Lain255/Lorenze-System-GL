#version 460
#define DEGREE (3.1415926538/180)


layout (std430, binding = 0) restrict buffer Particles
{
  vec4 particle_pos[1000000];
};

uniform Data {
    vec3 c;
    vec2 view_scale;
    float cosTheta;
    float sinTheta;
    float cosPhi;
    float sinPhi;
};

out float speed;
out float depth;

void main()
{
    //constants. TODO, make controllable and sent through ubo
    float minDrawDist=1;
    float maxDrawDist=100;
    float fov=120*DEGREE; 
    float dt = 0.01666;
    float minspeed = 0;
    float maxspeed = 200;

    //lorenze system differential equasion
    vec3 p = particle_pos[gl_VertexID].xyz;
    vec3 v = vec3(10*(p.y-p.x), p.x*(28-p.z)-p.y, p.x*p.y - (8/3)*p.z);
    p += v*dt;
    particle_pos[gl_VertexID] = vec4(p.xyz,0);

    
    //perspective transformation
    float w = (maxDrawDist-minDrawDist)/2;
    float l = minDrawDist*tan(fov/2);
    mat3 rot = mat3(
    cosTheta,  -sinTheta*sinPhi, sinTheta*cosPhi,
    0,                  cosPhi,           sinPhi,
    -sinTheta, -cosTheta*sinPhi, cosTheta*cosPhi
    );

    vec3 pos = particle_pos[gl_VertexID].xyz;

    pos = rot*(pos-c);
    pos.x = pos.x/(pos.z*l);
    pos.y = pos.y/(pos.z*l);
    pos.z = (pos.z-(w+minDrawDist))/w;


    //variables used in fragment shader for coloring
    depth = (1-pos.z)/2;
    depth *= depth;
    speed = clamp(length(v),minspeed,maxspeed)/(maxspeed-minspeed);

    gl_Position = vec4(pos.x, pos.y, pos.z, 1.0);
}
