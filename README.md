# 🐍 Snake Game in C

A classic Snake game built in C with ncurses, featuring colorful graphics, obstacles, persistent high scores, and smooth real-time gameplay.

## ✨ Features

- 🎮 Classic snake gameplay with smooth controls
- 💰 Rare golden food ($ symbol) worth 50 points
- 🚧 Random obstacles that spawn each game
- 🏆 Persistent high score saved to `~/.snake_highscore`
- 🔄 Wraparound walls (snake teleports instead of dying)
- 🎨 Beautiful colored graphics with ncurses
- ⏸️ Pause functionality (press P)
- 📈 Dynamic difficulty - game speeds up as you score
- 📊 Real-time score and high score display
- 🎯 Adaptive to any terminal window size

## 🚀 How to Build & Run

```bash
# Install ncurses (if needed)
# Fedora: sudo dnf install ncurses-devel
# Ubuntu: sudo apt install libncurses-dev
```

## Compile the game
```
gcc snake.c -o snake -lncurses
```
## Run the game
```
./snake
```
## 🎮 Controls
```
Key        Action
W / ↑      Move Up
S / ↓      Move Down
A / ←      Move Left
D / →      Move Right
P          Pause/Unpause
Q          Quit Game
```
## 🎯 Game Elements
```
O - Snake head (green)
o - Snake body (green)
* Regular food (red) - +10 points
$ - Golden food (yellow) - +50 points, grows snake by 3
# - Walls (cyan) - Snake wraps around
% - Obstacles (magenta) - Instant death on collision
```
## 🛠️ Technologies
```
Language: C (C99 standard)
Library: ncurses (terminal UI)
Compiler: GCC
```
## 📝 License
MIT License - feel free to use and modify!
