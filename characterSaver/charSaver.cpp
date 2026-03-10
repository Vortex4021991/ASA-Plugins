#include <API/ARK/Ark.h>
#include <API/ARK/UE.h>
#include <IApiUtils.h>
#include <Logger/Logger.h>

#include <filesystem>
#include <shellapi.h>

using std::filesystem::path;

namespace
{
    FString gSavePath;

    struct FServerIds
    {
        FString MapName;
        FString ServerKey;
    };

    bool TryGetAltSaveDirectoryName(FString& outName)
    {
        outName.Empty();

        int argc = 0;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (!argv)
            return false;

        const FString param = L"AltSaveDirectoryName=";

        for (int i = 0; i < argc; ++i)
        {
            FString arg(argv[i]);
            const int32 index = arg.Find(param);

            if (index != INDEX_NONE)
            {
                FString value = arg.Mid(index + param.Len());

                int32 qpos;
                if (value.FindChar('?', qpos))
                    value = value.Left(qpos);

                outName = value;
                break;
            }
        }

        LocalFree(argv);
        return !outName.IsEmpty();
    }

    FServerIds GetServerIds()
    {
        FServerIds ids;

        if (auto* gm = AsaApi::GetApiUtils().GetShooterGameMode())
            gm->GetMapName(&ids.MapName);

        TryGetAltSaveDirectoryName(ids.ServerKey);
        return ids;
    }

    FString GetPerServerSavePath()
    {
        const FServerIds ids = GetServerIds();

        if (ids.ServerKey.IsEmpty() || ids.MapName.IsEmpty())
            return FString();

        const auto currentDir = AsaApi::Tools::GetCurrentDir();
        std::filesystem::path base(currentDir);
        std::filesystem::path shooterGame;

        for (std::filesystem::path it = base; !it.empty(); it = it.parent_path())
        {
            if (_wcsicmp(it.filename().c_str(), L"ShooterGame") == 0)
            {
                shooterGame = it;
                break;
            }
        }

        if (shooterGame.empty())
            shooterGame = base.parent_path();

        const std::wstring serverKey(*ids.ServerKey);
        const std::wstring mapName(*ids.MapName);

        const std::filesystem::path target = shooterGame / L"Saved" / serverKey / mapName;

        std::error_code ec;
        std::filesystem::create_directories(target, ec);

        return FString(target.c_str());
    }

    bool DoesArkProfileExist(const FString& eosId)
    {
        const std::wstring filename = *eosId + std::wstring(L".arkprofile");
        const std::wstring basePath = *gSavePath;
        const std::wstring fullPath = basePath + L"\\" + filename;

        return std::filesystem::exists(fullPath);
    }

    void CheckPlayerSave(AShooterPlayerController* player)
    {
        if (gSavePath.IsEmpty())
            gSavePath = GetPerServerSavePath();

        const FString eosId = AsaApi::GetApiUtils().GetEOSIDFromController(player);

        if (!DoesArkProfileExist(eosId))
        {
            player->UseFastInventory();
            player->UseFastInventory();
        }
    }
}

DECLARE_HOOK(AShooterPlayerController_BeginPlay, void, AShooterPlayerController*);
void Hook_AShooterPlayerController_BeginPlay(AShooterPlayerController* player)
{
    AShooterPlayerController_BeginPlay_original(player);
    CheckPlayerSave(player);
}

DECLARE_HOOK(AShooterGameMode_Tick, void, AShooterGameMode*, float);
void Hook_AShooterGameMode_Tick(AShooterGameMode* gm, float deltaSeconds)
{
    AShooterGameMode_Tick_original(gm, deltaSeconds);

    static float accum = 0.0f;
    accum += deltaSeconds;

    if (accum < 60.0f)
        return;

    accum = 0.0f;

    UWorld* world = AsaApi::GetApiUtils().GetWorld();
    if (!world)
        return;

    const auto playerControllers = world->PlayerControllerListField();
    for (auto pcWeak : playerControllers)
    {
        if (auto* pc = static_cast<AShooterPlayerController*>(pcWeak.Get()))
            CheckPlayerSave(pc);
    }
}

extern "C" __declspec(dllexport) void Plugin_Init()
{
    gSavePath = GetPerServerSavePath();

    AsaApi::GetHooks().SetHook(
        "AShooterPlayerController.BeginPlay()",
        &Hook_AShooterPlayerController_BeginPlay,
        &AShooterPlayerController_BeginPlay_original);

    AsaApi::GetHooks().SetHook(
        "AShooterGameMode.Tick(float)",
        &Hook_AShooterGameMode_Tick,
        &AShooterGameMode_Tick_original);
}

extern "C" __declspec(dllexport) void Plugin_Unload()
{
    AsaApi::GetHooks().DisableHook(
        "AShooterPlayerController.BeginPlay()",
        &Hook_AShooterPlayerController_BeginPlay);

    AsaApi::GetHooks().DisableHook(
        "AShooterGameMode.Tick(float)",
        &Hook_AShooterGameMode_Tick);
}
