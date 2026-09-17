# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Rewritten CrowPanel 5.79" E-Paper driver (`firmware/lib/crowpanel_579/`): register-level (epd_ll), framebuffer/drawing (epd_gfx), and session API (epd_panel/CrowPanel579), with bounds-checked coordinates and a full/partial/fast refresh-mode guard
- `firmware/platformio.ini` pinned to the toolchain config required by this exact board (espressif32@6.3.2, qio_opi PSRAM, huge_app partition)
- `firmware/src/main.cpp` bring-up/template sketch
- `firmware/mfg_examples/`: full vendor example tree downloaded from Elecrow's GitHub repo, kept for reference
- `firmware/mfg_examples/AMAZON_REVIEW_GOTCHAS.md`: field-tested gotchas (framebuffer gap, coordinate overflow, refresh-mode mixing, ESPHome integration, physical fragility) distilled from real Amazon customer reviews
- Digested project architecture, constraints, key decisions, and gotchas into `AGENTS.md`'s PROJECT section

### Changed
- 

### Deprecated
- 

### Removed
- 

### Fixed
- Coordinate bounds-checking in the panel driver (vendor driver would underflow/panic the ESP32 if given raw width/height instead of the last valid pixel — now clamps instead)

### Security
- 

---

## [0.1.0] - YYYY-MM-DD

### Added
- Initial release
- Core functionality implementation

---

## Version Guidelines

### Semantic Versioning (MAJOR.MINOR.PATCH)

- **MAJOR**: Breaking changes, incompatible API modifications
- **MINOR**: New features, backwards-compatible additions
- **PATCH**: Bug fixes, documentation updates, typos

### Change Categories

- **Added**: New features or capabilities
- **Changed**: Changes to existing functionality
- **Deprecated**: Features marked for future removal (still working)
- **Removed**: Removed features or functionality
- **Fixed**: Bug fixes
- **Security**: Security patches or vulnerability fixes

### Example Entry Format

```markdown
## [1.2.0] - 2026-03-15

### Added
- New authentication system with JWT tokens
- Export functionality for CSV and JSON formats
- Dark mode toggle in user preferences

### Changed
- Improved database query performance by 40%
- Updated UI library from v2.1 to v3.0

### Fixed
- Fixed memory leak in background worker process
- Corrected timezone handling in date picker component

### Security
- Patched XSS vulnerability in user input validation
```

### Version Comparison Links

Add these at the bottom of the file (replace with your repo owner/name):

```markdown
[Unreleased]: https://github.com/owner/repo/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/owner/repo/releases/tag/v0.1.0
```

---

## Tips for Maintaining This Changelog

1. **Update as you work**: Add entries when making changes, not at release time
2. **Keep it scannable**: Use clear, concise descriptions
3. **Link to issues/PRs**: Include `(#123)` references when relevant
4. **Date format**: Use ISO 8601 (YYYY-MM-DD)
5. **Group by type**: Keep all Added items together, all Fixed items together, etc.
6. **User perspective**: Write what changed for users, not implementation details
7. **Unreleased section**: Keep active changes here, move to version section on release
