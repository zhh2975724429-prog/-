# Crane Real-Time Monitoring System

STM32F103C8 + Keil MDK project for crane motor speed, gear, direction, and load monitoring.

## Project Layout

- `User/`: application entry, GPIO, timer base, interrupt handlers, key service
- `BSP/`: board support modules for display, input capture, gear, direction, frequency, and weight curve storage
- `Library/` and `Start/`: STM32F10x standard peripheral library and startup files
- `test.uvprojx`: Keil project file

## Version Control Notes

Generated Keil outputs are ignored through `.gitignore`. Commit source files, headers, startup/library files, and `test.uvprojx` / `test.uvoptx`; do not commit `Objects/`, `Listings/`, local `.uvguix` files, or backup folders.
