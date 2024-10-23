# TESSERACT
### The Elder Scrolls System & Environment for Real-time Autonomous Character Technology

TESSERACT is a sophisticated agent framework for The Elder Scrolls V: Skyrim, integrating LLM capabilities to create dynamic, autonomous characters within the game world.

## Overview
TESSERACT leverages modern AI technology to enhance NPC behavior and interaction in Skyrim. By combining SKSE plugin architecture with OpenAI's capabilities, this framework aims to create more engaging and responsive characters that can adapt to player actions and game state.

## Requirements
- The Elder Scrolls V: Skyrim Special Edition (1.5.97+)
- [SKSE64](https://skse.silverlock.org/)
- Visual Studio 2022 
- xmake 2.8.2+
- OpenAI API Key (for LLM functionality)

## Building
1. Clone the repository with submodules:
```bash
git clone --recursive https://github.com/YourUsername/TESSERACT.git
cd TESSERACT
```

2. If you downloaded without submodules, initialize them with:
```bash
git submodule update --init --recursive
```

3. Build using xmake:
```bash
xmake
```

## Dependencies
- [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) - SKSE plugin development library
- [OpenAI-cpp](https://github.com/olrea/openai-cpp) - C++ OpenAI API wrapper
- [SKSE Menu Framework V2](https://github.com/Thiago099/SKSE-Menu-Framework-2-Example) - UI framework for SKSE plugins

## Features
> Note: Project is currently in development. Features will be listed here as they are implemented.

- [ ] Core agent framework
- [ ] OpenAI integration
- [ ] Custom UI implementation
- [ ] Character state management
- [ ] Event system integration

## Contributing
Contributions are welcome! Please feel free to submit a Pull Request.

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments
- The SKSE team for their incredible work on the Script Extender
- The CommonLibSSE-NG team for their modernized SKSE development framework
- Bethesda Game Studios for creating The Elder Scrolls V: Skyrim

## Author
@psych0v0yager