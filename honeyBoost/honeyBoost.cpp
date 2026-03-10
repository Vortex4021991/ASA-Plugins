#include <API/ARK/Ark.h>
#include <IApiUtils.h>

#include <Windows.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

namespace
{
    std::atomic_bool g_running{ false };
    std::thread g_worker;

    int g_scanIntervalSec = 300; // Default: 5 minutes
    int g_honeyAmount = 10;      // Default: 10 honey

    inline std::wstring ToWString(const std::string& str)
    {
        return std::wstring(str.begin(), str.end());
    }

    void WorkerLoop()
    {
        auto& utils = AsaApi::GetApiUtils();

        while (g_running.load())
        {
            try
            {
                UWorld* world = utils.GetWorld();
                if (world)
                {
                    const FString honeyPath =
                        L"Blueprint'/Game/PrimalEarth/CoreBlueprints/Items/Consumables/"
                        L"PrimalItemConsumable_Honey.PrimalItemConsumable_Honey'";

                    UClass* honeyClass = UVictoryCore::BPLoadClass(honeyPath);
                    if (honeyClass)
                    {
                        TArray<AActor*> actors;
                        UGameplayStatics::GetAllActorsOfClass(
                            world,
                            APrimalStructure::GetPrivateStaticClass(),
                            &actors);

                        for (AActor* actor : actors)
                        {
                            if (!actor || !actor->IsPrimalStructure())
                                continue;

                            const FString bpPath = utils.GetClassBlueprint(actor->ClassField());
                            if (!bpPath.Contains("BeeHive"))
                                continue;

                            if (!actor->IsA(APrimalStructureItemContainer::StaticClass()))
                                continue;

                            auto* hive = static_cast<APrimalStructureItemContainer*>(actor);
                            auto* inventory = hive->MyInventoryComponentField();
                            if (!inventory)
                                continue;

                            auto* defaultObject =
                                static_cast<UPrimalItem*>(honeyClass->GetDefaultObject(true));
                            if (!defaultObject)
                                continue;

                            UPrimalItem::AddNewItem(
                                TSubclassOf<UPrimalItem>(honeyClass),
                                inventory,
                                false,
                                false,
                                0.0f,
                                true,
                                g_honeyAmount,
                                false,
                                0.0f,
                                true,
                                TSubclassOf<UPrimalItem>(),
                                0.0f,
                                false,
                                false,
                                true,
                                false,
                                true);
                        }
                    }
                }
            }
            catch (...)
            {
            }

            for (int i = 0; i < g_scanIntervalSec && g_running.load(); ++i)
                std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

extern "C" __declspec(dllexport) void Plugin_Init()
{
    const std::wstring iniPath = ToWString(AsaApi::Tools::GetCurrentDir()) + L"\\HoneyBoost.ini";

    wchar_t buffer[32]{};

    // [HoneyBoost] IntervalMinutes
    GetPrivateProfileStringW(
        L"HoneyBoost",
        L"IntervalMinutes",
        L"5",
        buffer,
        32,
        iniPath.c_str());

    int interval = _wtoi(buffer);
    if (interval < 1)
        interval = 1;

    g_scanIntervalSec = interval * 60;

    // [HoneyBoost] HoneyAmount
    GetPrivateProfileStringW(
        L"HoneyBoost",
        L"HoneyAmount",
        L"10",
        buffer,
        32,
        iniPath.c_str());

    int amount = _wtoi(buffer);
    if (amount < 1)
        amount = 1;

    g_honeyAmount = amount;

    g_running = true;
    g_worker = std::thread(WorkerLoop);
}

extern "C" __declspec(dllexport) void Unload()
{
    g_running = false;

    if (g_worker.joinable())
        g_worker.join();
}
