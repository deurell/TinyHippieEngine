#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>

void onResize(GLFWwindow* window, int height, int width);
void processInput(GLFWwindow* window);
std::string loadShader(std::string path);

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "gfxlab", nullptr, nullptr);
    if(window == nullptr) {
        std::cout << "failed to create window.";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "failed to init glad";
        return -1;
    }

    glViewport(0,0,800,600);
    glfwSetFramebufferSizeCallback(window, onResize);

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    auto shaderSource = loadShader("vertex.shader");
    const char* cstr = shaderSource.c_str();
    glShaderSource(vertexShader, 1,&cstr, nullptr);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "error compiling vertex shader\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    shaderSource = loadShader("fragment.shader");
    cstr = shaderSource.c_str();
    glShaderSource(fragmentShader, 1, &cstr, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cout << "error compiling fragment shader\n" << infoLog << success << std::endl;
    }

    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cout << "error linking shader program\n" << infoLog << std::endl;
    }

    glUseProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {
            -0.5f, -0.5f, 0.0f,
            0.5f, -0.5f, 0.0f,
            0.0f,  0.5f, 0.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    while(!glfwWindowShouldClose(window)){
        processInput(window);

        glClearColor(0.5,0.5,0.5,1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void onResize(GLFWwindow* window, int height, int width) {
}

void processInput(GLFWwindow* window){
    if(glfwGetKey(window, GLFW_KEY_ESCAPE)){
        glfwSetWindowShouldClose(window, true);
    }
}

std::string loadShader(std::string path) {
    std::ifstream fileStream(path);
    std::stringstream stringStrem;
    stringStrem << fileStream.rdbuf();
    return stringStrem.str();
}
