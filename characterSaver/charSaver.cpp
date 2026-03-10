#include <API/ARK/Ark.h>
#include <API/ARK/UE.h>
#include <IApiUtils.h>
#include <IHooks.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <string>

namespace
{
    FString g_serverSaveDir;

    void BuildServerSaveDir()
    {
        UWorld* world = AsaApi::GetApiUtils().GetWorld();
        AShooterGameMode* gameMode = AsaApi::GetApiUtils().GetShooterGameMode();

        if (!world || !gameMode)
            return;

        const FString saveDir = gameMode->GetSaveDirectoryName(world, ESaveType::Map, false, true);

        std::string baseDir = AsaApi::Tools::GetCurrentDir();
        const size_t pos = baseDir.rfind("Binaries\\Win64");

        if (pos != std::string::npos)
            baseDir.replace(pos, std::strlen("Binaries\\Win64"), "Saved\\");
        else
            baseDir += "\\Saved\\";

        FTCHARToUTF8 conv(*saveDir);
        std::string relativeDir(conv.Get(), conv.Length());

        std::replace(relativeDir.begin(), relativeDir.end(), '/', '\\');

        while (!relativeDir.empty() && (relativeDir.back() == '\\' || relativeDir.back() == '/'))
            relativeDir.pop_back();

        if (!baseDir.empty() && baseDir.back() != '\\')
            baseDir.push_back('\\');

        while (!relativeDir.empty() && relativeDir.front() == '\\')
            relativeDir.erase(relativeDir.begin());

        g_serverSaveDir = UTF8_TO_TCHAR((baseDir + relativeDir).c_str());
    }

    bool DoesArkProfileExist(const FString& eosId)
    {
        if (g_serverSaveDir.IsEmpty() || eosId.IsEmpty())
            return false;

        const std::wstring fullPath =
            std::wstring(*g_serverSaveDir) + L"\\" + std::wstring(*eosId) + L".arkprofile";

        return std::filesystem::exists(fullPath);
    }

    void EnsurePlayerSave(AShooterPlayerController* player)
    {
        if (!player)
            return;

        const FString eosId = AsaApi::GetApiUtils().GetEOSIDFromController(player);

        if (!DoesArkProfileExist(eosId))
        {
            player->UseFastInventory();
            player->UseFastInventory();
        }
    }
}

DECLARE_HOOK(AShooterPlayerController_OnPossess, void, AShooterPlayerController*, APawn*);
void Hook_AShooterPlayerController_OnPossess(AShooterPlayerController* playerController, APawn* inPawn)
{
    AShooterPlayerController_OnPossess_original(playerController, inPawn);

    if (!inPawn || !inPawn->IsA(AShooterCharacter::StaticClass()))
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
