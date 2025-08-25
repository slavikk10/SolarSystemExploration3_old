//
// Created by Vyacheslav on 30.03.2025.
//

#ifndef OPENGLTEST_SHADER_H
#define OPENGLTEST_SHADER_H

#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath)
    {
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            vShaderFile.close();
            fShaderFile.close();

            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch(std::ifstream::failure e) {
            std::cout << "Error reading shader file.\n" << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        unsigned int vertex, fragment;
        int success;
        char infoLog[512];

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);

        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cout << "Error compiling vertex shader. Info log:\n" <<
                    infoLog << std::endl;
        }

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);

        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            std::cout << "Error compiling fragment shader. Info log:\n" <<
                      infoLog << std::endl;
        }

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);

        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if(!success) {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cout << "Error linking shader program. Info log:\n" <<
                    infoLog << std::endl;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    Shader(const char* vertexPath, const char* fragmentPath, const char* geomPath)
    {
        std::string vertexCode;
        std::string fragmentCode;
        std::string geomCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::ifstream gShaderFile;

        vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        gShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            gShaderFile.open(geomPath);
            std::stringstream vShaderStream, fShaderStream, gShaderStream;

            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            gShaderStream << gShaderFile.rdbuf();

            vShaderFile.close();
            fShaderFile.close();
            gShaderFile.close();

            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
            geomCode = gShaderStream.str();
        }
        catch(std::ifstream::failure e) {
            std::cout << "Error reading shader file.\n" << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        const char* gShaderCode = geomCode.c_str();

        unsigned int vertex, fragment, geometry;
        int success;
        char infoLog[512];

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);

        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cout << "Error compiling vertex shader. Info log:\n" <<
                    infoLog << std::endl;
        }

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);

        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            std::cout << "Error compiling fragment shader. Info log:\n" <<
                      infoLog << std::endl;
        }

        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &gShaderCode, NULL);
        glCompileShader(geometry);

        glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(geometry, 512, NULL, infoLog);
            std::cout << "Error compiling geometry shader. Info log:\n" <<
                      infoLog << std::endl;
        }


        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glAttachShader(ID, geometry);
        glLinkProgram(ID);

        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if(!success) {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cout << "Error linking shader program. Info log:\n" <<
                    infoLog << std::endl;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
        glDeleteShader(geometry);
    }

    Shader(const char* vertexPath, const char* fragmentPath, const char* tescPath, const char* tesePath)
    {
        std::string vertexCode;
        std::string fragmentCode;
        std::string tesscCode;
        std::string tesseCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::ifstream tcShaderFile;
        std::ifstream teShaderFile;

        vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        tcShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        teShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            tcShaderFile.open(tescPath);
            teShaderFile.open(tesePath);
            std::stringstream vShaderStream, fShaderStream, tcShaderStream, teShaderStream;

            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            tcShaderStream << tcShaderFile.rdbuf();
            teShaderStream << teShaderFile.rdbuf();

            vShaderFile.close();
            fShaderFile.close();
            tcShaderFile.close();
            teShaderFile.close();

            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
            tesscCode = tcShaderStream.str();
            tesseCode = teShaderStream.str();
        }
        catch(std::ifstream::failure e) {
            std::cout << "Error reading shader file.\n" << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        const char* tcShaderCode = tesscCode.c_str();
        const char* teShaderCode = tesseCode.c_str();

        unsigned int vertex, fragment, tess_control, tess_eval;
        int success;
        char infoLog[512];

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);

        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cout << "Error compiling vertex shader. Info log:\n" <<
                    infoLog << std::endl;
        }

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);

        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            std::cout << "Error compiling fragment shader. Info log:\n" <<
                      infoLog << std::endl;
        }

        tess_control = glCreateShader(GL_TESS_CONTROL_SHADER);
        glShaderSource(tess_control, 1, &tcShaderCode, NULL);
        glCompileShader(tess_control);

        glGetShaderiv(tess_control, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(tess_control, 512, NULL, infoLog);
            std::cout << "Error compiling tessellation control shader. Info log:\n" <<
                      infoLog << std::endl;
        }

        tess_eval = glCreateShader(GL_TESS_EVALUATION_SHADER);
        glShaderSource(tess_eval, 1, &teShaderCode, NULL);
        glCompileShader(tess_eval);

        glGetShaderiv(tess_eval, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(tess_eval, 512, NULL, infoLog);
            std::cout << "Error compiling tessellation evaluation shader. Info log:\n" <<
                      infoLog << std::endl;
        }


        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glAttachShader(ID, tess_control);
        glAttachShader(ID, tess_eval);
        glLinkProgram(ID);

        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if(!success) {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cout << "Error linking shader program. Info log:\n" <<
                    infoLog << std::endl;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
        glDeleteShader(tess_control);
        glDeleteShader(tess_eval);
    }

    void use()
    {
        glUseProgram(ID);
    }

    void setBool(const std::string &name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void setInt(const std::string &name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setFloat(const std::string &name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setVec2(const std::string &name, glm::vec2 value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec3(const std::string &name, glm::vec3 value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec4(const std::string &name, const glm::vec4 &value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setMat2(const std::string &name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat3(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
};

#endif //OPENGLTEST_SHADER_H
