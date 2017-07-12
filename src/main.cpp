#include <iostream>
#include <SDL.h>

static const unsigned SCREEN_WIDTH = 1920;
static const unsigned SCREEN_HEIGHT = 1080;

class Game {
  public:
    SDL_Window* sdlWindow;
    SDL_Renderer* sdlRenderer;
    SDL_Event sdlEvent;

    bool isRunning;

    Game(int argc, char* argv[]);
    ~Game();

    bool preload();
    bool create();
    void unload();
    int run();
    void processEvents();
    void update();
    void render();
};

int main(int argc, char* argv[]) {

  Game game(argc, argv);
  return game.run();
}

Game::Game(int argc, char* argv[]) {

}

Game::~Game() {

}

bool Game::preload() {
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    std::cerr << "SDL_Init Failed: " << SDL_GetError() << std::endl;
    return false;
  }

  atexit(SDL_Quit);

  if (SDL_CreateWindowAndRenderer(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    SDL_WINDOW_FULLSCREEN_DESKTOP,
    &sdlWindow,
    &sdlRenderer
  ) != 0) {
    std::cerr << "SDL_CreateWindowAndRenderer Failed: " << SDL_GetError() << std::endl;
    return false;
  }

  SDL_SetWindowTitle(sdlWindow, "YARPGP");

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_RenderSetLogicalSize(sdlRenderer, SCREEN_WIDTH, SCREEN_HEIGHT);

  return true;
}

bool Game::create() {
  return true;
}

void Game::unload() {

}

int Game::run() {
  if (!preload()) {
    return -1;
  }

  if (!create()) {
    return -1;
  }

  isRunning = true;

  while (isRunning) {
    processEvents();
    update();
    render();
    SDL_Delay(20);
  }

  unload();

  return 0;
}

void Game::processEvents() {
  while (SDL_PollEvent(&sdlEvent)) {
    switch (sdlEvent.type) {
      case SDL_QUIT: {
        isRunning = false;
      } break;

      case SDL_KEYDOWN: {
        if (sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
          isRunning = false;
        }
      } break;
    }
  }
}

void Game::update() {

}

void Game::render() {
  SDL_SetRenderDrawColor(sdlRenderer, 30, 60, 90, 255);
  SDL_RenderClear(sdlRenderer);

  SDL_RenderPresent(sdlRenderer);
}
