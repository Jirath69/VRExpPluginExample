// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "Components/PrimitiveComponent.h"
#include "Grippables/GrippablePhysicsReplication.h"
#include "CollisionIgnoreSubsystem.generated.h"
//#include "GrippablePhysicsReplication.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(VRE_CollisionIgnoreLog, Log, All);

struct FCollisionPrimPair
{
	TWeakObjectPtr<UPrimitiveComponent> Prim1;
	TWeakObjectPtr<UPrimitiveComponent> Prim2;

	FORCEINLINE bool operator==(const FCollisionPrimPair& Other) const
	{
		return (
			(Prim1 == Other.Prim1 || Prim1 == Other.Prim2) &&
			(Prim2 == Other.Prim1 || Prim2 == Other.Prim1)
			);
	}

	friend uint32 GetTypeHash(FCollisionPrimPair InKey)
	{
		return GetTypeHash(InKey.Prim1) ^ GetTypeHash(InKey.Prim2);
	}
};

struct FCollisionIgnorePair
{
	///FCollisionPrimPair PrimitivePair;
	FPhysicsActorHandle Actor1;
	FName BoneName1;
	FPhysicsActorHandle Actor2;
	FName BoneName2;

	FORCEINLINE bool operator==(const FCollisionIgnorePair& Other) const
	{
		return (
			(BoneName1 == Other.BoneName1 || BoneName1 == Other.BoneName2) &&
			(BoneName2 == Other.BoneName2 || BoneName2 == Other.BoneName1)
			);
	}

	FORCEINLINE bool operator==(const FName& Other) const
	{
		return (BoneName1 == Other || BoneName2 == Other);
	}
};

UCLASS()
class VREXPANSIONPLUGIN_API UCollisionIgnoreSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:


	UCollisionIgnoreSubsystem() :
		Super()
	{
	}

	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override
	{
		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
		// Not allowing for editor type as this is a replication subsystem
	}

	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override
	{
		Super::Initialize(Collection);
	}

	virtual void Deinitialize() override
	{
		Super::Deinitialize();

		if (UpdateHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(UpdateHandle);
		}
	}

	TMap<FCollisionPrimPair, TArray<FCollisionIgnorePair>> CollisionTrackedPairs;
	//TArray<FCollisionIgnorePair> CollisionTrackedPairs;
	TMap<FCollisionPrimPair, TArray<FCollisionIgnorePair>> RemovedPairs;
	//TArray<FCollisionIgnorePair> RemovedPairs;

	//
	void UpdateTimer()
	{

#if PHYSICS_INTERFACE_PHYSX
		for (const TPair<FCollisionPrimPair, TArray<FCollisionIgnorePair>>& Pair : RemovedPairs)
		{
			bool bSkipPrim1 = false;
			bool bSkipPrim2 = false;

			if (!Pair.Key.Prim1.IsValid())
				bSkipPrim1 = true;

			if (!Pair.Key.Prim2.IsValid())
				bSkipPrim2 = true;

			if (!bSkipPrim1 || !bSkipPrim2)
			{
				for (const FCollisionIgnorePair& BonePair : Pair.Value)
				{
					bool bPrim1Exists = false;
					bool bPrim2Exists = false;

					for (const TPair<FCollisionPrimPair, TArray<FCollisionIgnorePair>>& KeyPair : CollisionTrackedPairs)
					{
						if (!bPrim1Exists && !bSkipPrim1)
						{
							if (KeyPair.Key.Prim1 == Pair.Key.Prim1)
							{
								bPrim1Exists = KeyPair.Value.ContainsByPredicate([BonePair](const FCollisionIgnorePair& Other)
									{
										return BonePair.BoneName1 == Other.BoneName1;
									});
							}
							else if (KeyPair.Key.Prim2 == Pair.Key.Prim1)
							{
								bPrim1Exists = KeyPair.Value.ContainsByPredicate([BonePair](const FCollisionIgnorePair& Other)
									{
										return BonePair.BoneName1 == Other.BoneName2;
									});
							}
						}

						if (!bPrim2Exists && !bSkipPrim2)
						{
							if (KeyPair.Key.Prim1 == Pair.Key.Prim2)
							{
								bPrim2Exists = KeyPair.Value.ContainsByPredicate([BonePair](const FCollisionIgnorePair& Other)
									{
										return BonePair.BoneName2 == Other.BoneName1;
									});
							}
							else if (KeyPair.Key.Prim2 == Pair.Key.Prim2)
							{
								bPrim2Exists = KeyPair.Value.ContainsByPredicate([BonePair](const FCollisionIgnorePair& Other)
									{
										return BonePair.BoneName2 == Other.BoneName2;
									});
							}
						}


						if ((bPrim1Exists || bSkipPrim1) && (bPrim2Exists || bSkipPrim2))
						{
							break; // Exit early
						}
					}

					if (!bPrim1Exists && !bSkipPrim1)
					{
						Pair.Key.Prim1->GetBodyInstance(BonePair.BoneName1)->SetContactModification(false);
					}


					if (!bPrim2Exists && !bSkipPrim2)
					{
						Pair.Key.Prim2->GetBodyInstance(BonePair.BoneName2)->SetContactModification(false);
					}
				}
			}
		}

#endif
		RemovedPairs.Reset();

		if (CollisionTrackedPairs.Num() > 0)
		{
			if (!UpdateHandle.IsValid())
			{
				GetWorld()->GetTimerManager().SetTimer(UpdateHandle, this, &UCollisionIgnoreSubsystem::CheckActiveFilters, 1.0f, true, 1.0f);
			}
		}
		else if (UpdateHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(UpdateHandle);
		}
	}

	UFUNCTION(Category = "Collision")
		void CheckActiveFilters();

	void InitiateIgnore();

	void SetComponentCollisionIgnoreState(bool bIterateChildren1, bool bIterateChildren2, UPrimitiveComponent* Prim1, FName OptionalBoneName1, UPrimitiveComponent* Prim2, FName OptionalBoneName2, bool bIgnoreCollision);

private:

	FTimerHandle UpdateHandle;

};
