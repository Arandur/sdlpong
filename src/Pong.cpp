#include "Pong.h"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

auto bound (const Rect& r)
{
  struct Bound { float min_x, max_x, min_y, max_y; };

  return Bound { r.x - r.w / 2, r.x + r.w / 2, r.y - r.h / 2, r.y + r.h / 2 };
}

auto center (const Rect& r)
{
  struct Center { float x, y; };

  return Center { r.x, r.y };
}

auto sdl_rect (const Rect& r)
{
  return SDL_Rect { r.x - r.w / 2, r.y - r.h / 2, r.w, r.h };
}

void Ball::change_angle (float a)
{
  x_v = v * std::cos (a);
  y_v = v * std::sin (a);
}

Pong::Pong (SDL_Renderer* _ren) :
  ren (_ren)
  ren_rect (
      [] (auto&& _ren)
      {
        int w, h;
        SDL_GetRendererOutputSize (ren, &w, &h);
        return Rect (w / 2, h / 2, w, h);
      }),
  left_paddle (25.0f, ren_rect.y, 10.0f, 50.0f),
  right_paddle (ren_rect.w - 25.0f, ren_rect.y, 10.0f, 50.0f),
  top_wall (ren_rect.x, 0.0f, ren_rect.w, 10.0f),
  bottom_wall (ren_rect.x, ren_rect.h, ren_rect.w, 10.0f),
  left_zone (-5.0f, ren_rect.y, 10.0f, ren_rect.h),
  right_zone (-5.0f, ren_rect.y, 10.0f, ren_rect.h)
{
  SDL_ShowCursor (SDL_DISABLE);
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

    const auto ball_rect = sdl_rect (ball);
    const auto left_paddle_rect = sdl_rect (left_paddle);
    const auto right_paddle_rect = sdl_rect (right_paddle);

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
  left_paddle =
    Paddle (25.0f, ren_rect.h / 2, 10.0f, 50.0f);
  right_paddle = 
    Paddle (ren_rect.w - 25.0f, ren_rect.h / 2, 10.0f, 50.0f);
  ball =
    Ball (ren_
