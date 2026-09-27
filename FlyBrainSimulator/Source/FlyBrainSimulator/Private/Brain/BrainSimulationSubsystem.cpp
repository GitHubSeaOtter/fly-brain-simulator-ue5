#include "Brain/BrainSimulationSubsystem.h"
#include "Stats/Stats.h"

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
    if (bRunning && bReady) { Core.Advance(static_cast<double>(DeltaTime)); }
}

void UBrainSimulationSubsystem::Start() { bRunning = bReady; }
void UBrainSimulationSubsystem::Stop() { bRunning = false; }
void UBrainSimulationSubsystem::Reset()
{
    Stop();
    Core.Reset();
}
