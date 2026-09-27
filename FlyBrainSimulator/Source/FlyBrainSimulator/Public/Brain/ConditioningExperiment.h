#pragma once
#include "Brain/BrainCore.h"
#include <array>

namespace FlyBrain
{
enum class ExperimentCommand { ProbeA, ProbeB, TrainA, Compare };
enum class TrialKind { ProbeA, ProbeB, PairA };
struct ProbeResult
{
    bool Valid = false;
    double Hertz = 0; // Motor spikes / 250 cells / 0.4 seconds of cue.
};

// Synthetic reward-gated Hebbian conditioning, not a biological fly connectome.
// 250 cue A + 250 cue B + 250 motor A + 250 motor B cells, 8,000 feed-forward edges.
class ConditioningExperiment
{
public:
    bool Initialize(BrainCore& Core);
    bool Begin(BrainCore& Core, ExperimentCommand Command, bool Learning = true, bool Reward = true);
    void Cancel(BrainCore& Core); // Clears dynamic state/results; retains learned weights.
    void ClearComparison() { BeforeA = BeforeB = AfterA = AfterB = {}; }
    void BeforeStep(BrainCore& Core);
    void AfterStep(BrainCore& Core);
    static void BeforeHook(BrainCore& Core, void* Self) { static_cast<ConditioningExperiment*>(Self)->BeforeStep(Core); }
    static void AfterHook(BrainCore& Core, void* Self) { static_cast<ConditioningExperiment*>(Self)->AfterStep(Core); }
    bool IsReady() const { return Ready; }
    bool IsActive() const { return TrialIndex < TrialCount; }
    int GetCue() const;
    bool IsRewardActive() const;
    int GetTrialNumber() const { return IsActive() ? TrialIndex + 1 : TrialCount; }
    int GetTrialCount() const { return TrialCount; }
    int GetTrainingCount() const { return TrainingCount; }
    double GetActivityA() const { return ActivityA; }
    double GetActivityB() const { return ActivityB; }
    double GetMeanWeight(const BrainCore& Core, int Cue) const;
    const char* GetPhaseName() const;
    ProbeResult BeforeA, BeforeB, AfterA, AfterB, LastProbe;
    int LastProbeCue = 0;
    static constexpr int Population = 250;
    static constexpr int StimulusSteps = 400;
    static constexpr int TrialSteps = 600;
    static constexpr double InitialWeight = 0.12;
    static constexpr double MaximumWeight = 1.5;
private:
    std::array<TrialKind, 10> Trials{};
    std::array<double, 500> PreTrace{};
    int TrialIndex = 0, TrialCount = 0, StepInTrial = 0, TrainingCount = 0;
    std::uint64_t MotorSpikes = 0;
    double ActivityA = 0, ActivityB = 0;
    bool Ready = false, Learn = true, Reward = true, Comparing = false;
};
}
