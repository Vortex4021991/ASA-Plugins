#include <windows.h>

#include <API/ARK/Ark.h>
#include <API/ARK/UE.h>
#include <IApiUtils.h>
#include <IHooks.h>

#include <filesystem>
#include <string>

// Needs startparam -ThisServerSave="PathToSavefolder"
static FString g_serverSaveDir;

FString GetThisServerSaveFromCmd()
{
    LPCWSTR cmd = GetCommandLineW();
    if (!cmd || !*cmd)
        return TEXT("");

    std::wstring s(cmd);

    const std::wstring key1 = L"-ThisServerSave=";
    const std::wstring key2 = L"ThisServerSave=";

    size_t pos = s.find(key1);
    size_t keyLen = key1.length();

    if (pos == std::wstring::npos)
    {
        pos = s.find(key2);
        keyLen = key2.length();
    }

    if (pos == std::wstring::npos)
        return TEXT("");

    pos += keyLen;
    if (pos >= s.length())
        return TEXT("");

    std::wstring value;

    if (s[pos] == L'"')
    {
        ++pos;
        size_t end = s.find(L'"', pos);
        if (end == std::wstring::npos)
            value = s.substr(pos);
        else
            value = s.substr(pos, end - pos);
    }
    else
    {
        size_t end = s.find(L' ', pos);
        if (end == std::wstring::npos)
            value = s.substr(pos);
        else
            value = s.substr(pos, end - pos);
    }

    return FString(value.c_str());
}

void BuildServerSaveDir()
{
    g_serverSaveDir = GetThisServerSaveFromCmd();

    while (!g_serverSaveDir.IsEmpty())
    {
        const TCHAR lastChar = g_serverSaveDir[g_serverSaveDir.Len() - 1];
        if (lastChar == TEXT('\\') || lastChar == TEXT('/'))
            g_serverSaveDir.LeftChopInline(1, false);
        else
            break;
    }
}

bool DoesArkProfileExist(const FString& eosId)
{
    if (g_serverSaveDir.IsEmpty() || eosId.IsEmpty())
        return false;

    const std::wstring fullPath =
        std::wstring(*g_serverSaveDir) + L"\\" + std::wstring(*eosId) + L".arkprofile";

    return std::filesystem::exists(std::filesystem::path(fullPath));
}

bool WriteArkProfile(AShooterPlayerController* pc)
{
    if (!pc)
        return false;

    auto* gameMode = AsaApi::GetApiUtils().GetShooterGameMode();
    if (!gameMode)
        return false;

    pc->UseFastInventory();
    pc->UseFastInventory();
    gameMode->SaveWorld(true, false, false);
    return true;
}

void EnsurePlayerSave(AShooterPlayerController* player)
{
    if (!player)
        return;

    const FString eosId = AsaApi::GetApiUtils().GetEOSIDFromController(player);
    if (eosId.IsEmpty())
        return;

    if (!DoesArkProfileExist(eosId))
    {
        WriteArkProfile(player);
    }
}

DECLARE_HOOK(AShooterPlayerController_OnPossess, void, AShooterPlayerController*, APawn*);
void Hook_AShooterPlayerController_OnPossess(AShooterPlayerController* playerController, APawn* inPawn)
{
    AShooterPlayerController_OnPossess_original(playerController, inPawn);

    if (!playerController || !inPawn)
        return;

    if (!inPawn->IsA(AShooterCharacter::StaticClass()))
        return;

    EnsurePlayerSave(playerController);
}

extern "C" __declspec(dllexport) void Plugin_Init()
{
    BuildServerSaveDir();

    AsaApi::GetHooks().SetHook(
        "AShooterPlayerController.OnPossess(APawn*)",
        &Hook_AShooterPlayerController_OnPossess,
        &AShooterPlayerController_OnPossess_original);
}

extern "C" __declspec(dllexport) void Plugin_Unload()
{
    AsaApi::GetHooks().DisableHook(
        "AShooterPlayerController.OnPossess(APawn*)",
        &Hook_AShooterPlayerController_OnPossess);
}
