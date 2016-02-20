#include "SDL.h"

#include <unistd.h> // usleep
#include <time.h>   // time
#include <stdlib.h> // rand

struct BallState {
  SDL_Rect pos;
  SDL_Texture* texture;

  float x_velocity; 
  float y_velocity;
};

void init_ball   (SDL_Renderer *, struct BallState *, int, int);
void update_ball (struct BallState *);
void render_ball (SDL_Renderer *, struct BallState *);

const float ball_velocity = 5.0f;
const int   window_width  = 640;
const int   window_height = 480;

int main ()
{
  struct BallState ball;
  const unsigned int refresh_rate = 16667; // 60 Hz
  const int ball_width = 10, ball_height = 10;

  srand (time (NULL));

  if (SDL_Init (SDL_INIT_VIDEO) != 0) {
    SDL_LogError (
        SDL_LOG_CATEGORY_APPLICATION, 
        "Couldn't initialize SDL: %s", SDL_GetError ());
    return EXIT_FAILURE;
  }

  atexit (SDL_Quit);

  SDL_Window *win;
  SDL_Renderer *ren;
  
  if (SDL_CreateWindowAndRenderer (
          window_width, window_height, 0, &win, &ren) != 0) {
    SDL_LogError (
        SDL_LOG_CATEGORY_APPLICATION, 
        "Couldn't create window and renderer: %s", SDL_GetError ());
    return EXIT_FAILURE;
  }

  init_ball (ren, &ball, ball_width, ball_height);

  // main loop
  while (1) {
    SDL_Event event;

    while (SDL_PollEvent (&event)) {
      if (event.type == SDL_QUIT) {
        exit (EXIT_SUCCESS);
      }
    }

    update_ball (&ball);
    render_ball (ren, &ball);
    usleep (refresh_rate);
  }

  return EXIT_FAILURE;
}

void init_ball (SDL_Renderer *ren, struct BallState *ball, int w, int h)
{
  const float angle = 2.0f * M_PI * rand () / RAND_MAX;
  SDL_Surface *surface;

  ball->pos.x = window_width  / 2;
  ball->pos.y = window_height / 2;
  ball->pos.w = w;
  ball->pos.h = h;
  ball->x_velocity = ball_velocity * cos (angle);
  ball->y_velocity = ball_velocity * sin (angle);

  surface = SDL_CreateRGBSurface (0, w, h, 32, 0, 0, 0, 0);
  SDL_FillRect (surface, NULL, SDL_MapRGB (surface->format, 255, 255, 255));
  ball->texture = SDL_CreateTextureFromSurface (ren, surface);
  SDL_FreeSurface (surface);
}

void update_ball (struct BallState *ball)
{
  ball->pos.x += ball->x_velocity;
  ball->pos.y += ball->y_velocity;

  if (ball->pos.x < 0) {
    ball->pos.x *= -1.0f;
    ball->x_velocity *= -1.0f;
  }

  if (ball->pos.y < 0) {
    ball->pos.y *= -1.0f;
    ball->y_velocity *= -1.0f;
  }

  if (ball->pos.x + ball->pos.w > window_width) {
    ball->pos.x -= 2 * (ball->pos.x + ball->pos.w - window_width);
    ball->x_velocity *= -1.0f;
  }

  if (ball->pos.y + ball->pos.h > window_height) {
    ball->pos.y -= 2 * (ball->pos.y + ball->pos.h - window_height);
    ball->y_velocity *= -1.0f;
  }
}

void render_ball (SDL_Renderer *ren, struct BallState *ball)
{
  SDL_RenderClear (ren);
  SDL_RenderCopy (ren, ball->texture, NULL, &ball->pos);
  SDL_RenderPresent (ren);
}
