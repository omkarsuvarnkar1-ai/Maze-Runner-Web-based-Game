# Maze-Runner-Web-based-Game

A browser-based maze game and DSA practical project demonstrating DFS maze generation, BFS route checking and hints, and A* enemy pathfinding.

## Browser game

Open `outputs/maze-runner-3d.html` through a local web server. From the `outputs` folder, run:

```powershell
python -m http.server 8000
```

Then open `http://localhost:8000/maze-runner-3d.html`. The 3D version loads Three.js from a CDN, so an internet connection is needed. It supports keyboard and touch controls.

Other browser builds are in `outputs/index.html` and `outputs/maze-runner.html`.

## Terminal versions

- `outputs/maze_system.cpp` — C++ implementation
- `outputs/maze_system.py` — Python implementation

## Algorithms

- **DFS** generates the maze.
- **BFS** verifies a route and computes a shortest-path hint.
- **A\*** finds a route for the enemy to chase the player.
