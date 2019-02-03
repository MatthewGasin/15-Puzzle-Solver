# 15-Puzzle-Solver
Implementation of the A* algorithm to solve the Gem puzzle, multithreaded in C

  This program solves the 15 puzzle, also known as the Gem puzzle or the Game of 15 blocks. The premise of the problem is a 4x4 grid with 15 blocks and one empty slot. The challenge is to sort the blocks in order by moving them via the empty slot.
  
  This program is one of THE most time effective solutions to this problem. A* is the most efficient way to find a solution, with other searches taking way longer to find even non-optimal solutions. The multi-threaded aspect speeds up the most computation heavy area of the program (as described below), and the efficiency of C speeds the program even more. While other solves take minutes or hours for a problem more than 100 moves away from the goal state, this program does it in under 30 seconds.
  
  GenGemPuzzle.c generates the problems. The program takes one command line argument, the amount of moves that should be made on the goal state. The goal state is 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 0, or visualized: 
  
    1  2  3  4
    5  6  7  8
    9 10 11 12
    13 14 15 0 
    
 GenGemPuzzle moves the 0 around to create a problem that can definitely be solved (as it only makes valid moves on the goal state). Simply generating the numbers randomly may result in an unsolvable problem.

An example generation may be 0  5  2  4 10  1  3  6  9 14 11 12 13 15  8  7,

  0  5  2  4
  10 1  3  6
  9  14 11 12
  13 15 8  7
  
  The actual solver is 15PuzzleSolver.c. The command line arguments for this program are first '-s' or '-m' to determine whether to use the single-threaded or multi-threaded implementation of the solution. The second arguments onwards is the order of the 16 blocks.
  
  __EX:__
  ./15PuzzleSolver -m 0  5  2  4 10  1  3  6  9 14 11 12 13 15  8  7
  
  In this implementation of A*, each 'node' of the graph is a state of the board. The hueristic *h* is the max between the total manhattan distance of each block to its correct spot, and the number of blocks in incorrect positions. The *g* is how many moves have already been made to get to that state.
  
  On expanding a node, the algorithm considers all four directions the 0 can be moved. These successor nodes are not considered if they do not cause a difference in board state, or if their state is the same as a node already expanded (in closed list) or the same as a node waiting to be expanded (in open list). In the multithreaded solution, four threads do this filtering of successor nodes. 
  
  After the node with the lowest f(n) is expanded, its successor nodes are added to the open list and it is added to the closed list. The open list is a priority queue of f(n). Once the algorithm finds a node with the same state as the goal state, it has found the most optimal solution.
  
