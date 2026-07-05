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
	TableCamera->SetRelativeLocation(FVector(0.0f, -900.0f, 680.0f));
	TableCamera->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));
	TableCamera->ProjectionMode = ECameraProjectionMode::Orthographic;
	TableCamera->OrthoWidth = 1500.0f;
	TableCamera->bAutoActivate = true;
}

void ASGSTablePawn::BeginPlay()
{
	Super::BeginPlay();

	SetActorLocation(FVector::ZeroVector);
	SetActorRotation(FRotator::ZeroRotator);
}
