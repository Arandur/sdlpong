#include "Pong.h"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

Pong::Pong (SDL_Renderer* _ren) :
  ren (_ren)
{
  { // Arena init
    int w = 0, h = 0;
    SDL_GetRendererOutputSize (ren, &w, &h);
    arena.x = Arena::border;
    arena.y = Arena::border;
    arena.w = w - 2 * Arena::border;
    arena.h = h - 2 * Arena::border;
  }

  { // Paddle init
    left_paddle.x = arena.x + Paddle::behind;
    left_paddle.y = arena.y + arena.h / 2 - Paddle::height / 2;
    left_paddle.w = Paddle::width;
    left_paddle.h = Paddle::height;
    left_paddle.max_v = Paddle::ai_max_v;

    right_paddle.x = arena.x + arena.w - Paddle::behind;
    right_paddle.y = arena.y + arena.h / 2 - Paddle::height / 2;
    right_paddle.w = Paddle::width;
    right_paddle.h = Paddle::height;
    right_paddle.max_v = Paddle::ai_max_v;
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

    { // draw arena
      SDL_Rect rect;
      rect.x = arena.x;
      rect.y = arena.y;
      rect.w = arena.w;
      rect.h = arena.h;

      SDL_RenderDrawRect (ren, &rect);
    }

    {
      SDL_Rect rect;
      rect.x = ball.x;
      rect.y = ball.y;
      rect.w = ball.w;
      rect.h = ball.h;

      SDL_RenderFillRect (ren, &rect);
    }

    {
      SDL_Rect rect;
      rect.x = left_paddle.x;
      rect.y = left_paddle.y;
      rect.w = left_paddle.w;
      rect.h = left_paddle.h;

      SDL_RenderFillRect (ren, &rect);
    }

    {
      SDL_Rect rect;
      rect.x = right_paddle.x;
      rect.y = right_paddle.y;
      rect.w = right_paddle.w;
      rect.h = right_paddle.h;

      SDL_RenderFillRect (ren, &rect);
    }

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
  ball.x = arena.x + arena.w / 2;
  ball.y = arena.y + arena.h / 2;
  ball.w = Ball::width;
  ball.h = Ball::height;
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
  const auto max_diff = ball.h / 2 + paddle.h / 2;
  const auto min_diff = -max_diff;
  const auto diff     = paddle.y + paddle.h / 2 - (ball.y + ball.h / 2);

  // Angle will vary linearly from -5/12 pi to 5/12pi as diff varies from
  // min_diff to max_diff
  const auto angle =
    -((-5.0f * M_PI / 12) + 
      (10.0f * M_PI / 12) * (diff - min_diff) / (max_diff - min_diff));

  ball.x_v = Ball::v * std::cos (angle);
  ball.y_v = Ball::v * std::sin (angle);

  if (ball.x < paddle.x) {
    ball.x_v *= -1.0;
  }
}

void Pong::update_ball_in_play ()
{
  // Update paddles
  {
    int diff = ball.y + ball.h / 2 - (left_paddle.y + left_paddle.h / 2);

    if (std::abs (diff) > left_paddle.max_v) {
      diff = std::copysign (left_paddle.max_v, diff);
    }

    if (left_paddle.y + diff < arena.y) {
      left_paddle.y = arena.y;
    } else if (left_paddle.y + diff + left_paddle.h > arena.y + arena.h) {
      left_paddle.y = arena.y + arena.h - left_paddle.h;
    } else {
      left_paddle.y += diff;
    }
  }

  {
    int diff = ball.y + ball.h / 2 - (right_paddle.y + right_paddle.h / 2);

    if (std::abs (diff) > right_paddle.max_v) {
      diff = std::copysign (right_paddle.max_v, diff);
    }

    if (right_paddle.y + diff < arena.y) {
      right_paddle.y = arena.y;
    } else if (right_paddle.y + diff + right_paddle.h > arena.y + arena.h) {
      right_paddle.y = arena.y + arena.h - right_paddle.h;
    } else {
      right_paddle.y += diff;
    }
  }

  // Update ball
  ball.x += ball.x_v;
  ball.y += ball.y_v;

  // Collision detection
  if (ball.y < arena.y) { // Collided with top of arena
    ball.y_v *= -1;
    ball.y += arena.y - ball.y;
  }

  if (ball.y + ball.h > arena.y + arena.h) { // Collided with bottom of arena
    ball.y_v *= -1;
    ball.y += arena.y + arena.h - ball.y - ball.h;
  }

  if (ball.x + ball.w < 0) { // Gone off left side of screen
    state = State::ROUND_BEGIN;
  }

  if (ball.x > 2 * arena.x + arena.w) { // Gone off right side of screen
    state = State::ROUND_BEGIN;
  }

  { // Collision detection (paddles)
    SDL_Rect ball_rect;
    ball_rect.x = ball.x;
    ball_rect.y = ball.y;
    ball_rect.w = ball.w;
    ball_rect.h = ball.h;

    SDL_Rect left_paddle_rect;
    left_paddle_rect.x = left_paddle.x;
    left_paddle_rect.y = left_paddle.y;
    left_paddle_rect.w = left_paddle.w;
    left_paddle_rect.h = left_paddle.h;

    SDL_Rect right_paddle_rect;
    right_paddle_rect.x = right_paddle.x;
    right_paddle_rect.y = right_paddle.y;
    right_paddle_rect.w = right_paddle.w;
    right_paddle_rect.h = right_paddle.h;

    if (SDL_HasIntersection (&ball_rect, &left_paddle_rect)) {
      bounce (ball, left_paddle);
    } else if (SDL_HasIntersection (&ball_rect, &right_paddle_rect)) {
      bounce (ball, right_paddle);
    }
  }
}
