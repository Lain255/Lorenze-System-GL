#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <cstring>
#include <cstdlib>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

const float HALF_PI = 3.1415/2;

static void error_callback(int error, const char *description)
{
    fprintf(stderr, "Error: %s\n", description);
}

char *read_file(char *path)
{
    FILE *fp = fopen(path, "rb");

    if (fp == 0)
    {
        printf("Could not find file: \"%s\"\n", path);
        exit(-1);
    }

    long size;
    char *text;

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    text = (char *) malloc(size * sizeof(char)+1);
    

    fread(text, 1, size, fp);
    text[size] = 0;

    fclose(fp);

    return text;
}

int main()
{
    GLFWwindow *window;

    glfwSetErrorCallback(error_callback);

    glfwInit();

    //window creatiion
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(1600, 800, "ComplexDynamics", 0, 0);
    
    //gl and glfw configuration
    glfwMakeContextCurrent(window);
    gladLoadGL();
    glfwSwapInterval(1);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    glEnable(GL_BLEND);  
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    //loads in shaders text from files
    const char *vertex_shader_text = read_file("shaders/vert.glsl");
    const char *fragment_shader_text = read_file("shaders/frag.glsl");

    printf("%s\n", vertex_shader_text);
    printf("%s\n", fragment_shader_text);

    int  success;
    char infoLog[512];
    
    //vertex shader
    const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, 0);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
    }

    //fragment shader
    const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, 0);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
    }

    //program
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("ERROR::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }

    //initialize ubo for camera and configuration data that will be sent to shader
    typedef struct Data_s{
        float cx;
        float cy;
        float cz;
        float MOREMOREJUNK;

        float view_scale_x;
        float view_scale_y;

        float cosTheta;
        float sinTheta;
        float cosPhi;
        float sinPhi;
    } Data;

    GLuint data_ubo;
    glGenBuffers(1, &data_ubo);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, data_ubo, 0, sizeof(Data));

    //initializes particle data
    int numParticles = 1000000;
    int sizeParticles = 4*numParticles*sizeof(float);
    float* particles = (float*)malloc(sizeParticles);
    for(int i = 0; i<numParticles*4; i++){
        particles[i] = 5 + 1.0*(rand()%numParticles)/numParticles;
    }

    //ssbo holds all particle position data
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeParticles, particles, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    //since ssbo now has all the data, particles array is not needed any more
    free(particles);

    Data data = {0,0,-20};
    float zoom = 1.0;
    float theta = 0;
    float phi = 0;
    double dtheta = 0;
    double dphi = 0;
    float foreward = 0;
    float right = 0;
    float up = 0;


    const float dt = 10.0/60.0;
    float turbo = 1.0;

    //drawArrays needs a bound vertex array to run even though we dont use it
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    


    glfwSetCursorPos(window, 0, 0);
    while (!glfwWindowShouldClose(window))
    {
        //debug line
        //printf("pos: %f %f %f rot: %f %f\n", data.cx, data.cy, data.cz, theta, phi);

        //input
        turbo = 1.0;
        foreward = 0;
        right = 0;
        up = 0;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
            turbo = 2.0;
        }
        if (glfwGetKey(window, GLFW_KEY_W)) {
            foreward += turbo * zoom * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_S)) {
            foreward -= turbo * zoom * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_A)) {
            right -= turbo * zoom * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_D)) {
            right += turbo * zoom * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_Q)) {
            up -= turbo * zoom * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_E)) {
            up += turbo * zoom * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_EQUAL)) {
            zoom *= 1.05 + turbo/50;
        }
        if (glfwGetKey(window, GLFW_KEY_MINUS)) {
            zoom /= 1.05 + turbo/50;
        }
        glfwGetCursorPos(window, &dtheta, &dphi);
        glfwSetCursorPos(window, 0, 0);
        theta += dtheta / 1000;
        phi -= dphi / 1000;
        phi = phi > HALF_PI ? HALF_PI : phi; 
        phi = phi < -HALF_PI ? -HALF_PI : phi; 

        //setting up ubo
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        data.view_scale_x = zoom * (float) width / (float) height;
        data.view_scale_y = zoom;
        data.cosTheta = cos(theta);
        data.sinTheta = sin(theta);
        data.cosPhi = cos(phi);
        data.sinPhi = sin(phi);

        //camera movement
        data.cx +=  (foreward*data.cosPhi*data.sinTheta) + (-up*data.sinPhi*data.sinTheta) +  (right*data.cosTheta);    
        data.cy +=  (foreward*data.sinPhi)               + (up*data.cosPhi);
        data.cz +=  (foreward*data.cosPhi*data.cosTheta) + (-up*data.sinPhi*data.cosTheta) + (-right*data.sinTheta);

        //render program
        glViewport(0, 0, width, height);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);

        glBindBuffer(GL_UNIFORM_BUFFER, data_ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(Data), &data, GL_STATIC_DRAW);
        
        //glPointSize(5.0);
        glDrawArrays(GL_POINTS, 0, numParticles);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();


    return 0;
}