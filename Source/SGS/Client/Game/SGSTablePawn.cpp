#include "Client/Game/SGSTablePawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

ASGSTablePawn::ASGSTablePawn()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	TableCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TableCamera"));
	TableCamera->SetupAttachment(SceneRoot);
	// 牌桌是纯俯视视图。旧的 Y 轴偏移加 -55 度斜视没有对准世界原点，
	// 在 PIE 窗口边缘会暴露大片编辑器网格，且让 HUD 看起来像跟错了摄像机。
	TableCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 1200.0f));
	TableCamera->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	TableCamera->ProjectionMode = ECameraProjectionMode::Orthographic;
	TableCamera->OrthoWidth = 1500.0f;
	TableCamera->bConstrainAspectRatio = false;
	TableCamera->bAutoActivate = true;
}

void ASGSTablePawn::BeginPlay()
{
	Super::BeginPlay();

	SetActorLocation(FVector::ZeroVector);
	SetActorRotation(FRotator::ZeroRotator);
}
