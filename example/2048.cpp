#include "screen.h"
#include "graphics.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

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

// 数字位图 (3x5)
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

// 字母位图 (5x5) - 包含空格
const int letterPatterns[27][25] = {
    // A
    {0,1,1,1,0,
     1,0,0,0,1,
     1,1,1,1,1,
     1,0,0,0,1,
     1,0,0,0,1},
     // B
     {1,1,1,1,0,
      1,0,0,0,1,
      1,1,1,1,0,
      1,0,0,0,1,
      1,1,1,1,0},
      // C
      {0,1,1,1,1,
       1,0,0,0,0,
       1,0,0,0,0,
       1,0,0,0,0,
       0,1,1,1,1},
       // D
       {1,1,1,1,0,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,1,1,1,0},
        // E
        {1,1,1,1,1,
         1,0,0,0,0,
         1,1,1,1,0,
         1,0,0,0,0,
         1,1,1,1,1},
         // F
         {1,1,1,1,1,
          1,0,0,0,0,
          1,1,1,0,0,
          1,0,0,0,0,
          1,0,0,0,0},
          // G
          {0,1,1,1,0,
           1,0,0,0,0,
           1,0,1,1,1,
           1,0,0,0,1,
           0,1,1,1,0},
           // H
           {1,0,0,0,1,
            1,0,0,0,1,
            1,1,1,1,1,
            1,0,0,0,1,
            1,0,0,0,1},
            // I
            {1,1,1,1,1,
             0,0,1,0,0,
             0,0,1,0,0,
             0,0,1,0,0,
             1,1,1,1,1},
             // J
             {1,1,1,1,1,
              0,0,0,1,0,
              0,0,0,1,0,
              1,0,0,1,0,
              0,1,1,0,0},
              // K
              {1,0,0,0,1,
               1,0,0,1,0,
               1,1,1,0,0,
               1,0,0,1,0,
               1,0,0,0,1},
               // L
               {1,0,0,0,0,
                1,0,0,0,0,
                1,0,0,0,0,
                1,0,0,0,0,
                1,1,1,1,1},
                // M
                {1,0,0,0,1,
                 1,1,0,1,1,
                 1,0,1,0,1,
                 1,0,0,0,1,
                 1,0,0,0,1},
                 // N
                 {1,0,0,0,1,
                  1,1,0,0,1,
                  1,0,1,0,1,
                  1,0,0,1,1,
                  1,0,0,0,1},
                  // O
                  {0,1,1,1,0,
                   1,0,0,0,1,
                   1,0,0,0,1,
                   1,0,0,0,1,
                   0,1,1,1,0},
                   // P
                   {1,1,1,1,0,
                    1,0,0,0,1,
                    1,1,1,1,0,
                    1,0,0,0,0,
                    1,0,0,0,0},
                    // Q
                    {0,1,1,1,0,
                     1,0,0,0,1,
                     1,0,0,0,1,
                     1,0,0,1,0,
                     0,1,1,0,1},
                     // R
                     {1,1,1,1,0,
                      1,0,0,0,1,
                      1,1,1,1,0,
                      1,0,0,1,0,
                      1,0,0,0,1},
                      // S
                      {0,1,1,1,1,
                       1,0,0,0,0,
                       0,1,1,1,0,
                       0,0,0,0,1,
                       1,1,1,1,0},
                       // T
                       {1,1,1,1,1,
                        0,0,1,0,0,
                        0,0,1,0,0,
                        0,0,1,0,0,
                        0,0,1,0,0},
                        // U
                        {1,0,0,0,1,
                         1,0,0,0,1,
                         1,0,0,0,1,
                         1,0,0,0,1,
                         0,1,1,1,0},
                         // V
                         {1,0,0,0,1,
                          1,0,0,0,1,
                          1,0,0,0,1,
                          0,1,0,1,0,
                          0,0,1,0,0},
                          // W
                          {1,0,0,0,1,
                           1,0,0,0,1,
                           1,0,1,0,1,
                           1,0,1,0,1,
                           0,1,0,1,0},
                           // X
                           {1,0,0,0,1,
                            0,1,0,1,0,
                            0,0,1,0,0,
                            0,1,0,1,0,
                            1,0,0,0,1},
                            // Y
                            {1,0,0,0,1,
                             0,1,0,1,0,
                             0,0,1,0,0,
                             0,0,1,0,0,
                             0,0,1,0,0},
                             // Z
                             {1,1,1,1,1,
                              0,0,0,1,0,
                              0,0,1,0,0,
                              0,1,0,0,0,
                              1,1,1,1,1},
                              // 空格
                              {0,0,0,0,0,
                               0,0,0,0,0,
                               0,0,0,0,0,
                               0,0,0,0,0,
                               0,0,0,0,0}
};

Game2048::Game2048() : score(0), moved(false) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    init();
}

void Game2048::init() {
    // 清空棋盘
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            board[i][j] = 0;
        }
    }

    score = 0;
    moved = false;

    // 添加两个初始方块
    addRandomTile();
    addRandomTile();
}

void Game2048::reset() {
    init();
}

void Game2048::addRandomTile() {
    std::vector<std::pair<int, int>> emptyCells;

    // 收集所有空位置
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            if (board[i][j] == 0) {
                emptyCells.emplace_back(i, j);
            }
        }
    }

    if (emptyCells.empty()) return;

    // 随机选择一个空位置
    int index = std::rand() % emptyCells.size();
    int row = emptyCells[index].first;
    int col = emptyCells[index].second;

    // 90%概率生成2，10%概率生成4
    board[row][col] = (std::rand() % 10 == 0) ? 4 : 2;
}

bool Game2048::mergeTiles(std::vector<int>& line) {
    bool merged = false;

    // 保存原始行以检查是否发生变化
    std::vector<int> originalLine = line;

    // 移除0
    line.erase(std::remove(line.begin(), line.end(), 0), line.end());

    // 合并相邻的相同数字
    for (size_t i = 0; i + 1 < line.size(); ++i) {
        if (line[i] == line[i + 1]) {
            line[i] *= 2;
            score += line[i];
            line.erase(line.begin() + i + 1);
            merged = true;
        }
    }

    // 填充0
    while (line.size() < static_cast<size_t>(SIZE)) {
        line.push_back(0);
    }

    // 检查是否发生变化
    bool changed = false;
    for (int i = 0; i < SIZE; ++i) {
        if (originalLine[i] != line[i]) {
            changed = true;
            break;
        }
    }

    return changed;
}

void Game2048::moveLeft() {
    moved = false;

    for (int i = 0; i < SIZE; ++i) {
        std::vector<int> line;
        for (int j = 0; j < SIZE; ++j) {
            line.push_back(board[i][j]);
        }

        bool lineMoved = mergeTiles(line);
        moved = moved || lineMoved;

        for (int j = 0; j < SIZE; ++j) {
            board[i][j] = line[j];
        }
    }
}

void Game2048::moveRight() {
    moved = false;

    for (int i = 0; i < SIZE; ++i) {
        std::vector<int> line;
        for (int j = SIZE - 1; j >= 0; --j) {
            line.push_back(board[i][j]);
        }

        bool lineMoved = mergeTiles(line);
        moved = moved || lineMoved;

        for (int j = 0; j < SIZE; ++j) {
            board[i][j] = line[SIZE - 1 - j];
        }
    }
}

void Game2048::moveUp() {
    moved = false;

    for (int j = 0; j < SIZE; ++j) {
        std::vector<int> line;
        for (int i = 0; i < SIZE; ++i) {
            line.push_back(board[i][j]);
        }

        bool lineMoved = mergeTiles(line);
        moved = moved || lineMoved;

        for (int i = 0; i < SIZE; ++i) {
            board[i][j] = line[i];
        }
    }
}

void Game2048::moveDown() {
    moved = false;

    for (int j = 0; j < SIZE; ++j) {
        std::vector<int> line;
        for (int i = SIZE - 1; i >= 0; --i) {
            line.push_back(board[i][j]);
        }

        bool lineMoved = mergeTiles(line);
        moved = moved || lineMoved;

        for (int i = 0; i < SIZE; ++i) {
            board[i][j] = line[SIZE - 1 - i];
        }
    }
}

void Game2048::handleInput(char key) {
    switch (key) {
    case 'a':
    case 'A':
        moveLeft();
        break;
    case 'd':
    case 'D':
        moveRight();
        break;
    case 'w':
    case 'W':
        moveUp();
        break;
    case 's':
    case 'S':
        moveDown();
        break;
    default:
        return;
    }

    if (moved) {
        addRandomTile();
    }
}

bool Game2048::canMove() const {
    // 检查是否有空位
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            if (board[i][j] == 0) {
                return true;
            }
        }
    }

    // 检查是否有可以合并的相邻方块
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            if (j + 1 < SIZE && board[i][j] == board[i][j + 1]) {
                return true;
            }
            if (i + 1 < SIZE && board[i][j] == board[i + 1][j]) {
                return true;
            }
        }
    }

    return false;
}

bool Game2048::isGameOver() const {
    return !canMove();
}

int Game2048::getScore() const {
    return score;
}

Vec3 Game2048::getTileColor(int value) {
    switch (value) {
    case 0:    return Vec3(205, 193, 180); // 空方块
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
    default:   return Vec3(60, 58, 50); // 大于2048的方块
    }
}

void Game2048::drawTile(Screen& screen, int x, int y, int value) {
    const int tileSize = 16; // 方块大小（像素）
    const int padding = 2;   // 方块间距

    // 绘制方块背景
    Vec3 color = getTileColor(value);
    Vec2 topLeft(x, y);
    Vec2 topRight(x + tileSize - 1, y);
    Vec2 bottomLeft(x, y + tileSize - 1);
    Vec2 bottomRight(x + tileSize - 1, y + tileSize - 1);

    // 用两个三角形绘制矩形
    Triangle::scan_draw(screen, topLeft, topRight, bottomRight, color);
    Triangle::scan_draw(screen, topLeft, bottomRight, bottomLeft, color);

    // 如果有数字，绘制数字
    if (value > 0) {
        drawNumber(screen, x, y, value, tileSize);
    }
}

void Game2048::drawNumber(Screen& screen, int x, int y, int number, int tileSize) {
    std::string numStr = std::to_string(number);

    // 计算数字宽度（每个数字3像素宽，1像素间距）
    int charWidth = 3;
    int spacing = 1;
    int totalWidth = static_cast<int>(numStr.length()) * (charWidth + spacing) - spacing;

    // 计算起始位置使数字居中
    int startX = x + (tileSize - totalWidth) / 2;
    int startY = y + (tileSize - 5) / 2;

    // 选择数字颜色（深色用于浅色方块，浅色用于深色方块）
    //Vec3 textColor = (number <= 4) ? Vec3(119, 110, 101) : Vec3(249, 246, 242);
    Vec3 textColor = Vec3(255, 255, 255);
    for (size_t digitIdx = 0; digitIdx < numStr.length(); ++digitIdx) {
        int digit = numStr[digitIdx] - '0';
        if (digit < 0 || digit > 9) continue;

        int digitX = startX + digitIdx * (charWidth + spacing);

        for (int dy = 0; dy < 5; ++dy) {
            for (int dx = 0; dx < 3; ++dx) {
                if (digitPatterns[digit][dy * 3 + dx]) {
                    screen.set_pixel(digitX + dx, startY + dy, textColor, 0.9f);
                }
            }
        }
    }
}

void Game2048::drawText(Screen& screen, int x, int y, const std::string& text) {
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        int charX;
        if (i < 6)
            charX = x + static_cast<int>(i) * 6; // 每个字符5像素宽 + 1像素间距
        else
            charX = x + static_cast<int>(i-5) * 4 + 30;

        if (c >= 'A' && c <= 'Z') {
            int letterIndex = c - 'A';
            for (int dy = 0; dy < 5; ++dy) {
                for (int dx = 0; dx < 5; ++dx) {
                    if (letterPatterns[letterIndex][dy * 5 + dx]) {
                        screen.set_pixel(charX + dx, y + dy, Vec3(255, 255, 255), 1.0f);
                    }
                }
            }
        }
        else if (c == ':') {
            screen.set_pixel(charX + 2, y + 1, Vec3(255, 255, 255), 1.0f);
            screen.set_pixel(charX + 2, y + 3, Vec3(255, 255, 255), 1.0f);
        }
        else if (c >= '0' && c <= '9') {
            int digit = c - '0';
            for (int dy = 0; dy < 5; ++dy) {
                for (int dx = 0; dx < 3; ++dx) {
                    if (digitPatterns[digit][dy * 3 + dx]) {
                        screen.set_pixel(charX + dx, y + dy, Vec3(255, 255, 255), 1.0f);
                    }
                }
            }
        }
    }
}

void Game2048::draw(Screen& screen) {
    const int tileSize = 16;
    const int padding = 2;
    const int boardSize = SIZE * (tileSize + padding) - padding;

    // 计算棋盘在屏幕中的居中位置
    int startX = (screen.width - boardSize) / 2;
    int startY = (screen.height - boardSize) / 2 + 20; // 向上偏移20像素给分数留空间

    // 绘制背景
    Vec3 bgColor(187, 173, 160);
    for (int y = 0; y < screen.height; ++y) {
        for (int x = 0; x < screen.width; ++x) {
            screen.set_pixel(x, y, bgColor, 0.0f);
        }
    }

    // 绘制所有方块
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            int tileX = startX + j * (tileSize + padding);
            int tileY = startY + i * (tileSize + padding);
            drawTile(screen, tileX, tileY, board[i][j]);
        }
    }

    // 绘制分数（棋盘上方居中）
    std::string scoreText = "SCORE: " + std::to_string(score);
    int textWidth = static_cast<int>(scoreText.length()) * 4 + 9; // 每个字符5像素+1间距
    int scoreX = startX + (boardSize - textWidth) / 2;
    int scoreY = startY - 20;

    drawText(screen, scoreX, scoreY, scoreText);

    // 如果游戏结束，显示游戏结束信息（最后绘制，在棋盘上方）
    if (isGameOver()) {
        std::string gameOverText = "GAME OVER";
        int gameOverWidth = static_cast<int>(gameOverText.length()) * 6 - 1;
        int gameOverX = startX + (boardSize - gameOverWidth) / 2;
        int gameOverY = startY - 40; // 在分数上方

       
        // 绘制白色文字
        for (size_t i = 0; i < gameOverText.length(); ++i) {
            char c = gameOverText[i];
            int charX = gameOverX + static_cast<int>(i) * 6;

            if (c >= 'A' && c <= 'Z') {
                int letterIndex = c - 'A';
                for (int dy = 0; dy < 5; ++dy) {
                    for (int dx = 0; dx < 5; ++dx) {
                        if (letterPatterns[letterIndex][dy * 5 + dx]) {
                            screen.set_pixel(charX + dx, gameOverY + dy, Vec3(255, 255, 255), 1.0f);
                        }
                    }
                }
            }
        }
    }
}

#include <conio.h>
#include <windows.h>
#include <iostream>

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

        // 绘制游戏
        game.draw(screen);

        screen.draw();
        screen.show();

        // 处理输入
        if (_kbhit()) {
            char ch = _getch();

            switch (ch) {
            case 'w':
            case 'W':
            case 'a':
            case 'A':
            case 's':
            case 'S':
            case 'd':
            case 'D':
                game.handleInput(ch);
                break;

            case 'r':
            case 'R':
                game.reset();
                break;

            case 'q':
            case 'Q':
                running = false;
                break;

            default:
                break;
            }
        }

        // 控制帧率
        Sleep(50);
    }

    std::cout << "\nFinal Score: " << game.getScore() << std::endl;
    return 0;
}