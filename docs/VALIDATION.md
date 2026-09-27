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
