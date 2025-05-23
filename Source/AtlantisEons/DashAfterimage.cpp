#include "DashAfterimage.h"

ADashAfterimage::ADashAfterimage()
{
    MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;
}

void ADashAfterimage::Initialize(FVector Location, FRotator Rotation, USkeletalMesh* SkeletalMesh, TArray<UMaterialInterface*> Materials)
{
    SetActorLocation(Location);
    SetActorRotation(Rotation);
    if (MeshComponent)
    {
        MeshComponent->SetSkeletalMesh(SkeletalMesh);
        for (int32 i = 0; i < Materials.Num(); ++i)
        {
            if (i < MeshComponent->GetNumMaterials())
            {
                MeshComponent->SetMaterial(i, Materials[i]);
            }
        }
        UE_LOG(LogTemp, Log, TEXT("DashAfterimage initialized at location: %s"), *Location.ToString());  // Debug logging for initialization
    }
}
