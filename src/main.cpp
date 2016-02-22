#include "SDL.h"
#include "Pong.h"

#include <cstdlib>

int main ()
{
  const int window_width = 640, window_height = 480;

  if (SDL_Init (SDL_INIT_VIDEO) != 0) {
    SDL_LogError (
        SDL_LOG_CATEGORY_APPLICATION,
        "Couldn't initialize SDL: %s", SDL_GetError ());
    return EXIT_FAILURE;
  }

  std::atexit (SDL_Quit);

  SDL_Window* win;
  SDL_Renderer* ren;

  if (SDL_CreateWindowAndRenderer (
          window_width, window_height, 0, &win, &ren) != 0) {
    SDL_LogError (
        SDL_LOG_CATEGORY_APPLICATION,
        "Couldn't create window and renderer: %s", SDL_GetError ());
    return EXIT_FAILURE;
  }

  Pong pong (ren);
  pong.run ();

  SDL_DestroyRenderer (ren);
  SDL_DestroyWindow (win);

  return 0;
}
