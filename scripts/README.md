# Scripts

- **checksizes.sh** — Recursively scans images and prints each file’s pixel dimensions and DPI, flagging any that aren’t 128×128 at 96 dpi.
- **compile.sh** — Builds the project (main app and desktop variant), emitting compiler output and errors.
- **consistent_icons.sh** — Normalises icon PNGs: trims borders, resizes/pads to exactly 128×128 at 96 dpi, and writes results to `consistent_sizes/`.
- **debug_segfault.sh** — Runs the app under a debugger to capture a backtrace and help diagnose segmentation faults.
- **dedupe_md5.sh** — Finds duplicate files by MD5 hash, reports originals vs duplicates, and (optionally) moves/logs duplicates.
- **install_desktop_icon.sh** — Installs/updates the `.desktop` launcher for the app and assigns the application icon for your desktop/dock.
- **make_icon_xml.sh** — Generates `icons.xml` by base64-embedding all PNGs from `./icons/` into a simple XML structure.
- **mount_iphone.sh** — Mounts an iPhone filesystem (e.g., via `ifuse`) to a local mount point for file access.
