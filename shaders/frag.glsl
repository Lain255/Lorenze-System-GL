#version 460

out vec4 FragColor;
in float speed;
in float depth;

void main()
{
    vec4 blue = vec4(0,0,1,1);
    vec4 red = vec4(1,0,0,1);
    vec4 blend = blue*(1-speed) + red*speed;
    FragColor = vec4(blend.xyz, depth/2);
} 
