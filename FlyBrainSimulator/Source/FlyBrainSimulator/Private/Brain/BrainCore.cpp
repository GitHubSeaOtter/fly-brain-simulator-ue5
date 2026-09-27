#include "Brain/BrainCore.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace FlyBrain
{
bool BrainCore::Initialize(const Config& InConfig)
{
    const auto& C = InConfig;
    if (C.NeuronCount == 0 || C.NeuronCount > 139255 ||
        C.SynapsesPerNeuron >= C.NeuronCount ||
        static_cast<std::uint64_t>(C.NeuronCount) * C.SynapsesPerNeuron >
            std::numeric_limits<std::uint32_t>::max() ||
        !std::isfinite(C.TimestepSeconds) || !std::isfinite(C.MembraneTimeConstantSeconds) ||
        C.TimestepSeconds <= 0 || C.MembraneTimeConstantSeconds < C.TimestepSeconds ||
        !std::isfinite(C.RestPotential) || !std::isfinite(C.ResetPotential) ||
        !std::isfinite(C.Threshold) || !std::isfinite(C.Drive) || !std::isfinite(C.SynapseWeight) ||
        C.ResetPotential >= C.Threshold || C.RestPotential >= C.Threshold)
    {
        return false;
    }
    Settings = C;
    Voltages.resize(C.NeuronCount);
    PendingInputs.resize(C.NeuronCount);
    Refractory.resize(C.NeuronCount);
    Spikes.resize(C.NeuronCount);
    RowOffsets.resize(static_cast<std::size_t>(C.NeuronCount) + 1);
    Targets.resize(static_cast<std::size_t>(C.NeuronCount) * C.SynapsesPerNeuron);
    Weights.assign(Targets.size(), C.SynapseWeight);
    // Explicit integer PRNG: topology does not depend on STL distributions.
    std::uint32_t RandomState = C.Seed;
    for (std::uint32_t Source = 0; Source < C.NeuronCount; ++Source)
    {
        RowOffsets[Source] = Source * C.SynapsesPerNeuron;
        RandomState = RandomState * 1664525u + 1013904223u;
        const auto Start = C.NeuronCount > 1 ? RandomState % (C.NeuronCount - 1) : 0;
        for (std::uint32_t Edge = 0; Edge < C.SynapsesPerNeuron; ++Edge)
        {
            const auto Offset = 1 + (Start + Edge) % (C.NeuronCount - 1);
            Targets[RowOffsets[Source] + Edge] = (Source + Offset) % C.NeuronCount;
        }
    }
    RowOffsets[C.NeuronCount] = static_cast<std::uint32_t>(Targets.size());
    Reset();
    return true;
}

void BrainCore::Reset()
{
    std::fill(Voltages.begin(), Voltages.end(), Settings.RestPotential);
    std::fill(PendingInputs.begin(), PendingInputs.end(), 0.0);
    std::fill(Refractory.begin(), Refractory.end(), 0);
    std::fill(Spikes.begin(), Spikes.end(), std::uint8_t{0});
    StepCount = 0;
    Accumulator = 0.0;
}

void BrainCore::Step()
{
    if (Voltages.empty()) { return; }
    const double Alpha = Settings.TimestepSeconds / Settings.MembraneTimeConstantSeconds;
    for (std::size_t N = 0; N < Voltages.size(); ++N)
    {
        Spikes[N] = 0;
        if (Refractory[N] != 0)
        {
            --Refractory[N];
            Voltages[N] = Settings.ResetPotential;
        }
        else
        {
            Voltages[N] += Alpha * (Settings.RestPotential - Voltages[N] + Settings.Drive)
                + PendingInputs[N];
            if (Voltages[N] >= Settings.Threshold)
            {
                Spikes[N] = 1;
                Voltages[N] = Settings.ResetPotential;
                Refractory[N] = Settings.RefractorySteps;
            }
        }
        // Inputs during refractory time are discarded.
        PendingInputs[N] = 0.0;
    }
    // Fixed source/edge order, after ALL neurons have integrated.
    for (std::size_t Source = 0; Source < Voltages.size(); ++Source)
    {
        if (Spikes[Source] == 0) { continue; }
        for (auto Edge = RowOffsets[Source]; Edge < RowOffsets[Source + 1]; ++Edge)
        {
            PendingInputs[Targets[Edge]] += Weights[Edge];
        }
    }
    ++StepCount;
}

std::uint32_t BrainCore::Advance(double ElapsedSeconds, std::uint32_t MaxSteps)
{
    if (Voltages.empty() || !std::isfinite(ElapsedSeconds) || ElapsedSeconds < 0 ||
        !std::isfinite(Accumulator + ElapsedSeconds)) { return 0; }
    Accumulator += ElapsedSeconds;
    std::uint32_t Executed = 0;
    while (Accumulator >= Settings.TimestepSeconds && Executed < MaxSteps)
    {
        Step();
        Accumulator -= Settings.TimestepSeconds;
        ++Executed;
    }
    return Executed;
}

std::size_t BrainCore::GetStorageBytes() const
{
    return (Voltages.capacity() + PendingInputs.capacity() + Weights.capacity()) * sizeof(double)
        + (Refractory.capacity() + RowOffsets.capacity() + Targets.capacity()) * sizeof(std::uint32_t)
        + Spikes.capacity() * sizeof(std::uint8_t);
}

std::uint64_t BrainCore::StateHash() const
{
    // Numerical state only: wall-clock backlog is deliberately excluded.
    std::uint64_t Hash = 14695981039346656037ull;
    const auto Add = [&Hash](std::uint64_t Value)
    {
        for (int Byte = 0; Byte < 8; ++Byte)
        {
            Hash ^= Value & 0xffu;
            Hash *= 1099511628211ull;
            Value >>= 8;
        }
    };
    Add(StepCount);
    for (auto V : Voltages) { Add(std::bit_cast<std::uint64_t>(V)); }
    for (auto V : PendingInputs) { Add(std::bit_cast<std::uint64_t>(V)); }
    for (auto V : Refractory) { Add(V); }
    for (auto V : Spikes) { Add(V); }
    return Hash;
}
}
