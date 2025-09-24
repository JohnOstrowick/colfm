# ColFM — Multi-View File Manager

ColFM is a Qt6-based file manager prototype with three switchable view modes:

- **Tree/List View** — traditional hierarchical file browser.
- **Column View** — Finder-like side-by-side navigation.
- **Icon View** — grid of file/folder icons.

## Features
- Full file manager menubar on each window including all typical commands such as copy, link, move, rename, move to trash, move out of trash, folderize, archive, unzip, etc., etc.
- Mac OS X style columns and label menu to colour code items (useful when sorting photos to mark which ones you have processed or printed etc)
- Per-folder icon size, label colour, grid/column/icon view preferences
- Dark-grey preview pane in Column View / full get info screen (in column view the get info panel is the preview panel); spacebar = get info preview.
- Built using C++17 and Qt6.
- Desktop screen that inherits the desktop pattern from gnome session desktop wallpaper

## Build Instructions

```bash
# Install Qt6 dev tools if not already installed
sudo apt install qt6-base-dev
# search function uses plocate
sudo apt install plocate

# Clone the repo
git clone https://github.com/JohnOstrowick/colfm.git
cd colfm

# Build
./make

# Run
./colfm

# installation
cp colfm /usr/local/bin
bash scripts/install_desktop_icon.sh
```

## If you enjoy this

If you like this, please consider a small donation for me at

Bitcoin: 1HYvud8dJx5JMHA9FjWw6zp6NbT6DmivEF
