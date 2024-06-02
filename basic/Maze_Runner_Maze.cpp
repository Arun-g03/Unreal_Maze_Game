#include "Maze_Runner_Maze.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"  // For FMath::RandRange

// Sets default values
AMaze_Runner_Maze::AMaze_Runner_Maze()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // Create the Instanced Static Mesh Component
    InstancedMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedMeshComponent"));
    RootComponent = InstancedMeshComponent;

    // Default values
    MazeSize = 20;  // Larger grid for maze generation
    StartSize = 10;
    Spacing = 100.0f;
    NumExits = 1;  // Default to 1 exit

    // Initialize directions for carving paths (right, left, up, down)
    Directions.Add(FIntPoint(1, 0));
    Directions.Add(FIntPoint(-1, 0));
    Directions.Add(FIntPoint(0, 1));
    Directions.Add(FIntPoint(0, -1));

    // Default seeds for the paths
    NorthSeed = 0;
    SouthSeed = 1;
    EastSeed = 2;
    WestSeed = 3;

    // Initialize algorithm ids
    AlgIds.Add("N", 0);
    AlgIds.Add("S", 1);
    AlgIds.Add("E", 2);
    AlgIds.Add("W", 3);
}

// Called when the game starts or when spawned
void AMaze_Runner_Maze::BeginPlay()
{
    Super::BeginPlay();
    GenerateMaze();
}

// Called every frame
void AMaze_Runner_Maze::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AMaze_Runner_Maze::GenerateMaze()
{
    // Initialize the maze array with walls (1 represents wall, 0 represents path)
    MazeArray.SetNum(MazeSize);
    VisitedArray.SetNum(MazeSize);
    for (int32 i = 0; i < MazeSize; ++i)
    {
        MazeArray[i].SetNum(MazeSize);
        VisitedArray[i].SetNum(MazeSize);
        for (int32 j = 0; j < MazeSize; ++j)
        {
            MazeArray[i][j] = 1;  // Initialize all cells as walls
            VisitedArray[i][j] = false;
        }
    }

    // Define the central starting area and mark it
    int32 centerX = MazeSize / 2;
    int32 centerY = MazeSize / 2;
    int32 startX = centerX - StartSize / 2;
    int32 startY = centerY - StartSize / 2;

    for (int32 i = 0; i < StartSize; ++i)
    {
        for (int32 j = 0; j < StartSize; ++j)
        {
            MazeArray[startY + i][startX + j] = 0;
            VisitedArray[startY + i][startX + j] = true;
        }
    }

    // Create perimeter wall
    CreatePerimeterWall();

    // Create exits in the perimeter wall
    FRandomStream RandStream(NorthSeed + SouthSeed + EastSeed + WestSeed); // Use combined seeds for consistency
    CreateExits(RandStream);

    // Set starting points outside the central area
    Stacks.Add("N", { FIntPoint(centerX, centerY - StartSize / 2 - 1) });
    Stacks.Add("S", { FIntPoint(centerX, centerY + StartSize / 2) });
    Stacks.Add("E", { FIntPoint(centerX + StartSize / 2, centerY) });
    Stacks.Add("W", { FIntPoint(centerX - StartSize / 2 - 1, centerY) });

    // Mark the starting points as visited
    for (auto& Elem : Stacks)
    {
        FIntPoint StartPoint = Elem.Value[0];
        VisitedArray[StartPoint.Y][StartPoint.X] = true;
        MazeArray[StartPoint.Y][StartPoint.X] = 0;
    }

    // Carve paths from the starting points sequentially
    CarvePath("N", NorthSeed);
    CarvePath("S", SouthSeed);
    CarvePath("E", EastSeed);
    CarvePath("W", WestSeed);

    // Add instances based on the maze array
    for (int32 x = 0; x < MazeSize; x++)
    {
        for (int32 y = 0; y < MazeSize; y++)
        {
            if (MazeArray[y][x] == 1)
            {
                AddWallInstance(x - centerX, y - centerY);
            }
        }
    }
}

void AMaze_Runner_Maze::CarvePath(FString Direction, int32 Seed)
{
    TArray<FIntPoint>& Stack = Stacks[Direction];
    FRandomStream RandStream(Seed);

    while (Stack.Num() > 0)
    {
        FIntPoint Current = Stack.Last();
        int32 x = Current.X;
        int32 y = Current.Y;

        // Shuffle directions to ensure randomness
        ShuffleDirections(RandStream);

        TArray<FIntPoint> Neighbors = GetUnvisitedNeighbors(x, y, RandStream);
        if (Neighbors.Num() > 0)
        {
            FIntPoint Next = Neighbors[RandStream.RandRange(0, Neighbors.Num() - 1)];
            int32 nx = Next.X;
            int32 ny = Next.Y;

            // Carve the path between the current cell and the new cell
            MazeArray[ny][nx] = 0;
            MazeArray[(y + ny) / 2][(x + nx) / 2] = 0;
            VisitedArray[ny][nx] = true;

            Stack.Add(Next);
        }
        else
        {
            Stack.Pop();  // Backtrack if no unvisited neighbors
        }
    }
}

void AMaze_Runner_Maze::ShuffleDirections(FRandomStream& RandStream)
{
    for (int32 i = Directions.Num() - 1; i > 0; --i)
    {
        int32 j = RandStream.RandRange(0, i);
        Directions.Swap(i, j);
    }
}

void AMaze_Runner_Maze::ShuffleArray(TArray<FIntPoint>& Array, FRandomStream& RandStream)
{
    for (int32 i = Array.Num() - 1; i > 0; --i)
    {
        int32 j = RandStream.RandRange(0, i);
        Array.Swap(i, j);
    }
}

TArray<FIntPoint> AMaze_Runner_Maze::GetUnvisitedNeighbors(int32 x, int32 y, FRandomStream& RandStream)
{
    TArray<FIntPoint> Neighbors;
    for (const FIntPoint& Direction : Directions)
    {
        int32 nx = x + Direction.X * 2;
        int32 ny = y + Direction.Y * 2;

        // Check if the new position is within bounds and not yet visited
        if (nx >= 0 && nx < MazeSize && ny >= 0 && ny < MazeSize && !VisitedArray[ny][nx])
        {
            // Ensure the destination is not adjacent to any visited path
            bool AdjacentVisited = false;
            for (const FIntPoint& Adj : Directions)
            {
                int32 ax = nx + Adj.X;
                int32 ay = ny + Adj.Y;
                if (ax >= 0 && ax < MazeSize && ay >= 0 && ay < MazeSize && VisitedArray[ay][ax])
                {
                    AdjacentVisited = true;
                    break;
                }
            }
            if (!AdjacentVisited)
            {
                Neighbors.Add(FIntPoint(nx, ny));
            }
        }
    }
    return Neighbors;
}

void AMaze_Runner_Maze::AddWallInstance(int32 x, int32 y)
{
    FVector Location(x * Spacing, y * Spacing, 0.0f);
    FTransform InstanceTransform(Location);
    InstancedMeshComponent->AddInstance(InstanceTransform);
}

void AMaze_Runner_Maze::CreatePerimeterWall()
{
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

void AMaze_Runner_Maze::CreateExits(FRandomStream& RandStream)
{
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

    ShuffleArray(PotentialExits, RandStream);
    for (int32 i = 0; i < FMath::Min(NumExits, PotentialExits.Num()); i++)
    {
        FIntPoint Exit = PotentialExits[i];
        MazeArray[Exit.Y][Exit.X] = 0;
    }
}
