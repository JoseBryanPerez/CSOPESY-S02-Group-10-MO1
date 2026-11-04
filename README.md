# CSOPESY-S02-Group-10-MO1

## Group Members

- Adriano, Mark Luis
- Buenavente, Djuvalle Antoiney
- Lee, Jason Benedict
- Perez, Jose Bryan

## Build

Run from the project folder:

```powershell
g++ -std=c++17 -O2 -pthread src/*.cpp -o csopesy
```

## Run

From PowerShell, in the project folder:

```powershell
.\csopesy.exe
```

Enter `initialize` first, then use commands such as `scheduler-start`, `scheduler-stop`, `screen -ls`, `report-util`, and `exit`. Run from the project folder so `config.txt` is found.

## Run Instruction Unit Tests

Build from the project folder:

```powershell
g++ -std=c++17 -O2 -pthread tests/unit_process.cpp src/process.cpp src/instruction.cpp -o unit_process.exe
```

Run the tests:

```powershell
.\unit_process.exe
```

Program entry point: `src/main.cpp` (`main` function).
