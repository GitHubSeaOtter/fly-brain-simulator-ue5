#include "Brain/BrainCore.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>

void Require(bool Passed, const char* Message)
{
    if (!Passed) { std::cerr << "FAIL: " << Message << '\n'; std::exit(1); }
}

int main()
{
    using namespace FlyBrain;
    BrainCore A, B;
    Require(A.Initialize(Config{}) && B.Initialize(Config{}), "default initialization");
    Require(A.GetVoltages().size() == 1000 && A.GetTargets().size() == 16000, "milestone size");
    Require(std::equal(A.GetTargets().begin(), A.GetTargets().end(), B.GetTargets().begin()), "seeded topology");
    for (std::uint32_t N = 0; N < 1000; ++N)
    {
        const auto Rows = A.GetRowOffsets();
        Require(Rows[N + 1] - Rows[N] == 16, "CSR degree");
        std::set<std::uint32_t> Unique;
        for (auto E = Rows[N]; E < Rows[N + 1]; ++E)
        {
            Require(A.GetTargets()[E] < 1000 && A.GetTargets()[E] != N, "CSR target bounds/self edge");
            Unique.insert(A.GetTargets()[E]);
        }
        Require(Unique.size() == 16, "no duplicate edges");
    }
    const auto InitialHash = A.StateHash();
    for (int I = 0; I < 1000; ++I)
    {
        A.Step(); B.Step();
        Require(A.StateHash() == B.StateHash(), "identical per-step replay");
    }
    const auto ReplayHash = A.StateHash();
    A.Reset();
    Require(A.StateHash() == InitialHash && A.GetBacklogSeconds() == 0, "reset all state");
    Require(A.GetTotalSpikeCount() == 0, "reset spike telemetry");
    for (int I = 0; I < 1000; ++I) { A.Step(); }
    Require(A.StateHash() == ReplayHash, "replay after reset");

    Config Single;
    Single.NeuronCount = 1; Single.SynapsesPerNeuron = 0;
    Require(A.Initialize(Single), "isolated LIF");
    A.Step();
    Require(A.GetVoltages()[0] == -64.0 && A.GetSpikes()[0] == 0, "Euler analytic first step");
    for (int I = 1; I < 27; ++I) { A.Step(); }
    Require(A.GetSpikes()[0] == 0, "below threshold before step 28");
    A.Step();
    Require(A.GetSpikes()[0] == 1 && A.GetVoltages()[0] == -65.0, "threshold and reset");
    Require(A.GetTotalSpikeCount() == 1, "cumulative spike telemetry");
    A.Step(); A.Step();
    Require(A.GetVoltages()[0] == -65.0 && A.GetSpikes()[0] == 0, "two refractory steps");
    A.Step(); Require(A.GetVoltages()[0] == -64.0, "refractory expiry");

    Config Pair = Single;
    Pair.NeuronCount = 2; Pair.SynapsesPerNeuron = 1;
    Pair.RefractorySteps = 0; Pair.SynapseWeight = 2;
    Require(A.Initialize(Pair), "pair initialization");
    for (int I = 0; I < 28; ++I) { A.Step(); }
    Require(A.GetVoltages()[0] == -65 && A.GetVoltages()[1] == -65, "no same-step delivery");
    A.Step();
    Require(A.GetVoltages()[0] == -62 && A.GetVoltages()[1] == -62, "next-step delivery in both directions");
    Pair.RefractorySteps = 2;
    Require(A.Initialize(Pair), "refractory pair");
    for (int I = 0; I < 31; ++I) { A.Step(); }
    Require(A.GetVoltages()[0] == -64, "refractory inputs discarded");

    // Binary-exact timestep makes frame partition assertions independent of rounding.
    Config Schedule;
    Schedule.TimestepSeconds = 1.0 / 1024.0;
    Require(A.Initialize(Schedule) && B.Initialize(Schedule), "scheduler initialization");
    for (int I = 0; I < 128; ++I) { A.Advance(1.0 / 128.0); }
    for (int I = 0; I < 32; ++I) { B.Advance(1.0 / 32.0); }
    Require(A.GetStepCount() == 1024 && A.StateHash() == B.StateHash(), "frame partition independence");
    B.Reset();
    Require(B.Advance(1, 7) == 7 && B.GetBacklogSeconds() > 0.9, "bounded work retains backlog");
    while (B.Advance(0, 256) != 0) {}
    Require(A.StateHash() == B.StateHash(), "backlog never drops steps");
    Require(B.Advance(-1) == 0 && B.Advance(std::numeric_limits<double>::infinity()) == 0, "invalid elapsed time");
    const auto BeforeInvalid = B.StateHash();
    auto Invalid = Schedule; Invalid.NeuronCount = 0;
    Require(!B.Initialize(Invalid) && B.StateHash() == BeforeInvalid, "invalid config preserves state");
    Invalid = Schedule; Invalid.TimestepSeconds = std::numeric_limits<double>::quiet_NaN();
    Require(!B.Initialize(Invalid), "NaN rejected");

    Require(A.Initialize(Config{}) && B.Initialize(Config{}), "benchmark initialization");
    constexpr int Steps = 10000;
    const auto Start = std::chrono::steady_clock::now();
    for (int I = 0; I < Steps; ++I) { A.Step(); }
    const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();
    for (int I = 0; I < Steps; ++I) { B.Step(); }
    Require(A.StateHash() == B.StateHash(), "benchmark result replay");
    Require(A.StateHash() == 16884003706165038808ull, "pre-telemetry numerical baseline unchanged");
    std::cout << "PASS: all Brain Core tests\nneurons=1000 synapses=16000 steps=" << Steps
        << " steps/sec=" << Steps / Seconds << " core_storage_bytes=" << A.GetStorageBytes()
        << " state_hash=" << A.StateHash() << '\n';
}
