#include "Demo/FlyBrainDemo.h"

#include "Brain/BrainSimulationSubsystem.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FLinearColor Shell(0.065f, 0.11f, 0.12f);
const FLinearColor LegColor(0.13f, 0.21f, 0.22f);
const FLinearColor WingColor(0.43f, 0.78f, 0.72f);
}

AFlyBrainDemoFly::AFlyBrainDemoFly()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("FlyRoot"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Demo/M_FlyDemo.M_FlyDemo"));
    SurfaceMaterial = Material.Object;

    Part(TEXT("Abdomen"), RootComponent, Sphere.Object, FVector(-32, 0, 0), FVector(0.82, 0.46, 0.43), Shell);
    Part(TEXT("Thorax"), RootComponent, Sphere.Object, FVector(8, 0, 5), FVector(0.57, 0.51, 0.49), LegColor);
    Part(TEXT("Head"), RootComponent, Sphere.Object, FVector(40, 0, 9), FVector(0.39, 0.43, 0.36), Shell);
    for (int Side : { -1, 1 })
    {
        const FString Label = Side < 0 ? TEXT("L") : TEXT("R");
        Part(*(Label + TEXT("Eye")), RootComponent, Sphere.Object, FVector(46, Side * 16, 17),
            FVector(0.25, 0.23, 0.28), FLinearColor(0.82f, 0.13f, 0.055f));
        Part(*(Label + TEXT("EyeGlint")), RootComponent, Sphere.Object, FVector(53, Side * 23, 23),
            FVector(0.065), FLinearColor(1.0f, 0.64f, 0.37f));

        auto* Pivot = CreateDefaultSubobject<USceneComponent>(*(Label + TEXT("WingPivot")));
        Pivot->SetupAttachment(RootComponent);
        Pivot->SetRelativeLocation(FVector(0, Side * 16, 22));
        auto* Wing = Part(*(Label + TEXT("Wing")), Pivot, Sphere.Object, FVector(-18, Side * 45, 0),
            FVector(0.55, 1.16, 0.032), WingColor);
        Wing->SetRelativeRotation(FRotator(0, Side * 24, 0));
        if (Side < 0) { LeftWing = Pivot; } else { RightWing = Pivot; }

        for (int Leg = 0; Leg < 3; ++Leg)
        {
            const FVector Hip(20 - Leg * 25, Side * 16, -7);
            const FVector Knee(38 - Leg * 36, Side * 43, -22);
            const FVector Foot(49 - Leg * 48, Side * 58, -43);
            const FVector Points[] = { Hip, Knee, Foot };
            for (int Segment = 0; Segment < 2; ++Segment)
            {
                const FVector Delta = Points[Segment + 1] - Points[Segment];
                auto* Limb = Part(*FString::Printf(TEXT("%sLeg%d_%d"), *Label, Leg, Segment),
                    RootComponent, Cylinder.Object, (Points[Segment] + Points[Segment + 1]) * 0.5,
                    FVector(0.035, 0.035, Delta.Length() / 100.0), LegColor);
                Limb->SetRelativeRotation(FRotationMatrix::MakeFromZ(Delta).Rotator());
            }
        }
        auto* Antenna = Part(*(Label + TEXT("Antenna")), RootComponent, Cylinder.Object,
            FVector(60, Side * 9, 17), FVector(0.025, 0.025, 0.24), LegColor);
        Antenna->SetRelativeRotation(FRotator(0, 0, 0));
    }
    SetActorLocation(PositionAtTime(0));
}

UStaticMeshComponent* AFlyBrainDemoFly::Part(const TCHAR* Name, USceneComponent* Parent,
    UStaticMesh* Mesh, FVector Position, FVector Scale, FLinearColor Color)
{
    auto* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
    Component->SetupAttachment(Parent);
    Component->SetStaticMesh(Mesh);
    Component->SetRelativeLocation(Position);
    Component->SetRelativeScale3D(Scale);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    // Default data survives registration; all parts share a single material.
    Component->SetDefaultCustomPrimitiveDataVector4(0, FVector4(Color.R, Color.G, Color.B, 1));
    Component->SetMaterial(0, SurfaceMaterial);
    return Component;
}

FVector AFlyBrainDemoFly::PositionAtTime(double Seconds)
{
    const double Angle = Seconds * 0.35;
    return FVector(200 * FMath::Cos(Angle), 150 * FMath::Sin(Angle), 125 + 16 * FMath::Sin(Seconds * 2));
}

void AFlyBrainDemoFly::UpdateFromBrain()
{
    const auto* Brain = GetWorld()->GetSubsystem<UBrainSimulationSubsystem>();
    if (!Brain) { return; }
    const double Time = Brain->GetSimulationTimeSeconds();
    const double ResponseA = FMath::Clamp(Brain->GetMotorResponseA(), 0.0, 1.0);
    const double ResponseB = FMath::Clamp(Brain->GetMotorResponseB(), 0.0, 1.0);
    const double Response = FMath::Max(ResponseA, ResponseB);
    SetActorLocation(PositionAtTime(Time) + FVector(0, 0, Response * 55));
    const double Angle = Time * 0.35;
    const FVector Tangent(-200 * FMath::Sin(Angle), 150 * FMath::Cos(Angle), 0);
    SetActorRotation(FRotator(3 * FMath::Sin(Time * 2), Tangent.Rotation().Yaw + (ResponseA - ResponseB) * 65,
        -8 * FMath::Sin(Angle) + (ResponseA - ResponseB) * 20));
    // Slowed for legibility; this is not the biological wingbeat frequency.
    const double Flap = (24 + Response * 24) * FMath::Sin(Time * 2 * PI * 7);
    LeftWing->SetRelativeRotation(FRotator(0, 0, Flap));
    RightWing->SetRelativeRotation(FRotator(0, 0, -Flap));
}

void AFlyBrainDemoFly::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateFromBrain();
}

AFlyBrainDemoGameMode::AFlyBrainDemoGameMode()
{
    PlayerControllerClass = AFlyBrainDemoController::StaticClass();
    DefaultPawnClass = nullptr;
    HUDClass = nullptr;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Demo/M_FlyDemo.M_FlyDemo"));
    PlatformMesh = Cylinder.Object;
    SurfaceMaterial = Material.Object;
}

void AFlyBrainDemoGameMode::BeginPlay()
{
    Super::BeginPlay();
    GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->EnableConditioningDemo();
    auto* Fly = GetWorld()->SpawnActor<AFlyBrainDemoFly>();
    Fly->UpdateFromBrain();

    const auto Disk = [this](FVector Location, FVector Scale, FLinearColor Color)
    {
        auto* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
        auto* Mesh = Actor->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(PlatformMesh);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Actor->SetActorScale3D(Scale);
        Mesh->SetMaterial(0, SurfaceMaterial);
        Mesh->SetCustomPrimitiveDataVector4(0, FVector4(Color.R, Color.G, Color.B, 1));
    };
    Disk(FVector(0, 0, -12), FVector(9.0, 9.0, 0.20), FLinearColor(0.025f, 0.055f, 0.063f));
    Disk(FVector(0, 0, 0), FVector(8.5, 8.5, 0.025), FLinearColor(0.10f, 0.47f, 0.40f));
    Disk(FVector(0, 0, 2), FVector(8.35, 8.35, 0.025), FLinearColor(0.038f, 0.075f, 0.085f));
    Disk(FVector(0, 0, 4), FVector(4.7, 4.7, 0.02), FLinearColor(0.075f, 0.15f, 0.16f));
    Disk(FVector(0, 0, 6), FVector(4.63, 4.63, 0.02), FLinearColor(0.045f, 0.086f, 0.095f));
    for (int Index = 0; Index < 24; ++Index)
    {
        const double Angle = Index * 2 * PI / 24;
        Disk(FVector(398 * FMath::Cos(Angle), 398 * FMath::Sin(Angle), 5),
            FVector(0.065, 0.065, 0.04), FLinearColor(0.29f, 0.78f, 0.63f));
    }

    auto* Key = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 600), FRotator(-55, -35, 0));
    Key->GetLightComponent()->SetIntensity(5.0f);
    Key->GetLightComponent()->SetLightColor(FLinearColor(0.77f, 0.9f, 1.0f));
    auto* Fill = GetWorld()->SpawnActor<APointLight>(FVector(-150, 230, 330), FRotator::ZeroRotator);
    Fill->PointLightComponent->SetIntensity(18000);
    Fill->PointLightComponent->SetAttenuationRadius(1500);
    Fill->PointLightComponent->SetLightColor(FLinearColor(0.3f, 1.0f, 0.72f));
    Fill->PointLightComponent->SetCastShadows(false);

    auto* Camera = GetWorld()->SpawnActor<ACameraActor>(FVector(760, -1000, 900), FRotator::ZeroRotator);
    Camera->SetActorRotation((FVector(0, 0, 75) - Camera->GetActorLocation()).Rotation());
    Camera->GetCameraComponent()->SetFieldOfView(48);
    Camera->GetCameraComponent()->SetConstraintAspectRatio(false);
    if (auto* PC = GetWorld()->GetFirstPlayerController()) { PC->SetViewTarget(Camera); }
}
