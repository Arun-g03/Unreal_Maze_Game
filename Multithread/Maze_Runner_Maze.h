#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include <mutex>
#include "MazeGenerationRunnable.h"
#include "Maze_Runner_Maze.generated.h"

UCLASS()
class MAZE_API AMaze_Runner_Maze : public AActor
{
    GENERATED_BODY()

public:
    AMaze_Runner_Maze();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void StartMazeGeneration();
    void OnMazeGenerationCompleted();
    void AddWallInstance(int32 x, int32 y);
    void GenerateMaze();

    UPROPERTY(EditAnywhere)
    UInstancedStaticMeshComponent* InstancedMeshComponent;

    int32 MazeSize;
    int32 StartSize;
    float Spacing;
    int32 NumExits;

    TArray<FIntPoint> Directions;
    TMap<FString, int32> AlgIds;

    int32 NorthSeed;
    int32 SouthSeed;
    int32 EastSeed;
    int32 WestSeed;

    // Multithreading variables
    std::mutex MazeArrayMutex;
    MazeGenerationRunnable* Runnable;
    FRunnableThread* Thread;
};
