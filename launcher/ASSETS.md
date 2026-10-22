# Optional local launcher artwork

The repository intentionally does **not** ship JoJo/game artwork.

The launcher build supports two optional local files placed in this directory:

- `launcher_bg.png` — 1031×648 launcher background.
- `jojo_launcher.ico` — Windows icon.

If `launcher_bg.png` is absent, `BUILD_LAUNCHER.ps1` creates a neutral, original fallback UI background automatically and embeds it in the launcher executable.

If `jojo_launcher.ico` is absent, Windows' default application icon is used.

These local files are ignored by Git so personal/fan artwork cannot be committed accidentally.
