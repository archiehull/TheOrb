# Vulkan Crystal Ball Engine

A C++ rendering engine and environmental simulation built with the Vulkan API. This project implements a deferred-style rendering pipeline, a thermodynamic fire propagation system, and dynamic weather cycles within a contained "Crystal Ball" scene.

## Controls

| Key(s) | Action |
| :--- | :--- |
|**Camera** | |
| `[F1]` | Outside Camera |
| `[F2]` | Free Roam Camera |
| `[F3]` | Orbit Camera (Random Cactus) |
| `[F4]` | Ignite Orbit Target |
|**Movement** | |
| `[WASD]` / `[Arrows]` | Move Horizontal |
| `[Q]` / `[PageDown]` | Move Down |
| `[E]` / `[PageUp]` | Move Up |
| `[Shift]` | Sprint |
|**Environment** | |
| `[R]` | Reset Environment |
| `[T]` | Speed Up Time |
| `[T]` + `[Shift]` | Slow Down Time |
| `[T]` + `[Ctrl]` | Normal Time |
|**Dev Toggles** | |
| `[Y]` | Toggle Shading (Phong / Gouraud) |
| `[U]` | Toggle Shadows (Simple / Advanced) |
| `[I]` | Next Season |
| `[O]` | Toggle Weather |
| `[P]` | Spawn Dust Cloud |
| `[Esc]` | Exit Application |



## Features

### Rendering Engine
The rendering backend is modular, with wrappers for Vulkan components located in `src/vulkan/`.

* **Hybrid Shading:** Supports runtime toggling between per-pixel (Phong) and per-vertex (Gouraud) shading.
* **Shadow Mapping:** Implements directional shadow mapping using Percentage-Closer Filtering (PCF) for soft edges.
* **Lighting:** Handles multiple light sources, including orbiting directional lights (Sun/Moon) and dynamic point lights with distance-based attenuation.
* **Refraction Pass:** A dedicated render pass captures the scene to a texture to simulate glass distortion and chromatic aberration for the enclosure.
* **Particle System:** CPU-simulated, GPU-instanced particle rendering for fire, smoke, and precipitation. Supports both additive and alpha-blended pipelines.
* **Post-Processing:** Renders to an offscreen framebuffer before copying to the swapchain, enabling intermediate processing steps.

### Environmental Simulation
The simulation logic in `src/rendering/Scene.cpp` manages object states and environmental factors.

* **Thermodynamics:** Objects track internal temperature based on ambient heat and sun intensity.
* **State Machine:** Objects transition through `NORMAL`, `HEATING`, `BURNING`, `BURNT`, and `REGROWING` states. Burning objects modify their visual appearance via shader push constants.
* **Seasonal Cycles:** Simulates four distinct seasons that affect global temperature and fire ignition thresholds.
* **Weather:** Randomized weather events (rain, snow, dust) impact the simulation state; for example, rain increases fire suppression.
* **Orbital Mechanics:** Sun and Moon positions are calculated using defined orbital radii, speeds, and axis tilts, which update lighting direction in real-time.

## Built-in APIs

The engine exposes a high-level C++ API through the `Scene` class (`src/rendering/Scene.h`), allowing developers to manipulate the environment, spawn entities, and control simulation states programmatically.

### Scene Management
Methods for populating the world with static and dynamic geometry.

* `AddModel(name, pos, rot, scale, modelPath, texPath, isFlammable)`: Loads an OBJ model with a texture and adds it to the scene.
* `AddLight(name, pos, color, intensity, type)`: Creates a new light source. Supports types `0` (Directional/Sun) and `1` (Point/Fire).
* `AddTerrain(name, radius, rings, segments, heightScale, noiseFreq, pos, texPath)`: Generates a procedural terrain mesh based on noise parameters.
* `SetObjectOrbit(...)` / `SetLightOrbit(...)`: Attaches a celestial body or object to a specific orbital path defined by a center point, radius, speed, and axis.

### Particle System API
Helper methods to spawn particle emitters linked to specific locations or objects.

* `AddFire(position, scale)` / `AddSmoke(position, scale)`: Spawns a continuous emitter at the target location. Returns an `emitterId` for future control.
* `AddCampfire(name, position, scale)`: Convenience method that creates a log geometry and immediately attaches fire and smoke emitters to it.
* `SpawnDustCloud()`: Triggers a global, temporary dust storm event.
* `Ignite(SceneObject* obj)`: Triggered by the thermodynamics system, this attaches fire particles to a specific object and initiates its burning state.

### Procedural Generation
The engine includes a registry system for populating the terrain with randomized vegetation or rocks.

* `RegisterProceduralObject(modelPath, texPath, frequency, minScale, maxScale, ...)`: Defines a template for a procedural object, including its rarity (`frequency`) and randomization variance.
* `GenerateProceduralObjects(count, terrainRadius, ...)`: Raycasts against the terrain to place `count` instances of registered objects, ensuring they sit correctly on the surface.

## Tech Stack

* **Language:** C++17
* **Graphics API:** Vulkan
* **Math:** GLM
* **Windowing:** GLFW

## Configuration

Simulation parameters are defined in `src/core/Config.h`.

* **Seasons:** Duration, temperature ranges, and day/night differentials.
* **Orbits:** Axis, speed, and radius for celestial bodies.
* **Procedural Generation:** Rules for flora placement, including density and scale variation.

## Project Structure

```text
src/
├── core/           # Main loop and configuration
├── geometry/       # Mesh generation and loading
├── rendering/      # Render passes, scene logic, and particle systems
├── shaders/        # GLSL shaders
└── vulkan/         # Vulkan API wrappers
```

## Notes

While the core graphics engine components are fairly well abstracted, the majority of the game logic exists in *Scene*.

Future version should prioritise decoupling and abstracting the *Scene* class and *SceneObject*.

### To Do:
- Refactor and decouple `Scene` into an Entity-Component System (ECS)
  - Extract `Transform`, `Rendering`, `Physics`, `Thermodynamics`, and `Orbital` components
  - Migrate existing scene objects to ECS entities
  - Update loaders/serialisation to support ECS
- Refactor scene object responsibilities
  - Ensure `Transform` handles spatial data only
  - Ensure `Rendering` handles mesh/material data only
  - Ensure `Physics` handles collisions/forces
  - Ensure `Thermodynamics` handles temperature and fire logic
  - Ensure `Orbital` handles orbit/orientation updates
- Add specific pass members for the renderer
  - Define explicit passes: `ShadowPass`, `GeometryPass`, `LightingPass`, `SkyboxPass`, `PostProcessPass`
  - Make passes configurable and reorderable
- Implement an `InputManager` class
  - Centralise input state, mappings, and action contexts
  - Support key, mouse, and gamepad; provide file-based remapping
- Create a debug system with console output and ImGui integration
  - Runtime commands and variable inspection
  - On-screen overlays: FPS, camera position, scene stats
  - Hot-reload hooks for shaders and assets
- Implement an audio engine
  - 3D spatialized audio, event system, and volume controls
- Implement wind and fire spread simulation
  - Integrate with `Thermodynamics`
  - Propagation model and visual particle coupling
- Improve material and texture support
  - Bump mapping
  - Displacement mapping
- Implement a deferred rendering pipeline (MRT)
- Implement High Dynamic Range (HDR) rendering and tonemapping
- Add illuminating sparks and advanced particle effects (extend `ParticleLibrary`)
- Explore ray tracing (RTX / hybrid) for improved lighting and reflections
- Explore settling snow and puddles with reflection