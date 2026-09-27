#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace FlyBrain
{
struct Config
{
    std::uint32_t NeuronCount = 1000;
    std::uint32_t SynapsesPerNeuron = 16;
    std::uint32_t Seed = 1;
    double TimestepSeconds = 0.001;
    double MembraneTimeConstantSeconds = 0.020;
    double RestPotential = -65.0;
    double ResetPotential = -65.0;
    double Threshold = -50.0;
    double Drive = 20.0; // R * I, in mV.
    double SynapseWeight = 0.1; // Instantaneous mV increment, delivered next step.
    std::uint32_t RefractorySteps = 2;
};

// Single-thread-owned, UE-independent structure-of-arrays LIF simulation.
class BrainCore
{
public:
    bool Initialize(const Config& InConfig);
    void Reset();
    void Step();
    std::uint32_t Advance(double ElapsedSeconds, std::uint32_t MaxSteps = 256);
    const Config& GetConfig() const { return Settings; }
    std::uint64_t GetStepCount() const { return StepCount; }
    double GetBacklogSeconds() const { return Accumulator; }
    std::size_t GetStorageBytes() const;
    std::span<const double> GetVoltages() const { return Voltages; }
    std::span<const std::uint8_t> GetSpikes() const { return Spikes; }
    std::span<const std::uint32_t> GetRowOffsets() const { return RowOffsets; }
    std::span<const std::uint32_t> GetTargets() const { return Targets; }
    std::span<const double> GetWeights() const { return Weights; }
    std::uint64_t StateHash() const;

private:
    Config Settings;
    std::vector<double> Voltages, PendingInputs, Weights;
    std::vector<std::uint32_t> Refractory, RowOffsets, Targets;
    std::vector<std::uint8_t> Spikes;
    std::uint64_t StepCount = 0;
    double Accumulator = 0.0;
};
}
