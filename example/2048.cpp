#define RENDER_ENGINE_IMPLEMENTATION
#include "render_engine.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iostream>

class Game2048 {
public:
    Game2048();
    void init();
    void draw(Screen& screen);
    void handleInput(char key);
    bool isGameOver() const;
    int getScore() const;
    void reset();

private:
    static const int SIZE = 4;
    int board[SIZE][SIZE];
    int score;
    bool moved;

    void addRandomTile();
    bool canMove() const;
    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();
    bool mergeTiles(std::vector<int>& line);
    void drawTile(Screen& screen, int x, int y, int value);
    void drawNumber(Screen& screen, int x, int y, int number, int tileSize);
    void drawText(Screen& screen, int x, int y, const std::string& text);
    Vec3 getTileColor(int value);
};

// rest of 2048 game implementation unchanged...
const int digitPatterns[10][15] = {
    {1,1,1,1,0,1,1,0,1,1,0,1,1,1,1}, // 0
    {0,1,0,0,1,0,0,1,0,0,1,0,0,1,0}, // 1
    {1,1,1,0,0,1,1,1,1,1,0,0,1,1,1}, // 2
    {1,1,1,0,0,1,1,1,1,0,0,1,1,1,1}, // 3
    {1,0,1,1,0,1,1,1,1,0,0,1,0,0,1}, // 4
    {1,1,1,1,0,0,1,1,1,0,0,1,1,1,1}, // 5
    {1,1,1,1,0,0,1,1,1,1,0,1,1,1,1}, // 6
    {1,1,1,0,0,1,0,0,1,0,0,1,0,0,1}, // 7
    {1,1,1,1,0,1,1,1,1,1,0,1,1,1,1}, // 8
    {1,1,1,1,0,1,1,1,1,0,0,1,1,1,1}  // 9
};

const int letterPatterns[27][25] = {
    {0,1,1,1,0,1,0,0,0,1,1,1,1,1,1,1,0,0,0,1,1,0,0,0,1}, // A
    {1,1,1,1,0,1,0,0,0,1,1,1,1,1,0,1,0,0,0,1,1,1,1,1,0}, // B
    {0,1,1,1,1,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,1,1,1}, // C
    {1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0}, // D
    {1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,1,1,1,1,1}, // E
    {1,1,1,1,1,1,0,0,0,0,1,1,1,0,0,1,0,0,0,0,1,0,0,0,0}, // F
    {0,1,1,1,0,1,0,0,0,0,1,0,1,1,1,1,0,0,0,1,0,1,1,1,0}, // G
    {1,0,0,0,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,1,1,0,0,0,1}, // H
    {1,1,1,1,1,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,1,1,1,1,1}, // I
    {1,1,1,1,1,0,0,0,1,0,0,0,0,1,0,1,0,0,1,0,0,1,1,0,0}, // J
    {1,0,0,0,1,1,0,0,1,0,1,1,1,0,0,1,0,0,1,0,1,0,0,0,1}, // K
    {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,1,1,1,1}, // L
    {1,0,0,0,1,1,1,0,1,1,1,0,1,0,1,1,0,0,0,1,1,0,0,0,1}, // M
    {1,0,0,0,1,1,1,0,0,1,1,0,1,0,1,1,0,0,1,1,1,0,0,0,1}, // N
    {0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0}, // O
    {1,1,1,1,0,1,0,0,0,1,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0}, // P
    {0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,1,0,0,1,1,0,1}, // Q
    {1,1,1,1,0,1,0,0,0,1,1,1,1,1,0,1,0,0,1,0,1,0,0,0,1}, // R
    {0,1,1,1,1,1,0,0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,1,1,0}, // S
    {1,1,1,1,1,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0}, // T
    {1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0}, // U
    {1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0}, // V
    {1,0,0,0,1,1,0,0,0,1,1,0,1,0,1,1,0,1,0,1,0,1,0,1,0}, // W
    {1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,1,0,1,0,1,0,0,0,1}, // X
    {1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0}, // Y
    {1,1,1,1,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,1,1,1,1}, // Z
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}  // space
};

Game2048::Game2048() : score(0), moved(false) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    init();
}

void Game2048::init() {
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            board[i][j] = 0;
    score = 0;
    moved = false;
    addRandomTile();
    addRandomTile();
}

void Game2048::reset() { init(); }

void Game2048::addRandomTile() {
    std::vector<std::pair<int, int>> emptyCells;
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            if (board[i][j] == 0)
                emptyCells.emplace_back(i, j);
    if (emptyCells.empty()) return;
    int index = std::rand() % emptyCells.size();
    int row = emptyCells[index].first;
    int col = emptyCells[index].second;
    board[row][col] = (std::rand() % 10 == 0) ? 4 : 2;
}

bool Game2048::mergeTiles(std::vector<int>& line) {
    bool merged = false;
    std::vector<int> originalLine = line;
    line.erase(std::remove(line.begin(), line.end(), 0), line.end());
    for (size_t i = 0; i + 1 < line.size(); ++i) {
        if (line[i] == line[i + 1]) {
            line[i] *= 2;
            score += line[i];
            line.erase(line.begin() + i + 1);
            merged = true;
        }
    }
    while (line.size() < static_cast<size_t>(SIZE))
        line.push_back(0);
    bool changed = false;
    for (int i = 0; i < SIZE; ++i)
        if (originalLine[i] != line[i]) { changed = true; break; }
    return changed;
}

void Game2048::moveLeft() {
    moved = false;
    for (int i = 0; i < SIZE; ++i) {
        std::vector<int> line;
        for (int j = 0; j < SIZE; ++j) line.push_back(board[i][j]);
        bool lineMoved = mergeTiles(line);
        moved = moved || lineMoved;
        for (int j = 0; j < SIZE; ++j) board[i][j] = line[j];
    }
}

void Game2048::moveRight() {
    moved = false;
    for (int i = 0; i < SIZE; ++i) {
        std::vector<int> line;
        for (int j = SIZE - 1; j >= 0; --j) line.push_back(board[i][j]);
        bool lineMoved = mergeTiles(line);
        moved = moved || lineMoved;
        for (int j = 0; j < SIZE; ++j) board[i][j] = line[SIZE - 1 - j];
    }
}

void Game2048::moveUp() {
    moved = false;
    for (int j = 0; j < SIZE; ++j) {
        std::vector<int> line;
        for (int i = 0; i < SIZE; ++i) line.push_back(board[i][j]);
        bool lineMoved = mergeTiles(line);
        moved = moved || lineMoved;
        for (int i = 0; i < SIZE; ++i) board[i][j] = line[i];
    }
}

void Game2048::moveDown() {
    moved = false;
    for (int j = 0; j < SIZE; ++j) {
        std::vector<int> line;
        for (int i = SIZE - 1; i >= 0; --i) line.push_back(board[i][j]);
        bool lineMoved = mergeTiles(line);
        moved = moved || lineMoved;
        for (int i = 0; i < SIZE; ++i) board[i][j] = line[SIZE - 1 - i];
    }
}

void Game2048::handleInput(char key) {
    switch (key) {
        case 'a': case 'A': moveLeft(); break;
        case 'd': case 'D': moveRight(); break;
        case 'w': case 'W': moveUp(); break;
        case 's': case 'S': moveDown(); break;
        default: return;
    }
    if (moved) addRandomTile();
}

bool Game2048::canMove() const {
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            if (board[i][j] == 0) return true;
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j) {
            if (j + 1 < SIZE && board[i][j] == board[i][j + 1]) return true;
            if (i + 1 < SIZE && board[i][j] == board[i + 1][j]) return true;
        }
    return false;
}

bool Game2048::isGameOver() const { return !canMove(); }
int Game2048::getScore() const { return score; }

Vec3 Game2048::getTileColor(int value) {
    switch (value) {
        case 0:    return Vec3(205, 193, 180);
        case 2:    return Vec3(238, 228, 218);
        case 4:    return Vec3(237, 224, 200);
        case 8:    return Vec3(242, 177, 121);
        case 16:   return Vec3(245, 149, 99);
        case 32:   return Vec3(246, 124, 95);
        case 64:   return Vec3(246, 94, 59);
        case 128:  return Vec3(237, 207, 114);
        case 256:  return Vec3(237, 204, 97);
        case 512:  return Vec3(237, 200, 80);
        case 1024: return Vec3(237, 197, 63);
        case 2048: return Vec3(237, 194, 46);
        default:   return Vec3(60, 58, 50);
    }
}

void Game2048::drawTile(Screen& screen, int x, int y, int value) {
    const int tileSize = 16;
    Vec3 color = getTileColor(value);
    Vec2 topLeft(x, y);
    Vec2 topRight(x + tileSize - 1, y);
    Vec2 bottomLeft(x, y + tileSize - 1);
    Vec2 bottomRight(x + tileSize - 1, y + tileSize - 1);
    Triangle::scan_draw(screen, topLeft, topRight, bottomRight, color);
    Triangle::scan_draw(screen, topLeft, bottomRight, bottomLeft, color);
    if (value > 0) drawNumber(screen, x, y, value, tileSize);
}

void Game2048::drawNumber(Screen& screen, int x, int y, int number, int tileSize) {
    std::string numStr = std::to_string(number);
    int charWidth = 3;
    int spacing = 1;
    int totalWidth = static_cast<int>(numStr.length()) * (charWidth + spacing) - spacing;
    int startX = x + (tileSize - totalWidth) / 2;
    int startY = y + (tileSize - 5) / 2;
    Vec3 textColor = Vec3(255, 255, 255);
    for (size_t digitIdx = 0; digitIdx < numStr.length(); ++digitIdx) {
        int digit = numStr[digitIdx] - '0';
        if (digit < 0 || digit > 9) continue;
        int digitX = startX + digitIdx * (charWidth + spacing);
        for (int dy = 0; dy < 5; ++dy)
            for (int dx = 0; dx < 3; ++dx)
                if (digitPatterns[digit][dy * 3 + dx])
                    screen.set_pixel(digitX + dx, startY + dy, textColor, 0.9f);
    }
}

void Game2048::drawText(Screen& screen, int x, int y, const std::string& text) {
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        int charX;
        if (i < 6) charX = x + static_cast<int>(i) * 6;
        else charX = x + static_cast<int>(i-5) * 4 + 30;
        if (c >= 'A' && c <= 'Z') {
            int letterIndex = c - 'A';
            for (int dy = 0; dy < 5; ++dy)
                for (int dx = 0; dx < 5; ++dx)
                    if (letterPatterns[letterIndex][dy * 5 + dx])
                        screen.set_pixel(charX + dx, y + dy, Vec3(255, 255, 255), 1.0f);
        } else if (c == ':') {
            screen.set_pixel(charX + 2, y + 1, Vec3(255, 255, 255), 1.0f);
            screen.set_pixel(charX + 2, y + 3, Vec3(255, 255, 255), 1.0f);
        } else if (c >= '0' && c <= '9') {
            int digit = c - '0';
            for (int dy = 0; dy < 5; ++dy)
                for (int dx = 0; dx < 3; ++dx)
                    if (digitPatterns[digit][dy * 3 + dx])
                        screen.set_pixel(charX + dx, y + dy, Vec3(255, 255, 255), 1.0f);
        }
    }
}

void Game2048::draw(Screen& screen) {
    const int tileSize = 16;
    const int padding = 2;
    const int boardSize = SIZE * (tileSize + padding) - padding;
    int startX = (screen.width - boardSize) / 2;
    int startY = (screen.height - boardSize) / 2 + 20;

    Vec3 bgColor(187, 173, 160);
    for (int y = 0; y < screen.height; ++y)
        for (int x = 0; x < screen.width; ++x)
            screen.set_pixel(x, y, bgColor, 0.0f);

    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j) {
            int tileX = startX + j * (tileSize + padding);
            int tileY = startY + i * (tileSize + padding);
            drawTile(screen, tileX, tileY, board[i][j]);
        }

    std::string scoreText = "SCORE: " + std::to_string(score);
    int textWidth = static_cast<int>(scoreText.length()) * 4 + 9;
    int scoreX = startX + (boardSize - textWidth) / 2;
    int scoreY = startY - 20;
    drawText(screen, scoreX, scoreY, scoreText);

    if (isGameOver()) {
        std::string gameOverText = "GAME OVER";
        int gameOverWidth = static_cast<int>(gameOverText.length()) * 6 - 1;
        int gameOverX = startX + (boardSize - gameOverWidth) / 2;
        int gameOverY = startY - 40;
        for (size_t i = 0; i < gameOverText.length(); ++i) {
            char c = gameOverText[i];
            int charX = gameOverX + static_cast<int>(i) * 6;
            if (c >= 'A' && c <= 'Z') {
                int letterIndex = c - 'A';
                for (int dy = 0; dy < 5; ++dy)
                    for (int dx = 0; dx < 5; ++dx)
                        if (letterPatterns[letterIndex][dy * 5 + dx])
                            screen.set_pixel(charX + dx, gameOverY + dy, Vec3(255, 255, 255), 1.0f);
            }
        }
    }
}

int main() {
    Screen screen;
    screen.init();

    Game2048 game;

    std::cout << "2048 Game\n";
    std::cout << "Controls: W/A/S/D to move\n";
    std::cout << "Press R to restart, Q to quit\n\n";

    bool running = true;

    while (running) {
        screen.update();
        screen.clear();

        game.draw(screen);

        screen.draw();
        screen.show();

        if (platform::kbhit()) {
            char ch = platform::getch();
            switch (ch) {
                case 'w': case 'W': case 'a': case 'A':
                case 's': case 'S': case 'd': case 'D':
                    game.handleInput(ch);
                    break;
                case 'r': case 'R': game.reset(); break;
                case 'q': case 'Q': running = false; break;
                default: break;
            }
        }

        platform::sleep_ms(50);
    }

    std::cout << "\nFinal Score: " << game.getScore() << std::endl;
    return 0;
}
