#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#define NOMINMAX  // std::min/max と競合防止
#include <windows.h>
#include <string>
#include <vector>
#include <queue>
#include <random>
#include <algorithm>
#include <iostream>
#include <sstream>

// ===================== チェスエンジン（前回と同一） =====================
enum class Color { WHITE, BLACK, NONE };
enum class PieceType { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

struct Piece {
    PieceType type;
    Color color;
    bool moved;
    Piece(PieceType t = PieceType::EMPTY, Color c = Color::NONE) : type(t), color(c), moved(false) {}
};

struct Move {
    int fromX, fromY, toX, toY;
    PieceType promotion;
    bool is_castling;
    bool is_en_passant;
    Move(int fx = 0, int fy = 0, int tx = 0, int ty = 0)
        : fromX(fx), fromY(fy), toX(tx), toY(ty),
        promotion(PieceType::EMPTY), is_castling(false), is_en_passant(false) {
    }
};

inline Color opponent(Color c) { return (c == Color::WHITE) ? Color::BLACK : Color::WHITE; }

class ChessBoard {
public:
    static const int SIZE = 8;
    Piece board[SIZE][SIZE];
    Color turn;
    int move_count;
    Move last_move;

    ChessBoard() : turn(Color::WHITE), move_count(1), last_move(-1, -1, -1, -1) { init_board(); }

    void init_board() {
        auto setup = [&](int y, Color c) {
            board[0][y] = Piece(PieceType::ROOK, c);
            board[1][y] = Piece(PieceType::KNIGHT, c);
            board[2][y] = Piece(PieceType::BISHOP, c);
            board[3][y] = Piece(PieceType::QUEEN, c);
            board[4][y] = Piece(PieceType::KING, c);
            board[5][y] = Piece(PieceType::BISHOP, c);
            board[6][y] = Piece(PieceType::KNIGHT, c);
            board[7][y] = Piece(PieceType::ROOK, c);
            for (int x = 0; x < SIZE; ++x) board[x][y + (c == Color::WHITE ? -1 : 1)] = Piece(PieceType::PAWN, c);
        };
        setup(0, Color::BLACK);
        setup(7, Color::WHITE);
        for (int y = 2; y <= 5; ++y)
            for (int x = 0; x < SIZE; ++x)
                board[x][y] = Piece();
    }

    bool make_move(const Move& m) {
        std::vector<Move> legals = get_legal_moves();
        bool found = false;
        for (size_t i = 0; i < legals.size(); ++i) {
            if (legals[i].fromX == m.fromX && legals[i].fromY == m.fromY &&
                legals[i].toX == m.toX && legals[i].toY == m.toY) {
                found = true; break;
            }
        }
        if (!found) return false;
        apply_move_real(m);
        turn = opponent(turn);
        move_count++;
        return true;
    }

    std::vector<Move> get_legal_moves() const {
        std::vector<Move> pseudo = generate_pseudo_moves(turn);
        std::vector<Move> legal;
        for (size_t i = 0; i < pseudo.size(); ++i) {
            ChessBoard temp = *this;
            temp.apply_move_temp(pseudo[i]);
            if (!temp.is_check(turn)) legal.push_back(pseudo[i]);
        }
        return legal;
    }

    bool is_game_over() const { return get_legal_moves().empty(); }

    std::wstring get_result() const {
        if (!is_game_over()) return L"Ongoing";
        if (is_check(turn)) {
            return (turn == Color::WHITE) ? L"0-1 Checkmate" : L"1-0 Checkmate";
        }
        return L"1/2-1/2 Stalemate";
    }

private:
    bool in_bounds(int x, int y) const { return x >= 0 && x < SIZE && y >= 0 && y < SIZE; }

    bool is_check(Color c) const {
        int kx = -1, ky = -1;
        for (int y = 0; y < SIZE; ++y)
            for (int x = 0; x < SIZE; ++x)
                if (board[x][y].type == PieceType::KING && board[x][y].color == c) { kx = x; ky = y; }
        if (kx == -1) return false;
        return is_square_attacked(kx, ky, opponent(c));
    }

    bool is_square_attacked(int x, int y, Color by) const {
        int pd = (by == Color::WHITE) ? -1 : 1;
        int start = (by == Color::WHITE) ? 6 : 1;
        if (in_bounds(x - 1, y + pd) && board[x - 1][y + pd].type == PieceType::PAWN && board[x - 1][y + pd].color == by) return true;
        if (in_bounds(x + 1, y + pd) && board[x + 1][y + pd].type == PieceType::PAWN && board[x + 1][y + pd].color == by) return true;

        const int kx[8] = { 1,2,2,1,-1,-2,-2,-1 };
        const int ky[8] = { 2,1,-1,-2,-2,-1,1,2 };
        for (int i = 0; i < 8; ++i) {
            int nx = x + kx[i], ny = y + ky[i];
            if (in_bounds(nx, ny) && board[nx][ny].type == PieceType::KNIGHT && board[nx][ny].color == by) return true;
        }

        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                int nx = x + dx, ny = y + dy;
                if (in_bounds(nx, ny) && board[nx][ny].type == PieceType::KING && board[nx][ny].color == by) return true;
            }

        const int dx1[4] = { 1,-1,0,0 };
        const int dy1[4] = { 0,0,1,-1 };
        for (int d = 0; d < 4; ++d) {
            int nx = x + dx1[d], ny = y + dy1[d];
            while (in_bounds(nx, ny)) {
                if (board[nx][ny].color != Color::NONE) {
                    if (board[nx][ny].color == by && (board[nx][ny].type == PieceType::ROOK || board[nx][ny].type == PieceType::QUEEN)) return true;
                    break;
                }
                nx += dx1[d]; ny += dy1[d];
            }
        }

        const int dx2[4] = { 1,1,-1,-1 };
        const int dy2[4] = { 1,-1,1,-1 };
        for (int d = 0; d < 4; ++d) {
            int nx = x + dx2[d], ny = y + dy2[d];
            while (in_bounds(nx, ny)) {
                if (board[nx][ny].color != Color::NONE) {
                    if (board[nx][ny].color == by && (board[nx][ny].type == PieceType::BISHOP || board[nx][ny].type == PieceType::QUEEN)) return true;
                    break;
                }
                nx += dx2[d]; ny += dy2[d];
            }
        }
        return false;
    }

    std::vector<Move> generate_pseudo_moves(Color c) const {
        std::vector<Move> moves;
        for (int y = 0; y < SIZE; ++y)
            for (int x = 0; x < SIZE; ++x)
                if (board[x][y].color == c) generate_piece_moves(x, y, moves);
        return moves;
    }

    void generate_piece_moves(int x, int y, std::vector<Move>& moves) const {
        Piece p = board[x][y];
        switch (p.type) {
        case PieceType::PAWN: pawn_moves(x, y, moves); break;
        case PieceType::KNIGHT: knight_moves(x, y, moves); break;
        case PieceType::BISHOP: bishop_moves(x, y, moves); break;
        case PieceType::ROOK: rook_moves(x, y, moves); break;
        case PieceType::QUEEN: queen_moves(x, y, moves); break;
        case PieceType::KING: king_moves(x, y, moves); break;
        default: break;
        }
    }

    void add_move(int fx, int fy, int tx, int ty, std::vector<Move>& moves) const {
        if (!in_bounds(tx, ty)) return;
        if (board[tx][ty].color == board[fx][fy].color) return;
        Move m(fx, fy, tx, ty);
        if (board[fx][fy].type == PieceType::PAWN) {
            if ((board[fx][fy].color == Color::WHITE && ty == 0) ||
                (board[fx][fy].color == Color::BLACK && ty == SIZE - 1))
                m.promotion = PieceType::QUEEN;
        }
        moves.push_back(m);
    }

    void pawn_moves(int x, int y, std::vector<Move>& moves) const {
        Color c = board[x][y].color;
        int dir = (c == Color::WHITE) ? -1 : 1;
        int start = (c == Color::WHITE) ? 6 : 1;
        if (in_bounds(x, y + dir) && board[x][y + dir].type == PieceType::EMPTY)
            add_move(x, y, x, y + dir, moves);
        if (y == start && board[x][y + dir].type == PieceType::EMPTY && board[x][y + 2 * dir].type == PieceType::EMPTY)
            add_move(x, y, x, y + 2 * dir, moves);
        for (int dx2 : { -1, 1 }) {
            int nx = x + dx2, ny = y + dir;
            if (in_bounds(nx, ny) && board[nx][ny].color == opponent(c))
                add_move(x, y, nx, ny, moves);
        }
        if (last_move.fromX != -1) {
            int epy = (c == Color::WHITE) ? 3 : 4;
            if (y == epy) {
                for (int dx2 : { -1, 1 }) {
                    int nx = x + dx2;
                    if (in_bounds(nx, epy) && board[nx][epy].type == PieceType::PAWN && board[nx][epy].color == opponent(c)) {
                        if (last_move.fromX == nx && last_move.fromY == (c == Color::WHITE ? 1 : 6) && last_move.toX == nx && last_move.toY == epy) {
                            Move m(x, y, nx, y + dir); m.is_en_passant = true; moves.push_back(m);
                        }
                    }
                }
            }
        }
    }

    void knight_moves(int x, int y, std::vector<Move>& moves) const {
        const int kx[8] = { 1,2,2,1,-1,-2,-2,-1 };
        const int ky[8] = { 2,1,-1,-2,-2,-1,1,2 };
        for (int i = 0; i < 8; ++i) add_move(x, y, x + kx[i], y + ky[i], moves);
    }

    void slide_moves(int x, int y, const int dx[], const int dy[], int ndir, std::vector<Move>& moves) const {
        for (int d = 0; d < ndir; ++d) {
            int nx = x + dx[d], ny = y + dy[d];
            while (in_bounds(nx, ny)) {
                if (board[nx][ny].type != PieceType::EMPTY) { add_move(x, y, nx, ny, moves); break; }
                add_move(x, y, nx, ny, moves);
                nx += dx[d]; ny += dy[d];
            }
        }
    }

    void bishop_moves(int x, int y, std::vector<Move>& moves) const {
        const int dx[4] = { 1,1,-1,-1 }; const int dy[4] = { 1,-1,1,-1 };
        slide_moves(x, y, dx, dy, 4, moves);
    }

    void rook_moves(int x, int y, std::vector<Move>& moves) const {
        const int dx[4] = { 1,-1,0,0 }; const int dy[4] = { 0,0,1,-1 };
        slide_moves(x, y, dx, dy, 4, moves);
    }

    void queen_moves(int x, int y, std::vector<Move>& moves) const {
        bishop_moves(x, y, moves); rook_moves(x, y, moves);
    }

    void king_moves(int x, int y, std::vector<Move>& moves) const {
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                add_move(x, y, x + dx, y + dy, moves);
            }
        Color c = board[x][y].color;
        int rank = (c == Color::WHITE) ? 7 : 0;
        if (y != rank || x != 4 || board[x][y].moved) return;
        if (board[7][rank].type == PieceType::ROOK && !board[7][rank].moved &&
            board[5][rank].type == PieceType::EMPTY && board[6][rank].type == PieceType::EMPTY &&
            !is_square_attacked(4, rank, opponent(c)) && !is_square_attacked(5, rank, opponent(c)) && !is_square_attacked(6, rank, opponent(c))) {
            Move m(4, rank, 6, rank); m.is_castling = true; moves.push_back(m);
        }
        if (board[0][rank].type == PieceType::ROOK && !board[0][rank].moved &&
            board[1][rank].type == PieceType::EMPTY && board[2][rank].type == PieceType::EMPTY && board[3][rank].type == PieceType::EMPTY &&
            !is_square_attacked(4, rank, opponent(c)) && !is_square_attacked(3, rank, opponent(c)) && !is_square_attacked(2, rank, opponent(c))) {
            Move m(4, rank, 2, rank); m.is_castling = true; moves.push_back(m);
        }
    }

    void apply_move_temp(const Move& m) { apply_move_real(m); }

    void apply_move_real(const Move& m) {
        Piece moving = board[m.fromX][m.fromY];
        moving.moved = true;
        if (m.is_castling && moving.type == PieceType::KING) {
            int rank = m.fromY;
            if (m.toX == 6) { board[5][rank] = board[7][rank]; board[5][rank].moved = true; board[7][rank] = Piece(); }
            else if (m.toX == 2) { board[3][rank] = board[0][rank]; board[3][rank].moved = true; board[0][rank] = Piece(); }
        }
        if (m.is_en_passant) {
            int cy = (moving.color == Color::WHITE) ? m.toY + 1 : m.toY - 1;
            board[m.toX][cy] = Piece();
        }
        board[m.toX][m.toY] = moving;
        if (m.promotion != PieceType::EMPTY) board[m.toX][m.toY].type = m.promotion;
        board[m.fromX][m.fromY] = Piece();
        last_move = m;
    }
};

class RandomAgent {
    std::mt19937 rng;
public:
    explicit RandomAgent(unsigned seed = 42) : rng(seed) {}
    Move select_move(const ChessBoard& board) {
        std::vector<Move> legals = board.get_legal_moves();
        if (legals.empty()) return Move();
        std::uniform_int_distribution<size_t> dist(0, legals.size() - 1);
        return legals[dist(rng)];
    }
};

// ===================== GUI 関連 =====================
const int CELL_SIZE = 64;
const int BOARD_OFFSET_X = 20;
const int BOARD_OFFSET_Y = 20;
// windows.h の型・マクロに依存しない安全な定義（COLORREF = 0x00bbggrr）
const DWORD COLOR_LIGHT = 0x00B5D9F0;  // RGB(240, 217, 181)
const DWORD COLOR_DARK = 0x006388B5;  // RGB(181, 136, 99)
const DWORD COLOR_MOVE = 0x004FA86A;  // RGB(106, 168, 79) Green
// COLOR_HIGHLIGHT の定義を COLORREF 型に修正
const COLORREF COLOR_HIGHLIGHT = 0x00FFFF00;  // RGB(255, 255, 0) Yellow
ChessBoard g_board;
RandomAgent g_ai(123);
bool g_selected = false;
int g_selX = -1, g_selY = -1;
std::vector<Move> g_legalMoves;
bool g_aiThinking = false;
HWND g_hWnd = NULL;

// 駒のUnicode文字
const wchar_t* GetPieceChar(PieceType t, Color c) {
    if (c == Color::WHITE) {
        switch (t) {
        case PieceType::KING: return L"\u2654";
        case PieceType::QUEEN: return L"\u2655";
        case PieceType::ROOK: return L"\u2656";
        case PieceType::BISHOP: return L"\u2657";
        case PieceType::KNIGHT: return L"\u2658";
        case PieceType::PAWN: return L"\u2659";
        default: return L"";
        }
    }
    else if (c == Color::BLACK) {
        switch (t) {
        case PieceType::KING: return L"\u265A";
        case PieceType::QUEEN: return L"\u265B";
        case PieceType::ROOK: return L"\u265C";
        case PieceType::BISHOP: return L"\u265D";
        case PieceType::KNIGHT: return L"\u265E";
        case PieceType::PAWN: return L"\u265F";
        default: return L"";
        }
    }
    return L"";
}

// ===================== 描画 =====================
void DrawBoard(HDC hdc) {
    // 盤背景
    RECT rc;
    SetRect(&rc, BOARD_OFFSET_X, BOARD_OFFSET_Y,
        BOARD_OFFSET_X + CELL_SIZE * 8, BOARD_OFFSET_Y + CELL_SIZE * 8);
    FillRect(hdc, &rc, (HBRUSH)GetStockObject(GRAY_BRUSH));

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            int px = BOARD_OFFSET_X + x * CELL_SIZE;
            int py = BOARD_OFFSET_Y + y * CELL_SIZE;
            RECT cell = { px, py, px + CELL_SIZE, py + CELL_SIZE };

            // マス色
            COLORREF col = ((x + y) % 2 == 0) ? COLOR_LIGHT : COLOR_DARK;
            HBRUSH hBrush = CreateSolidBrush(col);
            FillRect(hdc, &cell, hBrush);
            DeleteObject(hBrush);

            // 選択ハイライト
            if (g_selected && g_selX == x && g_selY == y) {
                HBRUSH hHL = CreateSolidBrush(COLOR_HIGHLIGHT);
                FrameRect(hdc, &cell, hHL);
                FrameRect(hdc, &cell, hHL);
                DeleteObject(hHL);
            }
            // 合法手の表示
            if (g_selected) {
                for (size_t i = 0; i < g_legalMoves.size(); ++i) {
                    if (g_legalMoves[i].fromX == g_selX && g_legalMoves[i].fromY == g_selY &&
                        g_legalMoves[i].toX == x && g_legalMoves[i].toY == y) {
                        HBRUSH hDot = CreateSolidBrush(COLOR_MOVE);
                        RECT dot = { px + CELL_SIZE / 2 - 6, py + CELL_SIZE / 2 - 6,
                                    px + CELL_SIZE / 2 + 6, py + CELL_SIZE / 2 + 6 };
                        Ellipse(hdc, dot.left, dot.top, dot.right, dot.bottom);
                        DeleteObject(hDot);
                    }
                }
            }

            // 駒描画
            Piece p = g_board.board[x][y];
            if (p.type != PieceType::EMPTY) {
                const wchar_t* txt = GetPieceChar(p.type, p.color);
                SetTextColor(hdc, (p.color == Color::WHITE) ? 0x00FFFFFF : 0x00000000);
                SetBkMode(hdc, TRANSPARENT);
                HFONT hFont = CreateFont(40, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                    ANTIALIASED_QUALITY, VARIABLE_PITCH, L"Segoe UI Symbol");
                HFONT hOld = (HFONT)SelectObject(hdc, hFont);
                DrawText(hdc, txt, -1, &cell, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                SelectObject(hdc, hOld);
                DeleteObject(hFont);
            }
        }
    }

    // 座標ラベル
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, VARIABLE_PITCH, L"Microsoft Sans Serif");
    HFONT hOld = (HFONT)SelectObject(hdc, hFont);
    for (int i = 0; i < 8; ++i) {
        wchar_t file[2] = { wchar_t(L'a' + i), 0 };
        RECT r1 = { BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y - 18,
                    BOARD_OFFSET_X + (i + 1) * CELL_SIZE, BOARD_OFFSET_Y };
        DrawText(hdc, file, -1, &r1, DT_CENTER | DT_TOP);
        RECT r2 = { BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y + 8 * CELL_SIZE,
                    BOARD_OFFSET_X + (i + 1) * CELL_SIZE, BOARD_OFFSET_Y + 8 * CELL_SIZE + 18 };
        DrawText(hdc, file, -1, &r2, DT_CENTER | DT_TOP);

        wchar_t rank[2] = { wchar_t(L'8' - i), 0 };
        RECT r3 = { BOARD_OFFSET_X - 18, BOARD_OFFSET_Y + i * CELL_SIZE,
                    BOARD_OFFSET_X, BOARD_OFFSET_Y + (i + 1) * CELL_SIZE };
        DrawText(hdc, rank, -1, &r3, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        RECT r4 = { BOARD_OFFSET_X + 8 * CELL_SIZE, BOARD_OFFSET_Y + i * CELL_SIZE,
                    BOARD_OFFSET_X + 8 * CELL_SIZE + 18, BOARD_OFFSET_Y + (i + 1) * CELL_SIZE };
        DrawText(hdc, rank, -1, &r4, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // ステータス
    RECT rcStatus = { BOARD_OFFSET_X, BOARD_OFFSET_Y + 8 * CELL_SIZE + 30,
                      BOARD_OFFSET_X + 8 * CELL_SIZE, BOARD_OFFSET_Y + 8 * CELL_SIZE + 60 };
    std::wstring status = (g_board.turn == Color::WHITE) ? L"Turn: White" : L"Turn: Black";
    if (g_board.is_game_over()) status = L"Game Over: " + g_board.get_result();
    if (g_aiThinking) status = L"AI thinking...";
    DrawText(hdc, status.c_str(), -1, &rcStatus, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hOld);
    DeleteObject(hFont);
}

// ===================== マウス処理 =====================
void HandleMouseClick(int px, int py) {
    if (g_board.is_game_over() || g_aiThinking) return;
    if (g_board.turn != Color::WHITE) return; // 人間は白のみ

    int x = (px - BOARD_OFFSET_X) / CELL_SIZE;
    int y = (py - BOARD_OFFSET_Y) / CELL_SIZE;
    if (x < 0 || x >= 8 || y < 0 || y >= 8) return;

    if (!g_selected) {
        if (g_board.board[x][y].color == g_board.turn) {
            g_selX = x; g_selY = y; g_selected = true;
            g_legalMoves = g_board.get_legal_moves();
            InvalidateRect(g_hWnd, NULL, FALSE);
        }
    }
    else {
        // 同じマスならキャンセル
        if (x == g_selX && y == g_selY) {
            g_selected = false;
            InvalidateRect(g_hWnd, NULL, FALSE);
            return;
        }

        Move m(g_selX, g_selY, x, y);
        bool ok = g_board.make_move(m);
        g_selected = false;
        InvalidateRect(g_hWnd, NULL, FALSE);

        if (ok && !g_board.is_game_over() && g_board.turn == Color::BLACK) {
            g_aiThinking = true;
            InvalidateRect(g_hWnd, NULL, FALSE);
            UpdateWindow(g_hWnd); // 即座に描画更新

            // AI思考（簡易のためSleepで演出）
            Sleep(300);
            Move aiMove = g_ai.select_move(g_board);
            if (!(aiMove.fromX == 0 && aiMove.fromY == 0 && aiMove.toX == 0 && aiMove.toY == 0)) {
                g_board.make_move(aiMove);
            }
            g_aiThinking = false;
            InvalidateRect(g_hWnd, NULL, FALSE);
        }
    }
}

// ===================== ウィンドウプロシージャ =====================
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        g_hWnd = hWnd;
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        DrawBoard(hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_LBUTTONUP: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        HandleMouseClick(x, y);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

// ===================== WinMain =====================
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"ChessGUI";
    wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wcex)) return 1;

    int winW = BOARD_OFFSET_X * 2 + CELL_SIZE * 8 + 40;
    int winH = BOARD_OFFSET_Y * 2 + CELL_SIZE * 8 + 100;

    HWND hWnd = CreateWindowExW(0, L"ChessGUI", L"C++ Chess GUI (You: White, AI: Black)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, winW, winH,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return 1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}