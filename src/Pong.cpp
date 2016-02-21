#include "Pong.h"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

template <typename T>
auto rect (const T& t) ->
SDL_Rect
{
  return { 
    static_cast<int> (t.min_x), 
    static_cast<int> (t.min_y), 
    static_cast<int> (t.max_x - t.min_x), 
    static_cast<int> (t.max_y - t.min_y) };
}

template <typename T>
auto center (const T& t) ->
SDL_Point
{
  return { 
    static_cast<int> (t.min_x + (t.max_x - t.min_x) / 2), 
    static_cast<int> (t.min_y + (t.max_y - t.min_y) / 2) };
}

Pong::Pong (SDL_Renderer* _ren) :
  ren (_ren)
{
  { // Arena init
    int w = 0, h = 0;
    SDL_GetRendererOutputSize (ren, &w, &h);
    arena = { 
      Arena::border, Arena::border,
      w - 2 * Arena::border, h - 2 * Arena::border };
  }

  { // Paddle init
    left_paddle = {
      arena.min_x + Paddle::behind, 
      static_cast<float> (center (arena).y),
      arena.min_x + Paddle::behind + Paddle::width, 
      center (arena).y + Paddle::height,
      Paddle::ai_max_v };

    right_paddle = {
      arena.max_x - Paddle::behind - Paddle::width, 
      static_cast<float> (center (arena).y),
      arena.max_x - Paddle::behind, 
      center (arena).y + Paddle::height,
      Paddle::ai_max_v };
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
  ball.min_x = center (arena).x - Ball::height / 2;
  ball.max_x = center (arena).x + Ball::height / 2;
  ball.min_y = center (arena).y - Ball::width  / 2;
  ball.max_y = center (arena).y + Ball::width  / 2;
  ball.x_v = 0;
  ball.y_v = 0;

  std::this_thread::sleep_for (2s);

  state = State::BALL_IN_PLAY;
  const auto angle = init_ball_angle_dist (g);
  ball.x_v = Ball::v * std::cos (angle);
  ball.y_v = Ball::v * std::sin (angle);
}

void bounce (Ball& ball, const Paddle& paddle)
{
  const auto max_diff = Ball::height / 2 + Paddle::height / 2;
  const auto min_diff = -max_diff;
  const auto diff     = center (paddle).y - center (ball).y;

  // Angle will vary linearly from -5/12 pi to 5/12pi as diff varies from
  // min_diff to max_diff
  const auto angle =
    -((-5.0f * M_PI / 12) + 
      (10.0f * M_PI / 12) * (diff - min_diff) / (max_diff - min_diff));

  ball.x_v = Ball::v * std::cos (angle);
  ball.y_v = Ball::v * std::sin (angle);

  if (ball.min_x < paddle.min_x) {
    ball.x_v *= -1.0;
  }
}

void Pong::update_ball_in_play ()
{
  // Update paddles
  {
    int diff = center (ball).y - center (left_paddle).y;

    if (std::abs (diff) > left_paddle.max_v) {
      diff = std::copysign (left_paddle.max_v, diff);
    }

    if (left_paddle.min_y + diff < arena.min_y) {
      left_paddle.min_y = arena.min_y;
      left_paddle.max_y = left_paddle.min_y + Paddle::height;
    } else if (left_paddle.max_y + diff > arena.max_y) {
      left_paddle.max_y = arena.max_y;
      left_paddle.min_y = left_paddle.max_y - Paddle::height;
    } else {
      left_paddle.min_y += diff;
      left_paddle.max_y += diff;
    }
  }

  {
    int diff = center (ball).y - center (right_paddle).y;

    if (std::abs (diff) > right_paddle.max_v) {
      diff = std::copysign (right_paddle.max_v, diff);
    }

    if (right_paddle.min_y + diff < arena.min_y) {
      right_paddle.min_y = arena.min_y;
      right_paddle.max_y = right_paddle.min_y + Paddle::height;
    } else if (right_paddle.max_y + diff > arena.max_y) {
      right_paddle.max_y = arena.max_y;
      right_paddle.min_y = right_paddle.max_y - Paddle::height;
    } else {
      right_paddle.min_y += diff;
      right_paddle.max_y += diff;
    }
  }

  // Update ball
  ball.min_x += ball.x_v;
  ball.min_y += ball.y_v;
  ball.max_x += ball.x_v;
  ball.max_y += ball.y_v;

  // Collision detection
  if (ball.min_y < arena.min_y) { // Collided with top of arena
    ball.y_v *= -1;
    ball.min_y += arena.min_y - ball.min_y;
    ball.max_y = ball.min_y + Ball::height;
  }

  if (ball.max_y > arena.max_y) { // Collided with bottom of arena
    ball.y_v *= -1;
    ball.max_y += arena.max_y - ball.max_y;
    ball.min_y = ball.max_y - Ball::height;
  }

  if (ball.max_x < 0) { // Gone off left side of screen
    state = State::ROUND_BEGIN;
  }

  if (ball.min_x > arena.max_x + Arena::border) { // Gone off right side of screen
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
