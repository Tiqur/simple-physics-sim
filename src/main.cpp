#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

using std::cout, std::endl;
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  (void)window;
  glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
  if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

const char *vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aColor;
    out vec3 fColor;
    uniform mat4 transform;
    void main() {
        gl_Position = transform * vec4(aPos, 1.0);
        fColor = aColor;
    }
)";

const char *fragmentShaderSource = R"(
    #version 330 core
    in vec3 fColor;
    out vec4 FragColor;
    void main() {
        FragColor = vec4(fColor, 1.0f);
    }
)";

class Shader {
public:
  Shader(const char *shaderSource, GLenum shaderType) {
    cout << "Creating shader..." << endl;
    m_id = glCreateShader(shaderType);
    glShaderSource(m_id, 1, &shaderSource, NULL);
    glCompileShader(m_id);

    int success;
    char infoLog[512];
    glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(m_id, 512, NULL, infoLog);
      cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }
  }

  ~Shader() {
    cout << "Deleting shader..." << endl;
    if (m_id != 0) {
      glDeleteShader(m_id);
    }
  }

  GLuint id() const { return m_id; }

private:
  GLuint m_id{};
};

class Program {
public:
  Program(Shader vertexShader, Shader fragmentShader) {
    cout << "Creating program..." << endl;
    m_id = glCreateProgram();
    glAttachShader(m_id, vertexShader.id());
    glAttachShader(m_id, fragmentShader.id());
    glLinkProgram(m_id);

    GLint success;
    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
    if (!success) {
      char infoLog[512];
      glGetProgramInfoLog(m_id, 512, NULL, infoLog);
      cout << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }
  }

  ~Program() {
    if (m_id != 0) {
      cout << "Deleting program..." << endl;
      glDeleteProgram(m_id);
    }
  }

  GLuint id() const { return m_id; }

private:
  GLuint m_id{};
};

class Buffer {
public:
  Buffer(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    cout << "Creating buffer..." << endl;
    glGenBuffers(1, &m_id);
    glBindBuffer(target, m_id);
    glBufferData(target, size, data, usage);
  }

  ~Buffer() {
    if (m_id != 0) {
      cout << "Deleting buffer..." << endl;
      glDeleteBuffers(1, &m_id);
    }
  }

  GLuint id() const { return m_id; }

private:
  GLuint m_id{};
};

class VertexArray {
public:
  VertexArray() {
    cout << "Creating vertex array..." << endl;
    glGenVertexArrays(1, &m_id);
  }

  void bind() { glBindVertexArray(m_id); }

  ~VertexArray() {
    if (m_id != 0) {
      cout << "Deleting array..." << endl;
      glDeleteVertexArrays(1, &m_id);
    }
  }

  GLuint id() const { return m_id; }

private:
  GLuint m_id{};
};

int main() {
  std::vector<float> vertices;
  std::vector<unsigned int> indices;

  auto origin = glm::ivec2(0, 0);
  auto step_rad = 64;
  auto radius_px = 0.1f;
  auto gravitational_acceleration = -9.81;

  // Origin
  vertices.push_back(origin.x); // x
  vertices.push_back(origin.y); // y
  vertices.push_back(0.0f);     // z
  vertices.push_back(1.0f);     // r
  vertices.push_back(1.0f);     // g
  vertices.push_back(1.0f);     // b

  // Generate vertices around origin
  for (int i = 0; i < step_rad * 2; i++) {
    double theta = (M_PI / step_rad) * i;
    double y = radius_px * sin(theta);
    double x = radius_px * cos(theta);

    // Position
    vertices.push_back(x + origin.x);
    vertices.push_back(y + origin.y);
    vertices.push_back(0.0f);

    // Color - Varying colors for better visualization
    vertices.push_back(0.5f + 0.5f * sin(theta));
    vertices.push_back(0.5f + 0.5f * cos(theta));
    vertices.push_back(0.7f);
  }

  // Generate indices for triangles
  for (int i = 1; i <= step_rad * 2; i++) {
    indices.push_back(0); // Origin
    indices.push_back(i);

    if (i < step_rad * 2) {
      indices.push_back(i + 1);
    } else {
      indices.push_back(1);
    }
  }

  // Initialize GLFW
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(800, 600, "OpenGL Circle", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  // Make the OpenGL context current
  glfwMakeContextCurrent(window);

  // Initialize GLEW
  glewExperimental = GL_TRUE; // Ensure GLEW uses modern OpenGL techniques
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW" << std::endl;
    return -1;
  }

  // Initialize ImGui
  std::cout << "Initializing ImGui..." << std::endl;
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  std::cout << "Initializing ImGui GLFW backend..." << std::endl;
  if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
    std::cerr << "Failed to initialize ImGui GLFW backend!" << std::endl;
    return -1;
  }

  std::cout << "Initializing ImGui OpenGL backend..." << std::endl;
  if (!ImGui_ImplOpenGL3_Init("#version 330")) {
    std::cerr << "Failed to initialize ImGui OpenGL backend!" << std::endl;
    return -1;
  }

  // Set the viewport
  glViewport(0, 0, 800, 600);

  // Register the framebuffer size callback
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // Create buffers and shaders
  Buffer VBO(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(),
             GL_STATIC_DRAW);
  Buffer EBO(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
             indices.data(), GL_STATIC_DRAW);
  Shader vertexShader(vertexShaderSource, GL_VERTEX_SHADER);
  Shader fragmentShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
  Program shaderProgram(vertexShader, fragmentShader);

  VertexArray VAO;
  VAO.bind();

  glBindBuffer(GL_ARRAY_BUFFER, VBO.id());
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  auto last_time = std::chrono::high_resolution_clock::now();
  float a = gravitational_acceleration;
  float v = 0.0f;
  float y = 0.0f;

  while (!glfwWindowShouldClose(window)) {
    auto time_now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(time_now - last_time).count();
    last_time = time_now;

    v += a * dt;
    y += v * dt;

    // Collision detection
    if (y <= -1.0f) {
      y = -1.0f;
      v *= -0.95f;
      std::cout << "COLLISION" << std::endl;
    }

    processInput(window);

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Mesh debugger window
    if (ImGui::Begin("Mesh Debugger")) {
      ImGui::Text("Vertices: %zu", vertices.size() / 6);
      for (size_t i = 0; i < vertices.size() / 6; ++i) {
        size_t baseIdx = i * 6;
        ImGui::BulletText(
            "Vertex %zu: (%.2f, %.2f, %.2f) - Color: (%.2f, %.2f, %.2f)", i,
            vertices[baseIdx], vertices[baseIdx + 1], vertices[baseIdx + 2],
            vertices[baseIdx + 3], vertices[baseIdx + 4],
            vertices[baseIdx + 5]);
      }

      ImGui::Separator();

      ImGui::Text("Indices: %zu triangles", indices.size() / 3);
      for (size_t i = 0; i < indices.size(); i += 3) {
        ImGui::BulletText("Triangle %zu: %u, %u, %u", i / 3, indices[i],
                          indices[i + 1], indices[i + 2]);
      }
      ImGui::End();
    }

    // Settings window
    // bool active = true;
    // float min = 0.0f;
    // float max = 0.1f;
    // ImGui::Begin("Settings", &active, ImGuiWindowFlags_MenuBar);
    // ImGui::SliderScalar("Rotation speed", ImGuiDataType_Float,
    // &rotationDegrees,
    //                    &min, &max, "%.3f per frame");
    // ImGui::End();

    // Render OpenGL
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram.id());

    // Update transformation matrix
    glm::mat4 transformMatrix =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, y, 0.0f));
    GLint transformLoc = glGetUniformLocation(shaderProgram.id(), "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE,
                       glm::value_ptr(transformMatrix));

    // Draw the circle
    glBindVertexArray(VAO.id());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO.id());
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Unbind OpenGL objects
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);

  // Clean up and exit
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
