#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

UMaterialInstanceDynamic* ThrowDynamicInstance(float ScalarValue)
{
    static FSoftObjectPath MatPath(TEXT("/Game/Mats/UMG/M_SyncNotify.M_SyncNotify"));
    
    UMaterialInterface* BaseMat = Cast<UMaterialInterface>(MatPath.TryLoad());
    if (!BaseMat) return nullptr;

    UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, nullptr);
    if (DynMat)
    {
        DynMat->SetScalarParameterValue(FName("SwitchParameter"), ScalarValue);
    }

    UE_LOG(LogTemp, Log, TEXT("ThrowDynamicInstance::Native throw is fired."));

    return DynMat;
}