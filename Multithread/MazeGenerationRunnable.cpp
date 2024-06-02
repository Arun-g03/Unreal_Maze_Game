#include "MazeGenerationRunnable.h"
#include "HAL/RunnableThread.h"

MazeGenerationRunnable::MazeGenerationRunnable(int32 InMazeSize, int32 InStartSize, int32 InNumExits, int32 InNorthSeed, int32 InSouthSeed, int32 InEastSeed, int32 InWestSeed, std::mutex* InMutex)
    : MazeSize(InMazeSize), StartSize(InStartSize), NumExits(InNumExits), NorthSeed(InNorthSeed), SouthSeed(InSouthSeed), EastSeed(InEastSeed), WestSeed(InWestSeed), bFinished(false), Mutex(InMutex)
{
    Directions = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
    AlgIds.Add("N", 0);
    AlgIds.Add("S", 1);
    AlgIds.Add("E", 2);
    AlgIds.Add("W", 3);
}

MazeGenerationRunnable::~MazeGenerationRunnable() {}

bool MazeGenerationRunnable::Init()
{
    return true;
}

uint32 MazeGenerationRunnable::Run()
{
    GenerateMaze();
    bFinished = true;
    return 0;
}

void MazeGenerationRunnable::Stop()
{
    StopTaskCounter.Increment();
}

void MazeGenerationRunnable::EnsureCompletion()
{
    Stop();
    // Note: WaitForCompletion() is not a member function of FRunnableThread.
    // Use appropriate thread join or wait logic here.
    if (Thread)
    {
        Thread->WaitForCompletion();
    }
}

void MazeGenerationRunnable::GenerateMaze()
{
    std::lock_guard<std::mutex> guard(*Mutex);  // Lock the mutex for accessing shared data

    MazeArray.SetNum(MazeSize);
    for (int32 i = 0; i < MazeSize; ++i)
    {
        MazeArray[i].SetNum(MazeSize);
        for (int32 j = 0; j < MazeSize; ++j)
        {
            MazeArray[i][j] = 1;
        }
    }

    int32 centerX = MazeSize / 2;
    int32 centerY = MazeSize / 2;
    int32 startX = centerX - StartSize / 2;
    int32 startY = centerY - StartSize / 2;

    for (int32 i = 0; i < StartSize; ++i)
    {
        for (int32 j = 0; j < StartSize; ++j)
        {
            MazeArray[startY + i][startX + j] = 0;
        }
    }

    Stacks.Add("N", { FIntPoint(centerX, startY - 1) });
    Stacks.Add("S", { FIntPoint(centerX, startY + StartSize) });
    Stacks.Add("E", { FIntPoint(startX + StartSize, centerY) });
    Stacks.Add("W", { FIntPoint(startX - 1, centerY) });

    bool anyActive = true;
    while (anyActive && StopTaskCounter.GetValue() == 0)
    {
        anyActive = false;

        for (const auto& Dir : AlgIds)
        {
            bool stepResult = false;
            CarvePathStep(Dir.Key, Dir.Value, stepResult);
            anyActive = anyActive || stepResult;
        }
    }

    CreatePerimeterWall();
    FRandomStream RandStream(NorthSeed + SouthSeed + EastSeed + WestSeed);
    CreateExits(RandStream);
}

void MazeGenerationRunnable::CarvePathStep(FString Direction, int32 Seed, bool& bContinue)
{
    std::lock_guard<std::mutex> guard(*Mutex);  // Lock the mutex for accessing shared data

    TArray<FIntPoint>& stack = Stacks[Direction];
    if (!stack.IsEmpty())
    {
        FRandomStream RandStream(Seed);
        FIntPoint current = stack.Last();
        ShuffleDirections(RandStream);

        TArray<FIntPoint> neighbors = GetUnvisitedNeighbors(current.X, current.Y, RandStream);
        if (!neighbors.IsEmpty())
        {
            FIntPoint next = neighbors[RandStream.RandRange(0, neighbors.Num() - 1)];
            MazeArray[next.Y][next.X] = 0;
            MazeArray[(current.Y + next.Y) / 2][(current.X + next.X) / 2] = 0;

            stack.Add(next);
            bContinue = true;
        }
        else
        {
            stack.Pop();
        }
    }
}

void MazeGenerationRunnable::ShuffleDirections(FRandomStream& RandStream)
{
    std::lock_guard<std::mutex> guard(*Mutex);  // Lock the mutex for accessing shared data

    for (int32 i = Directions.Num() - 1; i > 0; --i)
    {
        int32 j = RandStream.RandRange(0, i);
        Directions.Swap(i, j);
    }
}

TArray<FIntPoint> MazeGenerationRunnable::GetUnvisitedNeighbors(int32 x, int32 y, FRandomStream& RandStream)
{
    std::lock_guard<std::mutex> guard(*Mutex);  // Lock the mutex for accessing shared data

    TArray<FIntPoint> Neighbors;
    for (const FIntPoint& Direction : Directions)
    {
        int32 nx = x + Direction.X * 2;
        int32 ny = y + Direction.Y * 2;

        if (nx >= 0 && nx < MazeSize && ny >= 0 && ny < MazeSize && MazeArray[ny][nx] == 1)
        {
            Neighbors.Add(FIntPoint(nx, ny));
        }
    }
    return Neighbors;
}

void MazeGenerationRunnable::CreatePerimeterWall()
{
    std::lock_guard<std::mutex> guard(*Mutex);  // Lock the mutex for accessing shared data

    for (int32 x = 0; x < MazeSize; x++)
    {
        MazeArray[0][x] = 1;
        MazeArray[MazeSize - 1][x] = 1;
    }
    for (int32 y = 0; y < MazeSize; y++)
    {
        MazeArray[y][0] = 1;
        MazeArray[y][MazeSize - 1] = 1;
    }
}

void MazeGenerationRunnable::CreateExits(FRandomStream& RandStream)
{
    std::lock_guard<std::mutex> guard(*Mutex);  // Lock the mutex for accessing shared data

    TArray<FIntPoint> PotentialExits;

    for (int32 x = 1; x < MazeSize - 1; x++)
    {
        PotentialExits.Add(FIntPoint(x, 0));  // Top row
        PotentialExits.Add(FIntPoint(x, MazeSize - 1));  // Bottom row
    }

    for (int32 y = 1; y < MazeSize - 1; y++)
    {
        PotentialExits.Add(FIntPoint(0, y));  // Left column
        PotentialExits.Add(FIntPoint(MazeSize - 1, y));  // Right column
    }

    for (int32 i = 0; i < FMath::Min(NumExits, PotentialExits.Num()); i++)
    {
        FIntPoint Exit = PotentialExits[i];
        MazeArray[Exit.Y][Exit.X] = 0;
    }
}
