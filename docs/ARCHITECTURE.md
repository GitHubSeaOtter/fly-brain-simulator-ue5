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

## Interactive sample

`AFlyBrainDemoGameMode` builds a small stage, two lights, camera and one `AFlyBrainDemoFly` in the dedicated demo map. Fly geometry is made of primitive mesh components, not neuron Actors. `AFlyBrainDemoController` owns the Slate dashboard and removes it at EndPlay; Start/Stop/Reset are routed to the existing subsystem. UI metrics sample at 4 Hz; the membrane-potential field reads the 1,000-element voltage span on the Game Thread. Unreal's viewport DPI scaling is applied once.

The fly's orbit is a function of completed simulation time. Additional yaw, lift and wing amplitude read the experiment's smoothed motor activity. Visual frame timing does not feed back into brain state. The pose mapping is illustrative, not physical flight control. Each mesh component supplies persistent custom primitive color data to a single shared material. Rendering uses conventional rasterization with FXAA, without Lumen or ray tracing.

The core now counts cumulative spikes for observation; Reset clears that counter. This counter does not participate in integration or StateHash. The previous 10,000-step numerical-state hash is unchanged. The subsystem measures the elapsed duration of each Advance call for the UI's brain-update cost. These are observation-only additions.

## Stimulus and conditioning experiment

The UE-independent `ConditioningExperiment` uses 1,000 LIF cells: cue A [0,250), cue B [250,500), motor A [500,750), motor B [750,1000). Each sensory cell projects to 16 distinct cells in its corresponding motor population; motor cells have no outputs. CSR therefore holds 8,000 edges, initially 0.12 mV. This fixed feed-forward circuit is separate from the original 16,000-edge random baseline. Motor resting drive is 10 mV (subthreshold), sensory resting drive is zero.

Core supports validated external drives and CSR replacement. Hooks run immediately before/after each **fixed** step, not once per rendered frame. A cue injects 60 mV of R*I into its sensory group for 400 steps. A pairing also injects 32 mV into motor A as an unconditioned teaching input; this drive replaces its ordinary 10 mV bias. Each 600-step trial has 400 cue steps followed by 200 silent steps.

Plasticity is a synthetic three-factor Hebbian rule. Each sensory trace updates `trace = 0.95 * trace + spike`. When learning and reward are both enabled during a pairing, every sensory-to-motor synapse whose postsynaptic cell spikes updates `w = min(1.5, w + 0.0005 * trace_pre)`. Inactive B traces remain zero. All loops have fixed order and operate on contiguous storage. No renderer, random sampling or wall time participates in learning. This is a bounded potentiation demonstration, not a fitted biological model or general reinforcement-learning agent.

COMPARE discards old weights, measures A and B, runs six A pairings, then probes A and B again. All probes disable teaching input and plasticity and start from identical resting voltages, empty pending inputs, refractory state and traces. Response is target motor spikes / (250 cells * 0.4 simulated seconds). Only weights carry between trials. Results become valid only after trial completion; interrupted runs do not fabricate a score. Learning/reward toggles are frozen while a trial is active; changing them afterward invalidates old comparisons.

Standalone manual training retains weights. RESET STATE clears transient state and measurements but retains weights; FORGET recreates initial weights and clears training count. Comparison starts fresh independently of those choices. New commands reset the protocol clock; adjacent trials preserve the running clock. Space pauses/resumes without advancing the protocol. No state is persisted to disk.

Activity bars use `activity = 0.98 * activity + 0.02 * (spikes_in_population / 250 * 10)` each millisecond, i.e. roughly 50 ms smoothing and a full-scale reference of 100 Hz. They are observers and do not alter the core. `StateHash` remains the legacy dynamic-state checksum; learned weights and external inputs are deliberately checked separately when validating experimental replay. Core storage now includes an external-current array (8,000 bytes), and the experiment owns a 500-double trace array in addition to core storage.
