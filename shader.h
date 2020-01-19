//
// Created by Mikael Deurell on 2019-01-26.
//

#pragma once

#include <fstream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>

class Shader {
public:
  Shader(const std::string &vertexPath, const std::string &fragmentPath,
         const std::string &glslVersion) {
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vertexFile;
    std::ifstream fragmentFile;

    vertexFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragmentFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
      vertexFile.open(vertexPath);
      fragmentFile.open(fragmentPath);
      std::stringstream vStream, fStream;
      vStream << vertexFile.rdbuf();
      fStream << fragmentFile.rdbuf();

      vertexCode = vStream.str();
      vertexCode.insert(0, glslVersion);
      fragmentCode = fStream.str();
      fragmentCode.insert(0, glslVersion);

    } catch (std::ifstream::failure &e) {
      std::cout << "failed to load shaders from file: " << e.what();
    }

    const char *vCode = vertexCode.c_str();
    const char *fCode = fragmentCode.c_str();

    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vCode, nullptr);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(vertex, 512, nullptr, infoLog);
      std::cout << "failed to compile vertex shader: " << infoLog;
    }
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fCode, nullptr);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(fragment, 512, nullptr, infoLog);
      std::cout << "failed to compile fragment shader: " << infoLog;
    }

    mId = glCreateProgram();
    glAttachShader(mId, vertex);
    glAttachShader(mId, fragment);
    glLinkProgram(mId);
    glGetProgramiv(mId, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(mId, 512, nullptr, infoLog);
      std::cout << "failed to link the shader program: " << infoLog;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
  }

  void use() { glUseProgram(mId); }

  void setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(mId, name.c_str()), value);
  }

  void setFloat(const std::string &name, float value) {
    glUniform1f(glGetUniformLocation(mId, name.c_str()), value);
  }

  void setVec4f(const std::string &name, float x, float y, float z, float w) {
    glUniform4f(glGetUniformLocation(mId, name.c_str()), x, y, z, w);
  }

  void setMat4f(const std::string &name, glm::mat4 &m) {
    glUniformMatrix4fv(glGetUniformLocation(mId, name.c_str()), 1, GL_FALSE,
                       glm::value_ptr(m));
  }

  unsigned int mId;
};
