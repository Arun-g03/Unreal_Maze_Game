#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include <mutex>

// Forward declaration to avoid circular dependency
class AMaze_Runner_Maze;

class MazeGenerationRunnable : public FRunnable
{
public:
    MazeGenerationRunnable(int32 InMazeSize, int32 InStartSize, int32 InNumExits, int32 InNorthSeed, int32 InSouthSeed, int32 InEastSeed, int32 InWestSeed, std::mutex* InMutex);
    virtual ~MazeGenerationRunnable();

    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    void EnsureCompletion();

    void GenerateMaze();
    void CarvePathStep(FString Direction, int32 Seed, bool& bContinue);
    void ShuffleDirections(FRandomStream& RandStream);
    TArray<FIntPoint> GetUnvisitedNeighbors(int32 x, int32 y, FRandomStream& RandStream);
    void CreatePerimeterWall();
    void CreateExits(FRandomStream& RandStream);

    const TArray<TArray<int32>>& GetMazeArray() const { return MazeArray; }

private:
    TArray<TArray<int32>> MazeArray;
    int32 MazeSize;
    int32 StartSize;
    int32 NumExits;
    int32 NorthSeed;
    int32 SouthSeed;
    int32 EastSeed;
    int32 WestSeed;
    bool bFinished;

    TArray<FIntPoint> Directions;
    TMap<FString, int32> AlgIds;
    TMap<FString, TArray<FIntPoint>> Stacks;

    FThreadSafeCounter StopTaskCounter;
    std::mutex* Mutex;
};
