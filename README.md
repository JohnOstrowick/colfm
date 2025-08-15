# ColFM — Multi-View File Manager

ColFM is a Qt6-based file manager prototype with three switchable view modes:

- **Tree/List View** — traditional hierarchical file browser.
- **Column View** — Finder-like side-by-side navigation.
- **Icon View** — grid of file/folder icons.

## Features
- Switch views instantly via toolbar buttons.
- Dark-grey preview pane in Column View.
- Basic toolbar actions (placeholders for now).
- Go-up-a-level button works in all views.
- Built using C++17 and Qt6.

## Build Instructions

```bash
# Install Qt6 dev tools if not already installed
sudo apt install qt6-base-dev

# Clone the repo
git clone https://github.com/YOURUSERNAME/colfm.git
cd colfm

# Build
g++ -std=c++17 colfm.cpp -o colfm `pkg-config --cflags --libs Qt6Widgets`

# Run
./colfm
