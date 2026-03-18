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

1. A theme places a `tokens.inc` file in its root directory declaring the tokens it supports.
2. Design Tokens discovers the file and loads the token definitions.
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

## Adding Design Tokens support to a theme

Place a `tokens.inc` file in your theme's root directory. The module detects it automatically.

```php
<?php
$info = array(
  // Static CSS file to remove when Design Tokens is active (prevents duplicate variables).
  'variables_file' => 'css/my-theme-variables.css',

  'groups' => array(
    'header'     => t('Header & Footer'),
    'typography' => t('Typography'),
  ),

  'tokens' => array(
    'color-primary' => array(
      'label'   => t('Background'),
      'type'    => 'color',       // Provided by design_tokens_color
      'default' => '#6e0e0a',
      'group'   => 'header',
      'utility' => TRUE,          // Generate a .dt-bg-primary utility class
    ),
    'font-heading' => array(
      'label'   => t('Heading font'),
      'type'    => 'font',        // Provided by design_tokens_font
      'default' => "'Merriweather', Georgia, serif",
      'group'   => 'typography',
    ),
  ),

  'schemes' => array(
    'default' => array(
      'title'  => t('Default'),
      'tokens' => array(
        'color-primary' => '#6e0e0a',
      ),
    ),
  ),
);
```

### Token keys

| Key | Required | Description |
|---|---|---|
| `label` | Yes | Human-readable label shown in the admin UI |
| `type` | Yes | Token type (`color`, `font`, `text`). Unknown types show a plain text field. |
| `default` | Yes | Default CSS value used before any config is saved |
| `group` | No | Groups the token under a collapsible fieldset |
| `utility` | No | If `TRUE`, `design_tokens_utility` generates a utility CSS class for this token |

### Preset schemes

Schemes are named sets of token values. When a scheme is selected in the admin UI, all
fields populate at once. The selected scheme name is saved to config and pre-selected on
the next visit. If any token value is changed after a scheme is applied, the selector
automatically shows "Custom" — verified server-side on save.

### Utility classes

When `design_tokens_utility` is enabled, tokens with `'utility' => TRUE` generate CSS
classes that site architects can apply to blocks in the layout editor's CSS class field:

- **Color tokens** — `.dt-bg-{slug}` sets background, text, and link color together
- **Font tokens** — `.dt-font-{slug}` (all text) and `.dt-title-{slug}` (block title only)

The reference page at **Appearance > Design Tokens > [Theme] > Utility Classes** shows all
available classes with live color swatches.

## Admin UI

| Path | Purpose |
|---|---|
| `admin/appearance/tokens` | Overview — all themes with token support |
| `admin/appearance/tokens/[theme]` | Configure tokens for a specific theme |
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
