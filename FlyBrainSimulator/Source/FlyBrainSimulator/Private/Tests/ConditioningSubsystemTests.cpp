#include "Brain/BrainSimulationSubsystem.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConditioningIntegrationTest, "FlyBrain.Learning.Controls",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FConditioningIntegrationTest::RunTest(const FString& Parameters)
{
    auto* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto* Brain = World->GetSubsystem<UBrainSimulationSubsystem>();
    TestTrue(TEXT("Enable experiment"), Brain->EnableConditioningDemo());
    TestEqual(TEXT("Experimental CSR edges"), Brain->GetSynapseCount(), 8000);
    TestTrue(TEXT("Begin comparison"), Brain->RunComparison());
    Brain->Tick(0.2f);
    Brain->Stop();
    const auto PausedHash = Brain->GetCore().StateHash();
    Brain->Tick(1.0f);
    TestTrue(TEXT("Pause freezes the trial"), PausedHash == Brain->GetCore().StateHash());
    TestFalse(TEXT("Busy trial rejects replacement"), Brain->StimulateB());
    Brain->SetLearningEnabled(false);
    TestTrue(TEXT("Controls locked while protocol active"), Brain->IsLearningEnabled());
    Brain->Start();
    for (int N = 0; N < 100 && Brain->IsExperimentActive(); ++N) { Brain->Tick(0.1f); }
    const auto& E = Brain->GetExperiment();
    TestTrue(TEXT("Comparison completed"), E.BeforeA.Valid && E.AfterA.Valid && E.AfterB.Valid && !E.IsActive());
    TestTrue(TEXT("A improves without reward during probe"), E.AfterA.Hertz > E.BeforeA.Hertz);
    TestEqual(TEXT("B remains unchanged"), E.AfterB.Hertz, E.BeforeB.Hertz);
    const double Weight = E.GetMeanWeight(Brain->GetCore(), 1);
    Brain->Reset();
    TestFalse(TEXT("Reset stops"), Brain->IsRunning());
    TestEqual(TEXT("Reset retains learned weights"), E.GetMeanWeight(Brain->GetCore(), 1), Weight);
    TestTrue(TEXT("Stimulus automatically starts"), Brain->StimulateA() && Brain->IsRunning());
    for (int N = 0; N < 10 && Brain->IsExperimentActive(); ++N) { Brain->Tick(0.1f); }
    TestTrue(TEXT("Learned probe remains responsive"), E.LastProbe.Valid && E.LastProbe.Hertz > 15);
    Brain->ForgetLearning();
    TestTrue(TEXT("Forget restores initial weights"), FMath::IsNearlyEqual(E.GetMeanWeight(Brain->GetCore(), 1), 0.12, 1e-10));
    TestFalse(TEXT("Forget stops and clears activity"), Brain->IsRunning() || Brain->IsExperimentActive());
    Brain->SetLearningEnabled(false);
    Brain->RunComparison();
    for (int N = 0; N < 100 && Brain->IsExperimentActive(); ++N) { Brain->Tick(0.1f); }
    TestEqual(TEXT("No learning control"), E.AfterA.Hertz, E.BeforeA.Hertz);
    World->DestroyWorld(false);
    return true;
}
#endif
