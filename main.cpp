#include <fstream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <ctime>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
struct Button {
    SDL_Rect rect;
    SDL_Color color;
    std::string text;
};
enum EnemyType{SLOW, FAST, RANGED};
struct Enemy {
    SDL_Rect rect;
    float vx, vy;
    EnemyType type;
};
struct Projectile {
    SDL_Rect rect;
    float vx, vy;
};
// Game states
enum GameState { MENU, PLAYING, PAUSED, GAME_OVER, VICTORY };

GameState gameState = MENU;
int score = 0;
int lives = 3;
int level = 1;
SDL_Texture* heartTex = nullptr;

int highScore = 0;
SDL_Rect playerRect = { 368,300,64,64 };
SDL_Rect itemRect = { 400,400,32,32 };
SDL_Texture* itemTex = nullptr;
const int playerSpeed = 50;

Uint32 gameStartTime = 0;
int timeLimit = 30;

std::vector<Enemy> enemies;
int enemiesPerLevel = 2;
float enemyBaseSpeed = 80.0f;

bool isInvulnerable = false;
Uint32 invulnerableStart = 0;
const int invulnerableDuration = 1000; //ms

int currentWave = 1;
int enemiesToSpawn = 0;
bool waveInProgress = false;
Uint32 waveStartTime = 0;
Uint32 lastWaveTime = 0;
Uint32 waveDelay = 3000; //ms

std::vector<Projectile> projectiles;
Uint32 lastShootTime = 0;
Uint32 shootInterval = 2000; //ms

bool Init(SDL_Window** window, SDL_Renderer** renderer, int w, int h) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
    if (TTF_Init() == -1) return false;
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) return false;

    *window = SDL_CreateWindow("Etapa 10", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN);
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    return *window && *renderer;
}

SDL_Texture* LoadTexture(const std::string& path, SDL_Renderer* renderer) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Error loading texture: " << path << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

SDL_Texture* RenderText(const std::string& msg, TTF_Font* font, SDL_Color color, SDL_Renderer* renderer) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, msg.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void RenderHUD(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Texture* scoreTex = RenderText("Score: " + std::to_string(score), font, white, renderer);
    SDL_Rect dest = { 20, 20, 0, 0 };
    SDL_QueryTexture(scoreTex, nullptr, nullptr, &dest.w, &dest.h);
    SDL_RenderCopy(renderer, scoreTex, nullptr, &dest);
    SDL_DestroyTexture(scoreTex);

    SDL_Texture* levelTex = RenderText("Level: " + std::to_string(level), font, white, renderer);
    SDL_Rect lRect = { 200,100,0,0 };
    SDL_QueryTexture(levelTex, nullptr, nullptr, &lRect.w, &lRect.h);
    SDL_RenderCopy(renderer, levelTex, nullptr, &lRect);
    SDL_DestroyTexture(levelTex);
    for (int i = 0; i < lives; i++) {
        SDL_Rect heartRect = { 20 + i * 40, 60, 32, 32 };
        SDL_RenderCopy(renderer, heartTex, nullptr, &heartRect);
    }

    if (gameState == PLAYING) {
        Uint32 now = SDL_GetTicks();
        int timeLeft = timeLimit - (now - gameStartTime) / 1000;
        if (timeLeft < 0) timeLeft = 0;

        SDL_Texture* timeTex = RenderText("Time: " + std::to_string(timeLeft), font, white, renderer);
        SDL_Rect tRect = { 700,20,0,0 };
        SDL_QueryTexture(timeTex, nullptr, nullptr, &tRect.w, &tRect.h);
        SDL_RenderCopy(renderer, timeTex, nullptr, &tRect);
        SDL_DestroyTexture(timeTex);
    }

    SDL_Texture* highTex = RenderText("High Score: "+std::to_string(highScore), font, white, renderer);
    SDL_Rect hRect = { 20,100,0,0 };
    SDL_QueryTexture(highTex, nullptr, nullptr, &hRect.w, &hRect.h);
    SDL_RenderCopy(renderer, highTex, nullptr, &hRect);
    SDL_DestroyTexture(highTex);

    SDL_Texture* waveTex = RenderText("Wave: " + std::to_string(currentWave), font, white, renderer);
    SDL_Rect wRect = { 600,60,0,0 };
    SDL_QueryTexture(waveTex, nullptr, nullptr, &wRect.w, &wRect.h);
    SDL_RenderCopy(renderer, waveTex, nullptr, &wRect);
    SDL_DestroyTexture(waveTex);
}

void RenderButton(SDL_Renderer* renderer, TTF_Font* font, Button btn, bool hovered) {
    SDL_Color color;
    if (hovered) {
        color.r = (Uint8)std::min(255, btn.color.r + 40);
        color.g = (Uint8)std::min(255, btn.color.g + 40);
        color.b = (Uint8)std::min(255, btn.color.a + 40);
        color.a = 255;
    }else {
        color = btn.color;
    }
    
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(renderer, &btn.rect);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &btn.rect);

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Texture* tex = RenderText(btn.text, font, white, renderer);
    int texW = 0, texH = 0;
    SDL_QueryTexture(tex, nullptr, nullptr, &texW, &texH);
    SDL_Rect textRect{
        btn.rect.x + (btn.rect.w - texW)/2,
        btn.rect.y + (btn.rect.h - texH)/2,
        texW, texH
    };
    SDL_RenderCopy(renderer, tex, nullptr, &textRect);
    SDL_DestroyTexture(tex);
}

void LoadHighScore() {
    std::ifstream file("score.txt");
    if (file.is_open()) {
        file >> highScore;
        file.close();
    }
}

void SaveHighScore() {
    std::ofstream file("score.txt");
    if (file.is_open()) {
        file << highScore;
        file.close();
    }
}

void SpawnEnemy(int screenW, int screenH) {
    Enemy e;
    e.rect = { rand() % (screenW - 32), rand() % (screenH - 32), 32, 32 };
    e.type = static_cast<EnemyType>(rand() % 3);
    float angle = (rand() % 360) * 3.14159f / 180.0f;
    float speed = 0.0f;
    switch (e.type) {
        case SLOW: speed = 50.0f; break;
        case FAST: speed = 150.0f; break;
        case RANGED: speed = 0.0f; break;
    }
    e.vx = cosf(angle) * speed;
    e.vy = sinf(angle) * speed;
    enemies.push_back(e);
}

void UpdateEnemies(float dt, int screenW, int screenH, SDL_Rect player) {
    //for (auto& e : enemies) {
    for (size_t i = 0; i < enemies.size();) {
        Enemy& e = enemies[i];
        if (e.type == SLOW || e.type == FAST) {
            float dx = (float)(player.x + player.w / 2) - (e.rect.x + e.rect.w / 2);
            float dy = (float)(player.y + player.h / 2) - (e.rect.y + e.rect.h / 2);
            float length = sqrtf(dx * dx + dy * dy);
            if (length != 0) {
                dx /= length;
                dy /= length;
            }

            float speed = enemyBaseSpeed + level * 15.0f;
            e.rect.x += static_cast<int>(dx * speed * dt);
            e.rect.y += static_cast<int>(dy * speed * dt);
        } else {
            Uint32 now = SDL_GetTicks();
            if (now - lastShootTime > shootInterval) {
                Projectile p;
                p.rect = { e.rect.x + e.rect.w / 2 - 4, e.rect.y + e.rect.h / 2 - 4 , 8, 8 };
                float dx = (player.x + player.w / 2.0f) - (e.rect.x + e.rect.w / 2.0f);
                float dy = (player.y + player.h / 2.0f) - (e.rect.y + e.rect.h / 2.0f);
                float lengthProjectile = sqrtf(dx * dx + dy * dy);
                if (lengthProjectile != 0) {
                    dx /= lengthProjectile;
                    dy /= lengthProjectile;
                }
                float bulletSpeed = 200.0f;
                p.vx = dx * bulletSpeed;
                p.vy = dy * bulletSpeed;
                projectiles.push_back(p);
                lastShootTime = now;
            }
        }
        /*if (e.rect.x < 0) e.rect.x = 0;
        if (e.rect.x + e.rect.w > screenW) e.rect.x = screenW - e.rect.w;
        if (e.rect.y < 0) e.rect.y = 0;
        if (e.rect.y + e.rect.h > screenH) e.rect.y = screenH - e.rect.h;*/
        if (e.rect.x + e.rect.w < 0 || e.rect.x > screenW ||
            e.rect.y + e.rect.h < 0 || e.rect.y > screenH) {
            enemies.erase(enemies.begin() + i);
            score += 5;
            if (score > highScore) {
                highScore = score;
                SaveHighScore();
            }
        } else {
            i++;
        }
    }
}

void UpdateProjectiles(float dt, int screenW, int screenH) {
    for (size_t i = 0; i < projectiles.size();) {
        auto& p = projectiles[i];
        p.rect.x += static_cast<int>(p.vx * dt);
        p.rect.y += static_cast<int>(p.vy * dt);
        if (SDL_HasIntersection(&p.rect, &playerRect) && !isInvulnerable) {
            lives--;
            isInvulnerable = true;
            invulnerableStart = SDL_GetTicks();
            projectiles.erase(projectiles.begin() + i);
            continue;
        }
        if (p.rect.x < 0 || p.rect.x > screenW || p.rect.y < 0 || p.rect.y > screenH) {
            projectiles.erase(projectiles.begin() + i);
        } else {
            i++;
        }
    }
}

void StartWave() {
    enemies.clear();
    enemiesToSpawn = currentWave * 3;
    for (int i = 0; i < enemiesToSpawn; i++) {
        SpawnEnemy(800, 600);
    }
    waveInProgress = true;
    waveStartTime = SDL_GetTicks();
    lastWaveTime = waveStartTime;

}

int main(int argc, char* argv[]) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* spritesheet = nullptr;
    if (!Init(&window, &renderer, 800, 600)) return 1;

    TTF_Font* font = TTF_OpenFont("assets/font.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font\n";
        return 1;
    }
    Button playButton = { {300,250,200,60}, {0,120,255}, "PLAY"};
    Button restartButton = { {300,330,200,60}, {0,200,100}, "RESTART"};
    bool running = true;
    SDL_Event event;
    Uint32 lastTick = SDL_GetTicks();
    int alpha = 0;
    Uint32 fadeStart = 0;

    heartTex = LoadTexture("assets/heart.png", renderer);
    if (!heartTex) return 1;
    
    spritesheet = LoadTexture("assets/spritesheet.png", renderer);
    if (!spritesheet) return 1;

    itemTex = LoadTexture("assets/coin.png", renderer);
    if (!itemTex) return 1;

    const int frameWidth = 64;
    const int frameHeight = 64;
    const int numFrames = 4;
    int currentFrame = 0;
    Uint32 frameDuration = 150;
    Uint32 lastFrameTime = SDL_GetTicks();

    Mix_Music* bgMusic = Mix_LoadMUS("assets/music.ogg");
    Mix_Chunk* correctSound = Mix_LoadWAV("assets/correct.wav");
    Mix_Chunk* wrongSound = Mix_LoadWAV("assets/wrong.wav");
    Mix_Chunk* winSound = Mix_LoadWAV("assets/victory.wav");
    Mix_Chunk* gameoverSound = Mix_LoadWAV("assets/gameover.wav");
    if (!bgMusic || !correctSound || !wrongSound || !winSound || !gameoverSound) return 1;

    srand(static_cast<unsigned>(time(nullptr)));


    LoadHighScore();
    //Playing game
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int x = event.button.x;
                int y = event.button.y;
                SDL_Point clickPoint = { x,y };

                switch (gameState) {
                case MENU:
                    if (SDL_PointInRect(&clickPoint, &playButton.rect)) {
                        gameState = PLAYING;
                        score = 0;
                        lives = 3;
                        level = 1;
                        currentWave = 1;
                        enemies.clear();
                        StartWave();
                        gameStartTime = SDL_GetTicks();

                        alpha = 0;
                        fadeStart = SDL_GetTicks();
                    }
                    break;
                case GAME_OVER:
                    if (SDL_PointInRect(&clickPoint, &restartButton.rect)) {
                        score = 0;
                        lives = 3;
                        level = 1;
                        enemies.clear();
                        isInvulnerable = false;
                        waveInProgress = true;
                        currentWave = 1;
                        gameState = MENU;
                    }
                    break;
                case VICTORY:
                    if (SDL_PointInRect(&clickPoint, &restartButton.rect)) {
                        score = 0;
                        lives = 3;
                        level = 1;
                        enemies.clear();
                        isInvulnerable = false;
                        waveInProgress = true;
                        currentWave = 1;
                        gameState = MENU;
                    }
                    break;
                }
            }
            if (event.type == SDL_KEYDOWN) {
                switch (gameState) {
                case PLAYING: { 
                    SDL_Keycode pressed = event.key.keysym.sym;
                    
                    timeLimit = 30 - (level - 1) * 5;
                    if (timeLimit < 10) timeLimit = 10;
                    Uint32 now = SDL_GetTicks();
                    int timeLeft = timeLimit - (now - gameStartTime) / 1000;
                    if (timeLeft <= 0) {
                        Mix_HaltMusic();
                        Mix_PlayChannel(-1, gameoverSound, 0);
                        gameState = GAME_OVER;
                    }
                    if (event.key.keysym.sym == SDLK_p) {
                        gameState = PAUSED;
                    }
                    if (pressed == SDLK_w) playerRect.y -= playerSpeed;
                    if (pressed == SDLK_s) playerRect.y += playerSpeed;
                    if (pressed == SDLK_a) playerRect.x -= playerSpeed;
                    if (pressed == SDLK_d) playerRect.x += playerSpeed;
                    
                    if (SDL_HasIntersection(&playerRect, &itemRect)) {
                        score += 10;
                        itemRect.x = rand() % (800 - itemRect.w);
                        itemRect.y = rand() % (600 - itemRect.h);

                        if (score % 30 == 0) {
                            level++;
                        }

                        if (score > highScore) {
                            highScore = score;
                            SaveHighScore();
                        }
                    }
                    if (!isInvulnerable) {
                        for (auto& e : enemies) {
                            if (SDL_HasIntersection(&playerRect, &e.rect)) {
                                lives--;
                                Mix_PlayChannel(-1, wrongSound, 0);
                                isInvulnerable = true;
                                invulnerableStart = SDL_GetTicks();

                                if (lives <= 0) {
                                    Mix_HaltMusic();
                                    Mix_PlayChannel(-1, gameoverSound, 0);
                                    gameState = GAME_OVER;
                                }
                                break;
                            }
                        }
                    }
                    if (isInvulnerable) {
                        Uint32 now = SDL_GetTicks();
                        if (now - invulnerableStart >= invulnerableDuration) isInvulnerable = false;
                    }
                    break;
                }
                case PAUSED:
                    if (event.key.keysym.sym == SDLK_p) {
                        gameState = PLAYING;
                    }
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        gameState = MENU;
                    }
                    break;
                }
            }
        }

        float dt = (SDL_GetTicks() - lastTick) / 1000.0f;
        lastTick = SDL_GetTicks();

        if (waveInProgress) {
            Uint32 now = SDL_GetTicks();
            if (now - waveStartTime >= waveDelay) {
                currentWave++;
                score += 20;
                StartWave();
            }
        }

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        //Rendering game
        switch (gameState) {
            case MENU: {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                SDL_Point mousePoint = { mx, my };

                bool hoveredPlay = SDL_PointInRect(&mousePoint, &playButton.rect);
                RenderButton(renderer, font, playButton, hoveredPlay);
                break;
            }
            case PLAYING: {
                RenderHUD(renderer, font);
                UpdateEnemies(dt, 800, 600, playerRect);
                UpdateProjectiles(dt, 800, 600);
                std::string msg = "Avoid the enemies!";
                SDL_Color c = { 0, 255, 255, 255 };
                SDL_Texture* t = RenderText(msg, font, c, renderer);
                SDL_Rect r = { 300,100,0,0 };
                SDL_QueryTexture(t, nullptr, nullptr, &r.w, &r.h);
                SDL_RenderCopy(renderer, t, nullptr, &r);
                SDL_DestroyTexture(t);
                
                Uint32 now = SDL_GetTicks();
                if (now - lastFrameTime >= frameDuration) {
                    currentFrame = (currentFrame + 1) % numFrames;
                    lastFrameTime = now;
                }
                if (alpha < 255) {
                    Uint32 now = SDL_GetTicks();
                    alpha = (int)(now - fadeStart) / 2;
                    if (alpha > 255) alpha = 255;
                    SDL_SetTextureAlphaMod(spritesheet, (Uint8)alpha);
                }
                if (isInvulnerable && ((SDL_GetTicks()/100)%2 == 0)){
                    SDL_SetTextureAlphaMod(spritesheet, 120);
                } else {
                    SDL_SetTextureAlphaMod(spritesheet, 255);
                }
                SDL_Rect srcRect = { currentFrame * frameWidth, 0, frameWidth, frameHeight };
                SDL_Rect dstRect = { 368, 300, frameWidth, frameHeight };
                SDL_RenderCopy(renderer, spritesheet, &srcRect, &playerRect);
                SDL_RenderCopy(renderer, itemTex, nullptr, &itemRect);
                for (auto& e : enemies) {
                    switch (e.type) {
                        case RANGED: SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); break;
                        case SLOW: SDL_SetRenderDrawColor(renderer, 0, 128, 255, 255); break;
                        case FAST: SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); break;
                    }
                    SDL_RenderFillRect(renderer, &e.rect);
                }
                for (auto& p : projectiles) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderFillRect(renderer, &p.rect);
                }
                break;
            }
            case VICTORY: {
                SDL_Color c = { 0, 255, 0, 255 };
                SDL_Texture* t = RenderText("You Win!", font, c, renderer);
                SDL_Rect r = { 150, 250, 0, 0 };
                SDL_QueryTexture(t, nullptr, nullptr, &r.w, &r.h);
                SDL_RenderCopy(renderer, t, nullptr, &r);
                SDL_DestroyTexture(t);

                int mx, my;
                SDL_GetMouseState(&mx, &my);
                SDL_Point mousePoint = { mx, my };

                bool hoveredRestart = SDL_PointInRect(&mousePoint, &restartButton.rect);
                RenderButton(renderer, font, restartButton, hoveredRestart);
                break;
            }
            case GAME_OVER: {
                SDL_Color c = { 255, 0, 0, 255 };
                SDL_Texture* t = RenderText("Game Over", font, c, renderer);
                SDL_Rect r = { 150, 250, 0, 0 };
                SDL_QueryTexture(t, nullptr, nullptr, &r.w, &r.h);
                SDL_RenderCopy(renderer, t, nullptr, &r);
                SDL_DestroyTexture(t);

                int mx, my;
                SDL_GetMouseState(&mx, &my);
                SDL_Point mousePoint = { mx, my };

                bool hoveredRestart = SDL_PointInRect(&mousePoint, &restartButton.rect);
                RenderButton(renderer, font, restartButton, hoveredRestart);
                break;
            }
            case PAUSED: {
                SDL_Color c = { 255, 255, 255, 255 };
                SDL_Texture* t = RenderText("Game Paused. Press P to Resume or ESC to Menu", font, c, renderer);
                SDL_Rect r = { 100, 250, 0, 0 };
                SDL_QueryTexture(t, nullptr, nullptr, &r.w, &r.h);
                SDL_RenderCopy(renderer, t, nullptr, &r);
                SDL_DestroyTexture(t);
                break;
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    Mix_FreeMusic(bgMusic);
    Mix_FreeChunk(correctSound);
    Mix_FreeChunk(wrongSound);
    Mix_FreeChunk(winSound);
    Mix_FreeChunk(gameoverSound);
    Mix_CloseAudio();
    TTF_CloseFont(font);
    SDL_DestroyTexture(heartTex);
    SDL_DestroyTexture(itemTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}