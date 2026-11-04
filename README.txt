CSOPESY-S02-Group-10-MO1

Group members:
Adriano, Mark Luis
Buenavente, Djuvalle Antoiney
Lee, Jason Benedict
Perez, Jose Bryan

Build (run from the project folder):
  g++ -std=c++17 -O2 -pthread src/*.cpp -o csopesy

Run:
  PowerShell: .\csopesy.exe

Enter initialize first, then use commands such as scheduler-start, scheduler-stop,
screen -ls, report-util, and exit. Run from the project folder so config.txt is found.

Run instruction unit tests:
  g++ -std=c++17 -O2 -pthread tests/unit_process.cpp src/process.cpp src/instruction.cpp -o unit_process.exe
  PowerShell: .\unit_process.exe

Program entry point: src/main.cpp (main function).
