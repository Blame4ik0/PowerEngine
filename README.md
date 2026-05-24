# PowerEngine
Small 3D engine with big plans
# ⚡ PowerEngine

> A custom-built, high-performance game engine written in C++20 — built from scratch with full control over every system.

PowerEngine is a personal game engine project targeting high-realism graphics, a fully custom physics simulation, an integrated Qt-based scene editor, and eventually its own scripting language. The goal is a complete, modern engine comparable in scope to commercial engines like Unity or Unreal — with no black boxes.

---

## Features

### 🖥️ Renderer
- DirectX 11 backend (DirectX 12 + ray tracing planned)
- Physically-Based Rendering (PBR) — albedo, roughness, metallic workflows
- Dynamic lighting — directional, point, and spot lights
- Shadow mapping with PCF filtering and cascaded shadow maps
- Full post-processing pipeline — bloom, SSAO, TAA, motion blur
- AI upscaling via DLSS / FSR integration *(planned)*
- Both 2D and 3D rendering supported

### 🎮 Input
- Keyboard & mouse with raw input support
- Gamepad support via XInput — full analog sticks, triggers, deadzone handling
- Clean abstraction layer — engine code never touches raw OS events directly

### 🧱 Scene System & Editor
- Entity Component System (ECS) architecture
- Custom binary scene file format
- Qt-based scene editor with embedded DX11 viewport, scene tree, and property panel

### ⚙️ Physics Engine
- Custom-built from scratch — no third-party physics library
- Broad phase collision with BVH trees
- Narrow phase with GJK / EPA algorithms
- Rigid body dynamics with RK4 integration
- Clean API — `.move()`, `.rotate()`, `.scale()`, `.applyForce()` and more
- 4D physics exploration *(experimental, long-term)*

### 🔊 Audio
- XAudio2 backend
- Full 3D positional / surround sound
- Music streaming
- Audio scripting support

### 🎞️ Animation
- Skeletal animation with full bone hierarchy
- Inverse Kinematics (IK)
- Blend trees and animation state machines

### 📜 Scripting *(planned)*
- Custom scripting language with a built-in compiler
- Transpiles to C++ — no interpreter overhead
- Designed so engine users never need to manually manage C++ libraries

---

## Roadmap

| Phase | System | Status |
|-------|--------|--------|
| 1 | Foundation — window, DX11 context, core loop | ✅ Done |
| 2 | Input system — keyboard, mouse, gamepad | ✅ Done |
| 3 | 2D renderer — sprites, batching, camera | 🔨 In progress |
| 4 | 3D renderer — meshes, PBR, lighting, shadows | ⏳ Planned |
| 5 | Scene system & Qt editor | ⏳ Planned |
| 6 | Physics engine | ⏳ Planned |
| 7 | Audio & animation | ⏳ Planned |
| 8 | DX12, ray tracing, AI upscaling, scripting language | ⏳ Planned |

---

## Tech Stack

| | |
|---|---|
| **Language** | C++20 |
| **Build system** | CMake |
| **Graphics API** | DirectX 11 → DirectX 12 |
| **Windowing** | SDL2 |
| **Editor UI** | Qt |
| **Audio** | XAudio2 |
| **Input** | XInput + Raw Input |
| **Package manager** | vcpkg |

---

> This project is in active early development. Architecture and APIs will change frequently.
