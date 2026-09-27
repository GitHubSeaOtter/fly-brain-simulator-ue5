#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "FlyBrainDemo.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class USceneComponent;
class SWidget;

// One visual actor for the entire fly, never one actor per neuron.
UCLASS()
class FLYBRAINSIMULATOR_API AFlyBrainDemoFly : public AActor
{
    GENERATED_BODY()
public:
    AFlyBrainDemoFly();
    virtual void Tick(float DeltaSeconds) override;
    void UpdateFromBrain();
    static FVector PositionAtTime(double Seconds);
private:
    UStaticMeshComponent* Part(const TCHAR* Name, USceneComponent* Parent, UStaticMesh* Mesh,
        FVector Position, FVector Scale, FLinearColor Color);
    UPROPERTY() TObjectPtr<UMaterialInterface> SurfaceMaterial;
    UPROPERTY() TObjectPtr<USceneComponent> LeftWing;
    UPROPERTY() TObjectPtr<USceneComponent> RightWing;
};

UCLASS()
class FLYBRAINSIMULATOR_API AFlyBrainDemoGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AFlyBrainDemoGameMode();
    virtual void BeginPlay() override;
private:
    // Constructor references ensure primitive meshes/material are included when cooking.
    UPROPERTY() TObjectPtr<UStaticMesh> PlatformMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> SurfaceMaterial;
};

UCLASS()
class FLYBRAINSIMULATOR_API AFlyBrainDemoController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    UFUNCTION(Exec) void DemoStart();
    UFUNCTION(Exec) void DemoStop();
    UFUNCTION(Exec) void DemoReset();
    UFUNCTION(Exec) void DemoCapture(float DelaySeconds = 5.0f);
    UFUNCTION(Exec) void DemoCueA();
    UFUNCTION(Exec) void DemoCueB();
    UFUNCTION(Exec) void DemoTrain();
    UFUNCTION(Exec) void DemoCompare();
    UFUNCTION(Exec) void DemoForget();
    void ToggleBrain();
private:
    TSharedPtr<SWidget> Dashboard;
};
