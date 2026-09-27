# Fly Brain Simulator — UE 5.8 CPU Brain Core

1,000 LIF neurons / 16,000 directed synapses / fixed 1 ms timestep / C++20.
Neuron state is stored in contiguous C++ arrays, with a CSR graph. No neuron Actors or UObjects.

## Blueprint

1. Build `FlyBrainSimulatorEditor` (Development / Win64), then open the project.
2. In a Level Blueprint or Widget with a game-world context, use **Get World Subsystem** and select **BrainSimulationSubsystem**.
3. Call **Start**, **Stop**, or **Reset** on the returned subsystem.

The subsystem starts stopped. Start resumes; Stop freezes state and backlog; Reset stops and restores the initial state while keeping the topology. Each PIE/game world has its own brain. Editor preview worlds do not run the simulation.

Blueprint getters expose neuron/synapse count, step count, simulation time, backlog, and core storage bytes. C++ consumers can inspect read-only voltage/spike spans through `GetCore()`. Spikes represent the most recently completed step. Do not retain spans across reinitialization or access the core from another thread.

## Validation

From an x64 Visual Studio Developer Command Prompt, in the repository root:

```bat
cmake -S Tests/BrainCore -B Tests/BrainCore/Intermediate -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/BrainCore/Intermediate
ctest --test-dir Tests/BrainCore/Intermediate -V
```

UE Automation test: `FlyBrain.Subsystem.Lifecycle` (Session Frontend / Automation). This covers subsystem creation, Blueprint function exposure, Start/Stop/Reset and replay.

See [architecture](docs/ARCHITECTURE.md) and [validation record](docs/VALIDATION.md).
