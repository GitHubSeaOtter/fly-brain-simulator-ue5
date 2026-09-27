#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Brain/BrainCore.h"
#include "BrainSimulationSubsystem.generated.h"

UCLASS()
class FLYBRAINSIMULATOR_API UBrainSimulationSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintCallable, Category = "Fly Brain")
    void Start();
    UFUNCTION(BlueprintCallable, Category = "Fly Brain")
    void Stop();
    UFUNCTION(BlueprintCallable, Category = "Fly Brain")
    void Reset();
    UFUNCTION(BlueprintPure, Category = "Fly Brain")
    bool IsRunning() const { return bRunning; }
    UFUNCTION(BlueprintPure, Category = "Fly Brain")
    int64 GetStepCount() const { return static_cast<int64>(Core.GetStepCount()); }
    UFUNCTION(BlueprintPure, Category = "Fly Brain")
    double GetSimulationTimeSeconds() const { return Core.GetStepCount() * Core.GetConfig().TimestepSeconds; }
    UFUNCTION(BlueprintPure, Category = "Fly Brain")
    double GetBacklogSeconds() const { return Core.GetBacklogSeconds(); }
    UFUNCTION(BlueprintPure, Category = "Fly Brain")
    int32 GetNeuronCount() const { return static_cast<int32>(Core.GetVoltages().size()); }
    UFUNCTION(BlueprintPure, Category = "Fly Brain")
    int32 GetSynapseCount() const { return static_cast<int32>(Core.GetTargets().size()); }
    UFUNCTION(BlueprintPure, Category = "Fly Brain")
    int64 GetCoreStorageBytes() const { return static_cast<int64>(Core.GetStorageBytes()); }

    const FlyBrain::BrainCore& GetCore() const { return Core; }
private:
    FlyBrain::BrainCore Core;
    bool bRunning = false;
    bool bReady = false;
};
