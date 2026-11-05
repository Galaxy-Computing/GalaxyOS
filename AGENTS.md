# Project instructions

## Code comments

- Use comments sparingly. Only comment non-obvious intent, invariants, or gotchas; do not restate the code.
- Keep comments to short, lowercase sentence fragments without a trailing period, for example:
  - `// max bytes per transfer (kmalloc panics on OOM)`
  - `// don't panic on userspace exhaustion`
- Avoid multi-line explanatory blocks. Prefer a single line, or a short trailing comment where it fits.
- Match the tone and wording of the existing comments around the code you are editing.

## Code style

- Match the existing style around the code you are editing, including naming, indentation, error handling, and control-flow patterns.

## Build

- Kernel: `make kernel`
- Userland: `PROJECTS="libc kernel userland sdk" ./build.sh`
- Do not run `make` directly in a `userland/bin/*` directory; it uses the host compiler.
