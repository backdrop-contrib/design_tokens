<?php
/**
 * @file
 * API documentation for Design Tokens hooks.
 */

/**
 * Registers one or more token types with the Design Tokens system.
 *
 * Sub-modules implement this hook to add support for a token type. The parent
 * module calls module_invoke_all('design_tokens_types') when building the admin
 * form and delegates field rendering to the registered callback.
 *
 * Token types that are not registered (because the sub-module is disabled) are
 * silently skipped in the admin UI — their values are not lost, just not shown.
 *
 * @return array
 *   Keyed by type machine name. Each value is an array with keys:
 *   - 'label' (string): Human-readable type name.
 *   - 'field callback' (string): Name of a function that builds the form
 *     element for a single token of this type. The function receives:
 *     - $token_name (string): The token machine name.
 *     - $token (array): The token definition from tokens.inc.
 *     - $current_values (array): Current saved/default values keyed by name.
 *     It must return a Form API element array.
 *
 * @see design_tokens_get_types()
 * @see design_tokens_build_field()
 */
function hook_design_tokens_types() {
  return array(
    'color' => array(
      'label'          => t('Color'),
      'field callback' => 'mymodule_design_tokens_build_color_field',
    ),
  );
}

/**
 * Alters token definitions after they are loaded from a theme's tokens.inc.
 *
 * @param array $info
 *   The token info array, passed by reference. Keys:
 *   - 'tokens': array of token definitions keyed by token name.
 *   - 'groups': array of group labels keyed by group machine name.
 *   - 'schemes': array of preset scheme definitions.
 * @param string $theme_name
 *   The machine name of the theme whose tokens are being loaded.
 *
 * @see design_tokens_load_info()
 */
function hook_design_tokens_info_alter(array &$info, $theme_name) {
  // Example: add a token to a specific theme programmatically.
  if ($theme_name === 'opera') {
    $info['tokens']['color-custom'] = array(
      'label'   => t('Custom Color'),
      'type'    => 'color',
      'default' => '#ff0000',
      'group'   => 'header',
    );
  }
}
