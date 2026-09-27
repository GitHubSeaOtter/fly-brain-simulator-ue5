# CPU Brain Core

`BrainCore` is plain C++20 and has no Unreal or rendering dependency. All state is owned by one thread. Separate `std::vector` arrays hold membrane potentials, pending synaptic inputs, refractory counters and spike flags. The CSR graph holds source row offsets, target indices and weights. No allocation occurs during Step or Advance.

Default parameters: 1,000 neurons, 16 outgoing edges each, seed 1, timestep 0.001 s, membrane time constant 0.020 s, resting/reset potential -65 mV, threshold -50 mV, constant R*I drive 20 mV, excitatory weight 0.1 mV, and 2 refractory steps. This is a synthetic smoke-test network, not a biological connectome. All neurons start at rest under identical drive; synchronized activity is expected.

## Numerical contract

Forward Euler: `V += (dt / tau) * (Vrest - V + drive) + pendingInput`.

At threshold, record a spike, set the reset potential, and enter refractory time. During refractory time, hold reset potential and discard incoming impulses. After integrating every neuron, accumulate outgoing weights into next-step inputs in ascending source/CSR-edge order. Thus all edges have one step of delay, and source index cannot cause same-step feedback.

Topology uses a specified 32-bit LCG and a cyclic selection of distinct non-self targets; it is deterministic but is not a uniformly sampled random graph. Graph generation happens only at initialization. Reset preserves graph/configuration and clears every dynamic array, the step counter and elapsed-time backlog.

Determinism is for the same configuration, seed, step count, executable and floating-point environment. Precise floating-point compilation is selected for the module and standalone MSVC tests. Bitwise equality across different CPUs/compilers/builds is not promised. `StateHash` covers voltage, queued input, refractory state, spikes and step count; it excludes wall-clock backlog and is meaningful only with the same graph/configuration.

## Unreal scheduling

One `UTickableWorldSubsystem` owns the entire brain per Game/PIE world. It only ticks when running. The Game Thread feeds elapsed **game time** to an accumulator; each brain step always uses the configured dt. Game pause/time dilation therefore affects elapsed simulation time. Render FPS never changes the integration timestep.

Advance performs at most 256 steps per frame and retains excess backlog, avoiding unbounded work during a hitch without silently discarding steps. A persistent overload makes simulation time lag behind game time; inspect GetBacklogSeconds. Floating-point frame durations can move a step across a frame boundary; compare deterministic state at equal integer step counts, not merely at nominal wall-clock times. Exact offline stepping is available via Step().

Start is idempotent and resumes existing state/backlog. Stop is idempotent and freezes them. Reset stops, clears elapsed simulation time and restores initial state. Frame data while stopped is not accumulated. Rendering can later consume read-only snapshots/spans without changing the numerical core; worker-thread/GPU execution will require explicit ownership and synchronization.
