#include "RTSPluginPrivatePCH.h"
#include "RTSCharacter.h"

#include "Kismet/GameplayStatics.h"

#include "RTSAttackComponent.h"
#include "RTSGameMode.h"
#include "RTSHealthComponent.h"


void ARTSCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Cache component references.
	AttackComponent = FindComponentByClass<URTSAttackComponent>();
}

void ARTSCharacter::Tick(float DeltaSeconds)
{
	// Update cooldown timer.
	if (AttackComponent && AttackComponent->RemainingCooldown > 0)
	{
		AttackComponent->RemainingCooldown -= DeltaSeconds;
	}
}

float ARTSCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	// Adjust health.
	URTSHealthComponent* HealthComponent = FindComponentByClass<URTSHealthComponent>();

	if (!HealthComponent)
	{
		return 0.0f;
	}

	HealthComponent->CurrentHealth -= Damage;
	
	UE_LOG(RTSLog, Log, TEXT("Character %s has taken %f damage from %s, reducing health to %f."),
		*GetName(),
		Damage,
		*DamageCauser->GetName(),
		HealthComponent->CurrentHealth);

	// Check if we've just died.
	if (HealthComponent->CurrentHealth <= 0)
	{
		// Get owner before destruction.
		AController* Owner = Cast<AController>(GetOwner());

		// Destroy this actor.
		Destroy();
		UE_LOG(RTSLog, Log, TEXT("Character %s has been killed."), *GetName());

		// Notify game mode.
		ARTSGameMode* GameMode = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(GetWorld()));

		if (GameMode != nullptr)
		{
			GameMode->NotifyOnCharacterKilled(this, Owner);
		}
	}

	return ActualDamage;
}

void ARTSCharacter::UseAttack(int AttackIndex, AActor* Target)
{
	if (AttackComponent == nullptr)
	{
		return;
	}

	// Check cooldown.
	if (AttackComponent->RemainingCooldown > 0)
	{
		return;
	}

	// Use attack.
	UE_LOG(RTSLog, Log, TEXT("Actor %s attacks %s."), *GetName(), *Target->GetName());

	const FRTSAttackData& Attack = AttackComponent->Attacks[0];
	Target->TakeDamage(Attack.Damage, FDamageEvent(Attack.DamageType), GetController(), this);

	// Start cooldown timer.
	AttackComponent->RemainingCooldown = Attack.Cooldown;

	// Notify listeners.
	NotifyOnUsedAttack(Attack, Target);
}

void ARTSCharacter::NotifyOnUsedAttack(const FRTSAttackData& Attack, AActor* Target)
{
	ReceiveOnUsedAttack(Attack, Target);
}
