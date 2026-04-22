#include <API/ARK/Ark.h>
#include <API/UE/Containers/Array.h>
#include <ICommands.h>
#include <IApiUtils.h>

static FString EnableMod_Impl(FString* cmd)
{
    if (!cmd || cmd->IsEmpty())
        return TEXT("Usage: enableMod <EOS> <AdminPassword>");

    TArray<FString> parsed;
    cmd->ParseIntoArray(parsed, TEXT(" "), true);

    FString eosId;
    FString adminPass;

 
    if (parsed.Num() >= 3)
    {
        eosId = parsed[1];
        adminPass = parsed[2];
    }
    else if (parsed.Num() == 2)
    {
        eosId = parsed[0];
        adminPass = parsed[1];
    }
    else
    {
        return TEXT("Usage: enableMod <EOS> <AdminPassword>");
    }

    AShooterPlayerController* pc = AsaApi::GetApiUtils().FindPlayerFromEOSID(eosId);
    if (!pc)
        return FString::Printf(TEXT("player not found / not online | eos='%s'"), *eosId);

    FString cmdEnable = TEXT("EnableCheats ") + adminPass;
    FString resultEnable;
    pc->ConsoleCommand(&resultEnable, &cmdEnable, true);

    FString cmdCheatPlayer = TEXT("SetCheatPlayer true");
    FString resultCheatPlayer;
    pc->ConsoleCommand(&resultCheatPlayer, &cmdCheatPlayer, true);

    return FString::Printf(
        TEXT("ok | EnableCheats='%s' | SetCheatPlayer='%s'"),
        *resultEnable,
        *resultCheatPlayer
    );
}

static FString DisableMod_Impl(FString* cmd)
{
    if (!cmd || cmd->IsEmpty())
        return TEXT("Usage: disableMod <EOS>");

    TArray<FString> parsed;
    cmd->ParseIntoArray(parsed, TEXT(" "), true);

    FString eosId;

 
    if (parsed.Num() >= 2)
        eosId = parsed[1];
    else if (parsed.Num() == 1)
        eosId = parsed[0];
    else
        return TEXT("Usage: disableMod <EOS>");

    AShooterPlayerController* pc = AsaApi::GetApiUtils().FindPlayerFromEOSID(eosId);
    if (!pc)
        return FString::Printf(TEXT("player not found / not online | eos='%s'"), *eosId);

    FString cmdCheatPlayer = TEXT("SetCheatPlayer false");
    FString resultCheatPlayer;
    pc->ConsoleCommand(&resultCheatPlayer, &cmdCheatPlayer, true);

    return FString::Printf(TEXT("ok | SetCheatPlayer='%s'"), *resultCheatPlayer);
}

static void EnableMod_Rcon(
    RCONClientConnection* rcon_connection,
    RCONPacket* rcon_packet,
    UWorld*)
{
    if (!rcon_connection || !rcon_packet)
        return;

    FString reply = EnableMod_Impl(&rcon_packet->Body);
    rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
}

static void DisableMod_Rcon(
    RCONClientConnection* rcon_connection,
    RCONPacket* rcon_packet,
    UWorld*)
{
    if (!rcon_connection || !rcon_packet)
        return;

    FString reply = DisableMod_Impl(&rcon_packet->Body);
    rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
}

extern "C" __declspec(dllexport) void Plugin_Init()
{
    AsaApi::GetCommands().AddRconCommand("tAdmin", &EnableMod_Rcon);
    AsaApi::GetCommands().AddRconCommand("disableMod", &DisableMod_Rcon);
}

extern "C" __declspec(dllexport) void Plugin_Unload()
{
    AsaApi::GetCommands().RemoveRconCommand("tAdmin");
    AsaApi::GetCommands().RemoveRconCommand("disableMod");
}
