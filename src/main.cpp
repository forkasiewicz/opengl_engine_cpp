#include "utils.hpp"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <fstream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

constexpr float TAU = 6.28318530718f;
constexpr float PI = 3.14159265359f;

struct Key {
  bool w = false;
  bool a = false;
  bool s = false;
  bool d = false;
  bool shift = false;
  bool space = false;
};

class Shader {
  u32 id;

public:
  Shader(const std::string &vertex_path, const std::string &fragment_path) {
    std::string vertex_source = loadFile(vertex_path);
    std::string fragment_source = loadFile(fragment_path);

    u32 vs = compileShader(GL_VERTEX_SHADER, vertex_source);
    u32 fs = compileShader(GL_FRAGMENT_SHADER, fragment_source);

    id = glCreateProgram();
    glAttachShader(id, vs);
    glAttachShader(id, fs);
    glLinkProgram(id);

    GLint success;
    glGetProgramiv(id, GL_LINK_STATUS, &success);

    if (!success) {
      char info_log[512];
      glGetProgramInfoLog(id, 512, nullptr, info_log);
      std::cerr << info_log << '\n';
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
  }

  ~Shader() { glDeleteProgram(id); }

  std::string loadFile(const std::string &path) {
    std::ifstream file(path);
    std::stringstream buffer;

    buffer << file.rdbuf();
    return buffer.str();
  }

  u32 compileShader(u32 type, const std::string &source) {
    u32 shader = glCreateShader(type);
    const char *src = source.c_str();

    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
      char info_log[512];
      glGetShaderInfoLog(shader, 512, nullptr, info_log);
      std::cerr << info_log << '\n';
    }

    return shader;
  }

  void use() { glUseProgram(id); }

  void setMat4(const std::string &name, const glm::mat4 &mat) {
    GLint loc = glGetUniformLocation(id, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
  }

  void setVec3(const std::string &name, const glm::vec3 &vec) {
    GLint loc = glGetUniformLocation(id, name.c_str());
    glUniform3fv(loc, 1, &vec[0]);
  }

  void setFloat(const std::string &name, const f32 &f) {
    GLint loc = glGetUniformLocation(id, name.c_str());
    glUniform1fv(loc, 1, &f);
  }
};

class Engine {
  SDL_Window *window = nullptr;
  std::unique_ptr<Shader> shader;
  SDL_GLContext gl_context = nullptr;
  u32 width = 1280;
  u32 height = 720;
  bool running = false;
  GLuint VAO, VBO, EBO;

  f32 yaw = glm::radians(-90.0f);
  f32 pitch = 0.0f;
  f32 sensitivity = 0.001f;

  u64 t_last = 0;
  u64 t_now = SDL_GetPerformanceCounter();
  f64 delta_time = 0;

  Key key;

  glm::vec3 camera_pos = {0.0f, 0.0f, 3.0f};

public:
  ~Engine() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    if (gl_context)
      SDL_GL_DestroyContext(gl_context);

    if (window)
      SDL_DestroyWindow(window);

    SDL_Quit();
  }

  bool initialize_sdl() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
      exit(EXIT_FAILURE);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow("Engine", width, height,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window) {
      std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
      return false;
    }

    gl_context = SDL_GL_CreateContext(window);

    if (!gl_context) {
      std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << '\n';
      return false;
    }

    if (!SDL_GL_MakeCurrent(window, gl_context)) {
      std::cerr << "SDL_GL_MakeCurrent failed: " << SDL_GetError() << '\n';
      return false;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
      std::cerr << "Failed to initialize GLAD\n";
      return false;
    }

    SDL_GL_SetSwapInterval(1);
    SDL_SetWindowRelativeMouseMode(window, true);

    return true;
  }

  void process_events() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_MOUSE_MOTION: {
        yaw += event.motion.xrel * sensitivity;
        pitch -= event.motion.yrel * sensitivity;

        pitch = glm::clamp(pitch, glm::radians(-89.0f), glm::radians(89.0f));
      } break;
      case SDL_EVENT_KEY_DOWN:
        switch (event.key.key) {
        case SDLK_W:
          key.w = true;
          break;
        case SDLK_A:
          key.a = true;
          break;
        case SDLK_S:
          key.s = true;
          break;
        case SDLK_D:
          key.d = true;
          break;
        case SDLK_LSHIFT:
          key.shift = true;
          break;
        case SDLK_SPACE:
          key.space = true;
          break;
        }
        break;
      case SDL_EVENT_KEY_UP:
        switch (event.key.key) {
        case SDLK_W:
          key.w = false;
          break;
        case SDLK_A:
          key.a = false;
          break;
        case SDLK_S:
          key.s = false;
          break;
        case SDLK_D:
          key.d = false;
          break;
        case SDLK_LSHIFT:
          key.shift = false;
          break;
        case SDLK_SPACE:
          key.space = false;
          break;
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        // code here
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        // code here
        break;
      case SDL_EVENT_QUIT:
        running = false;
        break;
      }
    }
  }

  void initialize_shader() {
    f32 vertices[] = {
        0.0f,  0.5f,  0.0f,  //
        -0.5f, -0.5f, 0.5f,  //
        0.5f,  -0.5f, 0.5f,  //
        0.5f,  -0.5f, -0.5f, //
        -0.5f, -0.5f, -0.5f  //
    };

    u32 indices[] = {
        0, 1, 2, //
        0, 2, 3, //
        0, 3, 4, //
        0, 4, 1, //
        1, 2, 4, //
        2, 3, 4  //
    };

    glEnable(GL_DEPTH_TEST);
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (void *)0);
    glEnableVertexAttribArray(0);

    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(f32),
    //                       (void *)(2 * sizeof(f32)));
    // glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    shader = std::make_unique<Shader>("res/vertex.glsl", "res/fragment.glsl");
  }

  void update() {
    t_last = t_now;
    t_now = SDL_GetPerformanceCounter();

    f64 freq = (f64)SDL_GetPerformanceFrequency();
    delta_time = (f64)(t_now - t_last) / freq;
  };

  void render() {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader->use();

    f32 aspect = (f32)width / (f32)height;

    glm::mat4 projection = glm::mat4(1.0f);
    projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    shader->setMat4("projection", projection);

    // glm::mat4 view = glm::mat4(1.0f);
    // view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

    glm::vec3 front;

    front.x = cos(yaw) * cos(pitch);
    front.y = sin(pitch);
    front.z = sin(yaw) * cos(pitch);

    front = glm::normalize(front);

    glm::mat4 view = glm::lookAt(camera_pos, camera_pos + front,
                                 glm::vec3(0.0f, 1.0f, 0.0f));

    f32 camera_speed = 0.03f;

    if (key.w) {
      camera_pos += camera_speed * front;
    }

    if (key.s) {
      camera_pos -= camera_speed * front;
    }

    if (key.a) {
      camera_pos -=
          glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f))) *
          camera_speed;
    }

    if (key.d) {
      camera_pos +=
          glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f))) *
          camera_speed;
    }

    if (key.shift) {
      camera_pos -= glm::vec3(0.0f, 1.0f, 0.0f) * camera_speed;
    }

    if (key.space) {
      camera_pos += glm::vec3(0.0f, 1.0f, 0.0f) * camera_speed;
    }

    shader->setMat4("view", view);

    glBindVertexArray(VAO);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0, 0, 0));
    shader->setMat4("model", model);

    const GLvoid *offset = 0;

    shader->setVec3("custom_color", glm::vec3(0.0f, 1.0f, 0.0f));
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, offset);
    offset = (const GLvoid *)((uintptr_t)offset + 3 * sizeof(u32));

    shader->setVec3("custom_color", glm::vec3(1.0f, 0.0f, 0.0f));
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, offset);
    offset = (const GLvoid *)((uintptr_t)offset + 3 * sizeof(u32));

    shader->setVec3("custom_color", glm::vec3(0.0f, 0.0f, 1.0f));
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, offset);
    offset = (const GLvoid *)((uintptr_t)offset + 3 * sizeof(u32));

    shader->setVec3("custom_color", glm::vec3(1.0f, 0.0f, 1.0f));
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, offset);
    offset = (const GLvoid *)((uintptr_t)offset + 3 * sizeof(u32));

    shader->setVec3("custom_color", glm::vec3(1.0f, 1.0f, 0.0f));
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, offset);
    offset = (const GLvoid *)((uintptr_t)offset + 3 * sizeof(u32));

    shader->setVec3("custom_color", glm::vec3(0.0f, 1.0f, 1.0f));
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, offset);

    SDL_GL_SwapWindow(window);
  }

  void run() {
    running = true;

    initialize_shader();

    while (running) {
      process_events();
      update();
      render();
    }
  }
};

int main(void) {
  Engine engine;

  if (!engine.initialize_sdl()) {
    return EXIT_FAILURE;
  }

  engine.run();

  return EXIT_SUCCESS;
}
