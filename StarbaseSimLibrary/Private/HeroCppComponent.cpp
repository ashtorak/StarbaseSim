// see readme.txt

#include "HeroCppComponent.h"
#include "Settings/LyraSettingsLocal.h"
#include <Kismet/GameplayStatics.h>
#include "Misc/CoreDelegates.h"
#include "Framework/Application/SlateApplication.h"

// Sets default values for this component's properties
UHeroCppComponent::UHeroCppComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UHeroCppComponent::BeginPlay()
{
	Super::BeginPlay();

	// Bind to application deactivation and reactivation events. ONLY WORKS ON MOBILE OR SO
	FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &UHeroCppComponent::OnApplicationDeactivate);
	FCoreDelegates::ApplicationHasReactivatedDelegate.AddUObject(this, &UHeroCppComponent::OnApplicationReactivate);

	// This works on PC (Windows)
	FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this, &UHeroCppComponent::OnWindowFocusChanged);
}


// Called every frame
void UHeroCppComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UHeroCppComponent::OnApplicationDeactivate()
{
	// Pause the game when the application loses focus
	UGameplayStatics::SetGamePaused(GetWorld(), true);
}

void UHeroCppComponent::OnApplicationReactivate()
{
	// Resume the game when the application regains focus
	UGameplayStatics::SetGamePaused(GetWorld(), false);
}

void UHeroCppComponent::OnWindowFocusChanged(bool bIsFocused)
{
	//UE_LOG(LogTemp, Log, TEXT("NetMode: %d"), GetWorld()->GetNetMode());
	//#if !WITH_EDITOR
	if (bIsFocused)
	{
		UGameplayStatics::SetGamePaused(GetWorld(), false);
	}
	else if (GetNetMode() == ENetMode::NM_Standalone && GetDefault<ULyraSettingsLocal>()->IsPauseOnFocusLostEnabled())
	{// pause only when in singleplayer and the setting is enabled
		UGameplayStatics::SetGamePaused(GetWorld(), true);
	}
//#endif
}

bool UHeroCppComponent::getLogChat()
{
	return GetDefault<ULyraSettingsLocal>()->IsLogChatEnabled();
}