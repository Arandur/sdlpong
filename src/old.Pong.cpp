#include "Pong.h"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

struct Bound {
  float min_x, max_x, min_y, max_y;
};

template <typename T>
auto bound (const T& t) ->
Bound
{
  return {
    t.x - t.width, t.x + t.width,
    t.y - t.width, t.y + t.width };
}

template <typename T>
auto rect (const T& t) ->
SDL_Rect
{
  return { 
    static_cast<int> (t.x - t.width), 
    static_cast<int> (t.y - t.height), 
    static_cast<int> (2 * t.width), 
    static_cast<int> (2 * t.height) };
}

template <typename T>
auto center (const T& t) ->
SDL_Point
{
  return { static_cast<int> (t.x), static_cast<int> (t.y) };
}

void Ball::change_angle (float a)
{
  x_v = v * std::cos (a);
  y_v = v * std::sin (a);
}

Pong::Pong (SDL_Renderer* _ren) :
  ren (_ren)
{
  SDL_ShowCursor (SDL_DISABLE);

  { // Arena init
    int w = 0, h = 0;
    SDL_GetRendererOutputSize (ren, &w, &h);
    arena = { 
      static_cast<float> (w) / 2, 
      static_cast<float> (h) / 2,
      w - 2 * Arena::border, h - 2 * Arena::border };
  }
}

void Pong::run ()
{
  state = State::ROUND_BEGIN;

  std::thread update_thread (&Pong::update, this);
  update_thread.detach ();

  std::thread render_thread (&Pong::render, this);
  render_thread.detach ();

  handle_events ();
}

[[noreturn]]
void Pong::render () const
{
  const static auto rate = 17ms;
  while (true) {
    std::this_thread::sleep_for (rate);

    SDL_SetRenderDrawColor (ren, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear (ren);

    SDL_SetRenderDrawColor (ren, 0xff, 0xff, 0xff, 0x00);

    const auto arena_rect = rect (arena);
    const auto ball_rect = rect (ball);
    const auto left_paddle_rect = rect (left_paddle);
    const auto right_paddle_rect = rect (right_paddle);

    SDL_RenderDrawRect (ren, &arena_rect);

    SDL_RenderFillRect (ren, &ball_rect);
    SDL_RenderFillRect (ren, &left_paddle_rect);
    SDL_RenderFillRect (ren, &right_paddle_rect);

    SDL_RenderPresent (ren);
  }
}

[[noreturn]]
void Pong::update ()
{
  const static auto rate = 17ms;
  while (true) {
    std::this_thread::sleep_for (rate);
    switch (state) {
    case State::ROUND_BEGIN:
      update_round_begin ();
      break;
    case State::BALL_IN_PLAY:
      update_ball_in_play ();
      break;
    }
  }
}

void Pong::handle_events ()
{
  SDL_Event e;
  while (true) {
    SDL_PollEvent (&e);
    switch (e.type) {
    case SDL_QUIT:
      return;
    }
  }
}

void Pong::update_round_begin ()
{
  { // Paddle init
    left_paddle = {
      bound (arena).min_x + Paddle::behind,
      arena.y };

    right_paddle = {
      bound (arena).max_x - Paddle::behind,
      arena.y };
  }

  { // Ball init
    ball = { arena.x, arena.y, 0, 0 };
  }

  std::this_thread::sleep_for (2s);

  state = State::BALL_IN_PLAY;
  ball.change_angle (init_ball_angle_dist (g));
}

void bounce (Ball& ball, const Paddle& paddle)
{
  const auto max_diff = Ball::height + Paddle::height;
  const auto min_diff = -max_diff;
  const auto diff     = paddle.y - ball.y;

  // Angle will vary linearly from -5/12 pi to 5/12pi as diff varies from
  // min_diff to max_diff
  const auto angle =
    (5.0f * M_PI / 12) - 
    (10.0f * M_PI / 12) * 
    (diff - min_diff) / (max_diff - min_diff);

  ball.change_angle (angle);

  if (ball.x < paddle.x) {
    ball.x_v *= -1.0;
  }
}

void Pong::update_ball_in_play ()
{
  // Update paddles
  { // Left paddle is user-controlled
    int y = 0;
    SDL_GetMouseState (nullptr, &y);

    if (y - Paddle::height < arena.y - arena.height) {
      left_paddle.y = bound (arena).min_y + Paddle::height;
    } else if (y + Paddle::height > arena.y + arena.height) {
      left_paddle.y = bound (arena).max_y - Paddle::height;
    } else {
      left_paddle.y = y;
    }
  }

  {
    int diff = ball.y - right_paddle.y;

    if (std::abs (diff) > right_paddle.ai_max_v) {
      diff = std::copysign (right_paddle.ai_max_v, diff);
    }

    if (bound (right_paddle).min_y + diff < bound (arena).min_y) {
      right_paddle.y = bound (arena).min_y + Paddle::height;
    } else if (bound (right_paddle).max_y + diff > bound (arena).max_y) {
      right_paddle.y = bound (arena).max_y - Paddle::height;
    } else {
      right_paddle.y += diff;
    }
  }

  // Update ball
  ball.x += ball.x_v;
  ball.y += ball.y_v;

  // Collision detection
  if (bound (ball).min_y < bound (arena).min_y) { // Collided with top of arena
    ball.y_v *= -1;
    ball.y += bound (ball).min_y - bound (ball).min_y;
  }

  if (bound (ball).max_y > bound (arena).max_y) { // Collided with bottom of arena
    ball.y_v *= -1;
    ball.y += bound (arena).max_y - bound (ball).max_y;
  }

  if (ball.x < 0) { // Gone off left side of screen
    state = State::ROUND_BEGIN;
  }

  if (ball.x > bound (arena).max_x + Arena::border) { // Gone off right side of screen
    state = State::ROUND_BEGIN;
  }

  { // Collision detection (paddles)
    SDL_Rect ball_rect = rect (ball);
    SDL_Rect left_paddle_rect = rect (left_paddle);
    SDL_Rect right_paddle_rect = rect (right_paddle);

    if (SDL_HasIntersection (&ball_rect, &left_paddle_rect)) {
      bounce (ball, left_paddle);
    } else if (SDL_HasIntersection (&ball_rect, &right_paddle_rect)) {
      bounce (ball, right_paddle);
    }
  }
}
