#pragma once

#include "SDL.h"

#include <random>

struct Ball {
  float x, y;
  float x_v, y_v;

  void change_angle (float);

  constexpr static float width  = 5.0f;
  constexpr static float height = 5.0f;
  constexpr static float v      = 5.0f;
};

struct Paddle {
  float x, y;

  constexpr static float width    =    5.0f;
  constexpr static float height   =   25.0f;
  constexpr static float behind   =   25.0f;
  constexpr static float ai_max_v =    4.5f;
};

struct Arena {
  float x, y, width, height;

  constexpr static float border = 5.0f;
};

class Pong {
public:
  Pong (SDL_Renderer*);

  void run ();

private:

  enum class State {
    ROUND_BEGIN,
    BALL_IN_PLAY
  };

  State state;

  [[noreturn]] void update ();
  [[noreturn]] void render () const;
  void handle_events ();

  void update_round_begin ();
  void update_ball_in_play ();

  SDL_Renderer* ren;
  Rect ren_rect;

  Ball ball;
  Paddle left_paddle, right_paddle;
  Arena arena;
  int left_score, right_score;

  std::default_random_engine g;
  std::uniform_real_distribution<float> init_ball_angle_dist {-5 * M_PI / 12, 5 * M_PI / 12};
};
