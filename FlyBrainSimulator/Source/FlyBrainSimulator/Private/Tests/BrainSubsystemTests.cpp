#include "Brain/BrainSimulationSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBrainSubsystemLifecycleTest,
    "FlyBrain.Subsystem.Lifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBrainSubsystemLifecycleTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Game world"), World)) { return false; }
    auto* Brain = World->GetSubsystem<UBrainSimulationSubsystem>();
    if (TestNotNull(TEXT("Automatically created brain subsystem"), Brain))
    {
        TestEqual(TEXT("Neuron count"), Brain->GetNeuronCount(), 1000);
        TestEqual(TEXT("Synapse count"), Brain->GetSynapseCount(), 16000);
        TestFalse(TEXT("Initially stopped"), Brain->IsTickable());
        for (const TCHAR* Name : { TEXT("Start"), TEXT("Stop"), TEXT("Reset") })
        {
            auto* Function = Brain->FindFunction(FName(Name));
            if (TestNotNull(Name, Function))
            {
                TestTrue(TEXT("Blueprint callable"), Function->HasAnyFunctionFlags(FUNC_BlueprintCallable));
            }
        }
        const auto InitialHash = Brain->GetCore().StateHash();
        Brain->Start(); Brain->Start();
        TestTrue(TEXT("Start enables ticking"), Brain->IsTickable());
        Brain->Tick(0.03125f);
        const auto Steps = Brain->GetStepCount();
        const auto Hash = Brain->GetCore().StateHash();
        TestTrue(TEXT("Started simulation advances"), Steps > 0);
        Brain->Stop(); Brain->Tick(1.0f);
        TestFalse(TEXT("Stop disables ticking"), Brain->IsTickable());
        TestEqual(TEXT("Stop preserves step count"), Brain->GetStepCount(), Steps);
        TestTrue(TEXT("Stop preserves state"), Brain->GetCore().StateHash() == Hash);
        Brain->Start(); Brain->Tick(0.03125f);
        TestTrue(TEXT("Start resumes"), Brain->GetStepCount() > Steps);
        Brain->Reset();
        TestFalse(TEXT("Reset stops simulation"), Brain->IsRunning());
        TestEqual(TEXT("Reset clears backlog"), Brain->GetBacklogSeconds(), 0.0);
        TestTrue(TEXT("Reset restores initial state"), Brain->GetCore().StateHash() == InitialHash);
        Brain->Start(); Brain->Tick(0.03125f);
        TestTrue(TEXT("Reset replay"), Brain->GetCore().StateHash() == Hash);
        Brain->Stop();
    }
    World->DestroyWorld(false);
    return true;
}
#endif
