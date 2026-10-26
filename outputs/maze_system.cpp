#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cmath>
#include <iostream>
#include <queue>
#include <random>
#include <string>
#include <vector>

using namespace std;

struct Cell {
    // North, east, south, west walls.
    array<bool, 4> wall{{true, true, true, true}};
};

struct SearchResult {
    vector<pair<int, int>> path;
    int explored = 0;
};

class MazeGame {
private:
    int n;
    vector<vector<Cell>> maze;
    pair<int, int> player{0, 0};
    pair<int, int> enemy{0, 0};
    bool finished = false;
    int turns = 0;
    int enemyDelay = 1;
    mt19937 rng{static_cast<unsigned int>(chrono::steady_clock::now().time_since_epoch().count())};
    const array<pair<int, int>, 4> delta{{{-1, 0}, {0, 1}, {1, 0}, {0, -1}}};

    bool inside(int r, int c) const {
        return r >= 0 && c >= 0 && r < n && c < n;
    }

    bool canStep(int r, int c, int nr, int nc) const {
        if (!inside(nr, nc)) return false;
        int direction = nr < r ? 0 : nc > c ? 1 : nr > r ? 2 : 3;
        return !maze[r][c].wall[direction];
    }

    // Recursive-backtracker equivalent, implemented with an explicit stack
    // to support large mazes without risking call-stack exhaustion.
    void generateMaze() {
        maze.assign(n, vector<Cell>(n));
        vector<vector<bool>> visited(n, vector<bool>(n, false));
        vector<pair<int, int>> stack{{0, 0}};
        visited[0][0] = true;

        while (!stack.empty()) {
            auto [r, c] = stack.back();
            vector<int> options;
            for (int d = 0; d < 4; ++d) {
                int nr = r + delta[d].first;
                int nc = c + delta[d].second;
                if (inside(nr, nc) && !visited[nr][nc]) options.push_back(d);
            }

            if (options.empty()) {
                stack.pop_back();
                continue;
            }

            uniform_int_distribution<int> pick(0, static_cast<int>(options.size()) - 1);
            int d = options[pick(rng)];
            int nr = r + delta[d].first;
            int nc = c + delta[d].second;
            maze[r][c].wall[d] = false;
            maze[nr][nc].wall[(d + 2) % 4] = false;
            visited[nr][nc] = true;
            stack.push_back({nr, nc});
        }
    }

    SearchResult bfs(pair<int, int> start, pair<int, int> goal) const {
        vector<vector<pair<int, int>>> parent(n, vector<pair<int, int>>(n, {-1, -1}));
        vector<vector<bool>> seen(n, vector<bool>(n, false));
        queue<pair<int, int>> q;
        q.push(start);
        seen[start.first][start.second] = true;
        int explored = 0;

        while (!q.empty()) {
            auto [r, c] = q.front(); q.pop();
            ++explored;
            if (make_pair(r, c) == goal) break;
            for (int d = 0; d < 4; ++d) {
                int nr = r + delta[d].first;
                int nc = c + delta[d].second;
                if (canStep(r, c, nr, nc) && !seen[nr][nc]) {
                    seen[nr][nc] = true;
                    parent[nr][nc] = {r, c};
                    q.push({nr, nc});
                }
            }
        }

        SearchResult result;
        result.explored = explored;
        if (!seen[goal.first][goal.second]) return result;
        for (pair<int, int> at = goal; at.first != -1; at = parent[at.first][at.second])
            result.path.push_back(at);
        reverse(result.path.begin(), result.path.end());
        return result;
    }

    SearchResult astar(pair<int, int> start, pair<int, int> goal) const {
        struct Entry { int f, g, r, c; };
        auto cmp = [](const Entry& a, const Entry& b) { return a.f > b.f; };
        priority_queue<Entry, vector<Entry>, decltype(cmp)> open(cmp);
        const int inf = 1'000'000;
        vector<vector<int>> best(n, vector<int>(n, inf));
        vector<vector<pair<int, int>>> parent(n, vector<pair<int, int>>(n, {-1, -1}));
        vector<vector<bool>> closed(n, vector<bool>(n, false));
        auto h = [&](int r, int c) { return abs(r - goal.first) + abs(c - goal.second); };
        best[start.first][start.second] = 0;
        open.push({h(start.first, start.second), 0, start.first, start.second});

        SearchResult result;
        while (!open.empty()) {
            Entry cur = open.top(); open.pop();
            if (closed[cur.r][cur.c]) continue;
            closed[cur.r][cur.c] = true;
            ++result.explored;
            if (make_pair(cur.r, cur.c) == goal) break;

            for (int d = 0; d < 4; ++d) {
                int nr = cur.r + delta[d].first;
                int nc = cur.c + delta[d].second;
                if (!canStep(cur.r, cur.c, nr, nc) || closed[nr][nc]) continue;
                int candidate = cur.g + 1;
                if (candidate < best[nr][nc]) {
                    best[nr][nc] = candidate;
                    parent[nr][nc] = {cur.r, cur.c};
                    open.push({candidate + h(nr, nc), candidate, nr, nc});
                }
            }
        }

        if (!closed[goal.first][goal.second]) return result;
        for (pair<int, int> at = goal; at.first != -1; at = parent[at.first][at.second])
            result.path.push_back(at);
        reverse(result.path.begin(), result.path.end());
        return result;
    }

    void draw(const vector<pair<int, int>>& hintPath = {}) const {
        vector<vector<bool>> isHint(n, vector<bool>(n, false));
        for (auto [r, c] : hintPath) isHint[r][c] = true;

        cout << "\n";
        for (int c = 0; c < n; ++c) cout << "+---";
        cout << "+\n";
        for (int r = 0; r < n; ++r) {
            cout << '|';
            for (int c = 0; c < n; ++c) {
                char icon = ' ';
                if (player == make_pair(r, c) && enemy == player) icon = 'X';
                else if (player == make_pair(r, c)) icon = 'P';
                else if (enemy == make_pair(r, c)) icon = 'E';
                else if (r == n - 1 && c == n - 1) icon = 'G';
                else if (isHint[r][c]) icon = '.';
                cout << ' ' << icon << ' ';
                cout << (maze[r][c].wall[1] ? '|' : ' ');
            }
            cout << "\n";
            for (int c = 0; c < n; ++c)
                cout << '+' << (maze[r][c].wall[2] ? "---" : "   ");
            cout << "+\n";
        }
        cout << "P = You   E = Enemy   G = Goal   . = BFS hint path\n";
    }

public:
    MazeGame(int size, int delay) : n(size), enemyDelay(delay) {}

    void run() {
        generateMaze();
        player = {0, 0};
        enemy = {0, n - 1};
        auto solvability = bfs(player, {n - 1, n - 1});
        if (solvability.path.empty()) {
            cout << "Maze generation failed to produce a solvable maze.\n";
            return;
        }

        cout << "\n=== DSA MAZE ESCAPE ===\n"
             << "DFS generated the maze; BFS verified a route exists.\n"
             << "Reach G at the bottom-right. The enemy uses A* to chase you.\n";

        while (!finished) {
            draw();
            cout << "Turns: " << turns << " | Move [W/A/S/D], hint [H], quit [Q]: ";
            string input;
            if (!getline(cin, input) || input.empty()) continue;
            char command = static_cast<char>(tolower(input[0]));

            if (command == 'q') {
                cout << "Run ended.\n";
                break;
            }
            if (command == 'h') {
                SearchResult hint = bfs(player, {n - 1, n - 1});
                cout << "BFS explored " << hint.explored << " cells; shortest route is "
                     << (hint.path.empty() ? 0 : hint.path.size() - 1) << " steps.\n";
                draw(hint.path);
                continue;
            }

            int direction = -1;
            if (command == 'w') direction = 0;
            if (command == 'd') direction = 1;
            if (command == 's') direction = 2;
            if (command == 'a') direction = 3;
            if (direction < 0) {
                cout << "Unknown command. Use W/A/S/D, H, or Q.\n";
                continue;
            }

            int nr = player.first + delta[direction].first;
            int nc = player.second + delta[direction].second;
            if (!canStep(player.first, player.second, nr, nc)) {
                cout << "There is a wall in that direction.\n";
                continue;
            }

            player = {nr, nc};
            ++turns;
            if (player == make_pair(n - 1, n - 1)) {
                draw();
                cout << "You escaped in " << turns << " turns!\n";
                finished = true;
                continue;
            }

            if (turns % enemyDelay == 0) {
                SearchResult chase = astar(enemy, player);
                cout << "A* explored " << chase.explored << " cells.\n";
                if (chase.path.size() > 1) enemy = chase.path[1];
                if (enemy == player) {
                    draw();
                    cout << "The enemy caught you after " << turns << " turns.\n";
                    finished = true;
                }
            }
        }
    }
};

int main() {
    cout << "Choose difficulty: 1) Easy  2) Medium  3) Hard\nChoice: ";
    string input;
    getline(cin, input);
    int choice = input.empty() ? 1 : input[0] - '0';

    int size = 9;
    int enemyDelay = 3;
    if (choice == 2) { size = 13; enemyDelay = 2; }
    else if (choice == 3) { size = 17; enemyDelay = 1; }

    MazeGame game(size, enemyDelay);
    game.run();
    return 0;
}
