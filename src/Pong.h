#pragma once

#include "SDL.h"

#include <random>

struct Ball {
  float x, y;
  int w, h;
  float x_v, y_v;

  const static int width  = 10;
  const static int height = 10;
  const static int v      = 5;
};

struct Paddle {
  float x, y;
  int w, h;
  float max_v;

  const static int   width        =   10;
  const static int   height       =   50;
  const static int   behind       =   20;
  constexpr static float ai_max_v = 4.0f;
};

struct Arena {
  float x, y;
  int w, h;

  const static int border = 5;
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

  Ball ball;
  Paddle left_paddle, right_paddle;
  Arena arena;
  int left_score, right_score;

  std::default_random_engine g;
  std::uniform_real_distribution<float> init_ball_angle_dist {-5 * M_PI / 12, 5 * M_PI / 12};
};
