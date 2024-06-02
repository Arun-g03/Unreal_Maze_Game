#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Maze_Runner_Maze.generated.h"

UENUM(BlueprintType)
enum class ETileType : uint8
{
    Straight,
    Junction,
    Intersection,
    Corner,
    DeadEnd
};

UCLASS()
class MAZE_API AMaze_Runner_Maze : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AMaze_Runner_Maze();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Instanced Static Mesh Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Maze")
    UInstancedStaticMeshComponent* InstancedMeshComponent;

    // Size of the maze
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze", meta = (ClampMin = "1", UIMin = "1"))
    int32 MazeSize;

    // Size of the central starting area
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze", meta = (ClampMin = "1", UIMin = "1"))
    int32 StartSize;

    // Spacing between the instances
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float Spacing;

    // Number of exits in the perimeter wall
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze", meta = (ClampMin = "0", UIMin = "0"))
    int32 NumExits;

    // Random seed for north path
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze")
    int32 NorthSeed;

    // Random seed for south path
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze")
    int32 SouthSeed;

    // Random seed for east path
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze")
    int32 EastSeed;

    // Random seed for west path
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze")
    int32 WestSeed;

    // Method to generate the maze
    UFUNCTION(BlueprintCallable, Category = "Maze")
    void GenerateMaze();

    // Array to store path tiles
    UPROPERTY(BlueprintReadOnly, Category = "Maze")
    TArray<FIntPoint> PathTiles;

    // Array to store tile types
    UPROPERTY(BlueprintReadOnly, Category = "Maze")
    TArray<ETileType> TileTypes;

private:
    // Iterative backtracking algorithm to carve paths using stacks
    void CarvePath(FString Direction, int32 Seed);

    // Directions for movement in the maze
    TArray<FIntPoint> Directions;

    // Maze and visited arrays
    TArray<TArray<int32>> MazeArray;
    TArray<TArray<bool>> VisitedArray;

    // Maze generation stacks for each direction
    TMap<FString, TArray<FIntPoint>> Stacks;
    TMap<FString, int32> AlgIds;

    // Manual shuffle function
    void ShuffleDirections(FRandomStream& RandStream);
    void ShuffleArray(TArray<FIntPoint>& Array, FRandomStream& RandStream);

    // Helper function to get unvisited neighbors
    TArray<FIntPoint> GetUnvisitedNeighbors(int32 x, int32 y, FRandomStream& RandStream);

    // Helper function to add wall instance
    void AddWallInstance(int32 x, int32 y);

    // Helper function to create perimeter wall
    void CreatePerimeterWall();

    // Helper function to create exits in the perimeter wall
    void CreateExits(FRandomStream& RandStream);

    // Helper function to assign tile types
    void AssignTileTypes();
    ETileType DetermineTileType(int32 x, int32 y);
};






