#include "Brain/BrainSimulationSubsystem.h"
#include "Stats/Stats.h"
#include "HAL/PlatformTime.h"

void UBrainSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bReady = Core.Initialize(FlyBrain::Config{});
    ensure(bReady);
}

void UBrainSimulationSubsystem::Deinitialize()
{
    Stop();
    bReady = false;
    Super::Deinitialize();
}

bool UBrainSimulationSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UBrainSimulationSubsystem::IsTickable() const
{
    return bReady && bRunning && Super::IsTickable();
}

TStatId UBrainSimulationSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UBrainSimulationSubsystem, STATGROUP_Tickables);
}

void UBrainSimulationSubsystem::Tick(float DeltaTime)
{
    if (bRunning && bReady)
    {
        const double Begin = FPlatformTime::Seconds();
        if (Experiment.IsReady())
        {
            Core.Advance(static_cast<double>(DeltaTime), 256,
                FlyBrain::ConditioningExperiment::BeforeHook, FlyBrain::ConditioningExperiment::AfterHook, &Experiment);
        }
        else { Core.Advance(static_cast<double>(DeltaTime)); }
        LastUpdateMilliseconds = (FPlatformTime::Seconds() - Begin) * 1000.0;
    }
}

void UBrainSimulationSubsystem::Start() { bRunning = bReady; }
void UBrainSimulationSubsystem::Stop() { bRunning = false; LastUpdateMilliseconds = 0.0; }
void UBrainSimulationSubsystem::Reset()
{
    Stop();
    if (Experiment.IsReady()) { Experiment.Cancel(Core); }
    else { Core.Reset(); }
}

bool UBrainSimulationSubsystem::EnableConditioningDemo()
{
    Stop();
    bReady = Experiment.Initialize(Core);
    return bReady;
}
bool UBrainSimulationSubsystem::BeginExperiment(FlyBrain::ExperimentCommand Command)
{
    if (!Experiment.IsReady() && !EnableConditioningDemo()) { return false; }
    if (!Experiment.Begin(Core, Command, bLearningEnabled, bRewardEnabled)) { return false; }
    Start(); return true;
}
bool UBrainSimulationSubsystem::StimulateA() { return BeginExperiment(FlyBrain::ExperimentCommand::ProbeA); }
bool UBrainSimulationSubsystem::StimulateB() { return BeginExperiment(FlyBrain::ExperimentCommand::ProbeB); }
bool UBrainSimulationSubsystem::TrainA() { return BeginExperiment(FlyBrain::ExperimentCommand::TrainA); }
bool UBrainSimulationSubsystem::RunComparison() { return BeginExperiment(FlyBrain::ExperimentCommand::Compare); }
void UBrainSimulationSubsystem::ForgetLearning() { EnableConditioningDemo(); }
