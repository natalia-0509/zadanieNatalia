#include <iostream>
#include <string>
#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"
#include "ObjLoader.h"

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif


// ---- Prosta kamera FPS w main.cpp (żeby nie dodawać kolejnego pliku) ----
struct CameraFPS {
    glm::vec3 pos{0.f, 1.2f, 3.0f};
    float yaw = -90.f;
    float pitch = 0.f;
    float fov = 60.f;

    float speed = 3.5f;
    float sensitivity = 0.12f;

    glm::vec3 front() const {
        float cy = cos(glm::radians(yaw));
        float sy = sin(glm::radians(yaw));
        float cp = cos(glm::radians(pitch));
        float sp = sin(glm::radians(pitch));
        glm::vec3 f{cy * cp, sp, sy * cp};
        return glm::normalize(f);
    }
    glm::vec3 right() const {
        return glm::normalize(glm::cross(front(), glm::vec3(0,1,0)));
    }
    glm::mat4 view() const {
        return glm::lookAt(pos, pos + front(), glm::vec3(0,1,0));
    }

    void mouseDelta(float dx, float dy) {
        yaw += dx * sensitivity;
        pitch -= dy * sensitivity;
        pitch = std::clamp(pitch, -89.f, 89.f);
    }
};

static int W = 1280, H = 720;
static CameraFPS cam;
static bool firstMouse = true;
static double lastX = 0, lastY = 0;
static float deltaTime = 0.f;
static float lastTime = 0.f;

static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    W = w; H = h;
    glViewport(0, 0, W, H);
}

static void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float dx = (float)(xpos - lastX);
    float dy = (float)(ypos - lastY);
    lastX = xpos; lastY = ypos;
    cam.mouseDelta(dx, dy);
}

static void scroll_callback(GLFWwindow*, double, double yoff) {
    cam.fov -= (float)yoff * 2.0f;
    cam.fov = std::clamp(cam.fov, 20.f, 90.f);
}

static void processInput(GLFWwindow* win) {
    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(win, 1);

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) cam.pos += cam.front() * cam.speed * deltaTime;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) cam.pos -= cam.front() * cam.speed * deltaTime;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) cam.pos -= cam.right() * cam.speed * deltaTime;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) cam.pos += cam.right() * cam.speed * deltaTime;
}

static GLuint LoadTexture2D(const std::string& path) {
    int w,h,comp;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &comp, 0);
    if (!data) {
        std::cerr << "Nie moge wczytac tekstury: " << path << "\n";
        return 0;
    }

    GLenum fmt = (comp == 4) ? GL_RGBA : GL_RGB;

    GLuint tex=0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (glfwExtensionSupported("GL_EXT_texture_filter_anisotropic")) {
        float aniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
        if (aniso < 1.0f) aniso = 1.0f;
        if (aniso > 16.0f) aniso = 16.0f; // bezpieczny limit
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
    }


    stbi_image_free(data);
    return tex;
}

int main() {
    // GLFW
    if (!glfwInit()) {
        std::cerr << "GLFW init fail\n";
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* win = glfwCreateWindow(W, H, "OBJ Viewer", nullptr, nullptr);
    if (!win) {
        std::cerr << "Window create fail\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetCursorPosCallback(win, mouse_callback);
    glfwSetScrollCallback(win, scroll_callback);

    // FPS: schowaj kursor
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD load fail\n";
        return 1;
    }
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Shader
    Shader sh("shaders/phong.vert", "shaders/phong.frag");

    // OBJ + MTL
    // U Ciebie: assets/girl OBJ.obj i assets/girl OBJ.mtl
    LoadedModel model = LoadOBJ_WithMTL("assets/girl OBJ.obj", "assets");

    // --- Auto ustawienie kamery na model (bounding box) ---
    glm::vec3 mn( 1e30f), mx(-1e30f);
    for (const auto& v : model.vertices) {
        mn = glm::min(mn, v.pos);
        mx = glm::max(mx, v.pos);
    }
    glm::vec3 center = (mn + mx) * 0.5f;
    float radius = glm::length(mx - mn) * 0.5f;
    if (radius < 0.0001f) radius = 1.0f;

    // jeśli skalujesz modelM w renderze, musisz uwzględnić tę samą skalę tutaj:
    float modelScale = 0.02f;  // <- ustaw tak jak w renderze
    float dist = radius * modelScale * 3.0f; // 3 promienie przed modelem

    // start kamery: przed modelem na osi Z
    cam.pos = center * modelScale + glm::vec3(0.0f, 0.0f, dist);

    // ustaw yaw/pitch, żeby patrzeć na środek
    glm::vec3 dir = glm::normalize(center * modelScale - cam.pos);
    cam.yaw = glm::degrees(atan2(dir.z, dir.x)) - 90.0f;
    cam.pitch = glm::degrees(asin(dir.y));


    // Tekstury materiałów
    for (auto& [name, mat] : model.materials) {
        if (!mat.mapKd.empty()) {
            mat.glTex = LoadTexture2D(mat.mapKd);
        }
    }

    // VAO/VBO/EBO
    GLuint VAO=0, VBO=0, EBO=0;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size()*sizeof(Vertex), model.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size()*sizeof(uint32_t), model.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nrm));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // domyślna biała tekstura (gdy brak map_Kd)
    GLuint whiteTex = 0;
    {
        unsigned char px[3] = {255,255,255};
        glGenTextures(1, &whiteTex);
        glBindTexture(GL_TEXTURE_2D, whiteTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1,1, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // Render
    while (!glfwWindowShouldClose(win)) {
        float t = (float)glfwGetTime();
        deltaTime = t - lastTime;
        lastTime = t;

        processInput(win);

        glClearColor(0.08f, 0.09f, 0.10f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 modelM(1.0f);
        // jeśli model jest gigantyczny/mały, możesz przeskalować:
        modelM = glm::scale(modelM, glm::vec3(0.02f));

        glm::mat4 view = cam.view();
        glm::mat4 proj = glm::perspective(glm::radians(cam.fov), (float)W/(float)H, 0.05f, 500.0f);

        sh.use();
        sh.setMat4("uModel", modelM);
        sh.setMat4("uView", view);
        sh.setMat4("uProj", proj);

        sh.setVec3("uViewPos", cam.pos);
        glm::vec3 camFront = cam.front();
        glm::vec3 camRight = cam.right();
        glm::vec3 camUp    = glm::vec3(0,1,0);

        glm::vec3 lightDir = -(camFront * 1.0f + camUp * 0.4f + camRight * 0.2f);
        sh.setVec3("uLightDir", glm::normalize(lightDir));
        sh.setVec3("uLightColor", glm::vec3(1.0f));
        sh.setVec3("uMat.Ks", glm::vec3(0.25f));
        sh.setFloat("uMat.Ns", 64.0f);
        sh.setVec3("uLightDir", glm::normalize(glm::vec3(-1.f, -1.f, -0.5f)));
        sh.setVec3("uLightColor", glm::vec3(1.f));
        sh.setInt("uDiffuse", 0);

        glBindVertexArray(VAO);

        for (const auto& sm : model.submeshes) {
            Material mat{};
            auto it = model.materials.find(sm.materialName);
            if (it != model.materials.end()) mat = it->second;
            else {
                mat.Kd = glm::vec3(1.f);
                mat.Ks = glm::vec3(0.2f);
                mat.Ns = 32.f;
                mat.glTex = 0;
            }

            sh.setVec3("uMat.Kd", mat.Kd);
            sh.setVec3("uMat.Ks", mat.Ks);
            sh.setFloat("uMat.Ns", mat.Ns);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mat.glTex ? mat.glTex : whiteTex);

            glDrawElements(GL_TRIANGLES,
                           (GLsizei)sm.indexCount,
                           GL_UNSIGNED_INT,
                           (void*)(uintptr_t)(sm.indexOffset * sizeof(uint32_t)));
        }

        glBindVertexArray(0);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
