#include "GameStateBocaBase.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "JsonUtilities.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameModeBase.h"
#include "RocketControllerComponent.h"
#include "PIDComponent.h"
#include "RocketEngines.h"
#include "Kismet/GameplayStatics.h"
#include <Net/UnrealNetwork.h>

AGameStateBocaBase::AGameStateBocaBase()
{
	// Enable ticking for real-time updates
	PrimaryActorTick.bCanEverTick = true;
	ServerSocket = nullptr;
	ClientSocket = nullptr;

	bReplicates = true;
}

void AGameStateBocaBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGameStateBocaBase, BoosterArray);
	DOREPLIFETIME(AGameStateBocaBase, ShipArray);
}

void AGameStateBocaBase::BeginPlay()
{
	Super::BeginPlay();

	ReceiveBuffer.Reserve(4096); // Pre-allocate 4 KB

	if (InitializeServer())
	{
		UE_LOG(LogTemp, Log, TEXT("TCP server started on localhost:12345"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to start TCP server"));
	}
}

bool AGameStateBocaBase::InitializeServer()
{
	ServerSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("TcpServer"), false);
	if (!ServerSocket)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create server socket"));
		return false;
	}

	// Set socket to non-blocking
	bool bSuccess = ServerSocket->SetNonBlocking(true);
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to set server socket to non-blocking"));
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ServerSocket);
		ServerSocket = nullptr;
		return false;
	}

	TSharedPtr<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool isValid;
	Addr->SetIp(*FString("127.0.0.1"), isValid); // Localhost
	Addr->SetPort(12345);

	if (!ServerSocket->Bind(*Addr))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to bind server socket"));
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ServerSocket);
		ServerSocket = nullptr;
		return false;
	}

	if (!ServerSocket->Listen(1))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to listen on server socket"));
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ServerSocket);
		ServerSocket = nullptr;
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Server socket initialized successfully"));
	return true;
}

bool AGameStateBocaBase::AcceptClient()
{
	if (!ServerSocket)
		return false;

	if (ClientSocket)
	{
		ESocketConnectionState ConnectionState = ClientSocket->GetConnectionState();
		if (ConnectionState != SCS_Connected)
		{
			UE_LOG(LogTemp, Log, TEXT("Client disconnected (ConnectionState: %d)"), (int32)ConnectionState);
			ClientSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
			ClientSocket = nullptr;
			ReceiveBuffer.Empty();
			return false;
		}
		else
		{
			return true; // Already connected
		}
	}

	// Accept new client
	TSharedPtr<FInternetAddr> ClientAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	ClientSocket = ServerSocket->Accept(*ClientAddr, TEXT("LocalClient"));
	if (ClientSocket)
	{
		ClientSocket->SetNonBlocking(true);
		UE_LOG(LogTemp, Log, TEXT("Accepted client connection from %s:%d"), *ClientAddr->ToString(true), ClientAddr->GetPort());
		return true;
	}

	return false;
}

void AGameStateBocaBase::SendRocketData(TObjectPtr<ARocketActor>& ARocket)
{
	if (!ClientSocket || ClientSocket->GetConnectionState() != SCS_Connected)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot send response: No client connected"));
		return;
	}

	URocketControllerComponent* rocketComp = ARocket->RocketControllerComp;

	if (!(ARocket && ARocket->RocketControllerComp && ARocket->fuelTank))
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot send response: Rocket or component not ready"));
		return;
	}

	// Serialize transform to JSON
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("objectname"), ARocket->IDstring);

	JsonObject->SetArrayField("location", TArray<TSharedPtr<FJsonValue>>{
		MakeShareable(new FJsonValueNumber(ARocket->GetTransform().GetLocation().X/100)),
			MakeShareable(new FJsonValueNumber(ARocket->GetTransform().GetLocation().Y/100)),
			MakeShareable(new FJsonValueNumber(ARocket->GetTransform().GetLocation().Z/100))
	});

	JsonObject->SetArrayField("rotation", TArray<TSharedPtr<FJsonValue>>{
		MakeShareable(new FJsonValueNumber(ARocket->GetTransform().GetRotation().X)),
			MakeShareable(new FJsonValueNumber(ARocket->GetTransform().GetRotation().Y)),
			MakeShareable(new FJsonValueNumber(ARocket->GetTransform().GetRotation().Z)),
			MakeShareable(new FJsonValueNumber(ARocket->GetTransform().GetRotation().W))
	});

	JsonObject->SetArrayField("velocity", TArray<TSharedPtr<FJsonValue>>{
		MakeShareable(new FJsonValueNumber(ARocket->Velocity.X)),
			MakeShareable(new FJsonValueNumber(ARocket->Velocity.Y)),
			MakeShareable(new FJsonValueNumber(ARocket->Velocity.Z)),
	});

	JsonObject->SetNumberField("enginesThatAreRunningBitmask", ARocket->RocketControllerComp->EnginesThatAreRunningBitmask);

	JsonObject->SetNumberField("oxidizerMass", ARocket->oxidizerTank->liquidMass);
	JsonObject->SetNumberField("oxidizerGasMass", ARocket->oxidizerTank->gasMass);
	JsonObject->SetNumberField("fuelMass", ARocket->fuelTank->liquidMass);
	JsonObject->SetNumberField("fuelGasMass", ARocket->fuelTank->gasMass);
	JsonObject->SetNumberField("turbopumpTemperature", ARocket->turbopumpTemperature);
	if(!(ARocket->RocketControllerComp->Engines->EngineProperties.IsEmpty()))
		JsonObject->SetNumberField("throttle", ARocket->RocketControllerComp->Engines->EngineProperties[0].Throttle);

	if (ARocket->RocketControllerComp->bTrajectoryEnabled && !rocketComp->PredictedPath.IsEmpty())
	{
		JsonObject->SetArrayField("trajectoryLastPoint", TArray<TSharedPtr<FJsonValue>>{
			MakeShareable(new FJsonValueNumber(rocketComp->PredictedPath.Last().Location.X/100)),
			MakeShareable(new FJsonValueNumber(rocketComp->PredictedPath.Last().Location.Y/100)),
			MakeShareable(new FJsonValueNumber(rocketComp->PredictedPath.Last().Location.Z/100)),
		});
	}


	FString jsonString;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&jsonString);
	if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to serialize JSON"));
		return;
	}
	jsonString += TEXT("\n");

	SendAndCheckConnection(jsonString);
}

void AGameStateBocaBase::SendAndCheckConnection(FString JsonString)
{
	int32 BytesSent;
	ClientSocket->Send((uint8*)TCHAR_TO_UTF8(*JsonString), JsonString.Len(), BytesSent);
	if (BytesSent > 0)
	{
		//UE_LOG(LogTemp, Log, TEXT("Sending JSON: %s"), *JsonString);
		//UE_LOG(LogTemp, Log, TEXT("JSON Packet Size: %d bytes"), JsonString.Len() * sizeof(TCHAR));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to send data"));
		// Test connection on send failure
		uint8 TestBuffer[1];
		int32 BytesRead = 0;
		bool bRecvSuccess = ClientSocket->Recv(TestBuffer, 1, BytesRead, ESocketReceiveFlags::None);
		ESocketErrors LastError = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
		if (!bRecvSuccess && ( LastError != SE_EWOULDBLOCK || BytesRead == 0))
		{
			UE_LOG(LogTemp, Log, TEXT("Client disconnected (BytesRead: %d, ErrorCode: %d)"), BytesRead, LastError);
			ClientSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
			
			ClientSocket = nullptr;
			ReceiveBuffer.Empty();
			BoosterToSendDataFrom = "B0"; // B0 = last spawned
			ShipToSendDataFrom = "S0";
		}
	}
}

bool AGameStateBocaBase::ReceiveCommands(TArray<FGameCommandData>& OutCommands)
{
	OutCommands.Empty();
	if (!ClientSocket || ClientSocket->GetConnectionState() != SCS_Connected)
		return false;

	uint8 Buffer[4096];
	int32 BytesRead = 0;
	bool bReceivedData = false;
	while (ClientSocket->Recv(Buffer, 4096, BytesRead, ESocketReceiveFlags::None) && BytesRead > 0)
	{
		Buffer[FMath::Min(BytesRead, 4095)] = 0;
		ReceiveBuffer += FString(UTF8_TO_TCHAR(Buffer));
		bReceivedData = true;
	}

	if (bReceivedData)
	{
		TArray<FString> Messages;
		ReceiveBuffer.ParseIntoArrayLines(Messages, false);

		FString NewBuffer;
		for (const FString& Message : Messages)
		{
			if (Message.IsEmpty())
				continue;

			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
			TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
			FString JsonField = "command";
			if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject->HasField(JsonField))
			{
				FGameCommandData CommandData;
				int32 CommandValue = JsonObject->GetIntegerField(JsonField);
				if (CommandValue >= 0 && CommandValue < static_cast<int32>(EGameCommand::LAST))
					CommandData.Command = static_cast<EGameCommand>(CommandValue);
				else
					CommandData.Command = EGameCommand::None;

				JsonField = "location";
				if (JsonObject->HasField(JsonField))
				{
					const TArray<TSharedPtr<FJsonValue>>* LocationArray;
					if (JsonObject->TryGetArrayField(JsonField, LocationArray) && LocationArray->Num() >= 3)
					{
						CommandData.Location = FVector(
							(*LocationArray)[0]->AsNumber(),
							(*LocationArray)[1]->AsNumber(),
							(*LocationArray)[2]->AsNumber()
						);
					}
				}

				JsonField = "rotation";
				if (JsonObject->HasField(JsonField))
				{
					const TArray<TSharedPtr<FJsonValue>>* RotationArray;
					if (JsonObject->TryGetArrayField(JsonField, RotationArray) && RotationArray->Num() >= 3)
					{
						CommandData.Rotation = FRotator(
							(*RotationArray)[1]->AsNumber(), // Pitch
							(*RotationArray)[0]->AsNumber(), // Yaw
							(*RotationArray)[2]->AsNumber()  // Roll
						);
					}
				}

				JsonField = "target";
				if (JsonObject->HasField(JsonField))
				{
					JsonObject->TryGetStringField(JsonField, CommandData.target);
				}
				
				JsonField = "state";
				if (JsonObject->HasField(JsonField))
				{
					JsonObject->TryGetBoolField(JsonField, CommandData.state);
				}

				JsonField = "value";
				if (JsonObject->HasField(JsonField))
				{
					JsonObject->TryGetNumberField(JsonField, CommandData.value);
					
				}

				JsonField = "parameters";
				if (JsonObject->HasField(JsonField))
				{
					JsonObject->TryGetStringField(JsonField, CommandData.parameters);
				}



				OutCommands.Add(CommandData);
				/*UE_LOG(LogTemp, Log, TEXT("Parsed command: %s, Location: %s, Rotation: %s"),
					*UEnum::GetValueAsString(CommandData.Command),
					*CommandData.Location.ToString(),
					*CommandData.Rotation.ToString());*/
			}
			else
			{
				NewBuffer += Message + TEXT("\n");
			}
		}
		ReceiveBuffer = NewBuffer;
		return OutCommands.Num() > 0;
	}
	return false;
}

void AGameStateBocaBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetWorld()->GetTimeSeconds() < 1.0f)
	{
		return;
	}

	AcceptClient();

	if (ClientSocket && ClientSocket->GetConnectionState() == SCS_Connected)
	{
		SendData();

		HandleCommands();
	}
}

void AGameStateBocaBase::SendData()
{
	if (GetWorld()->GetTimeSeconds() > LastTimeSendData + SendDataTick)
	{
		if (!BoosterArray.IsEmpty())
		{
			if (BoosterToSendDataFrom.Equals(TEXT("B0"), ESearchCase::IgnoreCase))
			{
				rocket = BoosterArray.Last();
			}
			else
			{
				for (TObjectPtr<ARocketActor> rocketIterator : BoosterArray)
				{
					if (rocketIterator && rocketIterator->IDstring.Equals(BoosterToSendDataFrom, ESearchCase::IgnoreCase))
					{
						rocket = rocketIterator;
						break;
					}
				}
			}
			if (rocket) SendRocketData(rocket);
		}
		if (!ShipArray.IsEmpty())
		{
			if (ShipToSendDataFrom.Equals(TEXT("S0"), ESearchCase::IgnoreCase))
			{
				rocket = ShipArray.Last();
			}
			else
			{
				for (TObjectPtr<ARocketActor> rocketIterator : ShipArray)
				{
					if (rocketIterator && rocketIterator->IDstring.Equals(ShipToSendDataFrom, ESearchCase::IgnoreCase))
					{
						rocket = rocketIterator;
						break;
					}
				}
			}
			if (rocket) SendRocketData(rocket);
		}
		// if rocket is null then no data has been sent, but also no connection check performed, so do it again here
		if (!rocket) SendAndCheckConnection(TEXT("Client still there?\n"));
		
		LastTimeSendData = GetWorld()->GetTimeSeconds();
	}
}

void AGameStateBocaBase::HandleCommands()
{
	TArray<FGameCommandData> Commands;
	while (ReceiveCommands(Commands))
	{
		for (const FGameCommandData& CommandData : Commands)
		{
			// setup reference to rocket if any string is given
			rocket = nullptr;
			targetString = CommandData.target;
			if (targetString.StartsWith(TEXT("b")))
			{
				if (targetString.Equals(TEXT("B0"), ESearchCase::IgnoreCase) && !BoosterArray.IsEmpty())
				{
					rocket = BoosterArray.Last();
				}
				else
				{
					for (TObjectPtr<ARocketActor> rocketIterator : BoosterArray)
					{
						if (rocketIterator && rocketIterator->IDstring.Equals(targetString, ESearchCase::IgnoreCase))
						{
							rocket = rocketIterator;
							break;
						}
					}
				}
			}
			else if (targetString.StartsWith(TEXT("s")))
			{
				if (targetString.Equals(TEXT("S0"), ESearchCase::IgnoreCase) && !ShipArray.IsEmpty())
				{
					rocket = ShipArray.Last();
				}
				else
				{
					for (TObjectPtr<ARocketActor> rocketIterator : ShipArray)
					{
						if (rocketIterator && rocketIterator->IDstring.Equals(targetString, ESearchCase::IgnoreCase))
						{
							rocket = rocketIterator;
							break;
						}
					}
				}
			}

			ProcessCommands(CommandData, rocket);
		}
	}
}

void AGameStateBocaBase::ProcessCommands(const FGameCommandData& CommandData, TObjectPtr<ARocketActor> ARocket)
{
	rocket = ARocket;
	targetString = CommandData.target;
	bool stateBool = CommandData.state;
	float valueFloat = CommandData.value;
	FString parameterString = CommandData.parameters;

	switch (CommandData.Command)
	{
	case EGameCommand::SendDataTick:
		SendDataTick = valueFloat;
		break;
	case EGameCommand::SetWhoSendsData:
		if (targetString.StartsWith(TEXT("s")))
		{
			ShipToSendDataFrom = targetString;
		}
		else BoosterToSendDataFrom = targetString;
		break;
	case EGameCommand::SetRocketSetting:
		CPPCommand_SetRocketSetting(parameterString, valueFloat);
		break;
	case EGameCommand::Engines:
		if (rocket)
		{
			if (stateBool)
			{
				CPPCommand_StartEngines();
			}
			else
			{
				CPPCommand_StopEngines();
			}
		}
		break;
	case EGameCommand::SpawnAtLocation:
		CPPCommand_SpawnAtLocation(parameterString, CommandData.Location, CommandData.Rotation);
		break;
	case EGameCommand::Raptor:
		if (rocket) CPPCommand_SelectRaptor((int)valueFloat, stateBool);
		break;
	case EGameCommand::Throttle:
		if (rocket) CPPCommand_Throttle(valueFloat);
		break;
	case EGameCommand::RCS:
		if (rocket) CPPCommand_RCS(stateBool);
		break;
	case EGameCommand::Flaps:
		if (rocket)	CPPCommand_Flaps(stateBool);
		break;
	case EGameCommand::FoldFlaps:
		if (rocket) CPPCommand_FoldFlaps(stateBool);
		break;
	case EGameCommand::GridFins:
		if (rocket) CPPCommand_GridFins(stateBool);
		break;
	case EGameCommand::Gimbals:
		if (rocket) CPPCommand_Gimbals(stateBool);
		break;
	case EGameCommand::SetRCSManual:
		if (rocket) CPPCommand_SetRCSManual(valueFloat, parameterString);
		break;
	case EGameCommand::SetDragManual:
		if (rocket) CPPCommand_SetDragManual(valueFloat, parameterString);
		break;
	case EGameCommand::SetGimbalManual:
		if (rocket) CPPCommand_SetGimbalManual(valueFloat, parameterString);
		break;
	case EGameCommand::Propellant:
		if (rocket) CPPCommand_Propellant(valueFloat);
		break;
	case EGameCommand::CryotankPressure:
		if (rocket) CPPCommand_CryotankPressure(valueFloat);
		break;
	case EGameCommand::HotStage:
		if (rocket) CPPCommand_HotStage();
		break;
	case EGameCommand::DetachHSR:
		if (rocket) CPPCommand_DetachHSR();
		break;
	case EGameCommand::FTS:
		if (rocket) CPPCommand_FTS();
		break;
	case EGameCommand::OuterGimbalEngines:
		if (rocket) CPPCommand_OuterGimbalEngines(stateBool);
		break;
	case EGameCommand::BoosterClamps:
		if (rocket) CPPCommand_BoosterClamps(stateBool);
		break;
	case EGameCommand::ControllerAltitude:
		if (rocket) CPPCommand_ControllerAltitude(stateBool);
		break;
	case EGameCommand::ControllerEastNorth:
		if (rocket) CPPCommand_ControllerEastNorth(stateBool);
		break;
	case EGameCommand::ControllerAttitude:
		if (rocket) CPPCommand_ControllerAttitude(parameterString);
		break;
	case EGameCommand::AttitudeTarget:
		if (rocket) CPPCommand_AttitudeTarget(valueFloat, parameterString);
		break;
	case EGameCommand::ChillValve:
		if (rocket) CPPCommand_ChillValve(stateBool);
		break;
	case EGameCommand::DumpFuel:
		if (rocket) CPPCommand_DumpFuel(stateBool);
		break;
	case EGameCommand::PopEngine:
		if (rocket) CPPCommand_PopEngine((int)valueFloat);
		break;
	case EGameCommand::BigFlame:
		if (rocket) CPPCommand_BigFlame(stateBool);
		break;
	case EGameCommand::Chopsticks:
		CPPCommand_Chopsticks(valueFloat, parameterString);
		break;
	case EGameCommand::PadADeluge:
		CPPCommand_PadADeluge(stateBool);
		break;
	case EGameCommand::PadASQDQuickRetract:
		CPPCommand_PadASQDQuickRetract();
		break;
	case EGameCommand::PadAOLMQuickRetract:
		CPPCommand_PadAOLMQuickRetract();
		break;
	case EGameCommand::PadABQDQuickRetract:
		CPPCommand_PadABQDQuickRetract();
		break;
	case EGameCommand::MasseyDeluge:
		CPPCommand_MasseyDeluge(stateBool);
		break;
	case EGameCommand::PadAOLMClampsExtend:
		CPPCommand_PadAOLMClampsExtend(stateBool);
		break;
	case EGameCommand::PadAOLMRQDExtend:
		CPPCommand_PadAOLMRQDExtend(stateBool);
		break;
	case EGameCommand::PadASpawnStack:
		CPPCommand_PadASpawnStack();
		break;
	case EGameCommand::DumpOxidizer:
		if (rocket) CPPCommand_DumpOxidizer(stateBool);
		break;
	}
}

void AGameStateBocaBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (ClientSocket)
	{
		ClientSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		ClientSocket = nullptr;
	}
	if (ServerSocket)
	{
		ServerSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ServerSocket);
		ServerSocket = nullptr;
	}
	ReceiveBuffer.Empty();
	
}