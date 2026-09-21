// main_wasm.cpp
#include <vector>
#include <string>
#include <set>
#include <map>
#include <random>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
using namespace emscripten;
#endif

const int BOARD_SIZE = 5;
enum Cell { EMPTY = 0, BLACK = 1, WHITE = 2 };

struct Point {
    int r, c;
    bool operator<(const Point& other) const {
        if (r != other.r) return r < other.r;
        return c < other.c;
    }
};

// --- C++ 囲碁コアロジック ---
class GoEngine {
private:
    std::vector<std::vector<Cell>> board;
    Cell current_turn;
    int black_captures;
    int white_captures;

    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    bool is_valid(int r, int c) const {
        return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE;
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

public:
    GoEngine() { reset(); }

    void reset() {
        board = std::vector<std::vector<Cell>>(BOARD_SIZE, std::vector<Cell>(BOARD_SIZE, EMPTY));
        current_turn = BLACK;
        black_captures = 0;
        white_captures = 0;
    }

    // JSへ盤面状態を1次元フラットな配列（std::vector<int>）で渡す
    std::vector<int> get_board_state() const {
        std::vector<int> flat_board(BOARD_SIZE * BOARD_SIZE);
        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                flat_board[r * BOARD_SIZE + c] = static_cast<int>(board[r][c]);
            }
        }
        return flat_board;
    }

    int get_current_turn() const { return static_cast<int>(current_turn); }
    int get_black_captures() const { return black_captures; }
    int get_white_captures() const { return white_captures; }

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

    // 着手処理
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
            board[r][c] = EMPTY;
            return -1; // 自殺手
        }

        current_turn = opponent;
        return captured;
    }
};

// --- 強化学習 (Q-Learning) エージェント ---
class QAgent {
private:
    double alpha = 0.2;
    double gamma = 0.95;
    double epsilon = 0.2;
    std::mt19937 rng{std::random_device{}()};
    std::map<std::string, std::vector<double>> q_table;

public:
    QAgent() {}

    int select_action(const std::string& state, const GoEngine& engine) {
        std::vector<int> valid_actions;
        std::vector<int> board = engine.get_board_state();

        for (int a = 0; a < BOARD_SIZE * BOARD_SIZE; ++a) {
            if (board[a] == 0) valid_actions.push_back(a);
        }

        if (valid_actions.empty()) return -1;

        std::uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng) < epsilon) {
            std::uniform_int_distribution<int> act_dist(0, valid_actions.size() - 1);
            return valid_actions[act_dist(rng)];
        } else {
            if (q_table.find(state) == q_table.end()) {
                q_table[state] = std::vector<double>(BOARD_SIZE * BOARD_SIZE, 0.0);
            }
            auto& q_vals = q_table[state];
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

    void train_step(GoEngine& engine) {
        std::string state = engine.get_state_string();
        int action = select_action(state, engine);

        if (action == -1) {
            engine.reset();
            return;
        }

        int r = action / BOARD_SIZE;
        int c = action % BOARD_SIZE;
        int captured = engine.play_move(r, c);

        double reward = (captured < 0) ? -10.0 : (captured * 10.0 + 0.1);
        std::string next_state = engine.get_state_string();

        if (q_table.find(state) == q_table.end()) q_table[state] = std::vector<double>(BOARD_SIZE * BOARD_SIZE, 0.0);
        if (q_table.find(next_state) == q_table.end()) q_table[next_state] = std::vector<double>(BOARD_SIZE * BOARD_SIZE, 0.0);

        double max_next_q = *std::max_element(q_table[next_state].begin(), q_table[next_state].end());
        q_table[state][action] += alpha * (reward + gamma * max_next_q - q_table[state][action]);
    }

    int get_states_count() const {
        return q_table.size();
    }
};

// --- Emscripten バインディング（JavaScriptからC++クラス・メソッドを直接使用できるように展開）---
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(go_engine_module) {
    register_vector<int>("VectorInt");

    class_<GoEngine>("GoEngine")
        .constructor<>()
        .function("reset", &GoEngine::reset)
        .function("play_move", &GoEngine::play_move)
        .function("get_board_state", &GoEngine::get_board_state)
        .function("get_current_turn", &GoEngine::get_current_turn)
        .function("get_black_captures", &GoEngine::get_black_captures)
        .function("get_white_captures", &GoEngine::get_white_captures)
        .function("get_state_string", &GoEngine::get_state_string);

    class_<QAgent>("QAgent")
        .constructor<>()
        .function("select_action", &QAgent::select_action)
        .function("train_step", &QAgent::train_step)
        .function("get_states_count", &QAgent::get_states_count);
}
#endif