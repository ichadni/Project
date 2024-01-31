#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>

#undef main

// Direction enum declaration
enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

// Constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int TILE_SIZE = 10;
const int REGULAR_FOOD_SIZE = TILE_SIZE;
const int BONUS_FOOD_SIZE = TILE_SIZE; // Double the size
const int TARGET_FRAME_RATE = 60;          // Frames per second
const int MOVEMENT_DELAY = 100;            // Milliseconds delay between movements
const int BONUS_FOOD_DURATION = 6000;      // 4 seconds

// Snake structure
struct SnakeSegment {
    int x, y;
};

// Function prototypes
void spawnFood();
void spawnBonusFood();
void eatBonusFood();
void update();
void render();
void handleInput();
void handleBonusFoodDuration();
void displayGameOver();
bool showWelcomeScreen();

// Obstacle rectangles
SDL_Rect wallRect1{220, 70, 200, 15};
SDL_Rect wallRect2{220, 380, 200, 15};
SDL_Rect wallRect3{95, 130, 15, 200};
SDL_Rect wallRect4{530, 150, 15, 200};


// Global variables
SDL_Window* window;
SDL_Renderer* renderer;
std::vector<SnakeSegment> snake;
SnakeSegment food, bonusFood;
Direction snakeDirection = Direction::RIGHT; // Initialize the direction
int score = 0;
int regularFoodEaten = 0;
Uint32 bonusFoodTimer = 0;
bool bonusFoodActive = false;
bool gamePaused = false;

// TTF Font and Textures
TTF_Font* font;
SDL_Texture* scoreTexture;
SDL_Texture* levelTexture;
SDL_Texture* welcomeTexture;

// Button Rectangles
SDL_Rect yesButton = {150, 350, 100, 50};
SDL_Rect noButton = {400, 350, 100, 50};

int main(int argc, char* args[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (TTF_Init() < 0) {
        std::cerr << "TTF initialization failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    std::srand(static_cast<unsigned>(std::time(0)));

    snake.push_back({SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2});

    spawnFood();

    // Load font
    font = TTF_OpenFont("Moonlight.otf", 40); // Replace "arial.ttf" with the path to your font file

    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

// Show welcome screen
    bool startGame = showWelcomeScreen();

    if (!startGame) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

     snake.push_back({SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2});

    spawnFood();

    // Main game loop
    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_p) {
                    gamePaused = !gamePaused;
                }
            }else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                if (mouseX >= yesButton.x && mouseX <= yesButton.x + yesButton.w &&
                    mouseY >= yesButton.y && mouseY <= yesButton.y + yesButton.h) {
                    startGame = true;
                    snake.clear();
                    snake.push_back({SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2});
                    spawnFood();
                    score = 0;
                    regularFoodEaten = 0;
                    bonusFoodActive = false;
                    bonusFoodTimer = 0;
                    gamePaused = false;
                } else if (mouseX >= noButton.x && mouseX <= noButton.x + noButton.w &&
                           mouseY >= noButton.y && mouseY <= noButton.y + noButton.h) {
                    startGame = false;
                    quit = true;
                }
            }

            handleInput();
        }

        if (!gamePaused && startGame) {
            update();
        }

        render();

        SDL_Delay(MOVEMENT_DELAY);
    }

    // Cleanup and exit
    displayGameOver();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

bool showWelcomeScreen() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color textColor = {255, 255, 255, 255};
    std::string welcomeText = " GAME START?";

    // Render welcome message
    SDL_Surface* welcomeSurface = TTF_RenderText_Solid(font, welcomeText.c_str(), textColor);
    welcomeTexture = SDL_CreateTextureFromSurface(renderer, welcomeSurface);

    SDL_Rect welcomeRect = {(SCREEN_WIDTH - welcomeSurface->w) / 2, (SCREEN_HEIGHT - welcomeSurface->h) / 2,
                             welcomeSurface->w, welcomeSurface->h};
    SDL_RenderCopy(renderer, welcomeTexture, nullptr, &welcomeRect);

    SDL_FreeSurface(welcomeSurface);

    // Render buttons
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &yesButton);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &noButton);

    // Render button text
    std::string yesButtonText = "Yes";
    SDL_Surface* yesButtonSurface = TTF_RenderText_Solid(font, yesButtonText.c_str(), textColor);
    SDL_Texture* yesButtonTexture = SDL_CreateTextureFromSurface(renderer, yesButtonSurface);

    SDL_Rect yesButtonRect = {yesButton.x + (yesButton.w - yesButtonSurface->w) / 2,
                              yesButton.y + (yesButton.h - yesButtonSurface->h) / 2,
                              yesButtonSurface->w, yesButtonSurface->h};
    SDL_RenderCopy(renderer, yesButtonTexture, nullptr, &yesButtonRect);

    SDL_FreeSurface(yesButtonSurface);

std::string noButtonText = "No";
    SDL_Surface* noButtonSurface = TTF_RenderText_Solid(font, noButtonText.c_str(), textColor);
        SDL_Texture* noButtonTexture = SDL_CreateTextureFromSurface(renderer, noButtonSurface);

    SDL_Rect noButtonRect = {noButton.x + (noButton.w - noButtonSurface->w) / 2,
                             noButton.y + (noButton.h - noButtonSurface->h) / 2,
                             noButtonSurface->w, noButtonSurface->h};
    SDL_RenderCopy(renderer, noButtonTexture, nullptr, &noButtonRect);

    SDL_FreeSurface(noButtonSurface);

    SDL_RenderPresent(renderer);

    // Wait for user input
    SDL_Event e;
    while (true) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                return false;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                if (mouseX >= yesButton.x && mouseX <= yesButton.x + yesButton.w &&
                    mouseY >= yesButton.y && mouseY <= yesButton.y + yesButton.h) {
                    return true;
                } else if (mouseX >= noButton.x && mouseX <= noButton.x + noButton.w &&
                           mouseY >= noButton.y && mouseY <= noButton.y + noButton.h) {
                    return false;
                }
            }
        }
    }
}

void spawnFood() {
    int maxX = (SCREEN_WIDTH - 2 * TILE_SIZE) / TILE_SIZE;
    int maxY = (SCREEN_HEIGHT - 2 * TILE_SIZE) / TILE_SIZE;

    food.x = TILE_SIZE + rand() % maxX * TILE_SIZE;
    food.y = TILE_SIZE + rand() % maxY * TILE_SIZE;

    if ((food.x + TILE_SIZE > wallRect1.x && food.x < wallRect1.x + wallRect1.w &&
     food.y + TILE_SIZE > wallRect1.y && food.y < wallRect1.y + wallRect1.h) ||
    (food.x + TILE_SIZE > wallRect2.x && food.x < wallRect2.x + wallRect2.w &&
     food.y + TILE_SIZE > wallRect2.y && food.y < wallRect2.y + wallRect2.h) ||
    (food.x + TILE_SIZE > wallRect3.x && food.x < wallRect3.x + wallRect3.w &&
     food.y + TILE_SIZE > wallRect3.y && food.y < wallRect3.y + wallRect3.h) ||
    (food.x + TILE_SIZE > wallRect4.x && food.x < wallRect4.x + wallRect4.w &&
     food.y + TILE_SIZE > wallRect4.y && food.y < wallRect4.y + wallRect4.h)) {
    spawnFood();
    return;
}

}

void spawnBonusFood() {
    int maxX = (SCREEN_WIDTH - 2 * TILE_SIZE) / TILE_SIZE;
    int maxY = (SCREEN_HEIGHT - 2 * TILE_SIZE) / TILE_SIZE;

    bonusFood.x = TILE_SIZE + rand() % maxX * TILE_SIZE;
    bonusFood.y = TILE_SIZE + rand() % maxY * TILE_SIZE;

    if ((bonusFood.x + TILE_SIZE > wallRect1.x && bonusFood.x < wallRect1.x + wallRect1.w &&
     bonusFood.y + TILE_SIZE > wallRect1.y && bonusFood.y < wallRect1.y + wallRect1.h) ||
    (bonusFood.x + TILE_SIZE > wallRect2.x && bonusFood.x < wallRect2.x + wallRect2.w &&
     bonusFood.y + TILE_SIZE > wallRect2.y && bonusFood.y < wallRect2.y + wallRect2.h) ||
    (bonusFood.x + TILE_SIZE > wallRect3.x && bonusFood.x < wallRect3.x + wallRect3.w &&
     bonusFood.y + TILE_SIZE > wallRect3.y && bonusFood.y < wallRect3.y + wallRect3.h) ||
    (bonusFood.x + TILE_SIZE > wallRect4.x && bonusFood.x < wallRect4.x + wallRect4.w &&
     bonusFood.y + TILE_SIZE > wallRect4.y && bonusFood.y < wallRect4.y + wallRect4.h) ) {
    spawnBonusFood();
    return;
}

    bonusFoodActive = true;
    bonusFoodTimer = SDL_GetTicks();
}

void eatBonusFood() {
    bonusFoodActive = false;
    // Add any other logic you may need after eating the bonus food
}

void update() {
    SnakeSegment head = snake.front();
    switch (snakeDirection) {
        case Direction::UP:
            head.y -= TILE_SIZE;
            break;
        case Direction::DOWN:
            head.y += TILE_SIZE;
            break;
        case Direction::LEFT:
            head.x -= TILE_SIZE;
            break;
        case Direction::RIGHT:
            head.x += TILE_SIZE;
            break;
    }

    /*if (head.x < TILE_SIZE || head.x >= SCREEN_WIDTH - TILE_SIZE ||
        head.y < TILE_SIZE || head.y >= SCREEN_HEIGHT - TILE_SIZE) {
        displayGameOver();
        return;
    }*/

    // Wrap around the screen if the head hits the boundary
    if (head.x < 0) {
        head.x = SCREEN_WIDTH - TILE_SIZE;
    } else if (head.x >= SCREEN_WIDTH) {
        head.x = 0;
    }

    if (head.y < 0) {
        head.y = SCREEN_HEIGHT - TILE_SIZE;
    } else if (head.y >= SCREEN_HEIGHT) {
        head.y = 0;
    }

    if ((head.x + TILE_SIZE > wallRect1.x && head.x < wallRect1.x + wallRect1.w &&
         head.y + TILE_SIZE > wallRect1.y && head.y < wallRect1.y + wallRect1.h) ||
        (head.x + TILE_SIZE > wallRect2.x && head.x < wallRect2.x + wallRect2.w &&
         head.y + TILE_SIZE > wallRect2.y && head.y < wallRect2.y + wallRect2.h) ||
        (head.x + TILE_SIZE > wallRect3.x && head.x < wallRect3.x + wallRect3.w &&
         head.y + TILE_SIZE > wallRect3.y && head.y < wallRect3.y + wallRect3.h) ||
        (head.x + TILE_SIZE > wallRect4.x && head.x < wallRect4.x + wallRect4.w &&
         head.y + TILE_SIZE > wallRect4.y && head.y < wallRect4.y + wallRect4.h)) {
        displayGameOver();
        return;
    }

    if (head.x == food.x && head.y == food.y) {
        regularFoodEaten++;

        if (regularFoodEaten == 3) {
            regularFoodEaten = 0;
            score += 10;
            spawnFood();
            spawnBonusFood();
        } else {
            score += 10;
            spawnFood();
        }
    } else if (bonusFoodActive && head.x == bonusFood.x && head.y == bonusFood.y) {
        score += 15;
        bonusFoodActive = false;
        spawnFood();
    } else {
        snake.pop_back();
    }

    for (const auto& segment : snake) {
        if (head.x == segment.x && head.y == segment.y) {
            displayGameOver();
            return;
        }
    }

    snake.insert(snake.begin(), head);

    handleBonusFoodDuration();
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 128, 128, 0);
    SDL_Rect topWall = {0, 0, SCREEN_WIDTH, TILE_SIZE};
    SDL_Rect bottomWall = {0, SCREEN_HEIGHT - TILE_SIZE, SCREEN_WIDTH, TILE_SIZE};
    SDL_Rect leftWall = {0, 0, TILE_SIZE, SCREEN_HEIGHT};
    SDL_Rect rightWall = {SCREEN_WIDTH - TILE_SIZE, 0, TILE_SIZE, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &topWall);
    SDL_RenderFillRect(renderer, &bottomWall);
    SDL_RenderFillRect(renderer, &leftWall);
    SDL_RenderFillRect(renderer, &rightWall);

    SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
    SDL_RenderFillRect(renderer, &wallRect1);
    SDL_RenderFillRect(renderer, &wallRect2);
    SDL_RenderFillRect(renderer, &wallRect3);
    SDL_RenderFillRect(renderer, &wallRect4);
    SDL_SetRenderDrawColor(renderer, 85, 107, 47, 255);
    for (const auto& segment : snake) {
        SDL_Rect rect = {segment.x, segment.y, TILE_SIZE, TILE_SIZE};
        SDL_RenderFillRect(renderer, &rect);
    }

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect foodRect = {food.x, food.y, REGULAR_FOOD_SIZE, REGULAR_FOOD_SIZE};
    SDL_RenderFillRect(renderer, &foodRect);

    if (bonusFoodActive) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_Rect bonusFoodRect = {bonusFood.x, bonusFood.y, BONUS_FOOD_SIZE, BONUS_FOOD_SIZE};
        SDL_RenderFillRect(renderer, &bonusFoodRect);
    }

    SDL_Color textColor = {255, 255, 255, 255};
    std::string scoreText = "Score: " + std::to_string(score);

    // Render score
    SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, scoreText.c_str(), textColor);
    scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);

    SDL_Rect scoreRect = {10, 10, scoreSurface->w, scoreSurface->h};
    SDL_RenderCopy(renderer, scoreTexture, nullptr, &scoreRect);

    SDL_FreeSurface(scoreSurface);

    // Render level board (you can customize it based on your game's logic)
    std::string levelText = "Level: 1"; // Customize based on your game's logic
    SDL_Surface* levelSurface = TTF_RenderText_Solid(font, levelText.c_str(), textColor);
    levelTexture = SDL_CreateTextureFromSurface(renderer, levelSurface);

    SDL_Rect levelRect = {SCREEN_WIDTH - levelSurface->w - 10, 10, levelSurface->w, levelSurface->h};
    SDL_RenderCopy(renderer, levelTexture, nullptr, &levelRect);

    SDL_FreeSurface(levelSurface);

    SDL_RenderPresent(renderer);
}

void handleInput() {
    const Uint8* currentKeyStates = SDL_GetKeyboardState(nullptr);

    if (currentKeyStates[SDL_SCANCODE_UP] && snakeDirection != Direction::DOWN) {
        snakeDirection = Direction::UP;
    } else if (currentKeyStates[SDL_SCANCODE_DOWN] && snakeDirection != Direction::UP) {
        snakeDirection = Direction::DOWN;
    } else if (currentKeyStates[SDL_SCANCODE_LEFT] && snakeDirection != Direction::RIGHT) {
        snakeDirection = Direction::LEFT;
    } else if (currentKeyStates[SDL_SCANCODE_RIGHT] && snakeDirection != Direction::LEFT) {
        snakeDirection = Direction::RIGHT;
    }
}

void handleBonusFoodDuration() {
    if (bonusFoodActive && (SDL_GetTicks() - bonusFoodTimer >= BONUS_FOOD_DURATION)) {
        bonusFoodActive = false;
        spawnFood();
    }
}

void displayGameOver() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color textColor = {255, 255, 255, 255};
    std::string gameOverText = "Game Over!!";

    // Render "Game Over" message with score
    SDL_Surface* gameOverSurface = TTF_RenderText_Solid(font, gameOverText.c_str(), textColor);
    SDL_Texture* gameOverTexture = SDL_CreateTextureFromSurface(renderer, gameOverSurface);

    SDL_Rect gameOverRect = {(SCREEN_WIDTH - gameOverSurface->w) / 2, (SCREEN_HEIGHT - gameOverSurface->h) / 2,
                             gameOverSurface->w, gameOverSurface->h};
    SDL_RenderCopy(renderer, gameOverTexture, nullptr, &gameOverRect);

    std::string scoreText = "Score: " + std::to_string(score);

    // Render score
    SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, scoreText.c_str(), textColor);
    scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);

    SDL_Rect scoreRect = {(SCREEN_WIDTH - gameOverSurface->w)/2, (SCREEN_HEIGHT/2 + gameOverSurface->h),
                             gameOverSurface->w, gameOverSurface->h};
    SDL_RenderCopy(renderer, scoreTexture, nullptr, &scoreRect);

    SDL_FreeSurface(scoreSurface);

    SDL_FreeSurface(gameOverSurface);
    SDL_DestroyTexture(gameOverTexture);

    SDL_RenderPresent(renderer);

    // Wait for a few seconds before exiting
    SDL_Delay(3000);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    exit(0);
}