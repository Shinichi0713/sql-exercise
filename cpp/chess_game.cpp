#include <iostream>
#include <vector>
#include <string>
#include <set>

const int BOARD_SIZE = 19;

// 盤面状態の定義
enum Cell {
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2
};

// 盤面上の位置を表す構造体
struct Point {
    int r, c;
    bool operator<(const Point& other) const {
        if (r != other.r) return r < other.r;
        return c < other.c;
    }
};

class GoBoard {
private:
    std::vector<std::vector<Cell>> board;
    Cell current_turn;
    int black_captures;
    int white_captures;

    // 上下左右の移動オフセット
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    bool is_valid_point(int r, int c) const {
        return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE;
    }

    // 連（繋がっている同じ色の石のグループ）とその呼吸点（空いている隣接マス）を取得
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

                if (!is_valid_point(nr, nc)) continue;

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
    GoBoard() : board(BOARD_SIZE, std::vector<Cell>(BOARD_SIZE, EMPTY)),
                current_turn(BLACK), black_captures(0), white_captures(0) {}

    // 盤面の描画
    void display() const {
        std::cout << "\n   ";
        for (int c = 0; c < BOARD_SIZE; ++c) {
            std::cout << (char)('A' + (c >= 8 ? c + 1 : c)) << " "; // 'I'をスキップする一般的な碁盤表記
        }
        std::cout << "\n";

        for (int r = 0; r < BOARD_SIZE; ++r) {
            if (BOARD_SIZE - r < 10) std::cout << " ";
            std::cout << BOARD_SIZE - r << " ";
            for (int c = 0; c < BOARD_SIZE; ++c) {
                if (board[r][c] == BLACK) std::cout << "● ";
                else if (board[r][c] == WHITE) std::cout << "◯ ";
                else std::cout << ". ";
            }
            std::cout << BOARD_SIZE - r << "\n";
        }

        std::cout << "   ";
        for (int c = 0; c < BOARD_SIZE; ++c) {
            std::cout << (char)('A' + (c >= 8 ? c + 1 : c)) << " ";
        }
        std::cout << "\n";
        std::cout << "アゲハ - 黒: " << black_captures << " | 白: " << white_captures << "\n";
        std::cout << "手番: " << (current_turn == BLACK ? "黒 (●)" : "白 (◯)") << "\n";
    }

    // 着手処理
    bool play_move(int r, int c) {
        if (!is_valid_point(r, c) || board[r][c] != EMPTY) {
            std::cout << "エラー: そこには打てません。\n";
            return false;
        }

        Cell opponent = (current_turn == BLACK) ? WHITE : BLACK;
        board[r][c] = current_turn;

        // 1. 隣接する相手の石のグループをチェックし、呼吸点が0になったものを除去（アゲハ獲得）
        int captured_count = 0;
        for (int i = 0; i < 4; ++i) {
            int nr = r + dr[i];
            int nc = c + dc[i];
            if (is_valid_point(nr, nc) && board[nr][nc] == opponent) {
                std::set<Point> group, liberties;
                get_group_and_liberties(nr, nc, group, liberties);
                if (liberties.empty()) {
                    for (const auto& p : group) {
                        board[p.r][p.c] = EMPTY;
                        captured_count++;
                    }
                }
            }
        }

        // アゲハの加算
        if (current_turn == BLACK) black_captures += captured_count;
        else white_captures += captured_count;

        // 2. 自分の石のグループの呼吸点チェック（自殺手の判定）
        std::set<Point> my_group, my_liberties;
        get_group_and_liberties(r, c, my_group, my_liberties);
        if (my_liberties.empty() && captured_count == 0) {
            // 相手の石を取れず、自分の呼吸点もない場合は着手不可（元に戻す）
            board[r][c] = EMPTY;
            std::cout << "エラー: 自殺手（呼吸点がない場所）です。\n";
            return false;
        }

        // 手番の交代
        current_turn = opponent;
        return true;
    }

    void pass() {
        std::cout << (current_turn == BLACK ? "黒" : "白") << "がパスしました。\n";
        current_turn = (current_turn == BLACK) ? WHITE : BLACK;
    }
};

int main() {
    GoBoard game;
    std::string input;

    std::cout << "=== C++ 囲碁ゲーム ===\n";
    std::cout << "入力例: K10 (列の英字 + 行の数字), pass (パス), quit (終了)\n";

    while (true) {
        game.display();
        std::cout << "\n着手を入力してください: ";
        std::cin >> input;

        if (input == "quit") break;
        if (input == "pass") {
            game.pass();
            continue;
        }

        if (input.length() < 2) {
            std::cout << "無効な入力です。\n";
            continue;
        }

        // 座標文字列のパース (例: "K10" -> c=9, r=9)
        char col_char = toupper(input[0]);
        int c = col_char - 'A';
        if (col_char > 'I') c--; // 'I'をスキップする一般的なNotation対応

        int row_num = 0;
        try {
            row_num = std::stoi(input.substr(1));
        } catch (...) {
            std::cout << "無効な入力形式です。\n";
            continue;
        }

        int r = BOARD_SIZE - row_num;

        game.play_move(r, c);
    }

    return 0;
}