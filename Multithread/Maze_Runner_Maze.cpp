#include "Maze_Runner_Maze.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "MazeGenerationRunnable.h"  // Include the header file for the runnable

// Sets default values
AMaze_Runner_Maze::AMaze_Runner_Maze()
{
    PrimaryActorTick.bCanEverTick = true;

    InstancedMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedStaticMeshComponent"));
    RootComponent = InstancedMeshComponent;

    MazeSize = 20;
    StartSize = 10;
    Spacing = 100.0f;
    NumExits = 1;

    Directions.Add(FIntPoint(1, 0));
    Directions.Add(FIntPoint(-1, 0));
    Directions.Add(FIntPoint(0, 1));
    Directions.Add(FIntPoint(0, -1));

    NorthSeed = 0;
    SouthSeed = 1;
    EastSeed = 2;
    WestSeed = 3;

    AlgIds.Add("N", 0);
    AlgIds.Add("S", 1);
    AlgIds.Add("E", 2);
    AlgIds.Add("W", 3);

    Runnable = nullptr;
    Thread = nullptr;
}

void AMaze_Runner_Maze::BeginPlay()
{
    Super::BeginPlay();
    StartMazeGeneration();
}

void AMaze_Runner_Maze::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (Runnable && Runnable->IsFinished())
    {
        PrimaryActorTick.bCanEverTick = false;
        OnMazeGenerationCompleted();
    }
}

void AMaze_Runner_Maze::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Thread && Runnable)
    {
        Runnable->EnsureCompletion();  // Ensure the runnable completes
        delete Thread;  // Delete the thread
        Thread = nullptr;
    }

    if (Runnable)
    {
        delete Runnable;  // Delete the runnable
        Runnable = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void AMaze_Runner_Maze::StartMazeGeneration()
{
    Runnable = new MazeGenerationRunnable(MazeSize, StartSize, NumExits, NorthSeed, SouthSeed, EastSeed, WestSeed, &MazeArrayMutex);
    Thread = FRunnableThread::Create(Runnable, TEXT("MazeGenerationThread"));
    PrimaryActorTick.bCanEverTick = true;
}

void AMaze_Runner_Maze::OnMazeGenerationCompleted()
{
    if (Runnable)
    {
        std::lock_guard<std::mutex> lock(MazeArrayMutex);  // Lock the mutex for accessing shared data
        const TArray<TArray<int32>>& RunnableMazeArray = Runnable->GetMazeArray();

        int32 centerX = RunnableMazeArray.Num() / 2;
        int32 centerY = RunnableMazeArray[0].Num() / 2;
        for (int32 x = 0; x < RunnableMazeArray.Num(); x++)
        {
            for (int32 y = 0; y < RunnableMazeArray[x].Num(); y++)
            {
                if (RunnableMazeArray[y][x] == 1)
                {
                    AddWallInstance(x - centerX, y - centerY);
                }
            }
        }

        delete Runnable;
        Runnable = nullptr;
        delete Thread;
        Thread = nullptr;
    }
}

void AMaze_Runner_Maze::AddWallInstance(int32 x, int32 y)
{
    FVector Location(x * Spacing, y * Spacing, 0.0f);
    FTransform InstanceTransform(Location);
    InstancedMeshComponent->AddInstance(InstanceTransform);
}

void AMaze_Runner_Maze::GenerateMaze()
{
    StartMazeGeneration();
}
