// thought to use this for the API instead of GameState class,
// but forgot that i need the rocket arrays, so reverted back to GameState class 
// for the API stuff. However, this is still here as example for how to use subsystem
// that is ticking in async physics.

//#pragma once
//
//#include "CoreMinimal.h"
//#include "Subsystems/WorldSubsystem.h"
//#include "Chaos/SimCallbackObject.h"
//#include "APISubsystem.generated.h"
//
//class FSubsystemAsyncPhysicsTickCallback : public Chaos::TSimCallbackObject<
//	Chaos::FSimCallbackNoInput,
//	Chaos::FSimCallbackNoOutput,
//	Chaos::ESimCallbackOptions::Presimulate | Chaos::ESimCallbackOptions::RunOnFrozenGameThread>
//{
//public:
//	FSubsystemAsyncPhysicsTickCallback() = default;
//
//	void SetSubsystem(class UAPISubsystem* InSubsystem) { Subsystem = InSubsystem; }
//
//	virtual void OnPreSimulate_Internal() override;
//
//	/*virtual FName GetFNameFor() override
//	{
//		static FLazyName StaticName("FSubsystemAsyncPhysicsTickCallback");
//		return StaticName;
//	}*/
//
//	virtual FName GetFNameForStatId() const override
//	{
//		static FLazyName StaticName("FSubsystemAsyncPhysicsTickCallback");
//		return StaticName;
//	}
//
//private:
//	TWeakObjectPtr<class UAPISubsystem> Subsystem;
//};
//
//class FSocket;
//class ARocketActor;
//
//UENUM(BlueprintType)
//enum class EGameCommand : uint8
//{
//	None,
//	SendDataTick,
//	SetWhoSendsData,
//	SetRocketSetting,
//	SpawnAtLocation,
//	Engines,
//	Raptor,
//	Throttle,
//	RCS,
//	Flaps,
//	FoldFlaps,
//	GridFins,
//	Gimbals,
//	SetRCSManual,
//	SetDragManual,
//	SetGimbalManual,
//	Propellant,
//	CryotankPressure,
//	HotStage,
//	DetachHSR,
//	FTS,
//	OuterGimbalEngines,
//	BoosterClamps,
//	ControllerAltitude,
//	ControllerEastNorth,
//	ControllerAttitude,
//	AttitudeTarget,
//	ChillValve,
//	DumpFuel,
//	PopEngine,
//	BigFlame,
//	Chopsticks,
//	PadADeluge,
//	PadASQDQuickRetract,
//	PadAOLMQuickRetract,
//	PadABQDQuickRetract,
//	MasseyDeluge,
//};
//
//USTRUCT(BlueprintType)
//struct FGameCommandData
//{
//	GENERATED_BODY()
//
//public:
//	UPROPERTY(BlueprintReadWrite)
//	EGameCommand Command = EGameCommand::None;
//
//	UPROPERTY(BlueprintReadWrite)
//	FString target = "";
//
//	UPROPERTY(BlueprintReadWrite)
//	bool state = false;
//
//	UPROPERTY(BlueprintReadWrite)
//	float value = 0;
//
//	UPROPERTY(BlueprintReadWrite)
//	FString parameters = "";
//
//	UPROPERTY(BlueprintReadWrite)
//	FVector Location = FVector::ZeroVector;
//
//	UPROPERTY(BlueprintReadWrite)
//	FRotator Rotation = FRotator::ZeroRotator;
//
//	float EventFloat = 0; // for event based scripting
//};
//
//UCLASS()
//class STARBASESIMLIBRARY_API UAPISubsystem : public UWorldSubsystem
//{
//    GENERATED_BODY()
//
//public:
//    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
//    virtual void Deinitialize() override;
//
//    // Async Physics Tick callback
//    void AsyncPhysicsTick(float DeltaTime);
//
//private:
//    // Delegate handle for the async physics tick
//    FDelegateHandle AsyncPhysicsTickHandle;
//
//	// Timer handle for delayed initialization
//	FTimerHandle DelayedInitTimerHandle;
//
//	// Custom async physics tick callback
//	FSubsystemAsyncPhysicsTickCallback* AsyncPhysicsCallback;
//
//	// Function to register async physics callback
//	void RegisterAsyncPhysicsCallback();
//
//private:
//	FSocket* ServerSocket;
//	FSocket* ClientSocket;
//
//	bool InitializeServer();
//	bool AcceptClient();
//	void SendAndCheckConnection(FString JsonString);
//	void SendData();
//	void HandleCommands();
//
//	void SendTransformData(const FString& ObjectName, const FTransform& Transform);
//
//	bool ReceiveCommands(TArray<FGameCommandData>& OutCommands);
//
//	FString ReceiveBuffer; // Accumulate partial messages
//
//	float LastTimeSendData = 0;
//
//
//	FString BoosterToSendDataFrom = "B0"; // B0 = last spawned
//	FString ShipToSendDataFrom = "S0";
//
//	FString targetString = "";
//
//public:
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//	TObjectPtr<ARocketActor> rocket = nullptr;
//
//	void ProcessCommands(const FGameCommandData& CommandData, TObjectPtr<ARocketActor> ARocket);
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//	TArray<TObjectPtr<ARocketActor>> BoosterArray;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//	TArray<TObjectPtr<ARocketActor>> ShipArray;
//
//	// not used currently
//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//	bool DoSendData;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//	float SendDataTick = 0.1f;
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_SetRocketSetting(const FString& parameter, float newValue);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_SpawnAtLocation(const FString& parameter, FVector loc, FRotator rot);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_StartEngines();
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_StopEngines();
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_SelectRaptor(int number, bool newState);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_Throttle(float newValue);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_RCS(bool newState);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_Flaps(bool useFlaps);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_FoldFlaps(bool foldFlaps);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_GridFins(bool newState);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_Gimbals(bool newState);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_SetRCSManual(float newValue, const FString& axis);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_SetDragManual(float newValue, const FString& axis);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_SetGimbalManual(float newValue, const FString& axis);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_Propellant(float newValue);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_CryotankPressure(float newValue);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_HotStage();
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_DetachHSR();
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_FTS();
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_OuterGimbalEngines(bool newState);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_BoosterClamps(bool newState);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_ControllerAltitude(bool newState);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_ControllerEastNorth(bool newState);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_ControllerAttitude(bool isOpenLoop, const FString& parameters);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_AttitudeTarget(float newValue, const FString& axis);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_ChillValve(bool newState);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_DumpFuel(bool newState);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_PopEngine(int number);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_BigFlame(bool newState);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_Chopsticks(bool newState, float newValue, const FString& parameters);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_PadADeluge(bool newState);
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_PadASQDQuickRetract();
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_PadAOLMQuickRetract();
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_PadABQDQuickRetract();
//
//	UFUNCTION(BlueprintImplementableEvent)
//	void CPPCommand_MasseyDeluge(bool newState);
//
//};