#include "SaveSystem/FoundrylineSaveSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

bool UFoundrylineSaveSubsystem::IsValidSlotName(const FString& Slot)
{
	if (Slot.IsEmpty() || Slot.Len() > 32) return false;
	for (const TCHAR Character : Slot)
	{
		if (!((Character >= TEXT('a') && Character <= TEXT('z'))
			|| (Character >= TEXT('A') && Character <= TEXT('Z'))
			|| (Character >= TEXT('0') && Character <= TEXT('9'))
			|| Character == TEXT('_') || Character == TEXT('-'))) return false;
	}
	return true;
}

EFoundrylineSaveError UFoundrylineSaveSubsystem::CheckTarget(
	const UFoundrylinePlacementSubsystem* Placement, const FString& Slot) const
{
	if (bBusy) return EFoundrylineSaveError::Busy;
	if (!IsValidSlotName(Slot)) return EFoundrylineSaveError::InvalidSlot;
	if (!IsValid(Placement) || !Placement->GetWorld() || Placement->GetWorld()->GetGameInstance() != GetGameInstance())
		return EFoundrylineSaveError::WrongWorld;
	if (Placement->GetWorld()->GetNetMode() == NM_Client) return EFoundrylineSaveError::NotAuthority;
	return EFoundrylineSaveError::None;
}

EFoundrylineSaveError UFoundrylineSaveSubsystem::SavePlacement(UFoundrylinePlacementSubsystem* Placement,
	const FString& Slot, bool bAllowOverwrite)
{
	const auto Check = CheckTarget(Placement, Slot);
	if (Check != EFoundrylineSaveError::None) return Check;
	const FString TargetSlot = StorageSlot(Slot);
	if (!bAllowOverwrite && UGameplayStatics::DoesSaveGameExist(TargetSlot, 0)) return EFoundrylineSaveError::SlotExists;
	PendingSnapshot = Placement->CreateSnapshot();
	if (!PendingSnapshot) return EFoundrylineSaveError::InvalidSnapshot;
	bBusy = true;
	const TWeakObjectPtr<UFoundrylineSaveSubsystem> WeakSelf(this);
	FAsyncSaveGameToSlotDelegate Delegate;
	Delegate.BindLambda([WeakSelf, Slot](const FString&, const int32, bool bSuccess)
	{
		if (!WeakSelf.IsValid()) return;
		auto* Self = WeakSelf.Get();
		Self->PendingSnapshot = nullptr;
		Self->bBusy = false;
		FFoundrylineSaveResult Result;
		Result.Slot = Slot;
		Result.Error = bSuccess ? EFoundrylineSaveError::None : EFoundrylineSaveError::WriteFailed;
		Self->OnCompleted.Broadcast(Result);
	});
	UGameplayStatics::AsyncSaveGameToSlot(PendingSnapshot, TargetSlot, 0, Delegate);
	return EFoundrylineSaveError::None;
}

EFoundrylineSaveError UFoundrylineSaveSubsystem::LoadPlacement(UFoundrylinePlacementSubsystem* Placement,
	const FString& Slot)
{
	const auto Check = CheckTarget(Placement, Slot);
	if (Check != EFoundrylineSaveError::None) return Check;
	bBusy = true;
	const TWeakObjectPtr<UFoundrylineSaveSubsystem> WeakSelf(this);
	const TWeakObjectPtr<UFoundrylinePlacementSubsystem> WeakPlacement(Placement);
	FAsyncLoadGameFromSlotDelegate Delegate;
	Delegate.BindLambda([WeakSelf, WeakPlacement, Slot](const FString&, const int32, USaveGame* Loaded)
	{
		if (!WeakSelf.IsValid()) return;
		auto* Self = WeakSelf.Get();
		FFoundrylineSaveResult Result;
		Result.Slot = Slot;
		if (!WeakPlacement.IsValid() || !WeakPlacement->GetWorld()
			|| Self->GetGameInstance()->GetWorld() != WeakPlacement->GetWorld()) Result.Error = EFoundrylineSaveError::WrongWorld;
		else if (auto* Snapshot = Cast<UFoundrylinePlacementSave>(Loaded))
		{
			for(const auto& O:Snapshot->Objects)
            if(!WeakPlacement->EnsureGeneratedDefinition(O.DefinitionId)) {Result.Error=EFoundrylineSaveError::RestoreRejected;break;}
            if(Result.Error==EFoundrylineSaveError::None) Result.RestoreError = WeakPlacement->RestoreSnapshot(Snapshot,true);
			if (Result.RestoreError != EFoundrylinePlacementError::None) Result.Error = EFoundrylineSaveError::RestoreRejected;
		}
		else Result.Error = EFoundrylineSaveError::ReadFailed;
		Self->bBusy = false;
		Self->OnCompleted.Broadcast(Result);
	});
	UGameplayStatics::AsyncLoadGameFromSlot(StorageSlot(Slot), 0, Delegate);
	return EFoundrylineSaveError::None;
}
