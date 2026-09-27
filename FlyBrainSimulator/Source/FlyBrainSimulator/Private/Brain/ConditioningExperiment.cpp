#include "Brain/ConditioningExperiment.h"
#include <algorithm>

namespace FlyBrain
{
bool ConditioningExperiment::Initialize(BrainCore& Core)
{
    Config Settings;
    Settings.Drive = 0;
    if (!Core.Initialize(Settings)) { return false; }
    std::vector<std::uint32_t> Rows(1001), Targets;
    std::vector<double> Weights;
    Targets.reserve(8000); Weights.reserve(8000);
    for (std::uint32_t N = 0; N < 1000; ++N)
    {
        Rows[N] = static_cast<std::uint32_t>(Targets.size());
        if (N < 500)
        {
            const std::uint32_t MotorStart = N < 250 ? 500 : 750;
            for (std::uint32_t E = 0; E < 16; ++E)
            {
                Targets.push_back(MotorStart + (N % 250 + E) % 250);
                Weights.push_back(InitialWeight);
            }
        }
    }
    Rows[1000] = static_cast<std::uint32_t>(Targets.size());
    Ready = Core.SetGraph(Rows, Targets, Weights);
    TrainingCount = 0;
    Cancel(Core);
    return Ready;
}

void ConditioningExperiment::Cancel(BrainCore& Core)
{
    TrialIndex = TrialCount = StepInTrial = 0;
    MotorSpikes = 0;
    ActivityA = ActivityB = 0;
    Comparing = false;
    PreTrace.fill(0);
    BeforeA = BeforeB = AfterA = AfterB = LastProbe = {};
    LastProbeCue = 0;
    Core.Reset();
}

bool ConditioningExperiment::Begin(BrainCore& Core, ExperimentCommand Command, bool Learning, bool WithReward)
{
    if (!Ready || IsActive()) { return false; }
    if (Command == ExperimentCommand::Compare && !Initialize(Core)) { return false; }
    // Every probe begins from the same resting state; only learned weights are retained.
    Core.Reset(); PreTrace.fill(0); ActivityA = ActivityB = 0;
    TrialIndex = StepInTrial = 0; MotorSpikes = 0;
    Learn = Learning; Reward = WithReward;
    Comparing = Command == ExperimentCommand::Compare;
    if (Comparing)
    {
        Trials = { TrialKind::ProbeA, TrialKind::ProbeB, TrialKind::PairA, TrialKind::PairA,
            TrialKind::PairA, TrialKind::PairA, TrialKind::PairA, TrialKind::PairA, TrialKind::ProbeA, TrialKind::ProbeB };
        TrialCount = 10;
    }
    else if (Command == ExperimentCommand::TrainA)
    {
        Trials.fill(TrialKind::PairA); TrialCount = 6;
        // A new training block invalidates old before/after comparisons.
        BeforeA = BeforeB = AfterA = AfterB = {};
        LastProbe = {};
    }
    else
    {
        TrialCount = 1;
        Trials[0] = Command == ExperimentCommand::ProbeA ? TrialKind::ProbeA : TrialKind::ProbeB;
        LastProbe = {};
    }
    return true;
}

int ConditioningExperiment::GetCue() const
{
    if (!IsActive() || StepInTrial >= StimulusSteps) { return 0; }
    return Trials[TrialIndex] == TrialKind::ProbeB ? 2 : 1;
}
bool ConditioningExperiment::IsRewardActive() const
{
    return GetCue() != 0 && Trials[TrialIndex] == TrialKind::PairA && Reward;
}

void ConditioningExperiment::BeforeStep(BrainCore& Core)
{
    if (IsActive() && StepInTrial == 0)
    {
        Core.ClearNeuronState(); PreTrace.fill(0); ActivityA = ActivityB = 0; MotorSpikes = 0;
    }
    Core.SetExternalDrive(0, 250, GetCue() == 1 ? 60 : 0);
    Core.SetExternalDrive(250, 250, GetCue() == 2 ? 60 : 0);
    Core.SetExternalDrive(500, 250, IsRewardActive() ? 32 : 10);
    Core.SetExternalDrive(750, 250, 10);
}

void ConditioningExperiment::AfterStep(BrainCore& Core)
{
    const auto Spikes = Core.GetSpikes();
    if (Spikes.size() != 1000) { return; }
    int CountA = 0, CountB = 0;
    for (int N = 500; N < 750; ++N) { CountA += Spikes[N]; CountB += Spikes[N + 250]; }
    // Exponential observer, 50 ms time constant at dt=1 ms; 1.0 means ~100 Hz.
    ActivityA = ActivityA * 0.98 + (CountA / 250.0 * 10.0) * 0.02;
    ActivityB = ActivityB * 0.98 + (CountB / 250.0 * 10.0) * 0.02;
    if (!IsActive()) { return; }
    if (StepInTrial < StimulusSteps)
    {
        MotorSpikes += Trials[TrialIndex] == TrialKind::ProbeB ? CountB : CountA;
    }
    for (int N = 0; N < 500; ++N) { PreTrace[N] = PreTrace[N] * 0.95 + Spikes[N]; }
    if (Learn && IsRewardActive())
    {
        const auto Rows = Core.GetRowOffsets();
        const auto Targets = Core.GetTargets();
        const auto Weights = Core.GetWeights();
        // Reward-gated causal Hebbian potentiation, bounded; probes never update weights.
        for (int N = 0; N < 500; ++N)
        {
            for (auto E = Rows[N]; E < Rows[N + 1]; ++E)
            {
                if (Spikes[Targets[E]])
                {
                    Core.SetSynapseWeight(E, std::min(MaximumWeight, Weights[E] + 0.0005 * PreTrace[N]));
                }
            }
        }
    }
    ++StepInTrial;
    if (StepInTrial == TrialSteps)
    {
        if (Trials[TrialIndex] == TrialKind::PairA) { ++TrainingCount; }
        else
        {
            LastProbe = { true, MotorSpikes / (Population * StimulusSteps * Core.GetConfig().TimestepSeconds) };
            LastProbeCue = Trials[TrialIndex] == TrialKind::ProbeB ? 2 : 1;
            if (Comparing)
            {
                if (TrialIndex == 0) { BeforeA = LastProbe; }
                if (TrialIndex == 1) { BeforeB = LastProbe; }
                if (TrialIndex == 8) { AfterA = LastProbe; }
                if (TrialIndex == 9) { AfterB = LastProbe; }
            }
        }
        ++TrialIndex; StepInTrial = 0;
    }
}

double ConditioningExperiment::GetMeanWeight(const BrainCore& Core, int Cue) const
{
    if (!Ready || (Cue != 1 && Cue != 2)) { return 0; }
    const auto Rows = Core.GetRowOffsets(); const auto Weights = Core.GetWeights();
    const auto First = Rows[(Cue - 1) * Population], End = Rows[Cue * Population];
    double Sum = 0;
    for (auto E = First; E < End; ++E) { Sum += Weights[E]; }
    return End > First ? Sum / (End - First) : 0;
}

const char* ConditioningExperiment::GetPhaseName() const
{
    if (!IsActive()) { return TrialCount ? "COMPLETE" : "READY"; }
    if (Trials[TrialIndex] == TrialKind::PairA)
    {
        if (!Reward) { return "TRAIN A (NO REWARD)"; }
        return Learn ? "TRAIN A + REWARD" : "PAIR A (LEARNING OFF)";
    }
    if (Comparing && TrialIndex < 2) { return Trials[TrialIndex] == TrialKind::ProbeA ? "BASELINE A" : "BASELINE B"; }
    return Trials[TrialIndex] == TrialKind::ProbeA ? "PROBE A (NO REWARD)" : "PROBE B (CONTROL)";
}
}
