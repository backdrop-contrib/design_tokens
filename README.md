# Design Tokens

Design Tokens is a Backdrop CMS module that provides a modern, flexible approach to theme
configuration using CSS custom properties (CSS variables). It lets themes declare the design
tokens they support — colors, fonts, spacing, and more — and gives site administrators a
clean UI to configure those values without writing any CSS.

This module is intended as a replacement for the Color module, which predates CSS custom
properties and works by doing fragile string replacement on CSS files. Design Tokens stores
values in Backdrop config, injects a single `:root {}` block in `<head>`, and never rewrites
any files.

# Status

Design Tokens is under active development. The core infrastructure (token discovery, config
storage, CSS injection, admin UI, live preview) is in place. The color sub-module is
functional. It is not too early to test it, file bug reports, and provide feedback.

# Who is this module for?

Design Tokens is for theme developers who want to give their users meaningful, well-organized
customization options that go beyond the Color module — and for site administrators who want
to configure a site's look and feel through the UI without custom CSS.

It is designed as an infrastructure module. Themes declare what they support; the module
handles the rest.

# How it works

1. A theme places a `tokens.inc` file in its root directory declaring the tokens it supports.
2. Design Tokens discovers the file and loads the token definitions.
3. Sub-modules register token types (e.g. `design_tokens_color`) and provide the field UI
   for editing them.
4. A site administrator visits **Appearance > Design Tokens** and configures values through
   the admin UI, with a live preview of the site alongside the form.
5. Values are stored in Backdrop config: `theme.tokens.THEMENAME.json`
6. On every page, Design Tokens injects a `<style>` block into `<head>`:

```html
<style>:root { --color-primary: #6e0e0a; --font-heading: Merriweather, serif; }</style>
```

The theme's CSS uses `var(--color-primary)` and friends. No CSS files are ever rewritten.

# Module structure

Design Tokens uses a parent + sub-module architecture. The parent module provides shared
infrastructure. Sub-modules add support for specific token types. Sites enable only what
they need.

```
design_tokens/               Infrastructure: token loading, CSS injection, admin UI, config
  modules/
    design_tokens_color/     Color token type — color pickers, preset scheme selection
    design_tokens_font/      Font token type — font selectors, stacks (planned)
```

A site that wants only color control enables `design_tokens` and `design_tokens_color`. A site
that wants both color and font control enables all three. Third-party developers can add new
token types as additional sub-modules without touching the parent.

# Adding Design Tokens support to a theme

Place a `tokens.inc` file in your theme's root directory. The module detects it automatically.

```php
<?php
$info = array(
  'variables_file' => 'css/my-theme-variables.css',

  'groups' => array(
    'header'     => t('Header & Footer'),
    'typography' => t('Typography'),
  ),

  'tokens' => array(
    'color-primary' => array(
      'label'   => t('Primary background'),
      'type'    => 'color',
      'default' => '#6e0e0a',
      'group'   => 'header',
    ),
    'font-heading' => array(
      'label'   => t('Heading font'),
      'type'    => 'font',
      'default' => 'Merriweather, Georgia, serif',
      'group'   => 'typography',
    ),
  ),

  'schemes' => array(
    'default' => array(
      'title'  => t('Default'),
      'tokens' => array(
        'color-primary' => '#6e0e0a',
        'font-heading'  => 'Merriweather, Georgia, serif',
      ),
    ),
  ),
);
```

The `variables_file` key tells Design Tokens which static CSS file to remove from the page
and replace with the injected `:root {}` block. This prevents duplicate variable definitions.

Token `type` values must match a type registered by an enabled sub-module. If the sub-module
for a type is not enabled, that token is silently skipped — the form simply won't show a
field for it, and its default value is still injected.

# Admin UI

- **Appearance > Design Tokens** — overview of all themes with token support
- **Appearance > Design Tokens > [Theme name]** — configure tokens for a specific theme

The settings page shows tokens organized by group, a preset scheme selector at the top, and
a live preview of the front page alongside the form. Changes are reflected in the preview
in real time without a page reload.

# Goals

- Provide a clean, config-based replacement for the Color module
- Support any token type (colors, fonts, spacing, border radius, etc.) through sub-modules
- Live preview with no server round-trips, using `postMessage`
- Exportable, deployable, version-controllable config
- Opera is the reference implementation and primary test case

# Reference implementation

The [Opera theme](https://github.com/backdrop-contrib/opera) is the first theme to support
Design Tokens and serves as the reference implementation during development.

Notes About Use of AI
---------------------

This module was developed with significant assistance from AI tools (specifically Claude by Anthropic). AI was used to generate code, plan features, and make iterative improvements throughout development. We welcome feedback.

Pull requests and issue reports are welcome.

LICENSE
---------------

This project is GPL v2 software. See the LICENSE.txt file in this directory
for complete text.

CURRENT MAINTAINERS
---------------

- Tim Erickson (https://github.com/stpaultim/)

CREDITS
---------------

Development supported by Simplo (by Triplo) - https://simplo.site
