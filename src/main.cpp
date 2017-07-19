#include <iostream>
#include <vector>
#include <map>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#ifdef IS_DEBUG
static const unsigned SCREEN_WIDTH = 256; // 1920;
static const unsigned SCREEN_HEIGHT = 244; // 1080;
static const unsigned WINDOW_WIDTH = 256 * 3; // 1920 / 2;
static const unsigned WINDOW_HEIGHT = 244 * 2; // 1080 / 2;
#else
static const unsigned SCREEN_WIDTH = 1920;
static const unsigned SCREEN_HEIGHT = 1080;
static const unsigned WINDOW_WIDTH = 1920;
static const unsigned WINDOW_HEIGHT = 1080;
#endif


static const unsigned mm[] = { 0, 0, 1, 3, 1, 2, 0, 0, 3, 1, 5, 5, 5, 4, 0, 0, 2, 1, 4, 5, 4, 4 };
static const float mt[] = { 0, 0.0f, 1.0f, -1.0f, 0.7071067811865475f, -0.7071067811865475f };


static const unsigned north = 0;
static const unsigned northEast = 1;
static const unsigned east = 2;
static const unsigned southEast = 3;
static const unsigned south = 4;
static const unsigned southWest = 5;
static const unsigned west = 6;
static const unsigned northWest = 7;


class Content {
  private:
    SDL_Renderer* sharedRenderer;

  public:
    std::vector<SDL_Texture*> textureCollection;
    std::map<const char*, unsigned> textureTable;

    std::vector<TTF_Font*> fontCollection;
    std::map<const char*, unsigned> fontTable;

    Content(SDL_Renderer* sdlRenderer);
    ~Content();
    bool loadTexture(const char* path, const char* alias);
    void unloadTexture(const char* alias);
    SDL_Texture* getTexture(const char* alias);

    bool loadFont(const char* path, const char* alias, unsigned fontSize);
    void unloadFont(const char* alias);
    TTF_Font* getFont(const char* alias);

};

Content::Content(SDL_Renderer* sdlRenderer) : sharedRenderer(sdlRenderer) {

}

Content::~Content() {
  textureTable.clear();
  fontTable.clear();

  for (auto&& texture : textureCollection) {
    if (texture) {
      SDL_DestroyTexture(texture);
    }
  }

  for (auto&& font : fontCollection) {
    if (font) {
      TTF_CloseFont(font);
    }
  }

  textureCollection.clear();
  fontCollection.clear();
}

bool Content::loadTexture(const char* path, const char* alias) {
  SDL_Surface* surface = IMG_Load(path);

  if (!surface) {
    std::cerr << "Failed to load texture from \"" << path << "\" : " << IMG_GetError() << std::endl;
    return false;
  }

  SDL_Texture* texture = SDL_CreateTextureFromSurface(sharedRenderer, surface);

  if (!texture) {
    std::cerr << "Failed to create texture from surface: " << SDL_GetError() << std::endl;
    return false;
  }

  SDL_FreeSurface(surface);

  unsigned assetIndex = textureCollection.size();

  textureCollection.push_back(texture);
  textureTable[alias] = assetIndex;

  return true;
}

void Content::unloadTexture(const char* alias) {
  unsigned assetIndex = textureTable[alias];
  SDL_Texture* texture = textureCollection[assetIndex];

  if (texture) {
    SDL_DestroyTexture(texture);
  }

  textureCollection.erase(textureCollection.begin() + assetIndex);
  textureTable.erase(alias);
}

SDL_Texture* Content::getTexture(const char* alias) {
  unsigned assetIndex = textureTable[alias];

  return textureCollection[assetIndex];
}

bool Content::loadFont(const char* path, const char* alias, unsigned fontSize) {
  TTF_Font* font = TTF_OpenFont(path, fontSize);

  if (!font) {
    std::cerr << "TTF_OpenFont Failed: " << TTF_GetError() << std::endl;
    return false;
  }

  unsigned assetIndex = fontCollection.size();

  fontCollection.push_back(font);
  fontTable[alias] = assetIndex;

  return true;
}

void Content::unloadFont(const char* alias) {
  unsigned assetIndex = fontTable[alias];
  TTF_Font* font = fontCollection[assetIndex];

  if (font) {
    TTF_CloseFont(font);
  }

  fontCollection.erase(fontCollection.begin() + assetIndex);
  fontTable.erase(alias);
}

TTF_Font* Content::getFont(const char* alias) {
  unsigned assetIndex = fontTable[alias];

  return fontCollection[assetIndex];
}

class Tileset {
  private:
    SDL_Texture* sharedAtlas;

  public:
    unsigned width;
    unsigned height;
    std::vector<SDL_Rect> tiles;

    Tileset(SDL_Texture* atlas, unsigned tileWidth, unsigned tileHeight);
    ~Tileset();
    void setAtlas(SDL_Texture* atlas);
    void slice();
    void load(unsigned count, unsigned* data);
    unsigned addTile(unsigned x, unsigned y);
    void drawTile(SDL_Renderer* renderer, unsigned tileId, SDL_Rect* destination);
};

Tileset::Tileset(SDL_Texture* atlas, unsigned tileWidth, unsigned tileHeight)
  : sharedAtlas(atlas), width(tileWidth), height(tileHeight) {

}

Tileset::~Tileset() {
  tiles.clear();
}

void Tileset::setAtlas(SDL_Texture* atlas) {
  sharedAtlas = atlas;
}

void Tileset::slice() {
  tiles.clear();

  int w, h;
  SDL_QueryTexture(sharedAtlas, NULL, NULL, &w, &h);

  unsigned columns = w / width;
  unsigned rows = h / height;
  unsigned y;

  for (unsigned row = 0; row < rows; row += 1) {
    y = row * height;

    for (unsigned column = 0; column < columns; column += 1) {
      SDL_Rect tile;

      tile.x = column * width;
      tile.y = y;
      tile.w = width;
      tile.h = height;

      tiles.push_back(tile);
    }
  }
}

void Tileset::load(unsigned count, unsigned* data) {
  unsigned length = count * 2;
  for (unsigned i = 0; i < length; i += 2) {
    addTile(data[i], data[i + 1]);
  }
}

unsigned Tileset::addTile(unsigned x, unsigned y) {
  unsigned tileId = tiles.size();

  SDL_Rect tile;
  tile.x = x;
  tile.y = y;
  tile.w = width;
  tile.h = height;

  tiles.push_back(tile);

  return tileId;
}

void Tileset::drawTile(SDL_Renderer* renderer, unsigned tileId, SDL_Rect* destination) {
  if (!renderer || tileId > tiles.size()) {
    return;
  }

  SDL_Rect* source = &tiles[tileId];

  SDL_RenderCopy(renderer, sharedAtlas, source, destination);
}

struct Animation {
  SDL_Rect region;
  unsigned* frames;
  unsigned count;
};

class Sprite {
  private:
    SDL_Texture* sharedAtlas;
    SDL_Rect renderRect;
    SDL_Rect frameSource;

  public:
    float x;
    float y;
    float xv;
    float yv;

    std::vector<Animation*> animationCollection;
    std::map<const char*, unsigned> animationTable;
    Animation* animation;
    const char* animationName;

    unsigned width;
    unsigned height;
    unsigned frame;

    float frameTime;
    float duration;
    float timePerFrame;

    bool playing;
    bool looping;

    bool active;
    bool visible;

    Sprite(SDL_Texture* texture);
    Sprite(SDL_Texture* atlas, unsigned frameWidth, unsigned frameHeight);
    ~Sprite();

    void addAnimation(const char* name, unsigned frameCount, unsigned* frames, SDL_Rect* region);
    void selectAnimation(const char* name);
    void setDuration(float duration);
    void gotoAndPlay(unsigned frame);
    void gotoAndStop(unsigned frame);
    void play();
    void stop();
    void update(float deltaTime);
    void draw(SDL_Renderer* renderer);

    inline bool isAnimation(const char* name) {
      return strncmp(animationName, name, strlen(name)) == 0;
    }
};

Sprite::Sprite(SDL_Texture* texture) {
  int w, h;
  SDL_QueryTexture(texture, NULL, NULL, &w, &h);
  sharedAtlas = texture;
  width = w;
  height = h;
  playing = false;
  looping = true;
  duration = 1.0;

  renderRect.x = 0;
  renderRect.y = 0;
  renderRect.w = w;
  renderRect.h = h;

  active = true;
  visible = true;
  std::cout << "creating sprite from single frame" << std::endl;
}

Sprite::Sprite(SDL_Texture* atlas, unsigned frameWidth, unsigned frameHeight) {
  sharedAtlas = atlas;
  width = frameWidth;
  height = frameHeight;
  playing = false;
  looping = true;
  duration = 1.0;

  renderRect.x = 0;
  renderRect.y = 0;
  renderRect.w = frameWidth;
  renderRect.h = frameHeight;

  frameSource.w = width;
  frameSource.h = height;

  active = true;
  visible = true;

  std::cout << "creating sprite from multiple frames" << std::endl;
}

Sprite::~Sprite() {
  animationTable.clear();

  for (auto&& anim : animationCollection) {
    if (anim) {
      delete [] anim->frames;
      delete anim;
    }
  }

  animationCollection.clear();
}

void Sprite::addAnimation(const char* name, unsigned frameCount, unsigned* frames, SDL_Rect* region) {
  Animation* anim = new Animation;

  anim->region.x = region->x;
  anim->region.y = region->y;
  anim->region.w = region->w;
  anim->region.h = region->h;
  anim->count = frameCount;
  anim->frames = new unsigned[frameCount];

  for (unsigned i = 0; i < frameCount; i += 1) {
    anim->frames[i] = frames[i];
  }

  unsigned animationId = animationCollection.size();

  animationCollection.push_back(anim);

  animationTable[name] = animationId;

  // std::cout << "adding animation " << name << " with " << frameCount << " frames" << std::endl;
}

void Sprite::selectAnimation(const char* name) {
  if (animationTable.find(name) != animationTable.end()) {
    unsigned animationId = animationTable[name];
    animation = animationCollection[animationId];
    frame = 0;
    frameTime = 0;
    // std::cout << "selecting animation " << name << std::endl;
    animationName = name;
  }
}

void Sprite::setDuration(float duration) {
  this->duration = duration;

  if (animation) {
    timePerFrame = (float)(duration / (float)animation->count);

    // std::cout << "setting sprite animation duration " << duration << " timePerFrame " << timePerFrame << std::endl;
  }
}

void Sprite::gotoAndPlay(unsigned frame) {
  play();
  this->frame = frame;
}

void Sprite::gotoAndStop(unsigned frame) {
  stop();
  this->frame = frame;
}

void Sprite::play() {
  playing = true;
  frameTime = 0;
}

void Sprite::stop() {
  playing = false;
  frameTime = 0;
}

void Sprite::update(float deltaTime) {
  if (animation && playing) {
    frameTime += deltaTime;

    if (frameTime >= timePerFrame) {
      frameTime -= timePerFrame;

      frame += 1;
      if (frame >= animation->count) {
        if (looping) {
          frame = 0;
        } else {
          stop();
        }
      }
    }
  }

  if (active) {
    x += xv * deltaTime;
    y += yv * deltaTime;
  }
}

void Sprite::draw(SDL_Renderer* renderer) {
  if (visible) {
    renderRect.x = (int)x;
    renderRect.y = (int)y;

    SDL_Rect* source = NULL;

    if (animation) {
      // first set the source to the region coordinates
      frameSource.x = animation->region.x;
      frameSource.y = animation->region.y;

      // now find the coordinates of the specific animation frame
      unsigned framesAcross = animation->region.w / frameSource.w;
      unsigned frameId = animation->frames[frame];
      unsigned frameX = frameId % framesAcross;
      unsigned frameY = frameId / framesAcross;

      // add the frame coordinates to the region coordinates to get atlas source coordinates
      frameSource.x += frameX * frameSource.w;
      frameSource.y += frameY * frameSource.h;

      // std::cout << "rendering frame " << frame << " of " << animation->count
      //   << " [" << frameSource.x << ", " << frameSource.y << "] (" << frameSource.w << "x" << frameSource.h << ")"
      //   << " frameID " << frameId << " frameX " << frameX << " frameY " << frameY << std::endl;

      source = &frameSource;
    }

    SDL_RenderCopy(renderer, sharedAtlas, source, &renderRect);
  }
}


class NinePatch {
  private:
    SDL_Texture* sharedTexture;
    SDL_Rect sourceRect;
    SDL_Rect renderRect;

  public:
    unsigned sliceWidth;
    unsigned sliceHeight;

    NinePatch(SDL_Texture* texture);
    NinePatch(SDL_Texture* texture, unsigned sliceWidth, unsigned sliceHeight);
    ~NinePatch();

    void setAlpha(Uint8 alpha);
    void draw(SDL_Renderer* renderer, SDL_Rect* destination);
};

NinePatch::NinePatch(SDL_Texture* texture) {
  int w, h;
  SDL_QueryTexture(texture, NULL, NULL, &w, &h);
  sliceWidth = w / 3;
  sliceHeight = h / 3;
  sharedTexture = texture;
  renderRect.w = sliceWidth;
  renderRect.h = sliceHeight;
  sourceRect.w = sliceWidth;
  sourceRect.h = sliceHeight;
}

NinePatch::NinePatch(SDL_Texture* texture, unsigned sliceWidth, unsigned sliceHeight) {
  sharedTexture = texture;
  this->sliceWidth = sliceWidth;
  this->sliceHeight = sliceHeight;
  renderRect.w = sliceWidth;
  renderRect.h = sliceHeight;
  sourceRect.w = sliceWidth;
  sourceRect.h = sliceHeight;
}

NinePatch::~NinePatch() {

}

void NinePatch::setAlpha(Uint8 alpha) {
  if (alpha != 0xFF) {
    SDL_SetTextureBlendMode(sharedTexture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(sharedTexture, alpha);
  } else {
    SDL_SetTextureBlendMode(sharedTexture, SDL_BLENDMODE_NONE);
    SDL_SetTextureAlphaMod(sharedTexture, 0xFF);
  }
}

void NinePatch::draw(SDL_Renderer* renderer, SDL_Rect* destination) {
  // draw the center fill
  renderRect.x = destination->x + sliceWidth;
  renderRect.y = destination->y + sliceHeight;
  renderRect.w = (destination->w - (2 * sliceWidth));
  renderRect.h = (destination->h - (2 * sliceHeight));
  sourceRect.x = sliceWidth;
  sourceRect.y = sliceHeight;
  SDL_RenderCopy(renderer, sharedTexture, &sourceRect, &renderRect);

  // draw the top
  renderRect.x = destination->x + sliceWidth;
  renderRect.y = destination->y;
  renderRect.w = (destination->w - (2 * sliceWidth));
  renderRect.h = sliceHeight;
  sourceRect.x = sliceWidth;
  sourceRect.y = 0;
  SDL_RenderCopy(renderer, sharedTexture, &sourceRect, &renderRect);

  // draw the bottom
  renderRect.x = destination->x + sliceWidth;
  renderRect.y = destination->y + (destination->h - sliceHeight);
  renderRect.w = (destination->w - (2 * sliceWidth));
  renderRect.h = sliceHeight;
  sourceRect.x = sliceWidth;
  sourceRect.y = sliceHeight * 2;
  SDL_RenderCopy(renderer, sharedTexture, &sourceRect, &renderRect);

  // draw the left
  renderRect.x = destination->x;
  renderRect.y = destination->y + sliceHeight;
  renderRect.w = sliceWidth;
  renderRect.h = (destination->h - (2 * sliceHeight));
  sourceRect.x = 0;
  sourceRect.y = sliceHeight;
  SDL_RenderCopy(renderer, sharedTexture, &sourceRect, &renderRect);

  // draw the right
  renderRect.x = destination->x + (destination->w - sliceWidth);
  renderRect.y = destination->y + sliceHeight;
  renderRect.w = sliceWidth;
  renderRect.h = (destination->h - (2 * sliceHeight));
  sourceRect.x = sliceWidth * 2;
  sourceRect.y = sliceHeight;
  SDL_RenderCopy(renderer, sharedTexture, &sourceRect, &renderRect);

  renderRect.w = sliceWidth;
  renderRect.h = sliceHeight;

  // draw the top left
  sourceRect.x = 0;
  sourceRect.y = 0;
  renderRect.x = destination->x;
  renderRect.y = destination->y;
  SDL_RenderCopy(renderer, sharedTexture, &sourceRect, &renderRect);

  // draw the top right
  sourceRect.x = sliceWidth * 2;
  sourceRect.y = 0;
  renderRect.x = destination->x + (destination->w - sliceWidth);
  renderRect.y = destination->y;
  SDL_RenderCopy(renderer, sharedTexture, &sourceRect, &renderRect);

  // draw the bottom left
  sourceRect.x = 0;
  sourceRect.y = sliceHeight * 2;
  renderRect.x = destination->x;
  renderRect.y = destination->y + (destination->h - sliceHeight);
  SDL_RenderCopy(renderer, sharedTexture, &sourceRect, &renderRect);

  // draw the bottom right
  sourceRect.x = sliceWidth * 2;
  sourceRect.y = sliceHeight * 2;
  renderRect.x = destination->x + (destination->w - sliceWidth);
  renderRect.y = destination->y + (destination->h - sliceHeight);
  SDL_RenderCopy(renderer, sharedTexture, &sourceRect, &renderRect);
}

class Tilemap {
  private:
    unsigned* base;
    unsigned* object;
    unsigned* event;
    char* collision;

  public:
    unsigned width;
    unsigned height;

    Tilemap(unsigned width, unsigned height);
    ~Tilemap();

    inline unsigned xyToIndex(unsigned x, unsigned y) {
      return x + (y * width);
    }

    inline void indexToXY(unsigned index, unsigned* x, unsigned* y) {
      *x = index % width;
      *y = index / width;
    }

    unsigned getBaseValue(unsigned x, unsigned y);
    unsigned getObjectValue(unsigned x, unsigned y);
    unsigned getEventValue(unsigned x, unsigned y);
    char getCollisionValue(unsigned x, unsigned y);

    void setBaseValue(unsigned x, unsigned y, unsigned value);
    void setObjectValue(unsigned x, unsigned y, unsigned value);
    void setEventValue(unsigned x, unsigned y, unsigned value);
    void setCollisionValue(unsigned x, unsigned y, char value);

    inline unsigned* getBase() { return base; }
    inline unsigned* getObject() { return object; }
    inline unsigned* getEvent() { return event; }
    inline char* getCollision() { return collision; }

    void setBase(unsigned* values);
    void setCollision(unsigned* values);
};

Tilemap::Tilemap(unsigned width, unsigned height) {
  this->width = width;
  this->height = height;

  unsigned size = width * height;
  base = new unsigned[size];
  object = new unsigned[size];
  event = new unsigned[size];
  collision = new char[size];
}

Tilemap::~Tilemap() {
  delete [] base;
  delete [] object;
  delete [] event;
  delete [] collision;
}

unsigned Tilemap::getBaseValue(unsigned x, unsigned y) {
  return base[xyToIndex(x, y)];
}

unsigned Tilemap::getObjectValue(unsigned x, unsigned y) {
  return object[xyToIndex(x, y)];
}

unsigned Tilemap::getEventValue(unsigned x, unsigned y) {
  return event[xyToIndex(x, y)];
}

char Tilemap::getCollisionValue(unsigned x, unsigned y) {
  return collision[xyToIndex(x, y)];
}

void Tilemap::setBaseValue(unsigned x, unsigned y, unsigned value) {
  base[xyToIndex(x, y)] = value;
}

void Tilemap::setObjectValue(unsigned x, unsigned y, unsigned value) {
  object[xyToIndex(x, y)] = value;
}

void Tilemap::setEventValue(unsigned x, unsigned y, unsigned value) {
  event[xyToIndex(x, y)] = value;
}

void Tilemap::setCollisionValue(unsigned x, unsigned y, char value) {
  collision[xyToIndex(x, y)] = value;
}

void Tilemap::setBase(unsigned* values) {
  unsigned size = width * height;

  for (unsigned i = 0; i < size; i += 1) {
    base[i] = values[i];
  }
}

void Tilemap::setCollision(unsigned* values) {
  unsigned size = width * height;

  for (unsigned i = 0; i < size; i += 1) {
    collision[i] = values[i];
  }
}

class Game {
  public:
    SDL_Window* sdlWindow;
    SDL_Renderer* sdlRenderer;
    SDL_Event sdlEvent;

    bool isRunning;

    Content* content;
    Tileset* tileset;
    Tilemap* tilemap;

    Sprite* player;
    Sprite* npc;
    unsigned npcPath[2048];
    unsigned npcIP;
    float npcTimeToWait;
    float npcTimeWaited;
    unsigned npcState;
    unsigned npcDistanceToMove;
    unsigned npcDistanceMoved;
    unsigned npcDirection;

    NinePatch* dialoguePanel;

    Game(int argc, char* argv[]);
    ~Game();

    bool preload();
    bool create();
    bool createUI();
    bool createPlayer();
    bool createNPC();
    void unload();
    int run();
    void processEvents();
    void update();
    void updateNPC();
    void moveNPC(unsigned direction, bool stopped);
    void render();

    SDL_Texture* loadTexture(const char* path);
};

int main(int argc, char* argv[]) {

  Game game(argc, argv);
  return game.run();
}

Game::Game(int argc, char* argv[]) {

}

Game::~Game() {
  delete content;
  delete tileset;
  delete player;
  delete dialoguePanel;
  delete npc;
  delete tilemap;
}

bool Game::preload() {
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    std::cerr << "SDL_Init Failed: " << SDL_GetError() << std::endl;
    return false;
  }

  atexit(SDL_Quit);

  if (IMG_INIT_PNG != (IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    std::cerr << "IMG_Init Failed: " << IMG_GetError() << std::endl;
    return false;
  }

  atexit(IMG_Quit);

  if (TTF_Init() == -1) {
    std::cerr << "TTF_Init Failed: " << TTF_GetError() << std::endl;
    return false;
  }

  atexit(TTF_Quit);

  if (SDL_CreateWindowAndRenderer(
    WINDOW_WIDTH,
    WINDOW_HEIGHT,
#ifdef IS_DEBUG
    SDL_WINDOW_RESIZABLE,
#else
    SDL_WINDOW_FULLSCREEN_DESKTOP,
#endif
    &sdlWindow,
    &sdlRenderer
  ) != 0) {
    std::cerr << "SDL_CreateWindowAndRenderer Failed: " << SDL_GetError() << std::endl;
    return false;
  }

  SDL_SetWindowTitle(sdlWindow, "YARPGP");

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
  SDL_RenderSetLogicalSize(sdlRenderer, SCREEN_WIDTH, SCREEN_HEIGHT);

  content = new Content(sdlRenderer);
  content->loadTexture("content/terrain.png", "terrain");
  content->loadTexture("content/protagonist.png", "player");
  content->loadTexture("content/panel.png", "panel");
  content->loadTexture("content/npc_male.png", "npc-male");

  content->loadFont("content/quickly.ttf", "default", 16);

  return true;
}

bool Game::create() {
  if (!createUI()) { return false; }
  if (!createPlayer()) { return false; }
  if (!createNPC()) { return false; }

  tileset = new Tileset(content->getTexture("terrain"), 32, 32);
  tileset->slice();

  tilemap = new Tilemap(17, 16);

  // TODO: load tilemap from file maybe from tmx or compile tmx to proprietary binary?
  unsigned testmap[] = {
    191,191,191,234,191,234,191,191,191,191,191,233,191,245,191,191,191,
    191,234,191,191,191,191,191,191,233,191,191,236,191,191,191,233,191,
    191,191,191,236,191,234,191,191,191,191,191,191,191,191,191,237,191,
    245,191,191,234,191,232,191,191,233,191,191,233,191,233,191,191,191,
    191,191,236,191,191,191,191,191,191,191,191,191,191,236,233,191,191,
    191,191,191,191,191,191,233,191,233,191,191,191,191,191,233,233,191,
    191,232,191,191,233,191,191,191,191,245,191,234,191,191,191,191,191,
    236,191,191,191,191,233,191,191,191,191,191,191,191,232,191,234,191,
    191,191,191,233,233,191,245,191,191,234,233,191,191,191,233,191,234,
    191,191,191,233,191,234,191,191,191,191,191,191,191,191,191,236,191,
    191,233,233,191,233,233,191,191,233,191,191,191,233,191,234,191,191,
    233,233,233,191,191,191,191,191,191,191,234,191,191,236,191,191,234,
    191,191,191,234,191,191,232,191,191,191,191,191,245,191,232,191,191,
    233,191,236,191,191,191,191,191,191,191,233,234,301,302,302,302,303,
    191,191,233,245,191,233,191,191,191,301,302,302,282,323,124,323,281,
    191,191,191,191,191,191,191,191,233,322,323,124,323,126,323,323,126
  };

  tilemap->setBase(testmap);

  // std::cout << "placing player at " << player->x << ", " << player->y << std::endl;

  return true;
}

bool Game::createUI() {
  dialoguePanel = new NinePatch(content->getTexture("panel"));
  dialoguePanel->setAlpha(192);

  return true;
}

bool Game::createPlayer() {

  player = new Sprite(content->getTexture("player"), 64, 64);

  SDL_Rect walkNorthRegion;
  SDL_Rect walkSouthRegion;
  SDL_Rect walkWestRegion;
  SDL_Rect walkEastRegion;

  walkNorthRegion.x = 0;
  walkNorthRegion.y = 64 * 8;
  walkNorthRegion.w = 64 * 9;
  walkNorthRegion.h = 64;

  walkSouthRegion.x = 0;
  walkSouthRegion.y = 64 * 10;
  walkSouthRegion.w = 64 * 9;
  walkSouthRegion.h = 64;

  walkWestRegion.x = 0;
  walkWestRegion.y = 64 * 9;
  walkWestRegion.w = 64 * 9;
  walkWestRegion.h = 64;

  walkEastRegion.x = 0;
  walkEastRegion.y = 64 * 11;
  walkEastRegion.w = 64 * 9;
  walkEastRegion.h = 64;

  unsigned walkNorthFrames[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  unsigned walkSouthFrames[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  unsigned walkWestFrames[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  unsigned walkEastFrames[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

  unsigned walkNorthCount = sizeof(walkNorthFrames) / sizeof(unsigned);
  unsigned walkSouthCount = sizeof(walkSouthFrames) / sizeof(unsigned);
  unsigned walkWestCount = sizeof(walkWestFrames) / sizeof(unsigned);
  unsigned walkEastCount = sizeof(walkEastFrames) / sizeof(unsigned);

  player->addAnimation("walkNorth", walkNorthCount, walkNorthFrames, &walkNorthRegion);
  player->addAnimation("walkSouth", walkSouthCount, walkSouthFrames, &walkSouthRegion);
  player->addAnimation("walkWest", walkWestCount, walkWestFrames, &walkWestRegion);
  player->addAnimation("walkEast", walkEastCount, walkEastFrames, &walkEastRegion);

  player->addAnimation("walkNorthWest", walkNorthCount, walkNorthFrames, &walkNorthRegion);
  player->addAnimation("walkSouthWest", walkSouthCount, walkSouthFrames, &walkSouthRegion);
  player->addAnimation("walkNorthEast", walkNorthCount, walkNorthFrames, &walkNorthRegion);
  player->addAnimation("walkSouthEast", walkSouthCount, walkSouthFrames, &walkSouthRegion);


  unsigned faceFrames[] = { 0 };
  player->addAnimation("faceNorth", 1, faceFrames, &walkNorthRegion);
  player->addAnimation("faceSouth", 1, faceFrames, &walkSouthRegion);
  player->addAnimation("faceWest", 1, faceFrames, &walkWestRegion);
  player->addAnimation("faceEast", 1, faceFrames, &walkEastRegion);

  player->selectAnimation("faceSouth");

  player->x = (SCREEN_WIDTH - player->width) / 2;
  player->y = (SCREEN_HEIGHT - player->height) / 2;

  return true;
}

bool Game::createNPC() {
  npc = new Sprite(content->getTexture("npc-male"), 64, 64);

  SDL_Rect walkNorthRegion;
  SDL_Rect walkSouthRegion;
  SDL_Rect walkWestRegion;
  SDL_Rect walkEastRegion;

  walkNorthRegion.x = 0;
  walkNorthRegion.y = 64 * 8;
  walkNorthRegion.w = 64 * 9;
  walkNorthRegion.h = 64;

  walkSouthRegion.x = 0;
  walkSouthRegion.y = 64 * 10;
  walkSouthRegion.w = 64 * 9;
  walkSouthRegion.h = 64;

  walkWestRegion.x = 0;
  walkWestRegion.y = 64 * 9;
  walkWestRegion.w = 64 * 9;
  walkWestRegion.h = 64;

  walkEastRegion.x = 0;
  walkEastRegion.y = 64 * 11;
  walkEastRegion.w = 64 * 9;
  walkEastRegion.h = 64;

  unsigned walkNorthFrames[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  unsigned walkSouthFrames[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  unsigned walkWestFrames[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  unsigned walkEastFrames[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

  unsigned walkNorthCount = sizeof(walkNorthFrames) / sizeof(unsigned);
  unsigned walkSouthCount = sizeof(walkSouthFrames) / sizeof(unsigned);
  unsigned walkWestCount = sizeof(walkWestFrames) / sizeof(unsigned);
  unsigned walkEastCount = sizeof(walkEastFrames) / sizeof(unsigned);

  npc->addAnimation("walkNorth", walkNorthCount, walkNorthFrames, &walkNorthRegion);
  npc->addAnimation("walkSouth", walkSouthCount, walkSouthFrames, &walkSouthRegion);
  npc->addAnimation("walkWest", walkWestCount, walkWestFrames, &walkWestRegion);
  npc->addAnimation("walkEast", walkEastCount, walkEastFrames, &walkEastRegion);

  npc->addAnimation("walkNorthWest", walkNorthCount, walkNorthFrames, &walkNorthRegion);
  npc->addAnimation("walkSouthWest", walkSouthCount, walkSouthFrames, &walkSouthRegion);
  npc->addAnimation("walkNorthEast", walkNorthCount, walkNorthFrames, &walkNorthRegion);
  npc->addAnimation("walkSouthEast", walkSouthCount, walkSouthFrames, &walkSouthRegion);


  unsigned faceFrames[] = { 0 };
  npc->addAnimation("faceNorth", 1, faceFrames, &walkNorthRegion);
  npc->addAnimation("faceSouth", 1, faceFrames, &walkSouthRegion);
  npc->addAnimation("faceWest", 1, faceFrames, &walkWestRegion);
  npc->addAnimation("faceEast", 1, faceFrames, &walkEastRegion);

  npc->selectAnimation("faceSouth");

  npc->x = (SCREEN_WIDTH - npc->width) / 4;
  npc->y = (SCREEN_HEIGHT - npc->height) / 4;


  memset(npcPath, 0, 2048 * sizeof(unsigned));

  unsigned p = 0;
  auto pushMoveCommand = [&p](unsigned* path, unsigned direction, unsigned distance) {
    path[p + 0] = 0x1a;
    path[p + 1] = direction;
    path[p + 2] = distance;
    p += 3;
  };

  auto pushWaitCommand = [&p](unsigned* path, unsigned duration) {
    path[p + 0] = 0x2b;
    path[p + 1] = duration;
    p += 2;
  };

  auto pushTurnCommand = [&p](unsigned* path, unsigned direction) {
    path[p + 0] = 0x3c;
    path[p + 1] = direction;
    p += 2;
  };

  auto pushRepeatCommand = [&p](unsigned* path) {
    path[p + 0] = 0x4d;
    p += 1;
  };

  pushTurnCommand(npcPath, east);
  pushMoveCommand(npcPath, east, 8);
  pushWaitCommand(npcPath, 1500);

  pushTurnCommand(npcPath, south);
  pushMoveCommand(npcPath, south, 4);
  pushWaitCommand(npcPath, 1500);

  pushTurnCommand(npcPath, west);
  pushMoveCommand(npcPath, west, 8);
  pushWaitCommand(npcPath, 1500);

  pushTurnCommand(npcPath, north);
  pushMoveCommand(npcPath, north, 4);
  pushWaitCommand(npcPath, 1500);

  pushRepeatCommand(npcPath);

  npcIP = 0;

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
  updateNPC();

  bool moving = false;

  const unsigned char* keys = SDL_GetKeyboardState(NULL);

  unsigned mcc = 0x00;

  if (keys[SDL_SCANCODE_UP]) {
    mcc |= 0x02;
    if (!player->isAnimation("walkNorth")) {
      player->selectAnimation("walkNorth");
      player->gotoAndPlay(0);
    }
  } else if (keys[SDL_SCANCODE_DOWN]) {
    mcc |= 0x04;
    if (!player->isAnimation("walkSouth")) {
      player->selectAnimation("walkSouth");
      player->gotoAndPlay(0);
    }
  }

  if (keys[SDL_SCANCODE_LEFT]) {
    mcc |= 0x08;

    if (mcc & 0x02) {
      if (!player->isAnimation("walkNorthWest")) {
        player->selectAnimation("walkNorthWest");
        player->gotoAndPlay(0);
      }
    } else if (mcc & 0x04) {
      if (!player->isAnimation("walkSouthWest")) {
        player->selectAnimation("walkSouthWest");
        player->gotoAndPlay(0);
      }
    } else {
      if (!player->isAnimation("walkWest")) {
        player->selectAnimation("walkWest");
        player->gotoAndPlay(0);
      }
    }
  } else if (keys[SDL_SCANCODE_RIGHT]) {
    mcc |= 0x10;

    if (mcc & 0x02) {
      if (!player->isAnimation("walkNorthEast")) {
        player->selectAnimation("walkNorthEast");
        player->gotoAndPlay(0);
      }
    } else if (mcc & 0x04) {
      if (!player->isAnimation("walkSouthEast")) {
        player->selectAnimation("walkSouthEast");
        player->gotoAndPlay(0);
      }
    } else {
      if (!player->isAnimation("walkEast")) {
        player->selectAnimation("walkEast");
        player->gotoAndPlay(0);
      }
    }
  }

  float mx = mt[mm[mcc]];
  float my = mt[mm[mcc + 1]];

  float speed = 48.0f;

  SDL_Keymod modState = SDL_GetModState();

  if (modState & KMOD_SHIFT) {
    speed = 96;
    player->setDuration(0.5f);
  } else {
    player->setDuration(1.0f);
  }

  mx *= speed;
  my *= speed;

  player->xv = mx;
  player->yv = my;

  moving = mcc != 0x00;

  if (!moving) {
    if (player->isAnimation("walkNorth") || player->isAnimation("walkNorthWest") || player->isAnimation("walkNorthEast")) {
      player->selectAnimation("faceNorth");
    } else if (player->isAnimation("walkSouth") || player->isAnimation("walkSouthWest") || player->isAnimation("walkSouthEast")) {
      player->selectAnimation("faceSouth");
    } else if (player->isAnimation("walkWest")) {
      player->selectAnimation("faceWest");
    } else if (player->isAnimation("walkEast")) {
      player->selectAnimation("faceEast");
    }

    player->gotoAndStop(0);
  }

  float deltaTime = 0.033f;
  player->update(deltaTime);
}

void Game::updateNPC() {
  switch (npcState) {
    // moving
    case 0x10: {
      moveNPC(npcDirection, false);
      npcDistanceMoved += 1;
      // std::cerr << "moving " << npcDistanceMoved << "/" << npcDistanceToMove << std::endl;
      if (npcDistanceMoved >= npcDistanceToMove) {
        moveNPC(npcDirection, true);
        npcState = 0xff;
      }
    } break;

    // waiting
    case 0x20: {
      npcTimeWaited += 0.033f;
      if (npcTimeWaited >= npcTimeToWait) {
        npcState = 0xff;
      }
    } break;

    // processing
    default: {
      unsigned nextCommand = npcPath[npcIP];
      switch (nextCommand) {
        // move
        case 0x1a: {
          std::cerr << "move command" << std::endl;
          unsigned direction = npcPath[npcIP + 1];
          unsigned distance = npcPath[npcIP + 2];
          npcDistanceToMove = distance * 16;
          npcDistanceMoved = 0;
          npcDirection = direction;

          moveNPC(npcDirection, false);

          npcState = 0x10;
          npcIP += 3;
        } break;

        // wait
        case 0x2b: {
          std::cerr << "wait command" << std::endl;
          unsigned duration = npcPath[npcIP + 1];
          npcTimeToWait = (float)duration * 0.001f;
          npcTimeWaited = 0;
          npcState = 0x20;
          npcIP += 2;
        } break;

        // turn
        case 0x3c: {
          std::cerr << "turn command" << std::endl;
          unsigned direction = npcPath[npcIP + 1];
          if (direction == north || direction == northWest || direction == northEast) {
            npc->selectAnimation("faceNorth");
          } else if (direction == south || direction == southWest || direction == southEast) {
            npc->selectAnimation("faceSouth");
          } else if (direction == west) {
            npc->selectAnimation("faceWest");
          } else if (direction == east) {
            npc->selectAnimation("faceEast");
          }

          npc->gotoAndStop(0);
          npcIP += 2;
        } break;

        // repeat
        case 0x4d: {
          std::cerr << "repeat command" << std::endl;
          npcIP = 0;
        } break;

        default: {
          std::cerr << "unknown command " << nextCommand << std::endl;
        } break;
      }
    } break;
  }
}

void Game::moveNPC(unsigned direction, bool stopped) {
  unsigned NPC_UP = 0;
  unsigned NPC_DOWN = 1;
  unsigned NPC_LEFT = 2;
  unsigned NPC_RIGHT = 3;

  unsigned keys[] = { 0, 0, 0, 0 };

  if (!stopped) {
    switch (direction) {
      case north: { keys[NPC_UP] = 1; } break;
      case northEast: { keys[NPC_UP] = 1; keys[NPC_RIGHT] = 1; } break;
      case east: { keys[NPC_RIGHT] = 1; } break;
      case southEast: { keys[NPC_RIGHT] = 1; keys[NPC_DOWN] = 1; } break;
      case south: { keys[NPC_DOWN] = 1; } break;
      case southWest: { keys[NPC_DOWN] = 1; keys[NPC_LEFT] = 1; } break;
      case west: { keys[NPC_LEFT] = 1; } break;
      case northWest: { keys[NPC_LEFT] = 1; keys[NPC_UP] = 1; } break;
      default: break;
    }
  }

  bool moving = false;

  unsigned mcc = 0x00;

  if (keys[NPC_UP]) {
    mcc |= 0x02;
    if (!npc->isAnimation("walkNorth")) {
      npc->selectAnimation("walkNorth");
      npc->gotoAndPlay(0);
    }
  } else if (keys[NPC_DOWN]) {
    mcc |= 0x04;
    if (!npc->isAnimation("walkSouth")) {
      npc->selectAnimation("walkSouth");
      npc->gotoAndPlay(0);
    }
  }

  if (keys[NPC_LEFT]) {
    mcc |= 0x08;

    if (mcc & 0x02) {
      if (!npc->isAnimation("walkNorthWest")) {
        npc->selectAnimation("walkNorthWest");
        npc->gotoAndPlay(0);
      }
    } else if (mcc & 0x04) {
      if (!npc->isAnimation("walkSouthWest")) {
        npc->selectAnimation("walkSouthWest");
        npc->gotoAndPlay(0);
      }
    } else {
      if (!npc->isAnimation("walkWest")) {
        npc->selectAnimation("walkWest");
        npc->gotoAndPlay(0);
      }
    }
  } else if (keys[NPC_RIGHT]) {
    mcc |= 0x10;

    if (mcc & 0x02) {
      if (!npc->isAnimation("walkNorthEast")) {
        npc->selectAnimation("walkNorthEast");
        npc->gotoAndPlay(0);
      }
    } else if (mcc & 0x04) {
      if (!npc->isAnimation("walkSouthEast")) {
        npc->selectAnimation("walkSouthEast");
        npc->gotoAndPlay(0);
      }
    } else {
      if (!npc->isAnimation("walkEast")) {
        npc->selectAnimation("walkEast");
        npc->gotoAndPlay(0);
      }
    }
  }

  float mx = mt[mm[mcc]];
  float my = mt[mm[mcc + 1]];

  float speed = 48.0f;

  npc->setDuration(1.0f);

  mx *= speed;
  my *= speed;

  npc->xv = mx;
  npc->yv = my;

  moving = mcc != 0x00;

  if (!moving) {
    if (npc->isAnimation("walkNorth") || npc->isAnimation("walkNorthWest") || npc->isAnimation("walkNorthEast")) {
      npc->selectAnimation("faceNorth");
    } else if (npc->isAnimation("walkSouth") || npc->isAnimation("walkSouthWest") || npc->isAnimation("walkSouthEast")) {
      npc->selectAnimation("faceSouth");
    } else if (npc->isAnimation("walkWest")) {
      npc->selectAnimation("faceWest");
    } else if (npc->isAnimation("walkEast")) {
      npc->selectAnimation("faceEast");
    }

    npc->gotoAndStop(0);
  }

  // std::cerr << "update mcc " << mcc << std::endl;

  float deltaTime = 0.033f;
  npc->update(deltaTime);
}

void Game::render() {
  SDL_SetRenderDrawColor(sdlRenderer, 30, 60, 90, 255);
  SDL_RenderClear(sdlRenderer);

  // static const unsigned ROWS = (unsigned)ceil(0.5 + SCREEN_WIDTH / 32);
  // static const unsigned COLUMNS = (unsigned)ceil(0.5 + SCREEN_HEIGHT / 32);
  static const unsigned ROWS = 17;
  static const unsigned COLUMNS = 16;

  // 17x16
  // std::cout << "" << ROWS << "x" << COLUMNS << std::endl;


  SDL_Rect destRect;
  destRect.w = tileset->width;
  destRect.h = tileset->height;

  bool f = true;
  for (unsigned row = 0; row < ROWS; row += 1) {
    destRect.y = row * destRect.h;
    for (unsigned column = 0; column < COLUMNS; column += 1) {
      destRect.x = column * destRect.w;

      // unsigned tileId = f ? 0 : 1;
      unsigned tileId = tilemap->getBaseValue(column, row) - 1;
      tileset->drawTile(sdlRenderer, tileId, &destRect);
      f = !f;
    }
    f = !f;
  }

  // TODO: proper renderlist with Y sorting of sprites
  if (player->y > npc->y) {
    npc->draw(sdlRenderer);
    player->draw(sdlRenderer);
  } else {
    player->draw(sdlRenderer);
    npc->draw(sdlRenderer);
  }

  SDL_Rect panelRect;
  panelRect.w = SCREEN_WIDTH * 0.95;
  panelRect.h = SCREEN_HEIGHT * 0.33;
  panelRect.x = (SCREEN_WIDTH - panelRect.w) / 2;
  panelRect.y = (SCREEN_HEIGHT - (panelRect.h + 8));
  dialoguePanel->draw(sdlRenderer, &panelRect);

  TTF_Font* font = content->getFont("default");
  SDL_Color white { 255, 255, 255, 255 };
  // SDL_Surface* textSurface = TTF_RenderText_Solid(font, "Funny-looking-guy: You have no idea how hard this is to code...", white);

  // 16 capital letters is the "safe" length for a line of dialogue
  // up to 4 lines of dialogue per "page"

  SDL_Surface* textSurface = TTF_RenderText_Solid(font, "Yet Another RPG Project...", white);

  SDL_Texture* textTexture = SDL_CreateTextureFromSurface(sdlRenderer, textSurface);
  SDL_FreeSurface(textSurface);

  destRect.x = panelRect.x + 8;
  destRect.y = panelRect.y + 8;
  SDL_QueryTexture(textTexture, NULL, NULL, &destRect.w, &destRect.h);
  SDL_RenderCopy(sdlRenderer, textTexture, NULL, &destRect);

  SDL_RenderPresent(sdlRenderer);
}

SDL_Texture* Game::loadTexture(const char* path) {
  SDL_Surface* surface = IMG_Load(path);

  if (!surface) {
    std::cerr << "Failed to load texture from \"" << path << "\" : " << IMG_GetError() << std::endl;
    return NULL;
  }

  SDL_Texture* texture = SDL_CreateTextureFromSurface(sdlRenderer, surface);

  if (!texture) {
    std::cerr << "Failed to create texture from surface: " << SDL_GetError() << std::endl;
    return NULL;
  }

  SDL_FreeSurface(surface);

  return texture;
}
