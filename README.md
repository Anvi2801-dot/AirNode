# AirNode ✋🖱️

**Control your computer with just your hand — no mouse required.**

AirNode is a gesture-controlled virtual mouse that uses real-time hand landmark detection to translate hand movements and finger gestures into mouse actions. It combines a high-performance C++/CUDA backend with a WebSocket server and a lightweight web frontend for live visualization and control.

---

## ✨ Features

- 🖐️ Real-time hand tracking via [MediaPipe](https://mediapipe.dev/) (single & double hand support)
- 🖱️ Gesture-to-mouse mapping — move, click, scroll, and drag with your hand
- ⚡ GPU-accelerated inference via Apple's Metal/Neural Engine for low-latency processing
- 🌐 WebSocket server for real-time communication between the backend and frontend
- 🎨 Web-based frontend for live hand landmark visualization
- 🔧 Supports both Bazel and CMake builds

---

## 🗂️ Project Structure

```
AirNode/
├── assets/          # Static assets (icons, images)
├── cuda/            # GPU-accelerated processing (legacy/experimental)
├── frontend/        # Web frontend (HTML/JS visualization)
├── include/         # C++ header files
├── rocm/            # ROCm support (legacy/experimental)
├── src/             # Core C++ source code
├── third_party/     # External dependencies
├── CMakeLists.txt   # CMake build config
├── MODULE.bazel     # Bazel module definition
├── WORKSPACE        # Bazel workspace
└── mediapipe-v0.10.14.tar.gz  # Bundled MediaPipe source
```

---

## 🛠️ Prerequisites

- **OS:** macOS (Apple Silicon — M1/M2/M3 recommended)
- **Dependencies:**
  - CMake 3.15+ or [Bazel](https://bazel.build/) (see `.bazelversion` for required version)
  - [Homebrew](https://brew.sh/) for installing dependencies
  - Python 3 (for serving the frontend)
  - A webcam or the built-in FaceTime camera

---

## 🚀 Getting Started

### 1. Clone the repository

```bash
git clone https://github.com/Anvi2801-dot/AirNode.git
cd AirNode
```

### 2. Build

**With CMake** (recommended):
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

**With Bazel:**
```bash
bazel build //src:airnode
```

### 3. Run AirNode

**With CMake:**
```bash
./build/airnode
```

**With Bazel:**
```bash
bazel run //src:airnode
```

---

## 🌐 Viewing the Frontend

Once the backend is running, serve the frontend with Python's built-in server:

```bash
cd frontend
python3 -m http.server 8080
```

Then open your browser and go to:

```
http://localhost:8080
```

The frontend will display live hand landmark visualization.

---

## 🖐️ Gesture Controls

| Gesture | Action |
|---|---|
| Index fingertip position | Move cursor |
| Pinch (thumb + index tip) | Left click |
| Closed fist | Pause/stop tracking |
| Three fingers swipe (index + middle + ring) | Swipe left/right |
| Second hand: thumb + middle pinch | Right click |
| Two hands: vertical wrist distance | Scroll up/down |
| Two hands: index tips apart/together | Pinch zoom |
