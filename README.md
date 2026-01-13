C++ Console Chess Engine

A lightweight, high-performance chess engine built from scratch in C++. This project uses chess programming concepts including the Minimax algorithm, Alpha-Beta pruning and heuristic evaluation.


Key Features

* Move Generation: Full pseudo-legal move generation for all pieces (Pawns, Knights, Kings, Rooks, Bishops, Queens).
* Smart Search: Implements the Minimax algorithm with Alpha-Beta Pruning to efficiently search game trees up to 4-6 ply deep.
* Evaluation Function: Uses a material weight system combined with Piece-Square Tables (PST) to give the engine positional awareness (e.g., controlling the center, developing pieces).
* Move Ordering: Optimizes search speed using MVV/LVA (Most Valuable Victim, Least Valuable Aggressor) sorting to prioritize captures.
* Console GUI: Features a text-based ASCII board visualization and an interactive game loop.
