#include "KadhemVRPawn.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/PlayerState.h"
#include "EnhancedInputComponent.h"  // For UEnhancedInputComponent
#include "EnhancedInputSubsystems.h" // For UEnhancedInputLocalPlayerSubsystem
#include "InputActionValue.h"        // For FInputActionValue
#include "EnhancedInputLibrary.h"
#include "EnhancedPlayerInput.h"

AKadhemVRPawn::AKadhemVRPawn() {
  PrimaryActorTick.bCanEverTick = true;

  RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

  VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
  VROrigin->SetupAttachment(RootComponent);

  VRCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VRCamera"));
  VRCamera->SetupAttachment(VROrigin);

  MotionControllerLeft = CreateDefaultSubobject<UMotionControllerComponent>(
      TEXT("MotionControllerLeft"));
  MotionControllerLeft->SetupAttachment(VROrigin);
  MotionControllerLeft->SetTrackingSource(EControllerHand::Left);

  MotionControllerRight = CreateDefaultSubobject<UMotionControllerComponent>(
      TEXT("MotionControllerRight"));
  MotionControllerRight->SetupAttachment(VROrigin);
  MotionControllerRight->SetTrackingSource(EControllerHand::Right);

  static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC_Ref(
      TEXT("/Game/Kadhem/Input/IMC_Player"));
  if (IMC_Ref.Succeeded()) {
    IMC_Player = IMC_Ref.Object;
    UE_LOG(LogTemp, Warning, TEXT("found IMC_Player!"));
  } else {
    UE_LOG(LogTemp, Error, TEXT("Failed to find IMC_Player!"));
  }

  static ConstructorHelpers::FObjectFinder<UInputAction> IM1_Ref(
      TEXT("/Game/Kadhem/Input/IA_MoveForward"));
  if (IM1_Ref.Succeeded()) {
    IA_MoveForward = IM1_Ref.Object;
    UE_LOG(LogTemp, Warning, TEXT("found IM1_Ref!"));
  } else {
    UE_LOG(LogTemp, Error, TEXT("Failed to find IM1_Ref!"));
  }

  static ConstructorHelpers::FObjectFinder<UInputAction> IM2_Ref(
      TEXT("/Game/Kadhem/Input/IA_MoveRight"));
  if (IM2_Ref.Succeeded()) {
    IA_MoveRight = IM2_Ref.Object;
    UE_LOG(LogTemp, Warning, TEXT("found IM2_Ref!"));
  } else {
    UE_LOG(LogTemp, Error, TEXT("Failed to find IM2_Ref!"));
  }
}

void AKadhemVRPawn::BeginPlay() {
  Super::BeginPlay();
  GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow,
                                   TEXT("Begin Play AKadhemVRPawn!"));

  AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AKadhemVRPawn::Tick(float DeltaTime) { Super::Tick(DeltaTime); }

void AKadhemVRPawn::SetupPlayerInputComponent(
    UInputComponent *PlayerInputComponent) {
  Super::SetupPlayerInputComponent(PlayerInputComponent);

  if (APlayerController *PC = Cast<APlayerController>(GetController())) {
    if (ULocalPlayer *LP = PC->GetLocalPlayer()) {
      if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
              LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) {
        Subsystem->AddMappingContext(IMC_Player, 0);
      }
    }
  }

  if (UEnhancedInputComponent *EnhancedInput =
          Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

    EnhancedInput->BindAction(IA_MoveForward, ETriggerEvent::Triggered, this,
                              &AKadhemVRPawn::MoveForward);
    EnhancedInput->BindAction(IA_MoveRight, ETriggerEvent::Triggered, this,
                              &AKadhemVRPawn::MoveRight);
  }
}

void AKadhemVRPawn::MoveForward(const FInputActionInstance &Instance) {

  float AxisValue = Instance.GetValue().Get<float>();

  if (FMath::Abs(AxisValue) < KINDA_SMALL_NUMBER)
    return;

  // Move based on camera direction (ignore pitch)
  FVector Forward = VRCamera->GetForwardVector();
  Forward.Z = 0;
  Forward.Normalize();

  RootComponent->AddWorldOffset(Forward * AxisValue * MoveSpeed *
                                GetWorld()->GetDeltaSeconds());
}

void AKadhemVRPawn::MoveRight(const FInputActionInstance &Instance) {

  float AxisValue = Instance.GetValue().Get<float>();

  if (FMath::Abs(AxisValue) < KINDA_SMALL_NUMBER)
    return;

  FVector Right = VRCamera->GetRightVector();
  Right.Z = 0;
  Right.Normalize();

  RootComponent->AddWorldOffset(Right * AxisValue * MoveSpeed *
                                GetWorld()->GetDeltaSeconds());
}
