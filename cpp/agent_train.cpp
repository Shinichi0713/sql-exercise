#include "raylib.h"
#include <vector>
#include <set>
#include <map>
#include <cmath>
#include <random>
#include <algorithm>
#include <string>

const int BOARD_SIZE = 5; // RLの学習を高速化するため5x5盤面
const int CELL_SIZE = 80;
const int MARGIN = 80;
const int WINDOW_WIDTH = MARGIN * 2 + CELL_SIZE * (BOARD_SIZE - 1);
const int WINDOW_HEIGHT = WINDOW_WIDTH + 100;

enum Cell { EMPTY = 0, BLACK = 1, WHITE = 2 };

struct Point {
    int r, c;
    bool operator<(const Point& other) const {
        if (r != other.r) return r < other.r;
        return c < other.c;
    }
};

class GoGame {
public:
    std::vector<std::vector<Cell>> board;
    Cell current_turn;
    int black_captures;
    int white_captures;
    bool is_game_over;

    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    GoGame() { reset(); }

    void reset() {
        board = std::vector<std::vector<Cell>>(BOARD_SIZE, std::vector<Cell>(BOARD_SIZE, EMPTY));
        current_turn = BLACK;
        black_captures = 0;
        white_captures = 0;
        is_game_over = false;
    }

    bool is_valid(int r, int c) const {
        return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE;
    }

    // 盤面を文字列に変換（Qテーブルのキーとして利用）
    std::string get_state_string() const {
        std::string s = "";
        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                s += std::to_string(board[r][c]);
            }
        }
        s += (current_turn == BLACK ? "B" : "W");
        return s;
    }

    void get_group_and_liberties(int r, int c, std::set<Point>& group, std::set<Point>& liberties) const {
        Cell color = board[r][c];
        if (color == EMPTY) return;

        std::vector<Point> stack = {{r, c}};
        group.insert({r, c});

        while (!stack.empty()) {
            Point p = stack.back();
            stack.pop_back();

            for (int i = 0; i < 4; ++i) {
                int nr = p.r + dr[i];
                int nc = p.c + dc[i];
                if (!is_valid(nr, nc)) continue;

                if (board[nr][nc] == EMPTY) {
                    liberties.insert({nr, nc});
                } else if (board[nr][nc] == color && group.find({nr, nc}) == group.end()) {
                    group.insert({nr, nc});
                    stack.push_back({nr, nc});
                }
            }
        }
    }

    // 着手処理（戻り値: 獲得したアゲハの数, 非合法手なら-1）
    int play_move(int r, int c) {
        if (!is_valid(r, c) || board[r][c] != EMPTY) return -1;

        Cell opponent = (current_turn == BLACK) ? WHITE : BLACK;
        board[r][c] = current_turn;

        int captured = 0;
        for (int i = 0; i < 4; ++i) {
            int nr = r + dr[i];
            int nc = c + dc[i];
            if (is_valid(nr, nc) && board[nr][nc] == opponent) {
                std::set<Point> group, liberties;
                get_group_and_liberties(nr, nc, group, liberties);
                if (liberties.empty()) {
                    for (const auto& p : group) {
                        board[p.r][p.c] = EMPTY;
                        captured++;
                    }
                }
            }
        }

        if (current_turn == BLACK) black_captures += captured;
        else white_captures += captured;

        std::set<Point> my_group, my_liberties;
        get_group_and_liberties(r, c, my_group, my_liberties);
        if (my_liberties.empty() && captured == 0) {
            board[r][c] = EMPTY; // 自殺手
            return -1;
        }

        current_turn = opponent;
        return captured;
    }
};

// Q学習エージェント
class QAgent {
private:
    double alpha = 0.2;   // 学習率
    double gamma = 0.95;  // 割引率
    double epsilon = 0.2; // 探索率
    std::mt19937 rng{std::random_device{}()};

public:
    // Qテーブル: StateString -> [ActionIndex -> QValue]
    std::map<std::string, std::vector<double>> q_table;

    std::vector<double>& get_q_values(const std::string& state) {
        if (q_table.find(state) == q_table.end()) {
            q_table[state] = std::vector<double>(BOARD_SIZE * BOARD_SIZE, 0.0);
        }
        return q_table[state];
    }

    int select_action(const std::string& state, const GoGame& game) {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        std::vector<int> valid_actions;

        for (int a = 0; a < BOARD_SIZE * BOARD_SIZE; ++a) {
            int r = a / BOARD_SIZE;
            int c = a % BOARD_SIZE;
            if (game.board[r][c] == EMPTY) valid_actions.push_back(a);
        }

        if (valid_actions.empty()) return -1; // パス

        if (dist(rng) < epsilon) {
            std::uniform_int_distribution<int> act_dist(0, valid_actions.size() - 1);
            return valid_actions[act_dist(rng)];
        } else {
            auto& q_vals = get_q_values(state);
            int best_action = valid_actions[0];
            double max_q = -1e9;
            for (int a : valid_actions) {
                if (q_vals[a] > max_q) {
                    max_q = q_vals[a];
                    best_action = a;
                }
            }
            return best_action;
        }
    }

    void update(const std::string& state, int action, double reward, const std::string& next_state) {
        auto& q_vals = get_q_values(state);
        auto& next_q_vals = get_q_values(next_state);
        double max_next_q = *std::max_element(next_q_vals.begin(), next_q_vals.end());

        q_vals[action] += alpha * (reward + gamma * max_next_q - q_vals[action]);
    }
};

int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "C++ Go RL (Raylib)");
    SetTargetFPS(60);

    GoGame game;
    QAgent agent;

    bool auto_train_mode = false;
    int total_episodes = 0;

    while (!WindowShouldClose()) {
        // --- 1. 自動学習（Self-Play）モードの更新処理 ---
        if (auto_train_mode) {
            for (int step = 0; step < 10; ++step) { // 描画遅延を防ぐため1フレームに10手進める
                std::string state = game.get_state_string();
                int action = agent.select_action(state, game);

                if (action == -1 || game.is_game_over) {
                    game.reset();
                    total_episodes++;
                    break;
                }

                int r = action / BOARD_SIZE;
                int c = action % BOARD_SIZE;
                int captured = game.play_move(r, c);

                double reward = 0.0;
                if (captured < 0) {
                    reward = -10.0; // 非合法手ペナルティ
                } else {
                    reward = captured * 10.0 + 0.1; // アゲハ報酬 + 生き残り報酬
                }

                std::string next_state = game.get_state_string();
                agent.update(state, action, reward, next_state);
            }
        }

        // --- 2. 人間対局時のマウス入力処理 ---
        if (!auto_train_mode && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse_pos = GetMousePosition();
            int c = std::round((mouse_pos.x - MARGIN) / CELL_SIZE);
            int r = std::round((mouse_pos.y - MARGIN) / CELL_SIZE);

            if (game.is_valid(r, c)) {
                // 黒（人間）の手
                std::string state = game.get_state_string();
                int action = r * BOARD_SIZE + c;
                int captured = game.play_move(r, c);

                if (captured >= 0) {
                    // 白（AI）の手
                    std::string ai_state = game.get_state_string();
                    int ai_action = agent.select_action(ai_state, game);
                    if (ai_action != -1) {
                        int ar = ai_action / BOARD_SIZE;
                        int ac = ai_action % BOARD_SIZE;
                        game.play_move(ar, ac);
                    }
                }
            }
        }

        // --- 3. キー操作による切り替え ---
        if (IsKeyPressed(KEY_SPACE)) {
            auto_train_mode = !auto_train_mode; // 自動学習のON/OFF切り替え
        }
        if (IsKeyPressed(KEY_R)) {
            game.reset();
        }

        // --- 4. 描画処理 ---
        BeginDrawing();
        ClearBackground((Color){ 220, 179, 92, 255 });

        // 格子線
        for (int i = 0; i < BOARD_SIZE; ++i) {
            int pos = MARGIN + i * CELL_SIZE;
            DrawLine(MARGIN, pos, MARGIN + (BOARD_SIZE - 1) * CELL_SIZE, pos, BLACK);
            DrawLine(pos, MARGIN, pos, MARGIN + (BOARD_SIZE - 1) * CELL_SIZE, BLACK);
        }

        // 石の描画
        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                int x = MARGIN + c * CELL_SIZE;
                int y = MARGIN + r * CELL_SIZE;
                if (game.board[r][c] == BLACK) {
                    DrawCircle(x, y, CELL_SIZE / 2 - 4, BLACK);
                } else if (game.board[r][c] == WHITE) {
                    DrawCircle(x, y, CELL_SIZE / 2 - 4, WHITE);
                    DrawCircleLines(x, y, CELL_SIZE / 2 - 4, BLACK);
                }
            }
        }

        // UI表示
        int ui_y = WINDOW_HEIGHT - 80;
        DrawText(TextFormat("モード: %s (SPACEで切替)", auto_train_mode ? "AI高速自己学習中" : "VS AI対局モード"), MARGIN - 20, ui_y, 18, DARKBLUE);
        DrawText(TextFormat("学習エピソード数: %d | 獲得Qステート数: %d", total_episodes, (int)agent.q_table.size()), MARGIN - 20, ui_y + 25, 16, DARKGRAY);
        DrawText("Rキー: 盤面リセット", MARGIN - 20, ui_y + 50, 16, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}