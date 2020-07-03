#ifndef CHESSAI_GRAPHICS_OPENGL_H_
#define CHESSAI_GRAPHICS_OPENGL_H_

#define GL_SILENCE_DEPRECATION
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <thread>
#include <fstream>

#include "shader.h"

#include "chess/piece.fwd.h"
#include "chess/game.fwd.h"
#include "player/player.h"

namespace graphics {

class OpenGL {
  public:
    static void run_graphics(game::Game* game, std::string game_name);
    static OpenGL* get_instance(game::Game* game, std::string game_name);
    ~OpenGL();

    void run();

  private:
    static std::map<GLFWwindow*, OpenGL*> _opengl_map;

    OpenGL(game::Game* g, std::string game_name);
    
    inline const static std::string ASSETS_2D_FILE_PATH_HEADER = "../assets/2D/";
    std::string asset_file_path_header;


    void initialize();

    GLuint getCoordBuffer(int r, int c);
    GLuint getTextureBuffer();

    GLuint loadTexture(std::string fileName);
    void loadTextures();

    inline static void onMouseButtonClick(GLFWwindow* window, int button, int action, int mods) {
      _opengl_map[window] -> mouseClicked(window, button, action, mods);
    }
    inline static void onKeyboardKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods) {
      _opengl_map[window] -> keyboardPressed(window, key, scancode, action, mods);
    }

    void mouseClicked(GLFWwindow* window, int button, int action, int mods);
    void keyboardPressed(GLFWwindow* window, int key, int scancode, int action, int mods);
    inline void promotionHelper(piece::PieceType type) {
      if (_human_white != nullptr)
        _human_white -> setPawnUpgradeType(type);
      if (_human_black != nullptr)
        _human_black -> setPawnUpgradeType(type);
    }


    GLFWwindow* _window;
    GLuint _shader_programID, _vaoID, _mvp_matrixID, _texture_samplerID;
    glm::mat4 _mvp_matrix;
    
    game::Game* _game;
    game::Board* _board;

    player::Player* _white;
    player::Player* _black;
    
    player::HumanPlayer* _human_white;
    player::HumanPlayer* _human_black;

    std::map<std::string, GLuint> _texture_map;
    std::string _window_title, _game_name;

    int _file_save_counter;
};

}

#endif // CHESSAI_GRAPHICS_OPENGL_H_