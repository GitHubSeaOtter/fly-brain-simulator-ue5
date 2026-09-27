#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Brain/BrainCore.h"
#include "Brain/ConditioningExperiment.h"
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
    UFUNCTION(BlueprintCallable, Category = "Fly Brain|Learning")
    bool EnableConditioningDemo();
    UFUNCTION(BlueprintCallable, Category = "Fly Brain|Learning")
    bool StimulateA();
    UFUNCTION(BlueprintCallable, Category = "Fly Brain|Learning")
    bool StimulateB();
    UFUNCTION(BlueprintCallable, Category = "Fly Brain|Learning")
    bool TrainA();
    UFUNCTION(BlueprintCallable, Category = "Fly Brain|Learning")
    bool RunComparison();
    UFUNCTION(BlueprintCallable, Category = "Fly Brain|Learning")
    void ForgetLearning();
    UFUNCTION(BlueprintCallable, Category = "Fly Brain|Learning")
    void SetLearningEnabled(bool Enabled)
    {
        if (!Experiment.IsActive() && bLearningEnabled != Enabled) { bLearningEnabled = Enabled; Experiment.ClearComparison(); }
    }
    UFUNCTION(BlueprintCallable, Category = "Fly Brain|Learning")
    void SetRewardEnabled(bool Enabled)
    {
        if (!Experiment.IsActive() && bRewardEnabled != Enabled) { bRewardEnabled = Enabled; Experiment.ClearComparison(); }
    }
    UFUNCTION(BlueprintPure, Category = "Fly Brain|Learning")
    bool IsLearningEnabled() const { return bLearningEnabled; }
    UFUNCTION(BlueprintPure, Category = "Fly Brain|Learning")
    bool IsRewardEnabled() const { return bRewardEnabled; }
    UFUNCTION(BlueprintPure, Category = "Fly Brain|Learning")
    bool IsExperimentActive() const { return Experiment.IsActive(); }
    UFUNCTION(BlueprintPure, Category = "Fly Brain|Learning")
    double GetMotorResponseA() const { return Experiment.GetActivityA(); }
    UFUNCTION(BlueprintPure, Category = "Fly Brain|Learning")
    double GetMotorResponseB() const { return Experiment.GetActivityB(); }
    UFUNCTION(BlueprintPure, Category = "Fly Brain")
    bool IsRunning() const { return bRunning; }
    UFUNCTION(BlueprintPure, Category = "Fly Brain")
    int64 GetStepCount() const { return static_cast<int64>(Core.GetStepCount()); }
    UFUNCTION(BlueprintPure, Category = "Fly Brain")
    int64 GetTotalSpikeCount() const { return static_cast<int64>(Core.GetTotalSpikeCount()); }
    UFUNCTION(BlueprintPure, Category = "Fly Brain")
    double GetLastUpdateMilliseconds() const { return LastUpdateMilliseconds; }
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
    const FlyBrain::ConditioningExperiment& GetExperiment() const { return Experiment; }
private:
    bool BeginExperiment(FlyBrain::ExperimentCommand Command);
    FlyBrain::BrainCore Core;
    FlyBrain::ConditioningExperiment Experiment;
    bool bLearningEnabled = true;
    bool bRewardEnabled = true;
    bool bRunning = false;
    bool bReady = false;
    double LastUpdateMilliseconds = 0.0;
};
