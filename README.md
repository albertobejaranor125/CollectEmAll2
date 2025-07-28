# CollectEmAll2

A wave-based 2D arcade minigame built in C++ using SDL2. Collect items, avoid or survive enemies with varied behaviors, and progress through increasingly difficult waves. This project is part of my learning journey in game development and programming fundamentals with C++.

---

## 📌 Game Description

**CollectEmAll2** is a real-time survival game where the player controls a character that must:
- Collect items to gain points.
- Avoid or survive enemies.
- Progress through wave-based levels of increasing difficulty.

Different types of enemies appear:
- 🐢 `SLOW`: Follows the player at low speed.
- ⚡ `FAST`: Aggressively chases the player at high speed.
- 🔫 `RANGED`: Shoots projectiles towards the player from a distance.

---

## 🚀 How to Play

- Use **WASD** to move.
- Collect coins to increase your **score**.
- Survive each **wave**.
- Avoid enemies or their projectiles. If you are hit, you lose a life.
- The game ends when you run out of lives.

---

## 🧱 Program Structure and Subprograms

This project is built using a modular structure with meaningful functions to separate logic and rendering. Key components:

### 📁 `main.cpp`
The central loop and game state manager.

### 🔁 Game Loop
```cpp
while (running) {
    handleEvents();   // SDL input handling
    updateGame();     // Game logic and physics
    renderGame();     // Drawing everything
}

🧠 Game States

enum GameState { MENU, PLAYING, PAUSED, GAME_OVER, VICTORY };

Each state is rendered and processed independently to separate logic flow.

🎮 Player Logic

    SDL_Rect playerRect handles the position and collision.

    Movement is processed via keyboard events (SDL_KEYDOWN).

💥 Enemy System

enum EnemyType { SLOW, FAST, RANGED };
struct Enemy {
    SDL_Rect rect;
    float vx, vy;
    EnemyType type;
};

    Enemies move toward the player.

    RANGED enemies shoot projectiles.

    Managed with std::vector<Enemy>.

📦 Projectile System

struct Projectile {
    SDL_Rect rect;
    float vx, vy;
};

Projectiles are fired by RANGED enemies, with damage and removal upon collision or out-of-bounds.

♻️ Wave System

Waves increase difficulty over time:

void StartWave() {
    enemies.clear();
    enemiesToSpawn = currentWave * 3;
    for (int i = 0; i < enemiesToSpawn; i++) {
        SpawnEnemy(...);
    }
}

❤️ HUD Rendering

Displays:

    Score

    Level

    Time

    Lives

    Wave number

void RenderHUD(SDL_Renderer* renderer, TTF_Font* font);

### 📂 File Handling

High scores are saved and loaded using:

void LoadHighScore();
void SaveHighScore();

Stored in score.txt.

⚙️ Dependencies

    SDL2

    SDL2_image

    SDL2_ttf

    SDL2_mixer

Make sure to link these libraries properly in your build system.

🧠 Learning Reflections

This project helped me consolidate concepts like:

    Structuring a game loop.

    Collision detection with SDL_HasIntersection.

    Managing state transitions and events.

    Modularizing game logic using C++ functions and structures.

    Basic enemy AI and variation.

It also taught me the importance of balancing difficulty and performance using timers, wave management, and player feedback systems.
