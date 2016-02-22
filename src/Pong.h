#pragma once

struct SDL_Renderer;

struct Rect {
  float x, y, w, h;

  Rect (float, float, float, float);
};

template <typename T>
class Affects
{
  virtual void affect (T&) const = 0;
};

struct Ball : 
  public Rect 
{
  using Rect::Rect;

  float v, x_v, y_v;

  void update ();
  void change_angle (float);
};

struct Paddle : 
  public Rect, public Affects<Ball> 
{
public:
  using Rect::Rect;

  virtual void affect (Ball&) const override final;
};

struct PlayerPaddle :
  public Paddle
{
  using Paddle::Paddle;

  void update ();
};

struct AIPaddle :
  public Paddle
{
  using Paddle::Paddle;

  void update ();
};

struct Wall :
  public Rect, public Affects<Ball>
{
public:
  using Rect::Rect;

  virtual void affect (Ball&) const override final;
};

struct Zone :
  public Rect, public Affects<Ball>
{
public:
  using Rect::Rect;

  virtual void affect (Ball&) const override final;
  unsigned int counter () const;

private:

  unsigned int count = 0;
};

class Pong 
{
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
  Wall top_wall, bottom_wall;
  Zone left_zone, right_zone;
  unsigned left_score = 0, right_score = 0;
};
