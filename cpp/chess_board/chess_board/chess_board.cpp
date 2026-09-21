// chess_board.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <iostream>



#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <cstdlib>

// ===================== 基本定義 =====================
enum class Color { WHITE, BLACK, NONE };
enum class PieceType { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

struct Piece {
    PieceType type;
    Color color;
    bool moved;  // キャスリング判定用

    Piece(PieceType t = PieceType::EMPTY, Color c = Color::NONE) : type(t), color(c), moved(false) {}
};

struct Move {
    int fromX, fromY;
    int toX, toY;
    PieceType promotion;  // 簡易: プロモーション先（通常はEMPTY）
    bool is_castling;
    bool is_en_passant;

    Move(int fx = 0, int fy = 0, int tx = 0, int ty = 0)
        : fromX(fx), fromY(fy), toX(tx), toY(ty),
        promotion(PieceType::EMPTY), is_castling(false), is_en_passant(false) {
    }
};

inline Color opponent(Color c) {
    return (c == Color::WHITE) ? Color::BLACK : Color::WHITE;
}

// ===================== チェス盤クラス =====================
class ChessBoard {
public:
    static const int SIZE = 8;
    Piece board[SIZE][SIZE];
    Color turn;
    int move_count;
    int halfmove_clock;  // 50手ルール用（簡易）
    Move last_move;      // en passant判定用

    ChessBoard() : turn(Color::WHITE), move_count(1), halfmove_clock(0) {
        init_board();
    }

    void init_board() {
        // ブラック（上側: y=0,1）
        board[0][0] = Piece(PieceType::ROOK, Color::BLACK);
        board[1][0] = Piece(PieceType::KNIGHT, Color::BLACK);
        board[2][0] = Piece(PieceType::BISHOP, Color::BLACK);
        board[3][0] = Piece(PieceType::QUEEN, Color::BLACK);
        board[4][0] = Piece(PieceType::KING, Color::BLACK);
        board[5][0] = Piece(PieceType::BISHOP, Color::BLACK);
        board[6][0] = Piece(PieceType::KNIGHT, Color::BLACK);
        board[7][0] = Piece(PieceType::ROOK, Color::BLACK);
        for (int x = 0; x < SIZE; ++x) board[x][1] = Piece(PieceType::PAWN, Color::BLACK);

        // ホワイト（下側: y=6,7）
        for (int x = 0; x < SIZE; ++x) board[x][6] = Piece(PieceType::PAWN, Color::WHITE);
        board[0][7] = Piece(PieceType::ROOK, Color::WHITE);
        board[1][7] = Piece(PieceType::KNIGHT, Color::WHITE);
        board[2][7] = Piece(PieceType::BISHOP, Color::WHITE);
        board[3][7] = Piece(PieceType::QUEEN, Color::WHITE);
        board[4][7] = Piece(PieceType::KING, Color::WHITE);
        board[5][7] = Piece(PieceType::BISHOP, Color::WHITE);
        board[6][7] = Piece(PieceType::KNIGHT, Color::WHITE);
        board[7][7] = Piece(PieceType::ROOK, Color::WHITE);

        // 空白
        for (int y = 2; y <= 5; ++y)
            for (int x = 0; x < SIZE; ++x)
                board[x][y] = Piece();
    }

    // --- 盤面表示 ---
    void print() const {
        std::cout << "\n    a   b   c   d   e   f   g   h\n";
        std::cout << "  +---+---+---+---+---+---+---+---+\n";
        for (int y = 0; y < SIZE; ++y) {
            std::cout << (8 - y) << " |";
            for (int x = 0; x < SIZE; ++x) {
                std::cout << " " << piece_char(board[x][y]) << " |";
            }
            std::cout << " " << (8 - y) << "\n";
            std::cout << "  +---+---+---+---+---+---+---+---+\n";
        }
        std::cout << "    a   b   c   d   e   f   g   h\n";
        std::cout << "  Turn: " << (turn == Color::WHITE ? "White" : "Black") << "\n\n";
    }

    // --- 合法手生成 ---
    std::vector<Move> get_legal_moves() const {
        std::vector<Move> pseudo = generate_pseudo_moves(turn);
        std::vector<Move> legal;
        for (size_t i = 0; i < pseudo.size(); ++i) {
            ChessBoard temp = *this;
            temp.apply_move_temp(pseudo[i]);
            if (!temp.is_check(turn)) {
                legal.push_back(pseudo[i]);
            }
        }
        return legal;
    }

    // --- 手を実行 ---
    bool make_move(const Move& m) {
        std::vector<Move> legals = get_legal_moves();
        bool found = false;
        for (size_t i = 0; i < legals.size(); ++i) {
            if (legals[i].fromX == m.fromX && legals[i].fromY == m.fromY &&
                legals[i].toX == m.toX && legals[i].toY == m.toY) {
                found = true;
                break;
            }
        }
        if (!found) return false;

        apply_move_real(m);
        turn = opponent(turn);
        move_count++;
        return true;
    }

    // --- ゲーム終了判定 ---
    bool is_game_over() const {
        return get_legal_moves().empty();
    }

    // --- 結果文字列 ---
    std::string get_result() const {
        if (!is_game_over()) return "ongoing";
        if (is_check(turn)) {
            return (turn == Color::WHITE) ? "0-1 (Black wins by checkmate)" : "1-0 (White wins by checkmate)";
        }
        return "1/2-1/2 (Stalemate)";
    }

private:
    static std::string piece_char(const Piece& p) {
        if (p.color == Color::WHITE) {
            switch (p.type) {
            case PieceType::KING:   return "K"; // ♔
            case PieceType::QUEEN:  return "Q"; // ♕
            case PieceType::ROOK:   return "R"; // ♖
            case PieceType::BISHOP: return "B"; // ♗
            case PieceType::KNIGHT: return "N"; // ♘
            case PieceType::PAWN:   return "P"; // ♙
            default: return " ";
            }
        }
        else if (p.color == Color::BLACK) {
            switch (p.type) {
            case PieceType::KING:   return "k"; // ♚
            case PieceType::QUEEN:  return "q"; // ♛
            case PieceType::ROOK:   return "r"; // ♜
            case PieceType::BISHOP: return "b"; // ♝
            case PieceType::KNIGHT: return "n"; // ♞
            case PieceType::PAWN:   return "p"; // ♟
            default: return " ";
            }
        }
        return " ";
    }

    bool in_bounds(int x, int y) const {
        return x >= 0 && x < SIZE && y >= 0 && y < SIZE;
    }

    // --- 王手判定 ---
    bool is_check(Color c) const {
        int kx = -1, ky = -1;
        for (int y = 0; y < SIZE; ++y)
            for (int x = 0; x < SIZE; ++x)
                if (board[x][y].type == PieceType::KING && board[x][y].color == c) {
                    kx = x; ky = y;
                }
        if (kx == -1) return false;
        return is_square_attacked(kx, ky, opponent(c));
    }

    // --- あるマスがcolor側に攻撃されているか ---
    bool is_square_attacked(int x, int y, Color by_color) const {
        // ポーン
        int pawn_dir = (by_color == Color::WHITE) ? -1 : 1;
        int start_y = (by_color == Color::WHITE) ? 6 : 1;
        if (in_bounds(x - 1, y + pawn_dir) && board[x - 1][y + pawn_dir].type == PieceType::PAWN && board[x - 1][y + pawn_dir].color == by_color) return true;
        if (in_bounds(x + 1, y + pawn_dir) && board[x + 1][y + pawn_dir].type == PieceType::PAWN && board[x + 1][y + pawn_dir].color == by_color) return true;

        // ナイト
        const int kx[8] = { 1,2,2,1,-1,-2,-2,-1 };
        const int ky[8] = { 2,1,-1,-2,-2,-1,1,2 };
        for (int i = 0; i < 8; ++i) {
            int nx = x + kx[i], ny = y + ky[i];
            if (in_bounds(nx, ny) && board[nx][ny].type == PieceType::KNIGHT && board[nx][ny].color == by_color) return true;
        }

        // キング（周囲1マス）
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                int nx = x + dx, ny = y + dy;
                if (in_bounds(nx, ny) && board[nx][ny].type == PieceType::KING && board[nx][ny].color == by_color) return true;
            }

        // 直線（ルーク、后）
        const int dx1[4] = { 1,-1,0,0 };
        const int dy1[4] = { 0,0,1,-1 };
        for (int d = 0; d < 4; ++d) {
            int nx = x + dx1[d], ny = y + dy1[d];
            while (in_bounds(nx, ny)) {
                if (board[nx][ny].color != Color::NONE) {
                    if (board[nx][ny].color == by_color &&
                        (board[nx][ny].type == PieceType::ROOK || board[nx][ny].type == PieceType::QUEEN))
                        return true;
                    break;
                }
                nx += dx1[d]; ny += dy1[d];
            }
        }

        // 斜め（ビショップ、后）
        const int dx2[4] = { 1,1,-1,-1 };
        const int dy2[4] = { 1,-1,1,-1 };
        for (int d = 0; d < 4; ++d) {
            int nx = x + dx2[d], ny = y + dy2[d];
            while (in_bounds(nx, ny)) {
                if (board[nx][ny].color != Color::NONE) {
                    if (board[nx][ny].color == by_color &&
                        (board[nx][ny].type == PieceType::BISHOP || board[nx][ny].type == PieceType::QUEEN))
                        return true;
                    break;
                }
                nx += dx2[d]; ny += dy2[d];
            }
        }

        return false;
    }

    // --- 擬似合法手生成 ---
    std::vector<Move> generate_pseudo_moves(Color c) const {
        std::vector<Move> moves;
        for (int y = 0; y < SIZE; ++y)
            for (int x = 0; x < SIZE; ++x)
                if (board[x][y].color == c)
                    generate_piece_moves(x, y, moves);
        return moves;
    }

    void generate_piece_moves(int x, int y, std::vector<Move>& moves) const {
        Piece p = board[x][y];
        switch (p.type) {
        case PieceType::PAWN:   pawn_moves(x, y, moves); break;
        case PieceType::KNIGHT: knight_moves(x, y, moves); break;
        case PieceType::BISHOP: bishop_moves(x, y, moves); break;
        case PieceType::ROOK:   rook_moves(x, y, moves); break;
        case PieceType::QUEEN:  queen_moves(x, y, moves); break;
        case PieceType::KING:   king_moves(x, y, moves); break;
        default: break;
        }
    }

    void add_move(int fx, int fy, int tx, int ty, std::vector<Move>& moves) const {
        if (!in_bounds(tx, ty)) return;
        Piece target = board[tx][ty];
        if (target.color == board[fx][fy].color) return; // 自分の駒を取れない
        Move m(fx, fy, tx, ty);
        // プロモーション（簡易: 常にQueen）
        if (board[fx][fy].type == PieceType::PAWN) {
            if ((board[fx][fy].color == Color::WHITE && ty == 0) ||
                (board[fx][fy].color == Color::BLACK && ty == SIZE - 1)) {
                m.promotion = PieceType::QUEEN;
            }
        }
        moves.push_back(m);
    }

    void pawn_moves(int x, int y, std::vector<Move>& moves) const {
        Color c = board[x][y].color;
        int dir = (c == Color::WHITE) ? -1 : 1;
        int start_y = (c == Color::WHITE) ? 6 : 1;

        // 前進1マス
        if (in_bounds(x, y + dir) && board[x][y + dir].type == PieceType::EMPTY)
            add_move(x, y, x, y + dir, moves);

        // 前進2マス（初動）
        if (y == start_y && board[x][y + dir].type == PieceType::EMPTY && board[x][y + 2 * dir].type == PieceType::EMPTY)
            add_move(x, y, x, y + 2 * dir, moves);

        // 斜め取り
        for (int dx2 : { -1, 1 }) {
            int nx = x + dx2, ny = y + dir;
            if (in_bounds(nx, ny) && board[nx][ny].color == opponent(c))
                add_move(x, y, nx, ny, moves);
        }

        // en passant
        if (last_move.fromX != -1) {
            int ep_y = (c == Color::WHITE) ? 3 : 4;
            if (y == ep_y) {
                for (int dx2 : { -1, 1 }) {
                    int nx = x + dx2;
                    if (in_bounds(nx, ep_y) && board[nx][ep_y].type == PieceType::PAWN && board[nx][ep_y].color == opponent(c)) {
                        // 直前に2歩進んだか
                        if (last_move.fromX == nx && last_move.fromY == (c == Color::WHITE ? 1 : 6) &&
                            last_move.toX == nx && last_move.toY == ep_y) {
                            Move m(x, y, nx, y + dir);
                            m.is_en_passant = true;
                            moves.push_back(m);
                        }
                    }
                }
            }
        }
    }

    void knight_moves(int x, int y, std::vector<Move>& moves) const {
        const int kx[8] = { 1,2,2,1,-1,-2,-2,-1 };
        const int ky[8] = { 2,1,-1,-2,-2,-1,1,2 };
        for (int i = 0; i < 8; ++i)
            add_move(x, y, x + kx[i], y + ky[i], moves);
    }

    void slide_moves(int x, int y, const int dx[], const int dy[], int ndir, std::vector<Move>& moves) const {
        for (int d = 0; d < ndir; ++d) {
            int nx = x + dx[d], ny = y + dy[d];
            while (in_bounds(nx, ny)) {
                if (board[nx][ny].type != PieceType::EMPTY) {
                    add_move(x, y, nx, ny, moves);
                    break;
                }
                add_move(x, y, nx, ny, moves);
                nx += dx[d]; ny += dy[d];
            }
        }
    }

    void bishop_moves(int x, int y, std::vector<Move>& moves) const {
        const int dx[4] = { 1,1,-1,-1 };
        const int dy[4] = { 1,-1,1,-1 };
        slide_moves(x, y, dx, dy, 4, moves);
    }

    void rook_moves(int x, int y, std::vector<Move>& moves) const {
        const int dx[4] = { 1,-1,0,0 };
        const int dy[4] = { 0,0,1,-1 };
        slide_moves(x, y, dx, dy, 4, moves);
    }

    void queen_moves(int x, int y, std::vector<Move>& moves) const {
        bishop_moves(x, y, moves);
        rook_moves(x, y, moves);
    }

    void king_moves(int x, int y, std::vector<Move>& moves) const {
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                add_move(x, y, x + dx, y + dy, moves);
            }

        // キャスリング
        Color c = board[x][y].color;
        int rank = (c == Color::WHITE) ? 7 : 0;
        if (y != rank || x != 4) return;
        if (board[x][y].moved) return;

        // キングサイド (g1/g8)
        if (board[7][rank].type == PieceType::ROOK && !board[7][rank].moved &&
            board[5][rank].type == PieceType::EMPTY && board[6][rank].type == PieceType::EMPTY) {
            if (!is_square_attacked(4, rank, opponent(c)) &&
                !is_square_attacked(5, rank, opponent(c)) &&
                !is_square_attacked(6, rank, opponent(c))) {
                Move m(4, rank, 6, rank);
                m.is_castling = true;
                moves.push_back(m);
            }
        }

        // クイーンサイド (c1/c8)
        if (board[0][rank].type == PieceType::ROOK && !board[0][rank].moved &&
            board[1][rank].type == PieceType::EMPTY && board[2][rank].type == PieceType::EMPTY &&
            board[3][rank].type == PieceType::EMPTY) {
            if (!is_square_attacked(4, rank, opponent(c)) &&
                !is_square_attacked(3, rank, opponent(c)) &&
                !is_square_attacked(2, rank, opponent(c))) {
                Move m(4, rank, 2, rank);
                m.is_castling = true;
                moves.push_back(m);
            }
        }
    }

    // --- 内部: 手の適用（テンポラリ）---
    void apply_move_temp(const Move& m) {
        apply_move_real(m);
    }

    // --- 内部: 手の適用（実際）---
    void apply_move_real(const Move& m) {
        Piece moving = board[m.fromX][m.fromY];
        moving.moved = true;

        // キャスリング: ルークも動かす
        if (m.is_castling && moving.type == PieceType::KING) {
            int rank = m.fromY;
            if (m.toX == 6) { // キングサイド
                board[5][rank] = board[7][rank];
                board[5][rank].moved = true;
                board[7][rank] = Piece();
            }
            else if (m.toX == 2) { // クイーンサイド
                board[3][rank] = board[0][rank];
                board[3][rank].moved = true;
                board[0][rank] = Piece();
            }
        }

        // en passant実行
        if (m.is_en_passant) {
            int captured_y = (moving.color == Color::WHITE) ? m.toY + 1 : m.toY - 1;
            board[m.toX][captured_y] = Piece();
        }

        board[m.toX][m.toY] = moving;
        // プロモーション
        if (m.promotion != PieceType::EMPTY) {
            board[m.toX][m.toY].type = m.promotion;
        }
        board[m.fromX][m.fromY] = Piece();

        last_move = m;
    }
};

// ===================== ランダムAI =====================
class RandomAgent {
    std::mt19937 rng;
public:
    explicit RandomAgent(unsigned seed = 42) : rng(seed) {}

    Move select_move(const ChessBoard& board) {
        std::vector<Move> legals = board.get_legal_moves();
        if (legals.empty()) return Move(); // 無効な手（呼び出し元で判定）
        std::uniform_int_distribution<size_t> dist(0, legals.size() - 1);
        return legals[dist(rng)];
    }
};

// ===================== メイン =====================
int main() {
    ChessBoard board;
    RandomAgent agent_white(123);
    RandomAgent agent_black(456);

    std::cout << "========================================\n";
    std::cout << "  Visual Studio C++ Chess Engine\n";
    std::cout << "========================================\n";
    board.print();

    int move_num = 1;
    while (!board.is_game_over() && move_num <= 200) {
        Move m = (board.turn == Color::WHITE)
            ? agent_white.select_move(board)
            : agent_black.select_move(board);

        // 合法手がない場合は終了
        if (m.fromX == 0 && m.fromY == 0 && m.toX == 0 && m.toY == 0 &&
            board.get_legal_moves().empty()) {
            break;
        }

        bool ok = board.make_move(m);
        if (!ok) {
            std::cout << "Illegal move attempted. Skipping.\n";
            break;
        }

        // 棋譜表示（e2e4 形式）
        char fc = 'a' + m.fromX;
        char tc = 'a' + m.toX;
        int fr = 8 - m.fromY;
        int tr = 8 - m.toY;

        std::cout << move_num << ". "
            << (board.turn == Color::BLACK ? "White" : "Black")  // make_moveでturnが反転済み
            << " " << fc << fr << tc << tr;
        if (m.is_castling) std::cout << " (O-O)";
        if (m.promotion != PieceType::EMPTY) std::cout << " (Q)";
        std::cout << "\n";

        if (move_num % 10 == 0) board.print();

        move_num++;
    }

    std::cout << "\n========== Game Over ==========\n";
    board.print();
    std::cout << "Result: " << board.get_result() << "\n";

    return 0;
}

