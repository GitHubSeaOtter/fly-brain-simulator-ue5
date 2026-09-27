#include "Brain/ConditioningExperiment.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace FlyBrain;
void Check(bool Value, const char* Message)
{
    if (!Value) { std::cerr << "FAIL: " << Message << '\n'; std::exit(1); }
}
void Run(BrainCore& Core, ConditioningExperiment& E)
{
    for (int N = 0; N < 20000 && E.IsActive(); ++N) { E.BeforeStep(Core); Core.Step(); E.AfterStep(Core); }
    Check(!E.IsActive(), "protocol terminates");
}
int main()
{
    BrainCore Core;
    ConditioningExperiment E;
    Check(E.Initialize(Core), "initialize circuit");
    Check(Core.GetVoltages().size() == 1000 && Core.GetTargets().size() == 8000, "feedforward population sizes");
    Check(!Core.SetExternalDrive(999, 2, 1) && !Core.SetExternalDrive(0, 1, std::numeric_limits<double>::infinity()), "input validation");
    Check(!Core.SetSynapseWeight(8000, 1), "weight bounds validation");
    for (int N = 0; N < 1000; ++N) { E.BeforeStep(Core); Core.Step(); E.AfterStep(Core); }
    Check(Core.GetTotalSpikeCount() == 0, "no spontaneous response without a cue");
    Check(E.Begin(Core, ExperimentCommand::Compare), "begin comparison");
    Check(!E.Begin(Core, ExperimentCommand::ProbeB), "do not interrupt active trial");
    const auto BeginTime = std::chrono::steady_clock::now();
    Run(Core, E);
    const double Duration = std::chrono::duration<double>(std::chrono::steady_clock::now() - BeginTime).count();
    std::cout << "neurons=1000 synapses=8000 protocol_steps=" << Core.GetStepCount()
        << " steps/sec=" << Core.GetStepCount() / Duration << " core_storage_bytes=" << Core.GetStorageBytes() << '\n';
    std::cout << "A " << E.BeforeA.Hertz << " -> " << E.AfterA.Hertz
        << " Hz; B " << E.BeforeB.Hertz << " -> " << E.AfterB.Hertz
        << " Hz; weights " << E.GetMeanWeight(Core, 1) << ", " << E.GetMeanWeight(Core, 2) << '\n';
    Check(E.BeforeA.Valid && E.AfterA.Valid && E.BeforeB.Valid && E.AfterB.Valid, "all comparisons measured");
    Check(E.AfterA.Hertz > E.BeforeA.Hertz + 10, "learned cue increases motor response");
    Check(E.AfterB.Hertz == E.BeforeB.Hertz, "untrained cue unchanged");
    Check(E.GetMeanWeight(Core, 1) > 0.12 && std::abs(E.GetMeanWeight(Core, 2) - 0.12) < 1e-12, "cue selective plasticity");
    const auto LearnedWeights = std::vector<double>(Core.GetWeights().begin(), Core.GetWeights().end());
    const double LearnedRate = E.AfterA.Hertz;
    Check(E.Begin(Core, ExperimentCommand::ProbeA), "repeat cue-only probe"); Run(Core, E);
    Check(E.LastProbe.Hertz == LearnedRate, "probe resets transients but retains learned response");
    Check(std::equal(LearnedWeights.begin(), LearnedWeights.end(), Core.GetWeights().begin()), "probe never learns");
    E.Cancel(Core);
    Check(E.GetActivityA() == 0 && !E.LastProbe.Valid, "reset clears protocol and transient response");
    Check(std::equal(LearnedWeights.begin(), LearnedWeights.end(), Core.GetWeights().begin()), "reset retains memory");
    Check(E.Begin(Core, ExperimentCommand::Compare, false, true), "learning off control"); Run(Core, E);
    Check(E.AfterA.Hertz == E.BeforeA.Hertz, "reward without plasticity does not learn");
    Check(E.Begin(Core, ExperimentCommand::Compare, true, false), "reward off control"); Run(Core, E);
    Check(E.AfterA.Hertz == E.BeforeA.Hertz, "unrewarded cue does not learn");
    E.Initialize(Core);
    Check(E.Begin(Core, ExperimentCommand::ProbeA), "forgotten cue"); Run(Core, E);
    Check(E.LastProbe.Hertz < LearnedRate, "forget restores baseline");

    BrainCore Replay;
    ConditioningExperiment R;
    E.Initialize(Core); R.Initialize(Replay);
    E.Begin(Core, ExperimentCommand::Compare); R.Begin(Replay, ExperimentCommand::Compare);
    for (int I = 0; I < 400; ++I)
    {
        Core.Advance(0.015625, 256, E.BeforeHook, E.AfterHook, &E);
        Replay.Advance(0.0078125, 256, R.BeforeHook, R.AfterHook, &R);
        Replay.Advance(0.0078125, 256, R.BeforeHook, R.AfterHook, &R);
    }
    // Floating elapsed time can place a boundary step in the adjacent frame. Compare at equal integer steps.
    Check(std::abs(static_cast<long long>(Core.GetStepCount()) - static_cast<long long>(Replay.GetStepCount())) <= 1, "frame rounding bounded");
    while (Core.GetStepCount() < Replay.GetStepCount()) { E.BeforeStep(Core); Core.Step(); E.AfterStep(Core); }
    while (Replay.GetStepCount() < Core.GetStepCount()) { R.BeforeStep(Replay); Replay.Step(); R.AfterStep(Replay); }
    Check(Core.StateHash() == Replay.StateHash(), "frame partition replay state");
    Check(std::equal(Core.GetWeights().begin(), Core.GetWeights().end(), Replay.GetWeights().begin()), "frame partition replay weights");
    Check(E.AfterA.Hertz == R.AfterA.Hertz, "frame partition response");
    for (double W : Core.GetWeights()) { Check(W >= 0 && W <= E.MaximumWeight, "bounded weights"); }
    std::cout << "PASS: conditioning, controls, memory, deterministic replay\n";
}
