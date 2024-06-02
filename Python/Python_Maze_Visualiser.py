import pygame
import numpy as np
import random
import threading

# Initialize Pygame
pygame.init()

# Maze parameters
maze_size = 100
window_size = 800  # Adjust window size for better visibility
cell_size = window_size // maze_size

# Set up the display
screen = pygame.display.set_mode((window_size, window_size))
pygame.display.set_caption("Maze Generation Visualization")

# Colors
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
GRAY = (200, 200, 200)  # Color for the starting area
COLORS = [(255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0)]  # Different colors for each algorithm

# Initialize the maze with all walls
maze = np.ones((maze_size, maze_size), dtype=np.int8)

# Initialize the visited array and the walls
visited = np.zeros((maze_size, maze_size), dtype=bool)
creator = np.full((maze_size, maze_size), -1, dtype=int)  # Track which algorithm created each path

# Define the central starting area and mark it
start_x = start_y = (maze_size - 10) // 2
for i in range(10):
    for j in range(10):
        maze[start_y + i][start_x + j] = 0
        visited[start_y + i][start_x + j] = True
        creator[start_y + i][start_x + j] = 4  # Mark as created by central area (id 4)

# Initialize stacks for each algorithm
stacks = {'N': [], 'S': [], 'E': [], 'W': []}
alg_ids = {'N': 0, 'S': 1, 'E': 2, 'W': 3}

# Set starting points outside the central area
starting_points = [
    (start_x + 5, start_y - 1),  # North
    (start_x + 5, start_y + 10),  # South
    (start_x - 1, start_y + 5),  # West
    (start_x + 10, start_y + 5)  # East
]

for direction, point in zip(stacks.keys(), starting_points):
    x, y = point
    stacks[direction].append((x, y))
    visited[y, x] = True
    creator[y, x] = alg_ids[direction]  # Mark as created by specific algorithm

# Lock and condition for synchronizing steps
lock = threading.Lock()
step_condition = threading.Condition(lock)
step_done = False

def draw_maze():
    for y in range(maze_size):
        for x in range(maze_size):
            rect = pygame.Rect(x * cell_size, y * cell_size, cell_size, cell_size)
            if creator[y, x] == 4:
                color = GRAY
            elif creator[y, x] != -1:
                color = COLORS[creator[y, x]]
            else:
                color = WHITE if maze[y, x] == 1 else BLACK
            pygame.draw.rect(screen, color, rect)

def get_unvisited_neighbors(x, y):
    directions = [(0, 2), (2, 0), (0, -2), (-2, 0)]
    neighbors = []
    for dx, dy in directions:
        nx, ny = x + dx, y + dy
        if 0 <= nx < maze_size and 0 <= ny < maze_size:
            if not visited[ny][nx] and all(not visited[ny + adj_y][nx + adj_x]
                for adj_x, adj_y in [(-1, 0), (1, 0), (0, -1), (0, 1)]
                if 0 <= nx + adj_x < maze_size and 0 <= ny + adj_y < maze_size):
                neighbors.append((nx, ny, x + dx//2, y + dy//2))
    return neighbors

def algorithm_step(direction):
    stack = stacks[direction]
    algorithm_id = alg_ids[direction]
    global step_done

    while stack:
        with step_condition:
            step_condition.wait_for(lambda: step_done)

            x, y = stack[-1]
            neighbors = get_unvisited_neighbors(x, y)
            if neighbors:
                nx, ny, wx, wy = random.choice(neighbors)
                maze[ny][nx] = 0
                maze[wy][wx] = 0
                visited[ny][nx] = True
                creator[ny][nx] = algorithm_id
                stack.append((nx, ny))
            else:
                stack.pop()  # Backtrack if no unvisited neighbors
            
            step_done = False
            step_condition.notify_all()

def step():
    global step_done

    while any(stacks.values()):
        with step_condition:
            step_done = True
            step_condition.notify_all()
            step_condition.wait_for(lambda: not step_done)

threads = []
for direction in stacks.keys():
    thread = threading.Thread(target=algorithm_step, args=(direction,))
    threads.append(thread)
    thread.start()

# Start the step management in a separate thread
step_thread = threading.Thread(target=step)
step_thread.start()

# Game loop
running = True
clock = pygame.time.Clock()
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    screen.fill(WHITE)
    with lock:
        draw_maze()
    pygame.display.flip()

    clock.tick(1000)  # Slow down the maze generation for visualization

# Ensure all threads have finished
for thread in threads:
    thread.join()

step_thread.join()

# Save the final maze as an image
pygame.image.save(screen, "final_maze.png")

pygame.quit()
