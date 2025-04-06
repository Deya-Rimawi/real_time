# Real-Time Tug-of-War Game

A real-time, process-based tug-of-war simulation built in C using OpenGL, pipes, and shared memory.

Each player is a separate process pulling on a rope. Their energy, effort, and state (falling or not) are visualized in real-time using an OpenGL viewer.

---

## 🛠️ Requirements

Make sure the following are installed:

- `gcc`
- `make`
- `libgl1-mesa-dev`
- `freeglut3-dev`

On Ubuntu/Debian:

```bash
sudo apt install build-essential libgl1-mesa-dev freeglut3-dev
