#pragma once

#include "CoreMinimal.h"
#include "GameModes/LyraGameState.h"
#include "GameStateBocaBase.generated.h"

class ARocketActor;
class FSocket;

UENUM(BlueprintType)
enum class EGameCommand : uint8
{
	None,
	SendDataTick,
	SetWhoSendsData,
	SetRocketSetting,
	SpawnAtLocation,
	Engines,
	Raptor,
	Throttle,
	RCS,
	Flaps,
	FoldFlaps,
	GridFins,
	Gimbals,
	SetRCSManual,
	SetDragManual,
	SetGimbalManual,
	Propellant,
	CryotankPressure,
	HotStage,
	DetachHSR,
	FTS,
	OuterGimbalEngines,
	BoosterClamps,
	ControllerAltitude,
	ControllerEastNorth,
	ControllerAttitude,
	AttitudeTarget,
	ChillValve,
	DumpFuel,
	PopEngine,
	BigFlame,
	Chopsticks,
	PadADeluge,
	PadASQDQuickRetract,
	PadAOLMQuickRetract,
	PadABQDQuickRetract,
	MasseyDeluge,
	PadAOLMClampsExtend,
	PadAOLMRQDExtend,
	PadASpawnStack,
	DumpOxidizer,
	LAST
}; // attention, don't change last field here, as this is used for range check in receive funtion!

USTRUCT(BlueprintType)
struct FGameCommandData
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	EGameCommand Command = EGameCommand::None;

	UPROPERTY(BlueprintReadWrite)
	FString target = "";

	UPROPERTY(BlueprintReadWrite)
	bool state = false;

	UPROPERTY(BlueprintReadWrite)
	float value = 0;

	UPROPERTY(BlueprintReadWrite)
	FString parameters = "";

	UPROPERTY(BlueprintReadWrite)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadWrite)
	float EventFloat = 0; // for event based scripting
};


/**
 * 
 */
UCLASS()
class STARBASESIMLIBRARY_API AGameStateBocaBase : public ALyraGameState
{
	GENERATED_BODY()
	
public:

	AGameStateBocaBase();

protected:
	virtual void BeginPlay() override;

private:
	FSocket* ServerSocket;
	FSocket* ClientSocket;

	bool InitializeServer();
	bool AcceptClient();
	void SendAndCheckConnection(FString JsonString);
	void SendData();
	void HandleCommands();

	void SendRocketData(TObjectPtr<ARocketActor>& ARocket);

	bool ReceiveCommands(TArray<FGameCommandData>& OutCommands);

	FString ReceiveBuffer; // Accumulate partial messages

	float LastTimeSendData = 0;


	FString BoosterToSendDataFrom = "B0"; // B0 = last spawned
	FString ShipToSendDataFrom = "S0";

	FString targetString = "";
	
public:

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	TArray<TObjectPtr<ARocketActor>> BoosterArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	TArray<TObjectPtr<ARocketActor>> ShipArray;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<ARocketActor> rocket = nullptr;

	void ProcessCommands(const FGameCommandData& CommandData, TObjectPtr<ARocketActor> ARocket);

	// not used currently
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool DoSendData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SendDataTick = 0.1f;

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_SetRocketSetting(const FString& parameter, float newValue);

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_SpawnAtLocation(const FString& parameter, FVector loc, FRotator rot);

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_StartEngines();

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_StopEngines();

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_SelectRaptor(int number, bool newState);

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_Throttle(float newValue);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_RCS(bool newState);

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_Flaps(bool useFlaps);

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_FoldFlaps(bool foldFlaps);

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_GridFins(bool newState);

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_Gimbals(bool newState);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_SetRCSManual(float newValue, const FString& axis);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_SetDragManual(float newValue, const FString& axis);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_SetGimbalManual(float newValue, const FString& axis);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_Propellant(float newValue);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_CryotankPressure(float newValue);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_HotStage();
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_DetachHSR();

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_FTS();

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_OuterGimbalEngines(bool newState);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_BoosterClamps(bool newState);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_ControllerAltitude(bool newState);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_ControllerEastNorth(bool newState);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_ControllerAttitude(const FString& parameters);

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_AttitudeTarget(float newValue, const FString& axis);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_ChillValve(bool newState);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_DumpFuel(bool newState);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_PopEngine(int number);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_BigFlame(bool newState);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_Chopsticks(float newValue, const FString& parameters);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_PadADeluge(bool newState);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_PadASQDQuickRetract();
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_PadAOLMQuickRetract();
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_PadABQDQuickRetract();
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_MasseyDeluge(bool newState);

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_PadAOLMClampsExtend(bool newState);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_PadAOLMRQDExtend(bool newState);
		
	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_PadASpawnStack();

	UFUNCTION(BlueprintImplementableEvent)
	void CPPCommand_DumpOxidizer(bool newState);
	
};
