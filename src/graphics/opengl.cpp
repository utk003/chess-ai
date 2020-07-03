#include "opengl.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <string>

#include "player/player.h"
#include "chess/game.h"

std::map<GLFWwindow*, graphics::OpenGL*> graphics::OpenGL::_opengl_map;

void graphics::OpenGL::run_graphics(game::Game* game, std::string game_name) {
  OpenGL* opengl = get_instance(game, game_name);
  opengl -> run();
  delete opengl;
}

graphics::OpenGL* graphics::OpenGL::get_instance(game::Game* game, std::string game_name) {
  OpenGL* instance = new OpenGL(game, game_name);
  instance -> initialize();

  assert(instance -> _window != nullptr);
  _opengl_map[instance -> _window] = instance;

  return instance;
}

graphics::OpenGL::OpenGL(game::Game* game, std::string game_name) {
  _game = game;
  _board = game -> board();

  asset_file_path_header = ASSETS_2D_FILE_PATH_HEADER;

  _white = game -> white_player();
  assert(_white != nullptr);
  if (_white -> type().isHumanPlayer())
    _human_white = (player::HumanPlayer*) _white;
  
  _black = game -> black_player();
  assert(_black != nullptr);
  if (_black -> type().isHumanPlayer())
    _human_black = (player::HumanPlayer*) _black;
  
  _game_name = game_name;
  _window_title = _game_name + ": " + _white -> type().toString() + " vs " + _black -> type().toString();

  _file_save_counter = 0;
}
graphics::OpenGL::~OpenGL() {
  _opengl_map.erase(_window);

  // Clean up shader
  glDeleteProgram(_shader_programID);
  glDeleteVertexArrays(1, &_vaoID);

  glfwTerminate();
}

GLuint graphics::OpenGL::getCoordBuffer(int r, int c) {
  r -= 4;
  c -= 4;
  
  GLfloat g_vertex_buffer_data[] = {
    2.0f * c,       2.0f * r,       0.0f,
    2.0f * c,       2.0f * (r + 1), 0.0f,
    2.0f * (c + 1), 2.0f * (r + 1), 0.0f,
    2.0f * c,       2.0f * r,       0.0f,
    2.0f * (c + 1), 2.0f * (r + 1), 0.0f,
    2.0f * (c + 1), 2.0f * r,       0.0f,
  };

  GLuint vertexbuffer;
  glGenBuffers(1, &vertexbuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

  return vertexbuffer;
}
GLuint graphics::OpenGL::getTextureBuffer() {
  GLfloat g_vertex_buffer_data[] = {
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
  };

  GLuint vertexbuffer;
  glGenBuffers(1, &vertexbuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

  return vertexbuffer;
}

GLuint graphics::OpenGL::loadTexture(std::string fileName) {
  int w, h, channels;
  stbi_uc* buf = stbi_load(fileName.c_str(), &w, &h, &channels, 4);
  assert(buf != nullptr);

  GLuint textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(buf);

  return textureID;
}
void graphics::OpenGL::loadTextures() {
  std::string pieceNames[6] = {"rook", "pawn", "king", "queen", "knight", "bishop"};
  std::string pieceColors[2] = {"black", "white"};
  std::string backColors[2] = {"0", "1"};

  std::string str;
  for (std::string backColor: backColors) {
    for (std::string name: pieceNames)
      for (std::string pieceColor: pieceColors) {
        str = asset_file_path_header + "piece/" + backColor + "-" + pieceColor + "_" + name + ".png";
        _texture_map[str] = loadTexture(str);
      }
    str = asset_file_path_header + "piece/" + backColor + "-transparent.png";
    _texture_map[str] = loadTexture(str);
  }

  str = asset_file_path_header + "overlay/selected.png";
  _texture_map[str] = loadTexture(str);
}

void graphics::OpenGL::mouseClicked(GLFWwindow* window, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    int w, h;
    glfwGetWindowSize(window, &w, &h);

    int r = 7 - (int) (8 * y / h), c = (int) (8 * x / w);

    if (_human_white != nullptr)
      _human_white -> clickSquare(r, c);
    if (_human_black != nullptr)
      _human_black -> clickSquare(r, c);
  }
}
void graphics::OpenGL::keyboardPressed(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (action == GLFW_RELEASE) {
    switch (key) {
      case GLFW_KEY_Q:
        promotionHelper(piece::PieceType::QUEEN);
        break;
      
      case GLFW_KEY_R:
        promotionHelper(piece::PieceType::ROOK);
        break;
      
      case GLFW_KEY_K:
        promotionHelper(piece::PieceType::KNIGHT);
        break;
      
      case GLFW_KEY_B:
        promotionHelper(piece::PieceType::BISHOP);
        break;
      
      case GLFW_KEY_Z:
        _board -> undoMove(_game);
        break;
      
      case GLFW_KEY_S:
        std::ofstream myfile;
        myfile.open ("../game_state/save " + std::to_string(_file_save_counter++) + ".txt");
        myfile << _board -> toString();
        myfile.close();
        break;
    }
  }
}

void graphics::OpenGL::initialize() {
  // Initialize GLFW
  if (!glfwInit())
  {
    fprintf(stderr, "Failed to initialize GLFW\n");
    assert(false);
  }

  glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

  // Open a window and create its OpenGL context
  _window = glfwCreateWindow(1024, 1024, _window_title.c_str(), nullptr, nullptr);
  if (_window == nullptr) {
    fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
    glfwTerminate();
    assert(false);
  }
  glfwMakeContextCurrent(_window);

  // Initialize GLEW
  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    glfwTerminate();
    assert(false);
  }

  // Ensure we can capture the escape key being pressed below
  glfwSetInputMode(_window, GLFW_STICKY_KEYS, GL_TRUE);

  // Dark blue background
  glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

  // OpenGL settings
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Load all textures
  loadTextures();

  // Create and compile our GLSL program from the shaders
  _shader_programID = graphics::loadShaders("../shaders/vertex_shader.vs", "../shaders/fragment_shader.fs");

  // Mouse button callback
  glfwSetMouseButtonCallback(_window, onMouseButtonClick);
  // Keyboard key callback
  glfwSetKeyCallback(_window, onKeyboardKeyPress);

  // Set up VAO
  glGenVertexArrays(1, &_vaoID);
  glBindVertexArray(_vaoID);

  // Get a handle for our "MVP" uniform
  _mvp_matrixID = glGetUniformLocation(_shader_programID, "MVP");
  glm::mat4 proj_matrix = glm::ortho(-8.0f, 8.0f, -8.0f, 8.0f, 0.0f, 100.0f);
  glm::mat4 view_matrix = glm::lookAt(glm::vec3(0,0,4), glm::vec3(0,0,0), glm::vec3(0,1,0));
  glm::mat4 model_matrix = glm::mat4(1.0f);
  _mvp_matrix = proj_matrix * view_matrix * model_matrix;

  // Get texture sampler ID
  _texture_samplerID = glGetUniformLocation(_shader_programID, "texture_sampler");
}

void graphics::OpenGL::run() {
  int r, c, len = _board -> length(), wid = _board -> width(), count = 0;
  do {
    // std::cout << count++ << std::endl;
    
    if (_game -> isOver()) {
      game::GameResult result = _game -> getResult();
      if (result.isBlackWin())
        _window_title = _game_name + ": Black won";
      else if (result.isWhiteWin())
        _window_title = _game_name + ": White won";
      else if (result.isStalemate())
        _window_title = _game_name + ": Stalemate";
    }
    // Update Window Title
    glfwSetWindowTitle(_window, _window_title.c_str());

    // Clear the screen.
    glClear(GL_COLOR_BUFFER_BIT);

    // Use our shaders
    glUseProgram(_shader_programID);

    // Send our transformation to the currently bound shader, 
    // in the "MVP" uniform
    glUniformMatrix4fv(_mvp_matrixID, 1, GL_FALSE, &_mvp_matrix[0][0]);
    glUniform1i(_texture_samplerID, 0);


    // Enable drawing attributes/textures
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glActiveTexture(GL_TEXTURE0);


    // Create VBOs
    GLuint textbuff = getTextureBuffer();

    // Draw Images
    for (r = 0; r < len; ++r)
      for (c = 0; c < wid; ++c) {
        GLuint buffer = getCoordBuffer(r, c);

        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glBindBuffer(GL_ARRAY_BUFFER, textbuff);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

        int backColorIndex = (r + c) % 2;
        piece::Piece* piece = _board -> getPiece(r, c);
        std::string filePath = asset_file_path_header + "piece/" + std::to_string(backColorIndex) + "-" + piece -> image_file_path();

        glBindTexture(GL_TEXTURE_2D, _texture_map[filePath]);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDeleteBuffers(1, &buffer);
      }
    if (_game -> selected_x() != -1 && _game -> selected_y() != -1) {
      GLuint selection_overlay = getCoordBuffer(_game -> selected_x(), _game -> selected_y());

      glBindBuffer(GL_ARRAY_BUFFER, selection_overlay);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

      glBindBuffer(GL_ARRAY_BUFFER, textbuff);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

      std::string filePath = asset_file_path_header + "overlay/selected.png";

      glBindTexture(GL_TEXTURE_2D, _texture_map[filePath]);

      glDrawArrays(GL_TRIANGLES, 0, 6);

      glDeleteBuffers(1, &selection_overlay);
    }
    // Clean up VBOs
    glDeleteBuffers(1, &textbuff);

    // Disable drawing attributes
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    
    // Swap buffers
    glfwSwapBuffers(_window);
    glfwPollEvents();

  } // Check if the ESC key was pressed or the window was closed
  while (glfwGetKey(_window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(_window) == 0);
}