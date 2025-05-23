#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DashAfterimage.generated.h"

UCLASS()
class ATLANTISEONS_API ADashAfterimage : public AActor
{
    GENERATED_BODY()

public:
    ADashAfterimage();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Afterimage", meta = (AllowPrivateAccess = "true"))
    USkeletalMeshComponent* MeshComponent;

    void Initialize(FVector Location, FRotator Rotation, USkeletalMesh* SkeletalMesh, TArray<UMaterialInterface*> Materials);
};
