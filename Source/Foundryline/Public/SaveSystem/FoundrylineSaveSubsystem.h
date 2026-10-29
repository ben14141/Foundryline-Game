#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "FoundrylineSaveSubsystem.generated.h"

enum class EFoundrylineSaveError : uint8
{
	None, Busy, InvalidSlot, WrongWorld, NotAuthority, SlotExists, InvalidSnapshot, ReadFailed, WriteFailed, RestoreRejected
};

struct FOUNDRYLINE_API FFoundrylineSaveResult
{
	FString Slot;
	EFoundrylineSaveError Error = EFoundrylineSaveError::None;
	EFoundrylinePlacementError RestoreError = EFoundrylinePlacementError::None;
	bool IsSuccess() const { return Error == EFoundrylineSaveError::None; }
};

DECLARE_MULTICAST_DELEGATE_OneParam(FFoundrylineSaveCompleted, const FFoundrylineSaveResult&);

// Explicit operations only: no automatic saves, overwrites, deletions or startup loads.
UCLASS()
class FOUNDRYLINE_API UFoundrylineSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	// None means asynchronous operation accepted; final result arrives via OnCompleted.
	EFoundrylineSaveError SavePlacement(UFoundrylinePlacementSubsystem* Placement,
		const FString& Slot, bool bAllowOverwrite = false);
	EFoundrylineSaveError LoadPlacement(UFoundrylinePlacementSubsystem* Placement, const FString& Slot);
	bool IsBusy() const { return bBusy; }
	static bool IsValidSlotName(const FString& Slot);
	FFoundrylineSaveCompleted OnCompleted;

private:
	EFoundrylineSaveError CheckTarget(const UFoundrylinePlacementSubsystem* Placement, const FString& Slot) const;
	static FString StorageSlot(const FString& Slot) { return TEXT("Foundryline_") + Slot; }
	bool bBusy = false;
	UPROPERTY(Transient)
	TObjectPtr<UFoundrylinePlacementSave> PendingSnapshot;
};
