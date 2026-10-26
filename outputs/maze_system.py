"""Terminal DSA maze game: DFS generation, BFS hints, and A* enemy chase."""

from collections import deque
import heapq
import random


# Directions use row/column offsets; each cell's walls use the same order.
DIRECTIONS = ((-1, 0), (0, 1), (1, 0), (0, -1))
NORTH, EAST, SOUTH, WEST = range(4)


def generate_maze(size, rng):
    """Generate a perfect maze with iterative DFS recursive backtracking."""
    walls = [[[True] * 4 for _ in range(size)] for _ in range(size)]
    visited = [[False] * size for _ in range(size)]
    stack = [(0, 0)]
    visited[0][0] = True

    while stack:
        row, col = stack[-1]
        choices = []
        for direction, (dr, dc) in enumerate(DIRECTIONS):
            nr, nc = row + dr, col + dc
            if 0 <= nr < size and 0 <= nc < size and not visited[nr][nc]:
                choices.append((direction, nr, nc))

        if not choices:
            stack.pop()
            continue

        direction, nr, nc = rng.choice(choices)
        walls[row][col][direction] = False
        walls[nr][nc][(direction + 2) % 4] = False
        visited[nr][nc] = True
        stack.append((nr, nc))

    return walls


def neighbors(walls, cell):
    row, col = cell
    size = len(walls)
    for direction, (dr, dc) in enumerate(DIRECTIONS):
        nr, nc = row + dr, col + dc
        if 0 <= nr < size and 0 <= nc < size and not walls[row][col][direction]:
            yield nr, nc


def bfs(walls, start, goal):
    """Return a shortest path and the number of dequeued cells."""
    queue = deque([start])
    parent = {start: None}
    explored = 0

    while queue:
        cell = queue.popleft()
        explored += 1
        if cell == goal:
            break
        for nxt in neighbors(walls, cell):
            if nxt not in parent:
                parent[nxt] = cell
                queue.append(nxt)

    if goal not in parent:
        return [], explored

    path = []
    cell = goal
    while cell is not None:
        path.append(cell)
        cell = parent[cell]
    path.reverse()
    return path, explored


def astar(walls, start, goal):
    """Return an optimal path using Manhattan distance as the heuristic."""
    def heuristic(cell):
        return abs(cell[0] - goal[0]) + abs(cell[1] - goal[1])

    frontier = [(heuristic(start), 0, start)]
    parent = {start: None}
    cost = {start: 0}
    closed = set()
    explored = 0

    while frontier:
        _, current_cost, cell = heapq.heappop(frontier)
        if cell in closed:
            continue
        closed.add(cell)
        explored += 1
        if cell == goal:
            break

        for nxt in neighbors(walls, cell):
            new_cost = current_cost + 1
            if new_cost < cost.get(nxt, float("inf")):
                cost[nxt] = new_cost
                parent[nxt] = cell
                heapq.heappush(frontier, (new_cost + heuristic(nxt), new_cost, nxt))

    if goal not in closed:
        return [], explored

    path = []
    cell = goal
    while cell is not None:
        path.append(cell)
        cell = parent[cell]
    path.reverse()
    return path, explored


def draw_maze(walls, player, enemy, hint=None):
    """Print an ASCII maze; this is the game's text interface, not a renderer."""
    size = len(walls)
    hint = set(hint or [])
    print()
    print("+" + "---+" * size)
    for row in range(size):
        line = "|"
        for col in range(size):
            cell = (row, col)
            if cell == player:
                icon = "P"
            elif cell == enemy:
                icon = "E"
            elif cell == (size - 1, size - 1):
                icon = "G"
            elif cell in hint:
                icon = "."
            else:
                icon = " "
            line += f" {icon} " + ("|" if walls[row][col][EAST] else " ")
        print(line)
        bottom = "+"
        for col in range(size):
            bottom += ("---" if walls[row][col][SOUTH] else "   ") + "+"
        print(bottom)
    print("P = player   E = enemy   G = exit   . = BFS hint")


def choose_difficulty():
    levels = {
        "1": (7, 4, "Easy"),
        "2": (9, 3, "Medium"),
        "3": (12, 2, "Hard"),
    }
    print("\nDIFFICULTY")
    print("1. Easy   (7x7 maze; enemy moves once every 4 player moves)")
    print("2. Medium (9x9 maze; enemy moves once every 3 player moves)")
    print("3. Hard   (12x12 maze; enemy moves once every 2 player moves)")
    while True:
        choice = input("Choose 1, 2, or 3 [default 1]: ").strip() or "1"
        if choice in levels:
            return levels[choice]
        print("Please enter 1, 2, or 3.")


def play_round(size, enemy_interval, difficulty, rng):
    walls = generate_maze(size, rng)
    player = (0, 0)
    enemy = (0, size - 1)
    goal = (size - 1, size - 1)
    turns = 0
    hint_path = []

    # A generated maze should always be connected; verify it with BFS.
    valid_path, visited = bfs(walls, player, goal)
    if not valid_path:
        print("Maze generation failed its BFS solvability check.")
        return False

    print(f"\n{difficulty} maze ready. DFS carved it; BFS checked {visited} cells.")
    print("Reach G. Controls: W/A/S/D move, H hint, M map, Q quit.")

    while True:
        draw_maze(walls, player, enemy, hint_path)
        command = input("Move [W/A/S/D, H, M, Q]: ").strip().lower()

        if command == "q":
            print("You left the maze.")
            return False
        if command == "m":
            continue
        if command == "h":
            hint_path, explored = bfs(walls, player, goal)
            print(f"BFS explored {explored} cells. The shortest route is "
                  f"{max(0, len(hint_path) - 1)} steps; dots show that route.")
            continue

        direction_by_key = {"w": NORTH, "d": EAST, "s": SOUTH, "a": WEST}
        if command not in direction_by_key:
            print("Use W/A/S/D to move, H for a hint, M to redraw, or Q to quit.")
            continue

        direction = direction_by_key[command]
        dr, dc = DIRECTIONS[direction]
        row, col = player
        nxt = (row + dr, col + dc)
        if nxt not in neighbors(walls, player):
            print("A wall blocks that way.")
            continue

        player = nxt
        turns += 1
        hint_path = []
        if player == goal:
            draw_maze(walls, player, enemy)
            print(f"You escaped the {difficulty.lower()} maze in {turns} moves!")
            return True

        if turns % enemy_interval == 0:
            chase_path, explored = astar(walls, enemy, player)
            print(f"A* explored {explored} cells.")
            if len(chase_path) > 1:
                enemy = chase_path[1]
            if enemy == player:
                draw_maze(walls, player, enemy)
                print(f"The enemy caught you after {turns} moves.")
                return False


def main():
    rng = random.Random()
    print("=" * 42)
    print("       DSA MAZE ESCAPE: TERMINAL EDITION")
    print("=" * 42)

    while True:
        size, enemy_interval, difficulty = choose_difficulty()
        won = play_round(size, enemy_interval, difficulty, rng)
        if input("Play another maze? [y/N]: ").strip().lower() != "y":
            print("Thanks for playing!")
            break


if __name__ == "__main__":
    main()
