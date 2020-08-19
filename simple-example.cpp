#include <stdio.h>
#include <fstream>
#include <sstream>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/freeglut.h>
#include <GL/GL.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

const int WinWidth = 640;
const int WinHeight = 480;

GLboolean should_rotate = GL_TRUE;
const glm::vec4 light_location(10.f,10.f,5.f, 1.f);

/* A simple function that prints a message, the error code returned by SDL, and quits the application */
void sdldie(std::string msg)
{
    printf("%s: %s\n", msg.c_str(), SDL_GetError());
    SDL_Quit();
    exit(1);
}

void error_check(std::string hint)
{
#ifndef NDEBUG
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        if (!hint.empty())
        {
            printf("error at %s\n", hint.c_str());
        }
        sdldie((char *)gluErrorString(error));
    }
#endif
}

const GLuint vertex_attributes = 0;
const GLuint colour_attributes = 1;
const GLuint normal_attributes = 2;
static GLuint vertex_buffer_object;
static GLuint cbo;
struct Square
{
    GLuint vao;
    GLuint ibo;
    GLuint nbo;
    Square(){};
    Square(const GLubyte indices[6], glm::vec3 normal)
    {
        glGenVertexArrays(1, &vao);
        error_check("square vao gen");
        glBindVertexArray(vao);
        error_check("square vao");

        // vertex data
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
        glVertexAttribPointer(vertex_attributes, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(vertex_attributes);

        // colour data
        glBindBuffer(GL_ARRAY_BUFFER, cbo);
        glVertexAttribPointer(colour_attributes, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(colour_attributes);

        // index buffer
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 6, indices, GL_STATIC_DRAW);
        error_check("square ibo");

        // normal buffer, need 8 copies so index always finds the same value
        GLfloat normals[3*8];
        for (int i =0; i < 8; ++i){
            normals[3*i] = normal[0];
            normals[3*i+1] = normal[1];
            normals[3*i+2] = normal[2];
        }
        glGenBuffers(1, &nbo);
        glBindBuffer(GL_ARRAY_BUFFER, nbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*8*3, normals, GL_STATIC_DRAW);
        glVertexAttribPointer(normal_attributes, 3, GL_FLOAT, GL_FALSE, 0,0);
        glEnableVertexAttribArray(normal_attributes);
        error_check("square nbo");
    }
    ~Square(){
        glDeleteBuffers(1,&ibo);
        glDeleteBuffers(1,&nbo);
        glDeleteVertexArrays(1,&vao);
    }

    void draw(){
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
        error_check("square draw");
    }
};


std::string readfile(std::string path)
{
    std::ifstream in(path);
    std::stringstream strStream;
    strStream << in.rdbuf();
    return strStream.str();
}

void setup_opengl()
{
    glewInit();

    /* Culling. */
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    error_check("opengl setup: cull");

    /* Set the clear color. */
    glClearColor(0, 0, 0, 0);

    /* Enable Z depth testing so objects closest to the viewpoint are in front of objects further away */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_CLAMP);
    glDepthRangef(-1.0f, 1.0f);

    /* Setup our viewport. */
    glViewport(0, 0, WinWidth, WinHeight);
    error_check("opengl setup: viewport");
}

static GLuint shader_program;
static GLuint vertex_shader;
static GLuint fragment_shader;
static Square* squares[6];
void setup_buffers()
{
    /* 
     *   7 ---- 6
     *   |      |
     * 3 ---- 2 |
     * | 4 ---- 5
     * |      |
     * 0 ---- 1
     */
    const glm::vec3 vertices[8] = {
        {-1.0f, -1.0f, 1.0f},
        {1.0f, -1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {-1.0f, 1.0f, 1.0f},
        {-1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f, -1.0f},
        {1.0f, 1.0f, -1.0f},
        {-1.0f, 1.0f, -1.0f}};
    const GLfloat colours[8][3] = {
        {0.0f, 0.0f, 1.0f},  //blue
        {1.0f, 0.0f, 1.0f},  //magenta
        {1.0f, 1.0f, 1.0f},  //white
        {0.0f, 1.0f, 1.0f},  //cyan
        {0.0f, 0.0f, 0.0f},  // black
        {1.0f, 0.0f, 0.0f},  // red
        {1.0f, 1.0f, 0.0f},  //yellow
        {0.0f, 1.0f, 0.0f}}; //green
    const GLubyte vertex_indices[6][2 * 3] = {
        {0, 1, 2,
        0, 2, 3},
        {1, 5, 6,
        1, 6, 2},
        {5, 4, 7,
        5, 7, 6},
        {4, 0, 3,
        4, 3, 7},
        {3, 2, 6,
        3, 6, 7},
        {1, 0, 4,
        1, 4, 5}};
    // vertex data
    glGenBuffers(1, &vertex_buffer_object);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 8 * 3, vertices, GL_STATIC_DRAW);
    error_check("vertex data");

    // colour data
    glGenBuffers(1, &cbo);
    glBindBuffer(GL_ARRAY_BUFFER, cbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 8 * 3, colours, GL_STATIC_DRAW);
    error_check("colour data");

    for (int i =0; i <6; ++i){
        auto A = vertices[vertex_indices[i][0]];
        auto B = vertices[vertex_indices[i][1]];
        auto C = vertices[vertex_indices[i][2]];
        auto AB = B - A;
        auto BC = C - B;
        auto normal = glm::cross(AB, BC);
        squares[i] = new Square(vertex_indices[i], normal);
    }
}

void setup_shader(){
    std::string vertex_source = readfile("shaders/example.vert");
    const char *vertex_c_source = vertex_source.c_str();
    std::string fragment_source = readfile("shaders/example.frag");
    const char *fragment_c_source = fragment_source.c_str();

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, (const GLchar *const *)&vertex_c_source, 0);
    glCompileShader(vertex_shader);

    int is_compiled_vs;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &is_compiled_vs);
    if (is_compiled_vs == FALSE)
    {
        int max_length;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &max_length);

        /* The maxLength includes the NULL character */
        char *vertexInfoLog = new char[max_length];

        glGetShaderInfoLog(vertex_shader, max_length, &max_length, vertexInfoLog);

        /* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
        /* In this simple program, we'll just leave */
        printf("%s\n", vertexInfoLog);
        delete[] vertexInfoLog;
        return;
    }

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, (const GLchar *const *const) & fragment_c_source, 0);
    glCompileShader(fragment_shader);

    int is_compiled_fs;
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &is_compiled_fs);
    if (is_compiled_fs == FALSE)
    {
        int max_length;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &max_length);

        /* The maxLength includes the NULL character */
        char *fragmentInfoLog = new char[max_length];

        glGetShaderInfoLog(vertex_shader, max_length, &max_length, fragmentInfoLog);

        /* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
        /* In this simple program, we'll just leave */
        printf("%s\n", fragmentInfoLog);
        delete[] fragmentInfoLog;
        return;
    }
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glBindAttribLocation(shader_program, vertex_attributes, "in_position");
    glBindAttribLocation(shader_program, colour_attributes, "in_colour");
    glBindAttribLocation(shader_program, normal_attributes, "in_normal");
    glLinkProgram(shader_program);

    int is_linked;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &is_linked);
    if (is_linked == FALSE)
    {
        int max_length;
        glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &max_length);
        char *shader_program_info_log = new char[max_length];
        glGetProgramInfoLog(shader_program, max_length, &max_length, shader_program_info_log);
        printf("%s\n", shader_program_info_log);
        delete[] shader_program_info_log;
        return;
    }

    glUseProgram(shader_program);
    glUniform3f(glGetUniformLocation(shader_program, "u_lightsource"), light_location.x, light_location.y, light_location.z);
    error_check("shader setup");
}

void takedown_buffers()
{
    glUseProgram(0);
    glDisableVertexAttribArray(vertex_attributes);
    glDisableVertexAttribArray(colour_attributes);
    glDeleteBuffers(1, &vertex_buffer_object);
    glDeleteBuffers(1, &cbo);
    for (int i = 0; i < 6; ++i){
        delete squares[i];
    }
    error_check("takedown buffers");
}

void takedown_shaders(){
    glDetachShader(shader_program, vertex_shader);
    glDetachShader(shader_program, fragment_shader);
    glDeleteProgram(shader_program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    error_check("takedown_shaders");
}

void draw_screen(SDL_Window *window)
{
    /* Our angle of rotation. */
    static float angle = 0.0f;

    /* Clear the color and depth buffers. */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)WinWidth / (float)WinHeight,
        .1f,
        10.f);

    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, -5.0));
    view = glm::rotate(view, glm::radians(angle), glm::vec3(0.0, 1.0, 0.0));
    view = glm::rotate(view, glm::radians(45.f), glm::vec3(1.0, 1.0, 1.0));

    glm::mat4 mvpmatrix = projection * view;
    if (should_rotate)
    {
        if (++angle > 360.0f)
        {
            angle = 0.0f;
        }
    }

    glUniformMatrix4fv(glGetUniformLocation(shader_program, "mvpmatrix"), 1, GL_FALSE, glm::value_ptr(mvpmatrix));
    glm::vec4 light = glm::vec4(
        light_location.x,
        light_location.y * glm::sin(glm::radians(angle)),
        light_location.z,
        light_location.w
        );
    glUniform3f(glGetUniformLocation(shader_program, "u_lightsource"), light.x, light.y, light.z);
    for (int i = 0; i < 6; ++i){
        squares[i]->draw();
    }

    /*
     * EXERCISE:
     * Draw text telling the user that 'Spc'
     * pauses the rotation and 'Esc' quits.
     * Do it using vetors and textured quads.
     */
    SDL_GL_SwapWindow(window);
}

void destroy_sdl(SDL_Window *window, SDL_GLContext *gl_context)
{
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char *argv[])
{

    if (SDL_Init(SDL_INIT_VIDEO) < 0)       /* Initialize SDL's Video subsystem */
        sdldie("Unable to initialize SDL"); /* Or die on error */

    /* Request an opengl 3.2 context.
     * SDL doesn't have the ability to choose which profile at this time of writing,
     * but it should default to the core profile */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    uint32_t WindowFlags = SDL_WINDOW_OPENGL;
    SDL_Window *window = SDL_CreateWindow(
        "OpenGL Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WinWidth, WinHeight, WindowFlags);
    assert(window);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    bool Running = true;
    bool FullScreen = false;

    setup_opengl();

    setup_buffers();
    setup_shader();
    while (Running)
    {
        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
            {
                switch (Event.key.keysym.sym)
                {
                case SDLK_ESCAPE:
                    Running = 0;
                    break;
                case SDLK_SPACE:
                    should_rotate = !should_rotate;
                    break;
                case 'f':
                    FullScreen = !FullScreen;
                    if (FullScreen)
                    {
                        SDL_SetWindowFullscreen(window, WindowFlags | SDL_WINDOW_FULLSCREEN_DESKTOP);
                    }
                    else
                    {
                        SDL_SetWindowFullscreen(window, WindowFlags);
                    }
                    break;
                default:
                    break;
                }
            }
            else if (Event.type == SDL_QUIT)
            {
                Running = 0;
            }
        }

        draw_screen(window);

        SDL_Delay(16);
    }

    takedown_buffers();
    takedown_shaders();

    destroy_sdl(window, &context);
    return 0;
}