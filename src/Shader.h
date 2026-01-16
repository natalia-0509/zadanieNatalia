#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

static inline std::string ReadTextFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Nie moge otworzyc pliku: " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

class Shader {
public:
    GLuint id = 0;

    Shader(const std::string& vsPath, const std::string& fsPath) {
        std::string vsSrc = ReadTextFile(vsPath);
        std::string fsSrc = ReadTextFile(fsPath);

        GLuint vs = compile(GL_VERTEX_SHADER, vsSrc.c_str());
        GLuint fs = compile(GL_FRAGMENT_SHADER, fsSrc.c_str());

        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);

        GLint ok = 0;
        glGetProgramiv(id, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[2048];
            glGetProgramInfoLog(id, 2048, nullptr, log);
            throw std::runtime_error(std::string("Shader link error: ") + log);
        }

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    void use() const { glUseProgram(id); }

    void setMat4(const char* name, const glm::mat4& m) const {
        glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, glm::value_ptr(m));
    }
    void setVec3(const char* name, const glm::vec3& v) const {
        glUniform3fv(glGetUniformLocation(id, name), 1, glm::value_ptr(v));
    }
    void setFloat(const char* name, float v) const {
        glUniform1f(glGetUniformLocation(id, name), v);
    }
    void setInt(const char* name, int v) const {
        glUniform1i(glGetUniformLocation(id, name), v);
    }

private:
    static GLuint compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);

        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[2048];
            glGetShaderInfoLog(s, 2048, nullptr, log);
            throw std::runtime_error(std::string("Shader compile error: ") + log);
        }
        return s;
    }
};
