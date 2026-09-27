# CPU baseline

Date: 2026-09-28. Intel Core i7-9750H, 16 GB RAM. Standalone x64 Release, MSVC 19.51, precise floating point. Measured without rendering; this is throughput of the isolated core, not in-game performance.

| Metric | Result |
| --- | --- |
| Neurons | 1,000 |
| Synapses | 16,000 |
| Brain steps/sec | 260,907 initial / 282,376 final (10,000-step runs; machine/load dependent) |
| Game Thread ms | Not measured; standalone executable |
| Render Thread ms | Not measured; no renderer |
| GPU ms | Not measured; CPU-only test |
| RAM | Core array capacity: 217,004 bytes; whole-process RAM not measured |
| VRAM | Core allocates no GPU resources; whole-process VRAM not measured |
| State hash at step 10,000 | 16884003706165038808 |

Tests cover CSR shape/bounds/unique non-self edges, seeded topology, per-step replay, Reset replay, analytic isolated LIF voltage/threshold, refractory duration, next-step synaptic delivery, refractory input discard, frame partition independence, catch-up limits without dropping steps, invalid elapsed time and invalid configuration. The benchmark checks final-state equality against a second run.

## Unreal validation

- UE 5.8.3, Development Editor / Win64, MSVC 14.50: game module compilation and DLL linking succeeded.
- Headless UE Automation: `FlyBrain.Subsystem.Lifecycle` passed, one test performed, process exit code 0. Verifies automatic world subsystem creation, BlueprintCallable reflection flags, Start/Stop/resume/Reset, and identical state after replay.
- Initial sandboxed UBT invocation crashed in dotnet before emitting a build log. Running outside the sandbox passed that stage. The whole-project build subsequently stalled in the existing Visual Studio integration plugin; the successful verification targeted the modified game module, with PCH and UBA detouring/cache disabled. The editor loaded and ran the test using the existing plugin binaries. A full rebuild of all plugins was not completed.

Successful build command (PowerShell, repository root):

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' FlyBrainSimulatorEditor Win64 Development '-Project=W:/Git/fly-brain-simulator-ue5/FlyBrainSimulator/FlyBrainSimulator.uproject' -Module=FlyBrainSimulator -WaitMutex -NoHotReloadFromIDE -NoUBA -NoPCH -NoSharedPCH -MaxParallelActions=1 -NoCache
```

Automation invocation:

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'W:/Git/fly-brain-simulator-ue5/FlyBrainSimulator/FlyBrainSimulator.uproject' -unattended -nop4 -nosound -nullrhi '-ExecCmds=Automation RunTests FlyBrain.Subsystem.Lifecycle' '-TestExit=Automation Test Queue Empty'
```

Local ignored logs: `FlyBrainSimulator/Saved/Logs/BrainCoreBuild.log` and `BrainAutomation.log`; standalone test output: `Tests/BrainCore/Intermediate/Testing/Temporary/LastTest.log`.

For in-engine performance, run the same map/configuration and record `stat unit` Game/Draw/GPU timings plus process RAM and graphics-memory usage. Record actual completed brain steps over a timed interval and backlog separately. A fixed dt of 1 ms requests 1,000 steps per game second; it is not itself a throughput measurement. Preserve the same seed/configuration and compare StateHash at the same step count for every later performance change.

## Interactive demo validation — 2026-09-28

Development Editor game module rebuilt with UE 5.8.3. `FlyBrain.Subsystem.Lifecycle` and `FlyBrain.Demo.Motion` pass. Motion checks the controller commands, running/paused/reset pose, mesh and material references. GPU rendering was additionally checked through a 1440 x 900 offscreen game launch; the actual screenshot is `docs/FlyDemo.png`. Mouse clicking itself was not automated. Existing SimulationMap and its Blueprint assets were preserved.

Core tests pass after cumulative spike telemetry was added, including a check against the **pre-change** hash `16884003706165038808` at 10,000 steps. Standalone Release throughput in this run was 301,331 brain steps/sec (not a controlled speed comparison).

| Metric | Demo observation |
| --- | --- |
| Neurons / synapses | 1,000 / 16,000 |
| Brain steps/sec | Dashboard screenshot sample: 209 actual steps per wall second; standalone capacity: 301,331 steps/sec |
| Brain update cost | 0.0081 ms at screenshot request |
| Game Thread | 2.09 ms mean |
| Render Thread | 3.30 ms mean |
| GPU | 2.61 ms mean |
| RAM | 2,374.5 MiB process physical at screenshot request (includes editor runtime) |
| VRAM | 441.31 MB mean, UE CSV `GPUMem/LocalUsedMB`; shared/system GPU memory 162.93 MB |

Thread/GPU/VRAM numbers use the final 60 complete records of the boot CSV capture, excluding initialization. The dashboard screenshot was taken later, so its FPS/steps-per-second are a different sampling interval. These are short offscreen smoke-test observations on the target i7-9750H / RTX 2060 machine, not a steady-state interactive benchmark. Runtime capture may stall, and elapsed game time can differ from wall time. Do not infer real-time capacity from that isolated 209 steps/sec UI sample.

Ignored evidence: `Saved/Logs/FlyDemoBuild.log`, `FlyDemoTests.log`, `FlyDemoVisual.log`, and `FlyDemoProfile.csv` beneath the project. The demo starts stopped; press Start to run it. Packaging was not tested.

## Stimulus / learning validation — 2026-09-28

UE 5.8.3 Development Editor module builds successfully. All three UE tests pass: `FlyBrain.Subsystem.Lifecycle`, `FlyBrain.Demo.Motion`, and `FlyBrain.Learning.Controls`. Standalone Core and Conditioning tests pass. The original 16,000-edge default network still produces the unchanged 10,000-step state hash `16884003706165038808`; its current array storage is 225,004 bytes including external drive.

The conditioning test measures the following under identical cue-only probes from resting state:

| Condition | A before → after (Hz) | B before → after (Hz) |
| --- | --- | --- |
| Learning ON, reward ON | 15 → 62.5 | 15 → 15 |
| Learning OFF, reward ON | 15 → 15 | 15 → 15 |
| Learning ON, reward OFF | 15 → 15 | 15 → 15 |

A's average synaptic weight increases from 0.12 to 0.381828; B stays at 0.12. Repeated cue-only probes retain the learned response without modifying weights. Forget restores the initial response. Reset retains weights while clearing state/results. With no stimulus there is no spontaneous firing in the experimental circuit. Busy protocols reject replacement commands and lock condition changes. State and weights agree across different frame partitions when compared at equal integer step counts (floating wall-time boundaries may differ by one step).

The GPU-rendered game was launched at 1440 x 900 with `DemoCompare,DemoCapture 8`. Its runtime log independently reports the same completed comparison and weights; the inspected screenshot is `docs/LearningDemo.png`. No input is active at that final screenshot, so the live motor bars have decayed while before/after measurements remain visible. UI mouse clicks were not separately automated; controller methods and Blueprint integration were tested.

| Recorded metric | Learning demo |
| --- | --- |
| Neurons / synapses | 1,000 / 8,000 |
| Brain steps/sec | 163,470 standalone, complete 6,000-step comparison including plasticity; screenshot wall-time sample 137 |
| Core array storage | 129,004 bytes, plus experiment trace array 4,000 bytes and scalar/plan state |
| Brain update cost | 0.0226 ms at capture request |
| Game Thread | 2.293 ms mean |
| Render Thread | 2.976 ms mean |
| GPU | 2.199 ms mean |
| RAM | 2,308.4 MiB process physical at capture request |
| VRAM | 442.180 MB mean UE CSV local GPU memory; system/shared GPU memory 162.180 MB |

Thread/GPU/memory CSV values are the final 60 data rows of the 1,200-frame boot capture, excluding the trailing repeated header/metadata. Missing trailing columns for newly introduced worker metrics are not grounds for discarding valid frame rows. This is a brief offscreen smoke test, not a controlled interactive performance comparison; the screenshot is a later sampling interval. Standalone capacity includes the learning protocol but excludes rendering. Evidence is in the ignored project logs `LearningBuild.log`, `LearningTests.log`, `LearningVisual.log`, `LearningProfile.csv`, and CTest output. Packaging and cross-platform bitwise identity were not tested.
