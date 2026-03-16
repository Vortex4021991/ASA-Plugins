#include <API/ARK/Ark.h>
#include <IApiUtils.h>
#include <ICommands.h>

#include <windows.h>
#include <string>

static int    g_scanIntervalSec = 300; // Default: 5 Minutes
static int    g_honeyAmount = 10;      // Default: 10 Honey
static int    g_elapsedSec = 0;
static bool   g_enabled = false;
static UClass* g_honeyClass = nullptr;

static const FString kHoneyClassPath =
L"Blueprint'/Game/PrimalEarth/CoreBlueprints/Items/Consumables/PrimalItemConsumable_Honey.PrimalItemConsumable_Honey'";

inline std::wstring ToWString(const std::string& str)
{
    return std::wstring(str.begin(), str.end());
}

static bool IsPlayerOwnedBeeHive(AActor* actor)
{
    // Changed to only player hives
    if (!actor || !actor->IsPrimalStructure())
        return false;

    const FString bpPath = AsaApi::GetApiUtils().GetClassBlueprint(actor->ClassField());
    return bpPath.Contains(TEXT("BeeHive_PlayerOwned"));
}

static void ScanBeeHivesAndAddHoney()
{
    auto& utils = AsaApi::GetApiUtils();

    UWorld* world = utils.GetWorld();
    if (!world)
        return;

    if (!g_honeyClass)
    {
        g_honeyClass = UVictoryCore::BPLoadClass(kHoneyClassPath);
        if (!g_honeyClass)
            return;
    }

    TArray<AActor*> actors;
    UGameplayStatics::GetAllActorsOfClass(world, APrimalStructure::GetPrivateStaticClass(), &actors);

    for (AActor* actor : actors)
    {
        if (!IsPlayerOwnedBeeHive(actor))
            continue;

        if (!actor->IsA(APrimalStructureItemContainer::StaticClass()))
            continue;

        auto* hive = static_cast<APrimalStructureItemContainer*>(actor);
        if (!hive)
            continue;

        auto* inv = hive->MyInventoryComponentField();
        if (!inv)
            continue;

        UPrimalItem::AddNewItem(
            TSubclassOf<UPrimalItem>(g_honeyClass),
            inv,
            false, false, 0.0f,
            true, g_honeyAmount,
            false, 0.0f, true,
            TSubclassOf<UPrimalItem>(),
            0.0f, false, false,
            true, false, true);
    }
}

static void OnHoneyBoostTimer()
{
    if (!g_enabled)
        return;

    ++g_elapsedSec;
    if (g_elapsedSec < g_scanIntervalSec)
        return;

    g_elapsedSec = 0;
    ScanBeeHivesAndAddHoney();
}

extern "C" __declspec(dllexport) void Plugin_Init()
{
    std::wstring iniPath = ToWString(AsaApi::Tools::GetCurrentDir()) + L"\\HoneyBoost.ini";

    wchar_t buffer[32]{};

    GetPrivateProfileStringW(L"HoneyBoost", L"IntervalMinutes", L"5", buffer, 32, iniPath.c_str());
    int interval = _wtoi(buffer);
    if (interval < 1)
        interval = 1;
    g_scanIntervalSec = interval * 60;

    GetPrivateProfileStringW(L"HoneyBoost", L"HoneyAmount", L"10", buffer, 32, iniPath.c_str());
    int amount = _wtoi(buffer);
    if (amount < 1)
        amount = 1;
    g_honeyAmount = amount;

    g_elapsedSec = 0;
    g_enabled = true;
    g_honeyClass = nullptr;

    // saver looptimer
    AsaApi::GetCommands().AddOnTimerCallback(TEXT("HoneyBoostTimer"), []()
        {
            OnHoneyBoostTimer();
        });
}

extern "C" __declspec(dllexport) void Unload()
{
    g_enabled = false;
    AsaApi::GetCommands().RemoveOnTimerCallback(TEXT("HoneyBoostTimer"));
    g_honeyClass = nullptr;
}
