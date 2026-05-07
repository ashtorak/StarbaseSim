// see readme.txt

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeroCppComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STARBASESIMLIBRARY_API UHeroCppComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHeroCppComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void OnApplicationDeactivate();
	void OnApplicationReactivate();
	void OnWindowFocusChanged(bool bIsFocused);

	UFUNCTION(BlueprintCallable)
	bool getLogChat();
};
