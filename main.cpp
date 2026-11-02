// ============================================================
//  TIC TAC TOE - CRAJI EDITION
//  Random mid-game event, one Steal per player, Undo, Replay.
//  Compile:  g++ -std=c++17 tictactoe.cpp -o tictactoe
// ============================================================
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

// ---------- Colors ----------
const string RESET = "\033[0m", BOLD = "\033[1m", DIM = "\033[2m";
const string RED = "\033[31m", GREEN = "\033[32m", YELLOW = "\033[33m";
const string BLUE = "\033[34m", CYAN = "\033[36m";

// ---------- Game data ----------
// Everything that changes during a match lives in State,
// so saving a snapshot (for undo / replay) is just copying it.
struct State {
    char board[4][4];
    int size = 3, win = 3;      // board size and marks needed in a row
    int occupied = 0, turns = 0;
    bool stealLeft[2] = {true, true};
    bool eventDone = false;

    State() {
        for (auto& row : board)
            for (char& c : row) c = ' ';
    }
};

struct Move {
    State after;                // game state right after this move
    int player;
    bool isSteal;
};

const char SYMBOL[2] = {'X', 'O'};
const string EVENT_NAMES[3] = {"Dimension Rift", "Polarity Reversal", "Entropy Decay"};

State game;
vector<Move> history;
int wins[2] = {0, 0}, draws = 0;
int eventType, eventTurn;

// ---------- Display ----------
void showBoard(const State& s) {
    auto border = [&](const string& left, const string& mid, const string& right) {
        cout << "    " << left;
        for (int j = 0; j < s.size; j++)
            cout << "───────" << (j == s.size - 1 ? right : mid);
        cout << "\n";
    };

    cout << "\n";
    border("┌", "┬", "┐");
    for (int i = 0; i < s.size; i++) {
        cout << "    ";
        for (int j = 0; j < s.size; j++) {
            char c = s.board[i][j];
            string label = (c == ' ') ? to_string(i * s.size + j + 1) : string(1, c);
            string color = (c == 'X') ? RED + BOLD : (c == 'O') ? BLUE + BOLD : DIM;
            int pad = 7 - (int)label.size();
            cout << "│" << color << string(pad / 2, ' ') << label
                 << string(pad - pad / 2, ' ') << RESET;
        }
        cout << "│\n";
        if (i < s.size - 1) border("├", "┼", "┤");
    }
    border("└", "┴", "┘");
}

void showScore() {
    cout << YELLOW << BOLD << "\n══════════ SCOREBOARD ══════════\n" << RESET;
    for (int p = 0; p < 2; p++) {
        cout << (p == 0 ? RED : BLUE) << "Player " << p + 1 << " (" << SYMBOL[p] << "): "
             << wins[p] << " wins" << RESET << "  |  Steal: "
             << (game.stealLeft[p] ? GREEN + "Available" : RED + "Used") << RESET << "\n";
    }
    cout << YELLOW << "Draws: " << draws << "\n"
         << "════════════════════════════════\n" << RESET;
}

void showHistory() {
    cout << YELLOW << BOLD << "\n══════════ MOVE HISTORY ═════════\n" << RESET;
    for (size_t i = 0; i < history.size(); i++)
        cout << "Move " << i + 1 << ": Player " << history[i].player + 1
             << (history[i].isSteal ? " used Steal Move" : " made a normal move") << "\n";
}

void replayGame() {
    cout << YELLOW << BOLD << "\n══════════ GAME REPLAY ══════════\n" << RESET;
    for (size_t i = 0; i < history.size(); i++) {
        cout << "\nMove " << i + 1 << " - Player " << history[i].player + 1
             << (history[i].isSteal ? " (Steal Move)" : " (Normal Move)") << "\n";
        showBoard(history[i].after);
        this_thread::sleep_for(chrono::milliseconds(700));
    }
}

// ---------- Rules ----------
bool checkWin(const State& s, char symbol) {
    const int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};  // → ↓ ↘ ↙

    for (int i = 0; i < s.size; i++)
        for (int j = 0; j < s.size; j++)
            for (auto& d : dirs) {
                int k = 0;
                while (k < s.win) {
                    int x = i + k * d[0], y = j + k * d[1];
                    if (x < 0 || x >= s.size || y < 0 || y >= s.size || s.board[x][y] != symbol)
                        break;
                    k++;
                }
                if (k == s.win) return true;
            }
    return false;
}

void triggerEvent(State& s) {
    if (s.eventDone || s.turns != eventTurn) return;
    s.eventDone = true;

    cout << "\n" << YELLOW << BOLD << "═════════ EVENT TRIGGERED ═════════\n" << RESET;
    cout << CYAN << BOLD << EVENT_NAMES[eventType] << "!\n" << RESET;

    if (eventType == 0) {                       // Dimension Rift
        s.size = s.win = 4;
        cout << "The board grew to 4x4! Marks stay in place.\n"
             << "You now need 4 in a row to win.\n";
    }
    else if (eventType == 1) {                  // Polarity Reversal
        for (int i = 0; i < s.size; i++)
            for (int j = 0; j < s.size; j++)
                if (s.board[i][j] != ' ')
                    s.board[i][j] = (s.board[i][j] == 'X') ? 'O' : 'X';
        cout << "Every X became O, and every O became X!\n";
    }
    else {                                      // Entropy Decay
        vector<pair<int, int>> xs, os;
        for (int i = 0; i < s.size; i++)
            for (int j = 0; j < s.size; j++) {
                if (s.board[i][j] == 'X') xs.push_back({i, j});
                if (s.board[i][j] == 'O') os.push_back({i, j});
            }

        if (!xs.empty() && !os.empty()) {
            auto x = xs[rand() % xs.size()], o = os[rand() % os.size()];
            s.board[x.first][x.second] = ' ';
            s.board[o.first][o.second] = ' ';
            s.occupied -= 2;
            cout << "One random X and one random O disappeared!\n";
        } else {
            cout << "Nothing to erase this time.\n";
        }
    }
    cout << YELLOW << "═══════════════════════════════════\n" << RESET;
}

// ---------- One full match; returns winner (0, 1) or -1 for a draw ----------
int playMatch() {
    int player = 0;

    while (true) {
        char symbol = SYMBOL[player];
        showBoard(game);
        showScore();

        cout << "\n" << BOLD << "Player " << player + 1 << " (" << symbol << "), your turn!\n" << RESET
             << "Enter a cell number  |  S = Steal  |  U = Undo\n"
             << "Your choice: ";

        string choice;
        if (!(cin >> choice)) { cout << "\nGoodbye!\n"; exit(0); }

        // ----- Undo -----
        if (choice == "U" || choice == "u") {
            if (history.empty()) { cout << RED << "Nothing to undo!\n" << RESET; continue; }
            player = history.back().player;
            history.pop_back();
            game = history.empty() ? State() : history.back().after;
            cout << GREEN << "Last move undone!\n" << RESET;
            continue;
        }

        // ----- Read the cell (normal move or steal) -----
        bool isSteal = (choice == "S" || choice == "s");
        if (isSteal) {
            if (!game.stealLeft[player]) {
                cout << RED << "You have already used your Steal Move!\n" << RESET;
                continue;
            }
            cout << "Enter the cell number you want to steal: ";
            cin >> choice;
        }

        int cell;
        try { cell = stoi(choice); }
        catch (...) { cout << RED << "Invalid input!\n" << RESET; continue; }

        if (cell < 1 || cell > game.size * game.size) {
            cout << RED << "Invalid cell number!\n" << RESET;
            continue;
        }

        char& spot = game.board[(cell - 1) / game.size][(cell - 1) % game.size];

        string error;
        if (isSteal && spot == ' ')         error = "You cannot steal an empty cell!";
        else if (isSteal && spot == symbol) error = "You cannot steal your own cell!";
        else if (!isSteal && spot != ' ')   error = "That cell is already occupied!";
        if (!error.empty()) { cout << RED << error << "\n" << RESET; continue; }

        // ----- Apply the move -----
        spot = symbol;
        if (isSteal) {
            game.stealLeft[player] = false;
            cout << GREEN << "Steal Move successful!\n" << RESET;
        } else {
            game.occupied++;
        }
        game.turns++;

        triggerEvent(game);
        history.push_back({game, player, isSteal});

        // ----- Check the result -----
        if (checkWin(game, 'X')) return 0;
        if (checkWin(game, 'O')) return 1;
        if (game.occupied == game.size * game.size) return -1;

        player = 1 - player;
    }
}

// ---------- Main ----------
int main() {
#ifdef _WIN32
    // Make the Windows console show the box-drawing characters and colors correctly
    SetConsoleOutputCP(CP_UTF8);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode))
        SetConsoleMode(out, mode | 0x0004);   // ENABLE_VIRTUAL_TERMINAL_PROCESSING
#endif

    srand(time(0));
    char again;

    do {
        game = State();
        history.clear();
        eventType = rand() % 3;
        eventTurn = 4 + rand() % 3;

        cout << YELLOW << BOLD
             << "\n╔═══════════════════════════════╗\n"
             << "║          TIC TAC TOE          ║\n"
             << "║       CRAJII EDITION!!!       ║\n"
             << "╚═══════════════════════════════╝\n" << RESET;

        int winner = playMatch();

        showBoard(game);
        if (winner == -1) {
            cout << YELLOW << BOLD << "It's a draw!\n" << RESET;
            draws++;
        } else {
            cout << GREEN << BOLD << "Player " << winner + 1 << " (" << SYMBOL[winner] << ") wins!\n" << RESET;
            wins[winner]++;
        }

        showHistory();
        replayGame();
        showScore();

        cout << "\nPlay another match? (Y/N): ";
        cin >> again;
    } while (again == 'Y' || again == 'y');

    cout << "\nThanks for playing!\n";
    return 0;
}








