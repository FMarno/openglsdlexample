#version 150
uniform mat4 mvpmatrix;
in vec3 in_position;
in vec3 in_colour;
in vec3 in_normal;
out vec3 ex_colour;
out vec3 ex_normal;
out vec4 ex_position;

void main(void){
    gl_Position = mvpmatrix* vec4(in_position, 1.0);
    ex_position = mvpmatrix* vec4(in_position, 1.0);
    ex_colour = in_colour;
    ex_normal = in_normal;
}