#include "Demo/FlyBrainDemo.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Brain/BrainSimulationSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyDemoMotionTest, "FlyBrain.Demo.Motion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlyDemoMotionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto* Brain = World->GetSubsystem<UBrainSimulationSubsystem>();
    auto* Fly = World->SpawnActor<AFlyBrainDemoFly>();
    auto* Controller = World->SpawnActor<AFlyBrainDemoController>();
    if (TestNotNull(TEXT("Fly"), Fly) && TestNotNull(TEXT("Brain"), Brain) && TestNotNull(TEXT("Controller"), Controller))
    {
        TArray<UStaticMeshComponent*> Parts;
        Fly->GetComponents(Parts);
        TestTrue(TEXT("Body, eyes, wings and six articulated legs"), Parts.Num() >= 23);
        for (auto* Part : Parts)
        {
            TestTrue(TEXT("Mesh asset"), Part->GetStaticMesh() != nullptr);
            TestNotNull(TEXT("Material asset"), Part->GetMaterial(0));
        }
        Fly->UpdateFromBrain();
        const FTransform Initial = Fly->GetActorTransform();
        Controller->DemoStart();
        Brain->Tick(0.125f);
        Fly->UpdateFromBrain();
        TestFalse(TEXT("Running moves the fly"), Fly->GetActorLocation().Equals(Initial.GetLocation()));
        const FTransform Moving = Fly->GetActorTransform();
        Controller->DemoStop();
        Brain->Tick(0.125f);
        Fly->UpdateFromBrain();
        TestTrue(TEXT("Stopped fly freezes"), Fly->GetActorTransform().Equals(Moving));
        Controller->DemoReset();
        Fly->UpdateFromBrain();
        TestTrue(TEXT("Reset restores pose"), Fly->GetActorTransform().Equals(Initial));
        Controller->ToggleBrain();
        TestTrue(TEXT("Space action starts"), Brain->IsRunning());
        Controller->ToggleBrain();
        TestFalse(TEXT("Space action stops"), Brain->IsRunning());
    }
    World->DestroyWorld(false);
    return true;
}
#endif
