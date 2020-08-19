#version 150

uniform vec3 u_lightsource;
in vec3 ex_colour;
in vec3 ex_normal;
in vec4 ex_position;

void main(void){
    float ambient = 0.2;

    vec3 light_direction = normalize(ex_position.xyz - u_lightsource);
    vec3 normal = normalize(ex_normal);
    float diffuse = max(dot(light_direction, normal), 0.0);

    gl_FragColor = vec4(ex_colour*(ambient+diffuse), 1.0);
}