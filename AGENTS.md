# dpx Agent Workflow & Documentation Standards - v1d6

This document provides operational directives for AI coding assistants (GitHub Copilot, Claude Code, Cursor, etc.) working on dubpixel projects. These rules ensure consistent workflow automation, code quality, and documentation maintenance across all repositories.

---

## PROJECT: dpx_elecrow_eink

**Status:** Firmware template flashed on real hardware twice (2026-09-17): v0.1.1 fixed a framebuffer-geometry bug (black bar + garbled text) but partial-refresh text still came out faint; v0.1.2 fixes that (missing partial-refresh reference-RAM sync). **v0.1.2 has not yet been confirmed on hardware** — I (the agent) cannot flash it myself, so treat it as fixed-by-analysis until someone reflashes and reports back. Still otherwise unvalidated: WiFi paths, ghosting mitigation, deep sleep power draw.
**Branch:** `main` (PR'd from `feature/crowpanel-579-firmware-template`)
**Version File:** `firmware/VERSION` (currently 0.1.2)

### Architecture (2-minute summary)

Hardware + firmware project built around the **ELECROW CrowPanel 5.79" E-Paper
HMI Display** (Amazon ASIN `B0FX4PDW6M`, ~$49.99), an ESP32-S3-WROOM-1-N8R8
(240MHz, 8MB flash, 8MB octal PSRAM) driving a 792×272 monochrome e-paper
panel over bit-banged SPI. The panel is physically **two cascaded SSD1683
controllers**, not one — see Gotchas below, this drives most of the firmware
design. Vendor examples are Arduino/PlatformIO C++. Goal: replace the
manufacturer's example firmware with a safer, better-documented driver +
template project.

| Component | Tech/Location | Purpose | Notes |
|-----------|---------------|---------|-------|
| Panel driver | C++ / `firmware/lib/crowpanel_579/` | Register-level (epd_ll) + framebuffer/drawing (epd_gfx) + session API (epd_panel/CrowPanel579) | Rewritten from vendor source with bounds-checked coordinates and a refresh-mode guard — see file headers for what changed and why |
| Template sketch | Arduino / `firmware/src/main.cpp` | Bring-up demo exercising full+partial refresh + sleep | Starting point for real projects, not a finished product |
| Build config | `firmware/platformio.ini` | Pinned toolchain (espressif32@6.3.2, qio_opi, huge_app partition) | Every pinned value here is load-bearing — see Gotchas |
| Vendor reference | `firmware/mfg_examples/` | ELECROW's own example code, downloaded from [Elecrow-RD/CrowPanel-ESP32-5.79-E-paper-HMI-Display-with-272-792](https://github.com/Elecrow-RD/CrowPanel-ESP32-5.79-E-paper-HMI-Display-with-272-792) | Kept for reference/diffing, not used directly by the template |
| Review gotchas | Markdown / `firmware/mfg_examples/AMAZON_REVIEW_GOTCHAS.md` | Field-tested bugs/workarounds pulled from real Amazon customer reviews | **Source of truth for why the driver is written the way it is** — read before "simplifying" epd_gfx.cpp |
| Hardware refs | Eagle sch/pcb, STEP, datasheets / `hardware/` | Schematics, 3D files, BOM, datasheets (mostly vendor-sourced) | See `hardware/spec/datasheet` and `hardware/schematic` |

### Agent Rules (for this repo)

**Before ANY code change:**
1. Work from `main`; create feature branch: `feature/brief-description`
2. Bump VERSION file per semantic versioning (AGENTS.md §1) once one exists — none does yet, create it at `firmware/VERSION` on the first real firmware change
3. Create git commit for version bump, tag it: `git tag vX.Y.Z`

**While coding:**
- Never change `firmware/platformio.ini`'s pinned `platform =`, `board_build.arduino.memory_type`, or `board_build.partitions` without re-reading the Gotchas below — each one exists because of a specific reported failure, not a preference
- Coordinates passed into any `epd_gfx`/`CrowPanel579` drawing call must be the **last valid pixel** (dimension − 1), never the raw width/height — the driver clamps instead of crashing now, but a clamp still means your layout is wrong
- Always call `panel.sleep()` at the end of a drawing session before returning to `loop()`/deep sleep — this is what gives the panel its near-zero idle draw
- Don't call `epd_ll_refresh_partial()`/`epd_ll_refresh_full()`/`epd_ll_refresh_fast()` directly from application code — go through `CrowPanel579::fullRefresh()`/`partialRefresh()`, which enforces the mode-mixing and ghosting-mitigation rules
- File header per AGENTS.md §3

**When done:**
- Update CHANGELOG.md with feature list
- Create PR per AGENTS.md §1 template
- Firmware changes: confirm it still compiles under the pinned toolchain before claiming done (PlatformIO is not installed in this dev environment as of 2026-09-15 — note that limitation rather than silently skipping the check)

### Critical Constraints

**MUST HAVE:**
- ✅ Arduino core **2.x** (`espressif32 @ 6.3.2`) — core 3.x breaks the WebServer/BLE APIs vendor examples (and likely future WiFi code here) depend on
- ✅ `board_build.arduino.memory_type = qio_opi` + `board_build.psram_type = opi` — required for the WROOM-1-N8R8's *octal* PSRAM; the default board profile assumes quad and silently strands most of the 8MB
- ✅ `huge_app` partition table — WiFi + JSON + the font-heavy graphics stack don't fit in the OTA-enabled default partition layout

**DO NOT:**
- ❌ Center a layout across the full 792px width without accounting for the hidden ~8px gap at logical x≈396–403 (see Gotchas)
- ❌ Pass raw width/height (792/272) as a line/rect endpoint — pass the inclusive last pixel (791/271); the driver now clamps this instead of panicking, but it's still a layout bug if you rely on the clamp
- ❌ Mix full/partial/fast refresh modes ad hoc — go through `CrowPanel579`'s guard, which forces a full refresh on demand instead of leaving a blank panel with a "successful" busy handshake
- ❌ Assume Elecrow's own image-conversion tool is usable here — it's Windows-only; use `image2cpp` ("Horizontal - 1 bit per pixel") for bitmap assets instead

### Key Decisions

- **Rewrote the vendor driver instead of using it as-is:** the factory `EPD.cpp`/`EPD_Init.cpp` has no coordinate bounds-checking (out-of-bounds write → ESP32 panic) and no refresh-mode guard — both are confirmed real-world failure modes from Amazon reviews, not theoretical. See `firmware/mfg_examples/AMAZON_REVIEW_GOTCHAS.md`.
- **Kept the register-level init sequences (epd_ll.cpp) byte-for-byte from vendor source:** these talk directly to the SSD1683 pair and there's no hardware here to validate a rewrite against — only renamed/reorganized, not reworked.
- **Kept the vendor's bundled ASCII bitmap font as-is (`epd_font_tables.h`):** it's a real hardware/asset limitation (only sizes 12/16/24/48 exist), not a driver bug — `epd_font.h` fails loud on unsupported sizes instead of vendor's silent no-op, but doesn't try to synthesize missing sizes.
- **Ghosting mitigation added that vendor examples don't have:** `CrowPanel579::partialRefresh()` forces a full refresh every `EPD_PANEL_PARTIAL_REFRESH_LIMIT` (default 10) partial refreshes — untested against real hardware, tune to taste.

### Gotchas & Landmines

1. **Hidden two-controller gap:** The panel is two SSD1683 chips side-by-side (RAM is 800px wide, only 792 visible), with a permanent ~8px hidden gap at logical x≈396–403. Anything centered on the visible width straddles it. `epd_set_pixel()` already offsets for this — don't add your own compensation on top.
2. **Coordinates aren't hardware-bounds-checked:** the SSD1683 RAM addressing will happily accept an out-of-range byte address. The rewritten driver clamps in `epd_set_pixel()`; the *original* vendor driver did not, and would underflow/panic the ESP32 given `width`/`height` instead of `width-1`/`height-1`. If you ever fall back to vendor code, watch for this.
3. **Mixing refresh modes blanks the panel:** switching between full/partial/fast update without an intervening full refresh leaves the panel blank even though the busy handshake reports success. Always go through `CrowPanel579`, which handles this automatically.
4. **Build banner falsely says "No PSRAM":** with the pinned `qio_opi` config this is a known false negative at compile-time board detection — confirm PSRAM at runtime with `psramFound()`/`ESP.getPsramSize()` instead.
5. **ESPHome does NOT work out of the box:** a customer review (Home Assistant integration) confirms the stock ESPHome e-paper config doesn't handle the dual-SSD1683 master/slave setup — a custom ESPHome driver is required. Not relevant unless this project adds Home Assistant integration.
6. **Physical fragility:** the side rotary/multi-control switch handle is thin plastic and can snap if dropped or bumped (per-review report) — plan for an enclosure early, don't leave it loose on a desk during dev.
7. **Full refresh wears the panel + is slow (~1.9s):** don't redraw more than about once a minute for anything beyond bring-up testing.
8. **`epd_paint_new()` has no width/height parameters — don't add them back.** The first hardware flash (2026-09-17) showed a black bar at the halfway point of the display and garbled text, caused by passing a halved buffer width into that function: it desynced the framebuffer's byte stride from what `epd_ll_write_frame()` assumes (fixed 100 bytes/row) and left half the buffer uncleared. Buffer packing geometry is now a hardware-fixed constant inside `epd_paint_new()`; the logical/visible canvas used for coordinate clamping is derived separately from rotation. See the comment on `epd_paint_new()` in `epd_gfx.h` before touching this again.
9. **Every refresh must sync the "reference" RAM bank afterward, or partial refreshes come out faint.** The SSD1683 pair diffs new content (registers 0x24/0xA4) against a separate "previous frame" bank (0x26/0xA6) for partial updates — nothing updates that second bank automatically. `epd_ll_clear_ram()` only sets it once at startup; skip syncing it after later refreshes and every partial refresh diffs against that stale baseline, so real content changes look like "no change" and barely darken. `CrowPanel579::fullRefresh()`/`partialRefresh()` both call `epd_ll_sync_reference_ram()` after triggering the actual refresh — don't call `epd_ll_refresh_full()`/`epd_ll_refresh_partial()` directly without also doing this.

Full sourcing for all of the above: `firmware/mfg_examples/AMAZON_REVIEW_GOTCHAS.md` (pulled from real Amazon customer reviews on the product listing) plus the [Elecrow wiki tutorial](https://www.elecrow.com/wiki/CrowPanel_ESP32_E-paper_5.79-inch_HMI_Display.html) and the [vendor GitHub repo](https://github.com/Elecrow-RD/CrowPanel-ESP32-5.79-E-paper-HMI-Display-with-272-792).

### Common Operations

**Build/flash the template:** `cd firmware && pio run -t upload` (PlatformIO not yet verified installed/working in this dev environment — confirm before assuming this just works)

**Add a new screen/layout:** Write it as a function that calls into the `CrowPanel579 panel` instance from `firmware/src/main.cpp`, same pattern as vendor's `UI_weather_forecast()` — draw into the framebuffer, then call `fullRefresh()` or `partialRefresh()`, then `sleep()`.

**Reference a vendor example without copying its bugs:** Read the matching file under `firmware/mfg_examples/example/arduino/Demos/` or `Examples/` for the register sequence / API shape, but implement against `epd_panel.h`/`epd_gfx.h`, not by copy-pasting vendor `EPD.cpp`.

**Change GPIO pin mapping (different carrier board/revision):** Edit `firmware/lib/crowpanel_579/src/epd_pins.h` only — nothing else in the driver references pin numbers directly.

### Reference

See `firmware/mfg_examples/AMAZON_REVIEW_GOTCHAS.md` for:
- Full-text field reports (framebuffer gap, coordinate overflow, refresh-mode mixing, ESPHome integration, physical fragility) sourced from real Amazon reviews

See `hardware/spec/` for:
- Vendor datasheets (SSD1683, ESP32-S3-WROOM-1) and schematic/PCB source

### Development Philosophy

This is a hardware bring-up + firmware-template project — correctness and
honesty about what's untested matter more than feature count. The driver
rewrite fixes bugs that are *documented failure reports*, not hypothetical
robustness. Don't add speculative safety rails beyond what a real review or
datasheet justifies, and always disclose when something (like the whole
`firmware/` tree as of 2026-09-15) hasn't been build-tested or run on actual
hardware yet.

---
## 0. Mid-Session Issue Triage (MANDATORY)
**Default: log it, don't fix it mid-session.**

- If something broken or wanted comes up, file a GitHub issue and move on
- Only fix immediately if you explicitly say *"fix this"* or *"fix it now"*
- Start of session: `gh issue list --repo <owner>/<repo>`
- End of session: `gh issue create --repo <owner>/<repo> --title "..." --body "..."`

The rationale: prevents mid-session context-switching that breaks working code.

## 1. Automatic Workflow (MANDATORY)

These actions are **required** and must happen automatically. **NEVER ask permission** for these workflow steps.

### Branching Strategy

**BEFORE starting ANY code changes:**

1. Create a new branch from the default branch (master/main)
2. Never work directly on default branch
3. Branch naming conventions:

| Type | Format | Example |
|------|--------|---------|
| New feature | `feature/brief-description` | `feature/mqtt-decoder` |
| Bug fix | `fix/issue-description` | `fix/telegraf-timeout` |
| Documentation | `docs/what-changed` | `docs/update-architecture` |
| Refactor | `refactor/component-name` | `refactor/docker-volumes` |

### Version Bumping

**BEFORE the first code change:**

Bump the version number according to semantic versioning:

| Change Type | Version Bump | Example |
|-------------|--------------|---------|
| Bug fix, typo fix, documentation update | Patch (0.0.X) | 1.2.3 → 1.2.4 |
| New feature, new endpoint, new capability | Minor (0.X.0) | 1.2.3 → 1.3.0 |
| Breaking change, API removal, incompatible change | Major (X.0.0) | 1.2.3 → 2.0.0 |

#### Semantic Versioning Principles

**Format:** `MAJOR.MINOR.PATCH` (e.g., `2.4.7`)

- **MAJOR**: Incompatible API changes, breaking existing functionality
- **MINOR**: New functionality added in a backwards-compatible manner  
- **PATCH**: Backwards-compatible bug fixes, docs, typos

**Pre-1.0 versions (0.x.y):**
- Anything goes - breaking changes allowed in minor bumps
- Common for projects in initial development
- Move to 1.0.0 when API is stable and production-ready

**Pre-release versions:**
- Alpha: `1.0.0-alpha.1` (early testing, unstable)
- Beta: `1.0.0-beta.2` (feature-complete, testing for bugs)
- Release Candidate: `1.0.0-rc.1` (final testing before release)

#### Version Bumping Decision Tree

**When multiple changes occur, use the highest level:**
- Bug fix + new feature → Minor bump (not patch)
- New feature + breaking change → Major bump (not minor)

**Edge cases:**

| Scenario | Bump Type | Reasoning |
|----------|-----------|-----------|
| Internal refactor, no API change | Patch | No external impact |
| New optional parameter with default | Minor | Backwards-compatible addition |
| Changed parameter order | Major | Breaks existing calls |
| Deprecated feature (still works) | Minor | Deprecation warning added |
| Removed deprecated feature | Major | Functionality removed |
| Performance improvement | Patch | Implementation detail |
| New dependency added | Minor | Expands capabilities |
| Security fix | Patch | Even if behavior changes slightly |
| Database schema change | Major | Requires migration |
| Config file format change | Major | Breaking existing configs |

#### Version Bump Workflow

1. **Determine bump type** based on changes planned
2. **Update version number** in code/config files
3. **Create git commit**: `bump version to X.Y.Z`
4. **Tag the commit**: `git tag vX.Y.Z` (note the `v` prefix)
5. **Push with tags**: `git push && git push --tags`
6. **Update CHANGELOG** (if present) with version and changes
7. **Proceed with feature/fix implementation**

**Version commit should be standalone** - don't mix version bump with other changes.

#### Changelog Integration

If project has CHANGELOG.md, update it with version bump:

```markdown
## [1.2.0] - 2026-02-13

### Added
- New feature description

### Fixed
- Bug fix description

### Changed
- Breaking change description
```
**If no CHANGELOG.MD file exists:** Create one in the root of the project.

**Where to bump version:**
- Python: `__version__` in `__init__.py` or `pyproject.toml`
- Node.js: `version` field in `package.json`
- General: `VERSION` file or constant in main entry point
- Docker: Version tag in `docker-compose.yml` or `Dockerfile` labels

**If no version file exists:** Create one in an appropriate location for the project.

### Version File Standards & Location

To ensure consistent version identification across projects, follow these standards:

#### Python Projects

**Preferred location: `app/__init__.py` or `src/__init__.py`**

```python
"""Project description."""

__version__ = "1.0.0"
__author__ = "dubpixel"
```

**Alternative: `pyproject.toml` (for modern Python packaging)**

```toml
[project]
name = "project-name"
version = "1.0.0"
```

**Alternative: `VERSION` file in project root**

```
1.0.0
```

Then read it in your module:
```python
from pathlib import Path
__version__ = (Path(__file__).parent / "VERSION").read_text().strip()
```

#### Node.js/JavaScript Projects

**Location: `package.json`** (standard)

```json
{
  "name": "project-name",
  "version": "1.0.0",
  "description": "Project description"
}
```

#### Docker Projects

**Location: `docker-compose.yml` labels AND `Dockerfile`**

`docker-compose.yml`:
```yaml
services:
  app:
    build: .
    labels:
      - "org.opencontainers.image.version=1.0.0"
      - "org.opencontainers.image.created=${BUILD_DATE}"
```

`Dockerfile`:
```dockerfile
LABEL org.opencontainers.image.version="1.0.0"
LABEL org.opencontainers.image.title="Project Name"
```

#### Bash Scripts/Utilities

**Location: Top of main script or separate `VERSION` file**

```bash
#!/bin/bash
VERSION="1.0.0"
SCRIPT_NAME="manage.sh"

# Or read from VERSION file:
# VERSION=$(cat VERSION)
```

#### Version Display (REQUIRED)

**Always provide a way to display the version:**

- Python CLI: `python -m myapp --version`
- Node.js: `npm run version` or built into CLI
- Docker: `docker inspect <image> | grep version`
- Bash: `./script.sh --version`

**Example implementations:**

```python
# In your main.py or CLI entry point
import argparse
from app import __version__

parser = argparse.ArgumentParser()
parser.add_argument('--version', action='version', version=f'%(prog)s {__version__}')
```

```bash
# In bash script
if [[ "$1" == "--version" ]] || [[ "$1" == "-v" ]]; then
    echo "$SCRIPT_NAME version $VERSION"
    exit 0
fi
```

#### Multi-Component Projects

For projects with multiple components (e.g., frontend + backend + Docker):

1. **Synchronized versioning**: All components share the same version
2. **Central `VERSION` file** in project root
3. **Scripts/tools read from central file**

Example structure:
```
project-root/
├── VERSION              # 1.0.0
├── backend/
│   └── __init__.py      # Reads ../VERSION
├── frontend/
│   └── package.json     # Reads ../VERSION via build script
└── docker-compose.yml   # Reads VERSION via envsubst or build args
```

### Pull Request Creation

**AFTER completing the task:**

Create a pull request with this format:

```markdown
## Changes
- [Brief list of what changed]
- [One item per significant change]

## Testing
- [How to verify the changes work]
- [Commands to run or steps to follow]

## User Prompt
[The original request from the user - verbatim]
```

**PR Title Format:** `[Component] Brief description`

Examples:
- `[MQTT] Add BLE decoder support`
- `[Docs] Consolidate architecture documentation`
- `[Telegraf] Fix enum processor deprecation`

**NEVER ask permission to create the PR - just do it.**

### Build Artifact Naming Convention

Non-`main` builds append the branch slug to the filename so artifacts are
self-identifying without opening the run log.

| Branch | Filename |
|--------|----------|
| `main` | `<name>-vX.Y.Z.<ext>` |
| anything else | `<name>-vX.Y.Z-<branch-slug>.<ext>` |

```bash
if [ "$BRANCH" = "main" ]; then
  OUT="myapp-v${VERSION}.ext"
else
  BRANCH_SLUG=$(echo "$BRANCH" | sed 's|/|-|g' | sed 's|[^a-zA-Z0-9._-]|-|g')
  OUT="myapp-v${VERSION}-${BRANCH_SLUG}.ext"
fi
```


## 2. Progress Tracking for Multi-Step Work

When working on tasks that span **more than 3 files** OR **more than 30 minutes of work**:

### Checkpoint Progress

Provide a status update using this template:

```markdown
## Progress Checkpoint

✅ **Completed:**
- Item 1 description
- Item 2 description

⬜ **Remaining:**
- Item 3 description
- Item 4 description

→ **Next Action:** [Specific next step you will take]
```

### When to Checkpoint

- After completing a logical phase of work
- Before switching to a different component
- When encountering a blocker or decision point
- Every 3-5 file edits in large refactors

### Resuming from Checkpoint

When continuing work after a checkpoint:
1. Read the last checkpoint status
2. Start with the "Next Action" item
3. Update checkpoint when that phase completes

**Purpose:** Prevents agents from getting lost in complex multi-step tasks and provides visibility to the user.

---

## 3. File Header Standards

All code files must include a comprehensive header comment section:

```
# ================================================================================
# [FILE TYPE] - [FILE PURPOSE]
# ================================================================================
# you can maybe write some stuff here - tagline etc.
# ================================================================================
# PROJECT: [project_name]
# ================================================================================
#
# File: [filename]
# Purpose: [what this file does]
# Dependencies: [key dependencies if any]
#
# CHANGE LOG: (if needed but should really be in the changelog for the git)
# 
# 2026-03-06: Complete rewrite - Interactive wizard (v2.1.0)
#
# ================================================================================
```

### Header Guidelines

- Use consistent separator lines (80 characters of `=`)
- Adjust comment syntax for the language (`#` for Python/bash, `//` for JS/C++, etc.)


---

## 4. Documentation Standards

### Project Context Documentation

Project-specific architecture, decisions, and operational knowledge should live in the **PROJECT section at the top of this file**. This keeps rules and context unified in one scannable document.

**When to use a separate CONTEXT.md:**
Only create a separate `CONTEXT.md` if reference data becomes large enough to be noisy:
- Long IP/VLAN tables
- Full API response examples  
- Hardware pinout references
- Extensive data schemas

If you create CONTEXT.md for overflow, add a reference in the PROJECT section at the top: "See CONTEXT.md for full network topology."

### How to Document Project Context

**DO:**
- ✅ Keep it clean, factual, and scannable
- ✅ Update when architecture changes
- ✅ Add information when you learn important project details
- ✅ Use tables, code blocks, and clear headings
- ✅ Think: "What does the next agent need to know?"
- ✅ Write in present tense, authoritative voice

**DON'T:**
- ❌ Append conversation transcripts
- ❌ Include timestamps like "On Feb 12 we discussed..."
- ❌ Make it a session log or diary
- ❌ Duplicate content from README.md (link instead)
- ❌ Let it become verbose or messy

**Update frequency:** Whenever you make architectural changes or learn critical project information.

---

## 5. Core Principles


### No Modifications to Working Code

- Do not refactor, optimize, or "improve" code that is working unless explicitly requested
- Avoid drive-by refactors when implementing a feature
- If you see potential improvements, mention them but don't implement without approval

### Comprehensive Commenting

- Document all code with clear, meaningful comments
- Preserve existing comments unless they become obsolete
- Remove or update comments that are no longer accurate
- Document WHY, not just WHAT (the code shows what, comments explain why)

### Small, Incremental Changes

- Make one logical change per commit
- Break large tasks into smaller steps
- Test each change before moving to the next
- Make it easy to review and roll back if needed

### Stay Focused

- Complete the current task before suggesting next steps
- Answer only what is asked
- Don't anticipate or propose additional work unless requested

### Document Everything

- README.md must be updated and maintaned when appropriate as per these guidelines
- CONTEXT.md must be updated and maintaned when appropriate as per these guidelines
- a comprehensive CHANGELOG.md must be kept updated and maintaned when appropriate as per these guidelines
- you can maintain a small changelog in the header if you wish but main changelog should be in the MD

---

## 6. Documentation Standards

### Inline Documentation

- Maintain comprehensive inline documentation
- Update comments when code changes (keep them in sync)
- Document all function parameters and return values
- Include usage examples for complex functions
- Explain algorithms and business logic


### README Files

- Keep README.md current and accurate
- README is user-facing - focus on how to USE the project
- **Confirm all changes to README with the user before committing**
- README should not duplicate CONTEXT.md (different audiences)

### CHANGELOG.MD

- Keep CHANGELOG.md current and accurate
- CHANGELOG is user-facing - focus on changes, version numbers, dates and git hashes if needed
- **keep this automated in background**
- changelog could be retroactively updated to reflect git commit names if that adds clariy
- **any changes to existing changelog line items should be confirmed with user**

### Markdown Style

- Use consistent heading hierarchy (don't skip levels)
- Use tables for structured information
- Include code blocks with language tags
- Use relative links to other project files
- Keep line length reasonable (~80-100 chars for prose)

---

## 7. Code Quality Guidelines

### General Principles

- Write clear, readable code with meaningful names
- Follow established coding patterns within the project
- Implement proper error handling (don't use bare `except:` or `catch`)
- Write testable code with clear interfaces
- Maintain consistent formatting and style

### Language-Specific

Agents should infer and follow the conventions of the language they're working in:
- Python: Follow PEP 8
- JavaScript: Follow project's ESLint config if present
- Bash: Follow Google Shell Style Guide principles
- Other languages: Use community-standard style guides

### Testing

- Add tests alongside new logic when appropriate
- Use deterministic inputs for tests (inject time/randomness, don't read system state)
- Name tests by behavior (e.g., `test_early_finish_extends_break`)
- Include both positive and negative test cases
- if the process includes: using ssh into a remote server, or user input in any way. open a terminal first for the user so you both can read it. 

---

## 8. Change Management

### Commit Practices

- **Commit message format:** Short, plain English, lowercase verb
  - Examples: `add mqtt decoder`, `fix telegraf config`, `update documentation`
- Make one logical change per commit
- Commit functional units (don't commit broken code)

### Before Committing

- Verify the code works (run/test it)
- Update all relevant documentation
- Update file change logs
- Remove debug code and console.log/print statements
- Check that no credentials or secrets are included

### After Committing

- Push to the feature branch
- Create PR (as described in Section 1)
- Include verification steps in PR description

---

## 9. Collaboration Standards

### Respect Existing Architecture

- Understand existing architectural decisions before changing them
- Ask for clarification when requirements are ambiguous
- Suggest alternatives when appropriate, but don't insist
- Consider the impact of changes on the broader codebase

### Maintain Backwards Compatibility

- Don't break existing APIs unless explicitly requested
- Provide migration paths for breaking changes
- Document any compatibility changes in PR description

### Communication

- Explain the reasoning behind suggested changes
- Provide rollback information when making significant changes
- Be transparent about limitations or uncertainties
- Keep responses concise and focused

---

## 10. Configuration & Secrets

### Environment Variables

- Use `.env` files for local development
- Provide `.env.example` with all required variables (use placeholder values)
- **NEVER commit** `.env` files or actual credentials to git
- Document all environment variables in CONTEXT.md or README.md

### Sensitive Data

- Keep credentials in environment variables, not hardcoded
- Use service account files in standard locations (e.g., `~/.config/gcloud/`)
- Add sensitive files to `.gitignore` immediately
- If secrets are accidentally committed, notify the user immediately

---

## Summary: Agent Checklist

Before starting work:
- [ ] Create feature branch
- [ ] Bump version appropriately

While working:
- [ ] Follow file header standards
- [ ] Update change logs in modified files
- [ ] Keep changes small and focused
- [ ] Checkpoint progress if task is large
- [ ] Update PROJECT section if architecture changes

After completing work:
- [ ] Test/verify the changes
- [ ] Update relevant documentation
- [ ] Create PR with proper format
- [ ] No credentials committed

---

*These standards ensure consistent, high-quality AI assistance across all Dubpixel projects.*
