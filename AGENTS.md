# Fly Brain Simulator UE5

## Environment
- Unreal Engine 5.8
- Windows 11
- C++20
- Target GPU: RTX 2060 6GB
- Target CPU: Intel Core i7-9750H
- RAM: 16GB

## Goals
Build a real-time fly brain simulation in Unreal Engine.

Initial milestones:
1. 1,000 neurons
2. 10,000 neurons
3. 50,000 neurons
4. 100,000 neurons
5. 139,255 neurons

## Architecture
- Do not create one Actor or UObject per neuron.
- Neurons must use contiguous C++ arrays.
- Synapses should use a compact graph representation such as CSR.
- Simulation must be independent from rendering.
- Rendering FPS and brain simulation timestep must be separated.
- Start with CPU implementation.
- GPU Compute Shader implementation comes later.

## Unreal Rules
- Prefer C++ for simulation code.
- Blueprint is allowed for UI and simple game logic.
- Do not enable Lumen or Ray Tracing unless required.
- Avoid unnecessary Tick() functions.
- Do not commit Binaries, Intermediate, Saved, or DerivedDataCache.

## Validation
Every performance change must preserve simulation results.

Record:
- neuron count
- synapse count
- brain steps/sec
- Game Thread ms
- Render Thread ms
- GPU ms
- RAM
- VRAM
