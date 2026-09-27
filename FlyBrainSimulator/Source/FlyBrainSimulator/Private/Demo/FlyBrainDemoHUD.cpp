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
                    + SVerticalBox::Slot().AutoHeight().Padding(1, 7)[Label(TEXT("NEURAL SIMULATION LAB     /     CPU DEMO 01"), 10, Muted)]
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
                            + SVerticalBox::Slot().AutoHeight()[Label(TEXT("LIF neurons  /  16,000 synapses"), 12, Muted)]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 18)[SNew(SSeparator).SeparatorImage(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(Muted * 0.3f)]
                            + SVerticalBox::Slot().AutoHeight()[Stats]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 10)[Label(TEXT("MEMBRANE POTENTIAL  /  ALL NEURONS"), 10, Muted)]
                            + SVerticalBox::Slot().AutoHeight()[SNew(SNeuronField).Brain(Brain)]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10)[Label(TEXT("-65 mV    REST                      THRESHOLD    -50 mV"), 8, Muted)]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 6)[Label(TEXT("Fixed 1 ms  /  Seed 1  /  Deterministic"), 10, Muted)]
                        ]
                    ]
                ]
                + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(32, 0, 32, 123)
                [ SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[Label(TEXT("DROSOPHILA / STYLIZED STUDY"), 12, Accent)]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 7)[Label(TEXT("Motion follows simulation time. Illustrative, not a motor model."), 10, Muted)]
                ]
                + SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(32, 0, 32, 24)
                [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Panel).Padding(20, 16)
                    [ SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("START"), Accent, [this] { if (Controller.IsValid()) { Controller->DemoStart(); } return FReply::Handled(); }, true)]
                        + SHorizontalBox::Slot().AutoWidth().Padding(10, 0)[Button(TEXT("STOP"), Ink, [this] { if (Controller.IsValid()) { Controller->DemoStop(); } return FReply::Handled(); }, false)]
                        + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("RESET"), Muted, [this] { if (Controller.IsValid()) { Controller->DemoReset(); } return FReply::Handled(); }, false)]
                        + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).HAlign(HAlign_Right)
                        [Label(TEXT("SPACE  Start / Stop      R  Reset"), 11, Muted)]
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
}
void AFlyBrainDemoController::DemoStart() { GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->Start(); }
void AFlyBrainDemoController::DemoStop() { GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->Stop(); }
void AFlyBrainDemoController::DemoReset() { GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->Reset(); }
void AFlyBrainDemoController::DemoCapture()
{
    FTimerHandle CaptureTimer;
    GetWorldTimerManager().SetTimer(CaptureTimer, FTimerDelegate::CreateWeakLambda(this, [this]
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/FlyDemo.png"), true, false);
        const auto* Brain = GetWorld()->GetSubsystem<UBrainSimulationSubsystem>();
        UE_LOG(LogTemp, Display, TEXT("FlyDemo capture: neurons=%d synapses=%d steps=%lld brain_update_ms=%.4f ram_mb=%.1f"),
            Brain->GetNeuronCount(), Brain->GetSynapseCount(), Brain->GetStepCount(), Brain->GetLastUpdateMilliseconds(),
            FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0));
    }), 5.0f, false);
}
void AFlyBrainDemoController::ToggleBrain()
{
    if (GetWorld()->GetSubsystem<UBrainSimulationSubsystem>()->IsRunning()) { DemoStop(); } else { DemoStart(); }
}
