#include "Demo/FlyBrainDemo.h"

#include "Brain/BrainSimulationSubsystem.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
const FLinearColor Ink(0.84f, 0.92f, 0.91f);
const FLinearColor Muted(0.36f, 0.51f, 0.54f);
const FLinearColor Accent(0.24f, 0.92f, 0.65f);
const FLinearColor Panel(0.013f, 0.025f, 0.033f, 0.97f);

TSharedRef<STextBlock> Label(const FString& Text, int Size = 12, FLinearColor Color = Ink)
{
    return SNew(STextBlock).Text(FText::FromString(Text))
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", Size)).ColorAndOpacity(Color);
}

class SNeuronField : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SNeuronField) {} SLATE_ARGUMENT(TWeakObjectPtr<UBrainSimulationSubsystem>, Brain) SLATE_END_ARGS()
    void Construct(const FArguments& Args) { Brain = Args._Brain; }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(304, 160); }
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Cull,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool Enabled) const override
    {
        if (!Brain.IsValid()) { return Layer; }
        const auto& Core = Brain->GetCore();
        const auto Voltages = Core.GetVoltages();
        const auto& Config = Core.GetConfig();
        constexpr int Columns = 40;
        const int Rows = FMath::Max(1, FMath::DivideAndRoundUp(static_cast<int>(Voltages.size()), Columns));
        const FVector2D Cell(Geometry.GetLocalSize().X / Columns, Geometry.GetLocalSize().Y / Rows);
        for (int N = 0; N < static_cast<int>(Voltages.size()); ++N)
        {
            const float V = FMath::Clamp(static_cast<float>((Voltages[N] - Config.RestPotential) /
                (Config.Threshold - Config.RestPotential)), 0.0f, 1.0f);
            const FLinearColor Color = FMath::Lerp(FLinearColor(0.035f, 0.10f, 0.12f), Accent, V);
            FSlateDrawElement::MakeBox(Elements, Layer,
                Geometry.ToPaintGeometry(FVector2D(FMath::Max(1.0, Cell.X - 2), FMath::Max(1.0, Cell.Y - 2)),
                    FSlateLayoutTransform(FVector2D((N % Columns) * Cell.X, (N / Columns) * Cell.Y))),
                FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, Color);
        }
        return Layer;
    }
private:
    TWeakObjectPtr<UBrainSimulationSubsystem> Brain;
};

class SBrainDashboard : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SBrainDashboard) {} SLATE_ARGUMENT(AFlyBrainDemoController*, Controller) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Controller = Args._Controller;
        Brain = Controller->GetWorld()->GetSubsystem<UBrainSimulationSubsystem>();
        LastSample = FPlatformTime::Seconds();
        auto Stats = SNew(SVerticalBox);
        AddStat(Stats, TEXT("SIMULATION TIME"), TimeText);
        AddStat(Stats, TEXT("COMPLETED STEPS"), StepsText);
        AddStat(Stats, TEXT("BRAIN STEPS / SEC"), SpeedText);
        AddStat(Stats, TEXT("SPIKES / NEURON / SEC"), RateText);
        AddStat(Stats, TEXT("BRAIN UPDATE"), CpuText);
        AddStat(Stats, TEXT("BACKLOG"), BacklogText);
        AddStat(Stats, TEXT("PROCESS RAM"), RamText);

        ChildSlot
        [
            // GameViewport already applies the project's DPI curve.
            SNew(SDPIScaler).DPIScale(1.0f)
            [
                SNew(SOverlay)
                + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(32, 24)
                [ SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()[Label(TEXT("FLY / BRAIN"), 27)]
                    + SVerticalBox::Slot().AutoHeight().Padding(1, 7)[Label(TEXT("NEURAL SIMULATION LAB     /     STIMULUS & LEARNING"), 10, Muted)]
                ]
                + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(32, 28)
                [ SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
                    [ SAssignNew(StatusText, STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 15)).ColorAndOpacity(Accent) ]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 8)
                    [ SAssignNew(FrameText, STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 11)).ColorAndOpacity(Muted) ]
                ]
                + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(32, 115, 0, 0)
                [ SNew(SBox).WidthOverride(352)
                    [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Panel).Padding(24)
                        [ SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[Label(TEXT("BRAIN CORE"), 12, Accent)]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 15, 0, 4)[Label(TEXT("1,000"), 42)]
                            + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(Muted)
                                .Text_Lambda([this] { return FText::FromString(FString::Printf(TEXT("LIF neurons  /  %s synapses"),
                                    *FText::AsNumber(Brain.IsValid() ? Brain->GetSynapseCount() : 0).ToString())); })]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 18)[SNew(SSeparator).SeparatorImage(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(Muted * 0.3f)]
                            + SVerticalBox::Slot().AutoHeight()[Stats]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 10)[Label(TEXT("MEMBRANE POTENTIAL  /  ALL NEURONS"), 10, Muted)]
                            + SVerticalBox::Slot().AutoHeight()[SNew(SNeuronField).Brain(Brain)]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10)[Label(TEXT("-65 mV    REST                      THRESHOLD    -50 mV"), 8, Muted)]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 6)[Label(TEXT("A / B sensory + A / B motor: 250 cells each"), 9, Muted)]
                        ]
                    ]
                ]
                + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0, 115, 32, 0)
                [LearningPanel()]
                + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(32, 0, 32, 123)
                [ SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[Label(TEXT("DROSOPHILA / STYLIZED STUDY"), 12, Accent)]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 7)[Label(TEXT("Motor activity drives turn / lift. Synthetic conditioning model."), 10, Muted)]
                ]
                + SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(32, 0, 32, 24)
                [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Panel).Padding(20, 16)
                    [ SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("START"), Accent, [this] { if (Controller.IsValid()) { Controller->DemoStart(); } return FReply::Handled(); }, true)]
                        + SHorizontalBox::Slot().AutoWidth().Padding(10, 0)[Button(TEXT("STOP"), Ink, [this] { if (Controller.IsValid()) { Controller->DemoStop(); } return FReply::Handled(); }, false)]
                        + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("RESET STATE"), Muted, [this] { if (Controller.IsValid()) { Controller->DemoReset(); } return FReply::Handled(); }, false)]
                        + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).HAlign(HAlign_Right)
                        [Label(TEXT("SPACE  Pause / Resume     R  Reset state (keep learning)"), 10, Muted)]
                    ]
                ]
            ]
        ];
        Refresh(0, 0, 0);
    }

    virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override
    {
        SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
        if (!Brain.IsValid()) { return; }
        const double Now = FPlatformTime::Seconds();
        const double Elapsed = Now - LastSample;
        ++Frames;
        if (Elapsed >= 0.25 || Brain->GetStepCount() < LastSteps || Brain->IsRunning() != WasRunning)
        {
            const int64 Steps = Brain->GetStepCount();
            const int64 Spikes = Brain->GetTotalSpikeCount();
            const bool Reset = Steps < LastSteps;
            const double Rate = !Reset && Elapsed > 0 ? FMath::Max<int64>(0, Spikes - LastSpikes) / Elapsed / Brain->GetNeuronCount() : 0;
            Refresh(!Reset && Elapsed > 0 ? FMath::Max<int64>(0, Steps - LastSteps) / Elapsed : 0,
                Rate, Elapsed > 0 ? Frames / Elapsed : 0);
            LastSteps = Steps; LastSpikes = Spikes; LastSample = Now; Frames = 0;
        }
        WasRunning = Brain->IsRunning();
    }
private:
    TSharedRef<SWidget> LearningPanel()
    {
        auto Content = SNew(SVerticalBox);
        Content->AddSlot().AutoHeight()[Label(TEXT("STIMULUS / CONDITIONING"), 12, Accent)];
        Content->AddSlot().AutoHeight().Padding(0, 10)[Label(TEXT("Compare cue-only responses before and after\n6 trials of A paired with a teaching reward."), 10, Muted)];
        auto Actions = SNew(SVerticalBox).IsEnabled_Lambda([this] { return Brain.IsValid() && !Brain->IsExperimentActive(); });
        Actions->AddSlot().AutoHeight()
        [ SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("CUE A  [1]"), Accent, [this] { if (Controller.IsValid()) { Controller->DemoCueA(); } return FReply::Handled(); }, false)]
            + SHorizontalBox::Slot().AutoWidth().Padding(8, 0)[Button(TEXT("CUE B  [2]"), Ink, [this] { if (Controller.IsValid()) { Controller->DemoCueB(); } return FReply::Handled(); }, false)]
        ];
        Actions->AddSlot().AutoHeight().Padding(0, 8)
        [ SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("TRAIN A  [T]"), Accent, [this] { if (Controller.IsValid()) { Controller->DemoTrain(); } return FReply::Handled(); }, false)]
            + SHorizontalBox::Slot().AutoWidth().Padding(8, 0)[Button(TEXT("COMPARE [C]"), Accent, [this] { if (Controller.IsValid()) { Controller->DemoCompare(); } return FReply::Handled(); }, true)]
        ];
        Content->AddSlot().AutoHeight()[Actions];
        Content->AddSlot().AutoHeight()[Label(TEXT("COMPARE starts fresh: baseline > train > retest"), 9, Muted)];
        auto Options = SNew(SHorizontalBox).IsEnabled_Lambda([this] { return Brain.IsValid() && !Brain->IsExperimentActive(); });
        Options->AddSlot().AutoWidth()
        [ SNew(SCheckBox).IsChecked_Lambda([this] { return Brain.IsValid() && Brain->IsLearningEnabled() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
            .OnCheckStateChanged_Lambda([this](ECheckBoxState S) { if (Brain.IsValid()) { Brain->SetLearningEnabled(S == ECheckBoxState::Checked); } })
            [Label(TEXT("Learning"), 11)] ];
        Options->AddSlot().AutoWidth().Padding(18, 0)
        [ SNew(SCheckBox).IsChecked_Lambda([this] { return Brain.IsValid() && Brain->IsRewardEnabled() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
            .OnCheckStateChanged_Lambda([this](ECheckBoxState S) { if (Brain.IsValid()) { Brain->SetRewardEnabled(S == ECheckBoxState::Checked); } })
            [Label(TEXT("Reward"), 11)] ];
        Content->AddSlot().AutoHeight().Padding(0, 12)[Options];
        Content->AddSlot().AutoHeight()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 11)).ColorAndOpacity(Accent)
            .Text_Lambda([this]
            {
                if (!Brain.IsValid()) { return FText::GetEmpty(); }
                const auto& E = Brain->GetExperiment();
                return FText::FromString(FString::Printf(TEXT("%s   %d / %d"), UTF8_TO_TCHAR(E.GetPhaseName()), E.GetTrialNumber(), E.GetTrialCount()));
            })];
        Content->AddSlot().AutoHeight().Padding(0, 6, 0, 12)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 10)).ColorAndOpacity(Ink)
            .Text_Lambda([this]
            {
                if (!Brain.IsValid()) { return FText::GetEmpty(); }
                const auto& E = Brain->GetExperiment();
                return FText::FromString(FString::Printf(TEXT("Input: %s     Teaching reward: %s"),
                    E.GetCue() == 1 ? TEXT("A") : E.GetCue() == 2 ? TEXT("B") : TEXT("none"), E.IsRewardActive() ? TEXT("ON") : TEXT("off")));
            })];
        for (int Cue = 1; Cue <= 2; ++Cue)
        {
            Content->AddSlot().AutoHeight().Padding(0, 5)[Label(Cue == 1 ? TEXT("MOTOR A  /  live response") : TEXT("MOTOR B  /  live response"), 9, Muted)];
            Content->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(10)
                [SNew(SProgressBar).FillColorAndOpacity(Cue == 1 ? Accent : FLinearColor(0.3f, 0.65f, 1.0f))
                    .Percent_Lambda([this, Cue]() -> TOptional<float>
                    { return Brain.IsValid() ? FMath::Clamp(static_cast<float>(Cue == 1 ? Brain->GetMotorResponseA() : Brain->GetMotorResponseB()), 0.f, 1.f) : 0.f; })]];
        }
        Content->AddSlot().AutoHeight().Padding(0, 16, 0, 6)[Label(TEXT("CUE-ONLY PROBES       BEFORE  >  AFTER"), 10, Muted)];
        for (int Cue = 1; Cue <= 2; ++Cue)
        {
            Content->AddSlot().AutoHeight().Padding(0, 4)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 15)).ColorAndOpacity(Ink)
                .Text_Lambda([this, Cue]
                {
                    if (!Brain.IsValid()) { return FText::GetEmpty(); }
                    const auto& E = Brain->GetExperiment();
                    const auto Before = Cue == 1 ? E.BeforeA : E.BeforeB;
                    const auto After = Cue == 1 ? E.AfterA : E.AfterB;
                    const FString A = Before.Valid ? FString::Printf(TEXT("%.1f"), Before.Hertz) : TEXT("--");
                    const FString B = After.Valid ? FString::Printf(TEXT("%.1f"), After.Hertz) : TEXT("--");
                    return FText::FromString(FString::Printf(TEXT("%s      %s  >  %s Hz"), Cue == 1 ? TEXT("A") : TEXT("B"), *A, *B));
                })];
        }
        Content->AddSlot().AutoHeight().Padding(0, 9)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 10)).ColorAndOpacity(Accent)
            .Text_Lambda([this]
            {
                if (!Brain.IsValid()) { return FText::GetEmpty(); }
                const auto& E = Brain->GetExperiment();
                return FText::FromString(FString::Printf(TEXT("Mean weight   A %.3f / B %.3f"), E.GetMeanWeight(Brain->GetCore(), 1), E.GetMeanWeight(Brain->GetCore(), 2)));
            })];
        Content->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 10)).ColorAndOpacity(Muted)
            .Text_Lambda([this]
            {
                if (!Brain.IsValid() || !Brain->GetExperiment().LastProbe.Valid) { return FText::FromString(TEXT("Latest cue-only probe: --")); }
                const auto& E = Brain->GetExperiment();
                return FText::FromString(FString::Printf(TEXT("Latest probe %s: %.1f Hz"), E.LastProbeCue == 1 ? TEXT("A") : TEXT("B"), E.LastProbe.Hertz));
            })];
        Content->AddSlot().AutoHeight()[Button(TEXT("FORGET"), Muted, [this] { if (Controller.IsValid()) { Controller->DemoForget(); } return FReply::Handled(); }, false)];
        return SNew(SBox).WidthOverride(380)
            [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Panel).Padding(22)[Content]];
    }
    void AddStat(TSharedRef<SVerticalBox> Box, const TCHAR* Name, TSharedPtr<STextBlock>& Value)
    {
        Box->AddSlot().AutoHeight().Padding(0, 5)
        [ SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[Label(Name, 9, Muted)]
            + SHorizontalBox::Slot().AutoWidth()[SAssignNew(Value, STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 13)).ColorAndOpacity(Ink)]
        ];
    }
    TSharedRef<SWidget> Button(const TCHAR* Text, FLinearColor Color, TFunction<FReply()> Click, bool Primary)
    {
        return SNew(SBox).WidthOverride(126).HeightOverride(40)
        [ SNew(SButton).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
            .ButtonColorAndOpacity(Primary ? FLinearColor(0.045f, 0.18f, 0.13f) : FLinearColor(0.025f, 0.05f, 0.06f))
            .HAlign(HAlign_Center).VAlign(VAlign_Center).IsFocusable(false).OnClicked_Lambda(MoveTemp(Click))
            [Label(Text, 11, Color)] ];
    }
    void Refresh(double Speed, double Rate, double Fps)
    {
        if (!Brain.IsValid()) { return; }
        const bool Running = Brain->IsRunning();
        const auto Set = [](TSharedPtr<STextBlock> Text, const FString& Value) { Text->SetText(FText::FromString(Value)); };
        Set(StatusText, Running ? TEXT("●  RUNNING") : TEXT("○  STOPPED"));
        StatusText->SetColorAndOpacity(Running ? Accent : Muted);
        Set(FrameText, FString::Printf(TEXT("%.0f FPS   /   CPU SIMULATION"), Fps));
        Set(TimeText, FString::Printf(TEXT("%.3f s"), Brain->GetSimulationTimeSeconds()));
        Set(StepsText, FText::AsNumber(Brain->GetStepCount()).ToString());
        Set(SpeedText, FString::Printf(TEXT("%.0f"), Running ? Speed : 0));
        Set(RateText, FString::Printf(TEXT("%.1f"), Running ? Rate : 0));
        Set(CpuText, FString::Printf(TEXT("%.3f ms"), Brain->GetLastUpdateMilliseconds()));
        Set(BacklogText, FString::Printf(TEXT("%.2f ms"), Brain->GetBacklogSeconds() * 1000));
        Set(RamText, FString::Printf(TEXT("%.0f MB"), FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0)));
    }
    TWeakObjectPtr<AFlyBrainDemoController> Controller;
    TWeakObjectPtr<UBrainSimulationSubsystem> Brain;
    TSharedPtr<STextBlock> StatusText, FrameText, TimeText, StepsText, SpeedText, RateText, CpuText, BacklogText, RamText;
    double LastSample = 0;
    int64 LastSteps = 0, LastSpikes = 0;
    int Frames = 0;
    bool WasRunning = false;
};
}

void AFlyBrainDemoController::BeginPlay()
{
    Super::BeginPlay();
    if (IsLocalController() && GEngine && GEngine->GameViewport)
    {
        Dashboard = SNew(SBrainDashboard).Controller(this);
        GEngine->GameViewport->AddViewportWidgetContent(Dashboard.ToSharedRef(), 10);
        bShowMouseCursor = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputMode);
    }
}

void AFlyBrainDemoController::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Dashboard.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(Dashboard.ToSharedRef());
    }
    Dashboard.Reset();
    Super::EndPlay(Reason);
}

void AFlyBrainDemoController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AFlyBrainDemoController::ToggleBrain);
    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AFlyBrainDemoController::DemoReset);
    InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AFlyBrainDemoController::DemoCueA);
    InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AFlyBrainDemoController::DemoCueB);
    InputComponent->BindKey(EKeys::T, IE_Pressed, this, &AFlyBrainDemoController::DemoTrain);
    InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AFlyBrainDemoController::DemoCompare);
}
void AFlyBrainDemoController::DemoStart() { GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->Start(); }
void AFlyBrainDemoController::DemoStop() { GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->Stop(); }
void AFlyBrainDemoController::DemoReset() { GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->Reset(); }
void AFlyBrainDemoController::DemoCueA() { GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->StimulateA(); }
void AFlyBrainDemoController::DemoCueB() { GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->StimulateB(); }
void AFlyBrainDemoController::DemoTrain() { GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->TrainA(); }
void AFlyBrainDemoController::DemoCompare() { GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->RunComparison(); }
void AFlyBrainDemoController::DemoForget() { GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->ForgetLearning(); }
void AFlyBrainDemoController::DemoCapture(float DelaySeconds)
{
    FTimerHandle CaptureTimer;
    GetWorldTimerManager().SetTimer(CaptureTimer, FTimerDelegate::CreateWeakLambda(this, [this]
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/FlyDemo.png"), true, false);
        const auto* Brain = GetWorld()->GetSubsystem<UBrainSimulationSubsystem>();
        UE_LOG(LogTemp, Display, TEXT("FlyDemo capture: neurons=%d synapses=%d steps=%lld brain_update_ms=%.4f ram_mb=%.1f"),
            Brain->GetNeuronCount(), Brain->GetSynapseCount(), Brain->GetStepCount(), Brain->GetLastUpdateMilliseconds(),
            FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0));
        const auto& E = Brain->GetExperiment();
        UE_LOG(LogTemp, Display, TEXT("Conditioning: phase=%s A=%.1f->%.1f B=%.1f->%.1f weightA=%.6f weightB=%.6f"),
            UTF8_TO_TCHAR(E.GetPhaseName()), E.BeforeA.Hertz, E.AfterA.Hertz, E.BeforeB.Hertz, E.AfterB.Hertz,
            E.GetMeanWeight(Brain->GetCore(), 1), E.GetMeanWeight(Brain->GetCore(), 2));
    }), FMath::Clamp(DelaySeconds, 0.1f, 60.0f), false);
}
void AFlyBrainDemoController::ToggleBrain()
{
    if (GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->IsRunning()) { DemoStop(); } else { DemoStart(); }
}
