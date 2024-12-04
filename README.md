# Real-time Graph Plotter Using Reverse Polish Notation (RPN)

## Overview

This project is a console-based application developed in C that takes an equation input from the user, processes it using Reverse Polish Notation (RPN), and plots the graph in real-time.

## Development Environment

- **Language**: C
- **Compiler**: GCC (GNU Compiler Collection)
- **IDE**: Any text editor or IDE that supports C (e.g., Visual Studio Code)

## Project Structure

- `graph.c`: Contains the main function and core logic for input handling and graph plotting.
- `Makefile`: Contains build instructions for compiling the project.

## Build Instructions

To build the project, navigate to the project directory and run:

```sh
make release
```
This will compile the source files and generate an executable named `graph_release.out`.

## Usage
Running the Application
To run the application, execute the following command in the terminal:
```sh
./graph_release.out
```

## Input Format
The application accepts mathematical equations in the following format:

`Operators`: +, -, *, /, ^ (exponentiation)

`Functions`: sin, cos, tan, ctg, ln, sqrt

`Variables`: x (the independent variable)

`Constants`: Any numeric value (integer or floating-point)

##Example Inputs
sin(x)

x^2 + 3*x + 2

log(x)

## Commands
Once the application is running, use the following commands to interact with it:

`<equation>`: Plots the graph of the specified equation.

## Supported Operations
The application supports the following operations:

Arithmetic Operators:
    
    + (summarize)
    
    - (subtraction)
    
    * (multiply)
    
    / (divide)
    
    ^ (exponentiation)
    
    - (unary minus)

## Functions:
    
    sin (sine)
    
    cos (cosine)
    
    tan (tangent)
    
    ctg (cotangent)
    
    ln (natural logarithm)
    
    sqrt (square root)
