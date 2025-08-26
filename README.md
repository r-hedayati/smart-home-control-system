
# Smart Home Control System

This repository contains all resources for a comprehensive smart home control system, including hardware designs, firmware/software, and documentation.


## Project Structure

```
/ 
├── hardware/
│   ├── ethernut/         # Ethernut hardware schematics, PCB, logs, previews
│   ├── udp/              # UDP networking hardware files
│   ├── datasheets/       # Datasheets for components
│   └── pcb/              # PCB design documentation, bill of materials, notes
├── software/
│   └── smart-home-app/   # Main software/firmware (Gradle project)
├── LICENSE
├── README.md
```

## Getting Started

1. **Hardware**: Find all PCB, schematic, and hardware design files under `hardware/`. Datasheets for all major components are in `hardware/datasheets/`.
2. **Software**: The main application and firmware are in `software/smart-home-app/`, organized as a Gradle project. See its README or documentation for build instructions.

3. **Documentation**: All project documentation, including PCB design and bill of materials, is now in `hardware/pcb/`.

## Usage

- To build or modify hardware, use the files in `hardware/ethernut/` and `hardware/udp/` with your preferred PCB/CAD tools.
- To develop or run the software, navigate to `software/smart-home-app/` and use Gradle commands (`./gradlew build`, etc.).
- Refer to `hardware/pcb/` for design details, instructions, and component information.

## Contributing

Contributions are welcome! Please open issues or pull requests for improvements, bug fixes, or new features.

## License

This project is licensed under the terms of the LICENSE file in the root directory.