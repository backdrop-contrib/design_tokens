# Design Tokens

Design Tokens is a Backdrop CMS module that provides a modern, flexible approach to theme
configuration using CSS custom properties (CSS variables). Themes declare the design tokens
they support — colors, fonts, border radius, button styles, and more — and site architects
configure those values through a clean admin UI without writing any CSS.

This module is a replacement for the Color module, which predates CSS custom properties and
works by doing fragile string replacement on CSS files. 

Design Tokens stores values inBackdrop config, injects a single `:root {}` block in `<head>`, 
and never rewrites any files.

```
  :root {
    --color-primary: #6e0e0a;
    --color-primary-text: #fff6ff;
  }
```

The theme's CSS is written to use those variable names — color: var(--color-primary) —
rather than hardcoded values. When an admin changes a color, Design Tokens just updates that
one block. No files are rewritten. No cache needs to be cleared for the change to take effect.

The practical differences for a site architect:

  - Faster — changes apply immediately without rebuilding CSS files
  - Safer — nothing is permanently rewritten; the original theme files are untouched
  - More flexible — one token can drive many things at once (Opera's primary color controls
    the nav bar, footer, buttons, links, and borders all from a single setting)

## Status

Design Tokens is functional and actively used with the Opera theme as the reference
implementation. The core infrastructure, color sub-module, font sub-module, and utility
sub-module are all complete. It is not too early to test it, file bug reports, and provide
feedback.

## Who is this module for?

**Theme developers** who want to give site architects meaningful, well-organized customization
options without the Color module's limitations.

**Site architects** who want to configure a site's look and feel through the admin UI — colors,
fonts, button styles, link behavior — without custom CSS.

It is an infrastructure module. Themes declare what they support; the module handles the rest.

## How it works

1. A theme places a `tokens.inc` file in its root directory declaring the tokens it supports, and keeps their default values in a variables CSS file.
2. Design Tokens discovers the file, loads the token definitions and reads the defaults from the variables file.
3. Sub-modules register token types (e.g. `design_tokens_color`) and provide the field UI.
4. A site architect visits **Appearance > Design Tokens** and configures values with a live
   preview of the site alongside the form.
5. Values are stored in Backdrop config: `theme.tokens.THEMENAME.json`
6. On every page, Design Tokens injects a `<style>` block into `<head>`:

```html
<style>:root { --color-primary: #6e0e0a; --font-heading: 'Merriweather', serif; }</style>
```

The theme's CSS uses `var(--color-primary)` and similar references. No CSS files are
ever rewritten.

## Module structure

Design Tokens uses a parent + sub-module architecture. The parent provides shared
infrastructure. Sub-modules add support for specific token types. Sites enable only what
they need.

```
design_tokens/               Infrastructure: token loading, CSS injection, admin UI, config
  modules/
    design_tokens_color/     Color tokens — color pickers, contrast ratio badges, schemes
    design_tokens_font/      Font tokens — font family selectors, Google Fonts integration
    design_tokens_utility/   Utility CSS — generates classes like .dt-bg-primary for use
                             in the layout editor; includes admin reference page
```

A site wanting only color control enables `design_tokens` + `design_tokens_color`. A site
wanting color, fonts, and utility classes enables all four. Third-party developers can add
new token types as additional sub-modules without touching the parent.

## Writing a theme for Design Tokens

A Design Tokens theme has two files that work together:

- **A variables file** (for example `css/my-theme-variables.css`) with a `:root {}` block of CSS custom properties. This is where every default value lives. Without Design Tokens, the browser uses it directly, so the theme works on its own.
- **`tokens.inc`** in the theme's root folder. It says which of those variables Site Architects can change and how they appear on the settings page: label, type and group. It doesn't repeat the values.

When Design Tokens is active, it reads the defaults from the variables file, removes that file from the page and injects a `:root {}` block with the values saved on the settings page.

The theme's own CSS only ever uses the variables, for example `background-color: var(--color-primary);`.

### The variables file

```css
:root {
  --color-primary:   #6e0e0a;
  --color-card-bg:   color-mix(in srgb, var(--color-primary) 5%, white);
  --font-heading:    'Merriweather', Georgia, serif;
}
```

- Write each value once, here. Token names in `tokens.inc` are the variable names without the leading `--`.
- **Calculated values** such as `color-mix()`, `var()` or `calc()` become "Automatic" defaults: Design Tokens injects the calculation, so the value keeps following the tokens it comes from until someone chooses their own. A color field shows "Automatic" and offers "Use automatic" to go back.

### tokens.inc

```php
<?php
$info = array(
  // The file with the default values. Can also be an array of files.
  'variables_file' => 'css/my-theme-variables.css',

  'groups' => array(
    'header'     => t('Header & Footer'),
    'typography' => t('Typography'),
  ),

  'tokens' => array(
    'color-primary' => array(
      'label'   => t('Background'),
      'type'    => 'color',       // Provided by design_tokens_color
      'group'   => 'header',
      'utility' => TRUE,          // Generate a .dt-bg-primary utility class
    ),
    'color-card-bg' => array(
      'label'   => t('Card background'),
      'type'    => 'color',
      'group'   => 'header',
    ),
    'font-heading' => array(
      'label'   => t('Heading font'),
      'type'    => 'font',        // Provided by design_tokens_font
      'group'   => 'typography',
    ),
  ),

  'schemes' => array(
    // No 'tokens': uses the defaults from the variables file.
    'default' => array(
      'title' => t('Default'),
    ),
    'night' => array(
      'title'  => t('Night'),
      'tokens' => array(
        'color-primary' => '#1a1a2e',
      ),
    ),
  ),
);
```

`tokens.inc` is PHP, so repeated tokens (for example one color per front page block) can be written in a loop.

### Token keys

| Key | Required | Description |
|---|---|---|
| `label` | Recommended | Label shown on the settings page. Falls back to the token name, but Site Architects see it, so give a real one. |
| `type` | Yes | Token type (`color`, `font`, `text`). Unknown types show a plain text field. |
| `group` | No | Groups the token under a collapsible fieldset |
| `default` | No | Only needed for a token that isn't in the variables file. A value in the variables file wins over it. |
| `utility` | No | If `TRUE`, `design_tokens_utility` generates a utility CSS class for this token |
| `contrast_with` | No | For a background color: the token to check its contrast against, or `auto`. Tokens ending in `-text` are paired with their background automatically. |

The settings page warns about any token that ends up without a default.

### Where defaults come from

For each token, the first of these that has a value wins, highest first:

1. A default set by a `hook_design_tokens_info_alter()` implementation.
2. The variables file(s). With a subtheme, the subtheme's file overrides its base theme's.
3. `default` in `tokens.inc`.

### Saved values and new defaults

When a theme is enabled (or Design Tokens is installed), its defaults are saved as the theme's token values, the way a module's default configuration is installed. After that, a change to a default in the theme's files only reaches new sites, and existing sites keep their values. **Reset all to defaults** on the settings page saves the theme's current defaults again.

### Preset schemes

Schemes are named sets of token values. When a scheme is selected in the admin UI, all fields populate at once. The selected scheme name is saved to config and pre-selected on the next visit. If any token value is changed after a scheme is applied, the selector automatically shows "Custom", verified server-side on save.

A scheme without a `tokens` list uses the defaults of the tokens the theme's other schemes set, so a "default" scheme switches back exactly what the others change. If no scheme lists tokens, it uses the defaults of all color tokens.

### Utility classes

When `design_tokens_utility` is enabled, tokens with `'utility' => TRUE` generate CSS classes that site architects can apply to blocks in the layout editor's CSS class field:

- **Color tokens**: `.dt-bg-{slug}` sets background, text, and link color together
- **Font tokens**: `.dt-font-{slug}` (all text) and `.dt-title-{slug}` (block title only)

The reference page at **Appearance > Design Tokens > [Theme] > Utility Classes** shows all available classes with live color swatches.

### Subthemes

A subtheme of a theme with a `tokens.inc` needs no `tokens.inc` of its own: the module uses the nearest base theme's file. The subtheme still gets its own settings page and saved values at `admin/appearance/tokens/SUBTHEME`.

- **To change defaults**, give the subtheme its own variables file with only the values it changes, and list both files in `variables_file` (each file is looked up in the folder of the active theme and of every base theme, base themes first). Everything the subtheme doesn't set comes from the base theme.
- **To add tokens, groups or schemes**, implement `hook_design_tokens_info_alter()` in the subtheme's `template.php` and check that `$theme_name` is the subtheme. New tokens can take their defaults from the subtheme's variables file too.
- The settings page has a collapsed **"Theme developers: Where the defaults come from"** panel listing the tokens whose default comes from a base theme, to spot a value missing from the subtheme's file.

## Admin UI

| Path | Purpose |
|---|---|
| `admin/appearance/tokens` | Overview — all themes with token support |
| `admin/appearance/tokens/[theme]` | Configure tokens for a specific theme |
| `admin/appearance/tokens/[theme]/reset` | Confirm resetting all of a theme's tokens to their defaults |
| `admin/appearance/tokens/[theme]/utilities` | Utility class reference page |

The settings page shows tokens organized by group, a preset scheme selector, WCAG contrast
ratio badges on color pairs, and a live preview of the front page. Changes reflect in the
preview instantly via `postMessage` — no page reload required.

## Theme-specific admin preview JS

If a theme provides `js/design-tokens-theme.js`, Design Tokens automatically loads it on
the token settings admin page. This allows themes to extend the live preview with logic
that can't be handled by token injection alone — for example, computed CSS properties
derived from token values at PHP render time.

## Goals

- Config-based replacement for the Color module
- Support any token type through sub-modules
- Live preview with no server round-trips
- Exportable, deployable, version-controllable config
- Accessible by default — WCAG contrast checking built in

## Reference implementation

The [Opera theme](https://github.com/backdrop-contrib/opera) is the first theme to support
Design Tokens and serves as the reference implementation during development.

Notes About Use of AI
---------------------

This module was developed with significant assistance from AI tools (specifically Claude by
Anthropic). AI was used to generate code, plan features, and make iterative improvements
throughout development. We welcome feedback.

Pull requests and issue reports are welcome.

LICENSE
---------------

This project is GPL v2 software. See the LICENSE.txt file in this directory for complete text.

CURRENT MAINTAINERS
---------------

- Tim Erickson (https://github.com/stpaultim/)

CREDITS
---------------

Development supported by Simplo (by Triplo) - https://simplo.site
