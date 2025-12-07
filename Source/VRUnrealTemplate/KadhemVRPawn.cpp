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
#include "UObject/ConstructorHelpers.h"
#include "Engine/LocalPlayer.h"

AKadhemVRPawn::AKadhemVRPawn() {
  PrimaryActorTick.bCanEverTick = true;

  RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

  VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
  VROrigin->SetupAttachment(RootComponent);

  VRCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VRCamera"));
  VRCamera->SetupAttachment(VROrigin);

  MotionControllerHead =
      CreateDefaultSubobject<UKadhemMotionControllerComponent>(
          TEXT("MotionControllerHead"));
  MotionControllerHead->SetupAttachment(VROrigin);
  MotionControllerHead->SetTrackingSource(EControllerHand::HMD);
  MotionControllerHead->bDisableLowLatencyUpdate = false;

  MotionControllerLeft =
      CreateDefaultSubobject<UKadhemMotionControllerComponent>(
          TEXT("MotionControllerLeft"));
  MotionControllerLeft->SetupAttachment(VROrigin);
  MotionControllerLeft->SetTrackingSource(EControllerHand::Left);
  MotionControllerLeft->bDisableLowLatencyUpdate = false;

  MotionControllerRight =
      CreateDefaultSubobject<UKadhemMotionControllerComponent>(
          TEXT("MotionControllerRight"));
  MotionControllerRight->SetupAttachment(VROrigin);
  MotionControllerRight->SetTrackingSource(EControllerHand::Right);
  MotionControllerRight->bDisableLowLatencyUpdate = false;

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

  static ConstructorHelpers::FObjectFinder<UInputAction> IM3_Ref(
      TEXT("/Game/Kadhem/Input/IA_EnableLeftLimbMotion"));
  if (IM3_Ref.Succeeded()) {
    IA_EnableLeftLimbMotion = IM3_Ref.Object;
    UE_LOG(LogTemp, Warning, TEXT("found IM3_Ref!"));
  } else {
    UE_LOG(LogTemp, Error, TEXT("Failed to find IM3_Ref!"));
  }
  static ConstructorHelpers::FObjectFinder<UInputAction> IM4_Ref(
      TEXT("/Game/Kadhem/Input/IA_EnableRightLimbMotion"));
  if (IM4_Ref.Succeeded()) {
    IA_EnableRightLimbMotion = IM4_Ref.Object;
    UE_LOG(LogTemp, Warning, TEXT("found IM4_Ref!"));
  } else {
    UE_LOG(LogTemp, Error, TEXT("Failed to find IM4_Ref!"));
  }
  static ConstructorHelpers::FObjectFinder<UInputAction> IM5_Ref(
      TEXT("/Game/Kadhem/Input/IA_SwitchLowerLeftLimb"));
  if (IM5_Ref.Succeeded()) {
    IA_SwitchLowerLeftLimb = IM5_Ref.Object;
    UE_LOG(LogTemp, Warning, TEXT("found IM5_Ref!"));
  } else {
    UE_LOG(LogTemp, Error, TEXT("Failed to find IM5_Ref!"));
  }
  static ConstructorHelpers::FObjectFinder<UInputAction> IM6_Ref(
      TEXT("/Game/Kadhem/Input/IA_SwitchLowerRightLimb"));
  if (IM6_Ref.Succeeded()) {
    IA_SwitchLowerRightLimb = IM6_Ref.Object;
    UE_LOG(LogTemp, Warning, TEXT("found IM6_Ref!"));
  } else {
    UE_LOG(LogTemp, Error, TEXT("Failed to find IM6_Ref!"));
  }
  static ConstructorHelpers::FObjectFinder<UInputAction> IM7_Ref(
      TEXT("/Game/Kadhem/Input/IA_SwitchUpperLeftLimb"));
  if (IM7_Ref.Succeeded()) {
    IA_SwitchUpperLeftLimb = IM7_Ref.Object;
    UE_LOG(LogTemp, Warning, TEXT("found IM7_Ref!"));
  } else {
    UE_LOG(LogTemp, Error, TEXT("Failed to find IM7_Ref!"));
  }
  static ConstructorHelpers::FObjectFinder<UInputAction> IM8_Ref(
      TEXT("/Game/Kadhem/Input/IA_SwitchUpperRightLimb"));
  if (IM8_Ref.Succeeded()) {
    IA_SwitchUpperRightLimb = IM8_Ref.Object;
    UE_LOG(LogTemp, Warning, TEXT("found IM8_Ref!"));
  } else {
    UE_LOG(LogTemp, Error, TEXT("Failed to find IM8_Ref!"));
  }
}

void AKadhemVRPawn::BeginPlay() { Super::BeginPlay(); }

void AKadhemVRPawn::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
  // notes on how this engine works (i think)
  // there is on tick happening in the game, actor ticks are ordered by their
  // TickGroup the main ones: TG_PrePhysics, TG_DuringPhysics, TG_PostPhysics
  // physics simulation may execute multiple steps internally.
  //
  // we can safely modify physics data in TG_PrePhysics, like add forces for our
  // case. Forces set are just used for one simulation (one game tick), so to
  // apply a force continiously we must add the force each tick.
  //
  // TODO:
  // where to apply the force, at tip of hand or COM ?
  // i feel tip of hand, but need to try both
  //
  // apply different join types for upper-forarm and forarm-hand
  //
  // General Unreal questions:
  //
  //
}

void AKadhemVRPawn::SetupPlayerInputComponent(
    UInputComponent *PlayerInputComponent) {
  Super::SetupPlayerInputComponent(PlayerInputComponent);

  if (APlayerController *PC = Cast<APlayerController>(GetController())) {
    if (ULocalPlayer *LP = PC->GetLocalPlayer()) {
      if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
              LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) {
        Subsystem->AddMappingContext(IMC_Player, 0);
        EnhancedInputLocalPlayerSubsystem = Subsystem;
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

FInputActionValue AKadhemVRPawn::GetEnableRightLimbMotion() {

  FInputActionValue Value =
      EnhancedInputLocalPlayerSubsystem->GetPlayerInput()->GetActionValue(
          IA_EnableRightLimbMotion);

  return Value;
}

FInputActionValue AKadhemVRPawn::GetEnableLeftLimbMotion() {

  FInputActionValue Value =
      EnhancedInputLocalPlayerSubsystem->GetPlayerInput()->GetActionValue(
          IA_EnableLeftLimbMotion);

  return Value;
}

PlayerInput AKadhemVRPawn::GetPlayerInput() {
  PlayerInput result;

  result.EnableRightLimbMotion =
      EnhancedInputLocalPlayerSubsystem->GetPlayerInput()->GetActionValue(
          IA_EnableRightLimbMotion);

  result.EnableLeftLimbMotion =
      EnhancedInputLocalPlayerSubsystem->GetPlayerInput()->GetActionValue(
          IA_EnableLeftLimbMotion);

  result.SwitchLowerLeftLimb =
      EnhancedInputLocalPlayerSubsystem->GetPlayerInput()->GetActionValue(
          IA_SwitchLowerLeftLimb);

  result.SwitchLowerRightLimb =
      EnhancedInputLocalPlayerSubsystem->GetPlayerInput()->GetActionValue(
          IA_SwitchLowerRightLimb);

  result.SwitchUpperLeftLimb =
      EnhancedInputLocalPlayerSubsystem->GetPlayerInput()->GetActionValue(
          IA_SwitchUpperLeftLimb);

  result.SwitchUpperRightLimb =
      EnhancedInputLocalPlayerSubsystem->GetPlayerInput()->GetActionValue(
          IA_SwitchUpperRightLimb);
  return result;
}
