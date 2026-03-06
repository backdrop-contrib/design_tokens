# Theme Tokens — Project Notes for Claude

## What This Module Is

**Theme Tokens** is a Backdrop CMS module that replaces the Color module with a modern,
CSS custom property-based approach to theme configuration. It provides the infrastructure
for token-based theming; sub-modules add support for specific token types (colors, fonts, etc.).

- **Machine name:** `theme_tokens`
- **Human name:** Theme Tokens
- **Repo:** TBD (independent project, developed alongside Opera for testing)
- **Maintainer:** Tim Erickson (stpaultim) / Simplo / Triplo

---

## Why This Exists

The Color module is widely considered a necessary evil in both Drupal and Backdrop. It works
by doing string replacement on CSS files — a fragile, inelegant approach invented before CSS
custom properties existed. Theme Tokens replaces this entirely:

| Color module | Theme Tokens |
|---|---|
| Rewrites CSS files | Injects a `:root {}` block in `<head>` |
| Stores values as database variables | Stores values in Backdrop config (JSON) |
| Fixed number of color fields | Theme declares any tokens it needs |
| Generates a palette image | Live preview via JS + postMessage |
| Pre-CSS-variables hack | CSS custom properties as the native mechanism |

---

## Module Structure

Theme Tokens is an infrastructure module. It knows nothing about colors or fonts on its own.
Sub-modules register token types and provide the field UI for editing them. Sites enable only
what they need.

```
theme_tokens/                           ← infrastructure only: token loading, :root{}
  theme_tokens.info                       injection, admin framework, config, menu
  theme_tokens.module
  theme_tokens.admin.inc
  theme_tokens.api.php
  js/theme-tokens-admin.js
  css/theme-tokens-admin.css
  CLAUDE.md
  modules/
    theme_tokens_color/                 ← color token type (color pickers, schemes)
      theme_tokens_color.info
      theme_tokens_color.module
    theme_tokens_font/                  ← font token type (font selectors, stacks)
      theme_tokens_font.info
      theme_tokens_font.module
```

### Why sub-modules, not separate modules?

- The parent provides shared infrastructure all token types depend on: file discovery,
  config storage, CSS injection, admin page framework, and live preview
- Sub-modules are separable (enable/disable independently) but share the codebase cleanly
- A site that wants only font control, only color control, or both can enable accordingly
- Third-party developers can add new token types (`theme_tokens_spacing`, etc.) as
  additional sub-modules without touching the parent

### Why not bake color/font support into the parent?

A given site may want one but not the other. The parent module with no sub-modules enabled
is intentionally inert — it provides no UI and injects nothing until at least one token
type sub-module is active.

---

## Architecture

### How It Works

1. A theme declares available tokens in a `tokens.inc` file in its root directory
2. The parent module discovers the file and loads the token definitions
3. Sub-modules register token types and tell the parent how to render their fields
4. Site admin edits values through the admin UI and saves
5. Values are stored in Backdrop config: `theme.tokens.THEMENAME.json`
6. On every page render, the parent injects a `<style>` tag into `<head>`:
   ```html
   <style>:root { --color-primary: #6e0e0a; --font-heading: Merriweather, serif; }</style>
   ```
7. The theme's CSS uses `var(--color-primary)` — no CSS files are ever rewritten

### CSS Custom Property Naming

- **No prefix** — the CSS custom property name is exactly what the theme declares
- Theme author controls the names: `color-primary` becomes `--color-primary`
- This keeps theme CSS clean and readable

### Token Type Registration

Sub-modules register their token type via `hook_theme_tokens_types()`:

```php
function theme_tokens_color_theme_tokens_types() {
  return array(
    'color' => array(
      'label'          => t('Color'),
      'field callback' => 'theme_tokens_color_build_field',
    ),
  );
}
```

The parent calls `module_invoke_all('theme_tokens_types')` when building the admin form
and delegates field rendering to whichever sub-module owns that type. Unknown token types
in a `tokens.inc` are silently ignored (graceful degradation if a sub-module is disabled).

### Storage

Config file per theme: `theme.tokens.THEMENAME.json`
```json
{
  "tokens": {
    "color-primary": "#6e0e0a",
    "color-primary-text": "#fff6ff",
    "font-heading": "Merriweather, Georgia, serif"
  }
}
```

No database variables. Config is exportable, deployable, and version-controllable.

---

## Token Declaration Format (`tokens.inc`)

### Why `tokens.inc` and not the `.info` file?

The `.info` format is INI-based — it can only handle flat key/value pairs. It cannot
express the nested structure needed for token definitions (label, type, group, default)
or preset schemes with grouped values. A PHP file also supports `t()` for translatable
labels, which `.info` files do not. The name `tokens.inc` is a deliberate parallel to
`color.inc`, making it immediately familiar to anyone who has worked with the Color module.

### Format

Themes place a `tokens.inc` file in their root directory. The module detects it automatically.

```php
<?php
$info = array(
  // Token groups for organizing the admin UI.
  'groups' => array(
    'header'     => t('Header & Footer'),
    'hero'       => t('Hero Blocks'),
    'typography' => t('Typography'),
  ),

  // Available tokens. Each token's 'type' must be provided by an
  // enabled sub-module, otherwise it is ignored gracefully.
  'tokens' => array(
    'color-primary' => array(
      'label'   => t('Background'),
      'type'    => 'color',       // handled by theme_tokens_color
      'default' => '#6e0e0a',
      'group'   => 'header',
    ),
    'font-heading' => array(
      'label'   => t('Heading Font'),
      'type'    => 'font',        // handled by theme_tokens_font
      'default' => 'Merriweather, Georgia, serif',
      'group'   => 'typography',
    ),
  ),

  // Pre-defined schemes. Only token types provided by enabled sub-modules
  // are applied when a scheme is selected.
  'schemes' => array(
    'default' => array(
      'title'  => t('Opera (default)'),
      'tokens' => array(
        'color-primary' => '#6e0e0a',
        'font-heading'  => 'Merriweather, Georgia, serif',
      ),
    ),
  ),
);
```

---

## Admin UI

### URL Structure

| Path | Purpose |
|---|---|
| `admin/appearance/tokens` | Overview — lists all themes with token support |
| `admin/appearance/tokens/%theme` | Token settings for a specific theme |

### Theme Settings Integration

The theme settings page (`admin/appearance/settings/THEMENAME`) gets a simple notice
with a link to the token settings page. No attempt to embed the UI there — Backdrop's
theme settings form has too many limitations (no tabs, constrained form structure).

### Settings Page Layout

- Preset scheme selector at the top
- Token fields grouped by `group`, each rendered by the appropriate sub-module
- Live preview iframe alongside the form
- Save / Reset to defaults buttons

### Live Preview

Modern approach — no image generation, no server round-trips:

1. Admin page renders an iframe showing the live site
2. JavaScript listens to field `input` events in real-time
3. Uses `postMessage` to send updated CSS variable values to the iframe
4. Iframe applies them instantly as inline `:root {}` overrides
5. No page reload required

---

## Key Hooks

### Hooks the parent module implements
- `hook_preprocess_html()` — injects the `:root {}` `<style>` block
- `hook_menu()` — registers admin pages
- `hook_permission()` — `administer theme tokens`

### Hooks sub-modules implement
- `hook_theme_tokens_types()` — registers a token type and its field callback

### Hooks themes/modules can implement
- `hook_theme_tokens_info_alter(&$info, $theme)` — alter token definitions after loading
- Documented in `theme_tokens.api.php`

---

## Sub-module: theme_tokens_color

Registers the `color` token type. Provides:
- Color swatch + hex text field for each color token
- Scheme preset selector (populates all color fields at once via JS)
- Live preview updates via postMessage

**Status:** Infrastructure exists in parent; needs refactor to move color-specific
field building into this sub-module.

---

## Sub-module: theme_tokens_font

Registers the `font` token type. Provides:
- Font selector field (select or autocomplete)
- System font stack presets (no external requests)
- Google Fonts integration (opt-in)
- Optionally: font size base + modular scale ratio

**Status:** Not started. Build color sub-module first.

---

## Opera Integration

Opera is the reference implementation and primary test case.

### What's already done (2.x branch)
- `tokens.inc` created with all 15 color tokens in 5 groups
- All color schemes from `color.inc` ported over (default, plume, bright, night)
- `opera-variables.css` updated to new semantic CSS custom property names
- All CSS files updated to reference new token names

### What remains
- Remove `color.inc` once Theme Tokens is stable (Color module dependency gone)
- Remove color fieldsets from `theme-settings.php`
- Add link to `admin/appearance/tokens/opera` from theme settings page
- Add font tokens to `tokens.inc` once `theme_tokens_font` sub-module exists

### Token naming (Color module → Theme Tokens)

| Old (`opera-variables.css`) | New token name | CSS custom property |
|---|---|---|
| `--color-one` | `color-primary` | `--color-primary` |
| `--text-one` | `color-primary-text` | `--color-primary-text` |
| `--link-one` | `color-primary-link` | `--color-primary-link` |
| `--color-two` | `color-hero` | `--color-hero` |
| `--color-three` | `color-block-1` | `--color-block-1` |
| `--color-four` | `color-block-2` | `--color-block-2` |
| `--color-five` | `color-block-3` | `--color-block-3` |

---

## Migration from Color Module (Future)

Not a priority for v1 but keep the door open. A migration path would:
- Read existing `color.inc` scheme definitions
- Map color fields to token names via a theme-provided mapping
- Import saved Color module values into Theme Tokens config

---

## Development Notes

- Developed in the Opera dev environment at:
  `/home/tim/sites/ddev/backdrop/opera-dev/docroot/modules/theme_tokens`
- Independent project — separate repo from Opera
- Opera is used purely as the test theme during development
- Use `bee` as preferred CLI tool for Backdrop operations
- Follow Backdrop coding standards (see global CLAUDE.md)
- PHP compatible to 7.1 minimum

---

## Build Order

1. ~~`theme_tokens.info`~~ ✓
2. ~~`theme_tokens.module`~~ ✓ (needs refactor: extract color-specific code to sub-module)
3. ~~`theme_tokens.admin.inc`~~ ✓ (needs refactor: delegate field rendering to sub-modules)
4. ~~`js/theme-tokens-admin.js`~~ ✓
5. ~~Opera `tokens.inc`~~ ✓
6. `theme_tokens_color` sub-module — move color field logic here, add `hook_theme_tokens_types()`
7. Test end-to-end with Opera
8. `theme_tokens_font` sub-module (later)
