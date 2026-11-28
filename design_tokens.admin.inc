<?php
/**
 * @file
 * Admin pages and forms for Design Tokens.
 */

/**
 * Overview page listing all themes that support Design Tokens.
 */
function design_tokens_overview_page() {
  $themes = design_tokens_get_supported_themes();

  if (empty($themes)) {
    return '<p>' . t('No themes currently support Design Tokens. Themes can add support by providing a <code>tokens.inc</code> file in their root directory.') . '</p>';
  }

  $rows = array();
  foreach ($themes as $name => $theme) {
    $rows[] = array(
      check_plain($theme->info['name']),
      l(t('Configure tokens'), 'admin/appearance/tokens/' . $name),
    );
  }

  return theme('table', array(
    'header' => array(t('Theme'), t('Actions')),
    'rows' => $rows,
    'empty' => t('No supported themes found.'),
  ));
}

/**
 * Settings form for a specific theme's tokens.
 *
 * @param array $form
 * @param array $form_state
 * @param string $theme_name
 *   The machine name of the theme.
 */
function design_tokens_settings_form($form, &$form_state, $theme_name) {
  $info = design_tokens_load_info($theme_name);
  if (empty($info)) {
    backdrop_set_message(t('Theme %theme does not support Design Tokens.', array('%theme' => $theme_name)), 'error');
    return $form;
  }

  $current_values = design_tokens_get_values($theme_name);
  $groups = isset($info['groups']) ? $info['groups'] : array();
  $tokens = isset($info['tokens']) ? $info['tokens'] : array();
  $schemes = isset($info['schemes']) ? $info['schemes'] : array();
  $types = design_tokens_get_types();

  // Tokens with no default from tokens.inc or the variables file have nothing
  // to reset to, and no value on a site that never saved one.
  $missing = array();
  foreach ($tokens as $token_name => $token) {
    if (!isset($token['default'])) {
      $missing[] = $token_name;
    }
  }
  if ($missing && empty($form_state['input'])) {
    backdrop_set_message(t('These tokens have no default value: %tokens. Add them to the theme\'s variables file, or give them a <code>default</code> in tokens.inc.', array('%tokens' => implode(', ', $missing))), 'warning');
  }

  $form['#theme_name'] = $theme_name;

  // Link to theme settings for options not managed by Design Tokens.
  $settings_url = url('admin/appearance/settings/' . $theme_name);
  $form['settings_link'] = array(
    '#type' => 'markup',
    '#markup' => '<p class="design-tokens-nav-link">' . t('For other theme options, see <a href="!url">theme settings</a>.', array('!url' => $settings_url)) . '</p>',
    '#weight' => -250,
  );

  // Sub-module status — shown if the theme declares submodules in tokens.inc.
  // Collapsed by default to save space; expand to see status and descriptions.
  if (!empty($info['submodules'])) {
    $rows = array();
    foreach ($info['submodules'] as $sub) {
      $enabled = module_exists($sub['machine']);
      $status = $enabled
        ? '<span style="color: green; font-weight: bold;">' . t('Enabled') . '</span>'
        : '<span style="color: #999;">' . t('Not enabled') . '</span>';
      $rows[] = array(
        check_plain($sub['name']),
        check_plain($sub['description']),
        array('data' => $status, 'style' => 'white-space: nowrap;'),
      );
    }
    $table = theme('table', array(
      'header' => array(t('Sub-module'), t('What it does'), t('Status')),
      'rows'   => $rows,
    ));
    $form['submodule_status'] = array(
      '#type'   => 'markup',
      '#markup' => '<details style="margin-bottom: 1em;">'
        . '<summary style="cursor: pointer;">' . t('Available sub-modules') . '</summary>'
        . '<p style="margin-top: 0.5em;">' . t('Enable or disable sub-modules at <a href="!url">Functionality &rsaquo; Modules</a>.', array('!url' => url('admin/modules'))) . '</p>'
        . $table
        . '</details>',
      '#weight' => -225,
    );
  }

  // Collapse/expand all toggle button.
  $form['collapse_toggle'] = array(
    '#type' => 'markup',
    '#markup' => '<button type="button" id="design-tokens-collapse-all" class="button">' . t('Collapse all') . '</button>',
    '#weight' => -200,
  );

  // Preset scheme selector.
  if (!empty($schemes)) {
    $scheme_options = array('' => t('— Custom —'));
    foreach ($schemes as $scheme_key => $scheme) {
      $scheme_options[$scheme_key] = check_plain($scheme['title']);
    }

    $form['scheme'] = array(
      '#type' => 'select',
      '#title' => t('Preset scheme'),
      '#description' => t('Choose a preset to load a curated set of colors. Selecting a preset overwrites the color fields below. <em>— Custom —</em> appears when you have modified any colors individually.'),
      '#options' => $scheme_options,
      '#default_value' => design_tokens_get_scheme($theme_name),
      '#weight' => -150,
      '#attributes' => array('id' => 'design-tokens-scheme-select'),
      '#attached' => array(
        'js' => array(
          array(
            'type' => 'setting',
            'data' => array(
              'designTokens' => array(
                'schemes' => $schemes,
              ),
            ),
          ),
        ),
      ),
    );
  }

  // Build a defaults map keyed by token name for use by the JS reset buttons.
  $defaults = array();
  foreach ($tokens as $token_name => $token) {
    if (isset($token['default'])) {
      $defaults[$token_name] = $token['default'];
    }
  }
  $form['#attached']['js'][] = array(
    'type' => 'setting',
    'data' => array(
      'designTokens' => array(
        'defaults' => $defaults,
      ),
    ),
  );

  // Sort tokens into groups.
  $ungrouped = array();
  $grouped = array();

  foreach ($tokens as $token_name => $token) {
    // Skip tokens whose type is not provided by any enabled sub-module.
    $type = isset($token['type']) ? $token['type'] : '';
    if ($type && !isset($types[$type])) {
      continue;
    }

    $group = isset($token['group']) ? $token['group'] : '';
    if ($group && isset($groups[$group])) {
      $grouped[$group][$token_name] = $token;
    }
    else {
      $ungrouped[$token_name] = $token;
    }
  }

  // Build a map of group_key => section label if the theme declares sections.
  $section_map = array();
  if (!empty($info['sections'])) {
    foreach ($info['sections'] as $section) {
      foreach ($section['groups'] as $group_key) {
        $section_map[$group_key] = $section['label'];
      }
    }
  }

  // Draw groups section by section, in the order the sections are declared,
  // so a group added to a section (for example by a subtheme's alter hook,
  // which appends it to the end of the groups list) appears inside that
  // section instead of starting the section a second time. Groups that are
  // in no section follow, in their own order.
  if (!empty($info['sections'])) {
    $ordered_groups = array();
    foreach ($info['sections'] as $section) {
      foreach ($section['groups'] as $group_key) {
        if (isset($groups[$group_key])) {
          $ordered_groups[$group_key] = $groups[$group_key];
        }
      }
    }
    $groups = $ordered_groups + $groups;
  }

  // Render grouped tokens as fieldsets. Groups are assigned sequential
  // negative weights so they render in tokens.inc declaration order. The first
  // group gets the most negative weight and is expanded; the rest are collapsed.
  // Groups can be either a simple string label or an array with 'label' and
  // optional 'description' keys.
  $first_group = TRUE;
  $group_weight = -10;
  $current_section = NULL;
  foreach ($groups as $group_key => $group_def) {
    if (empty($grouped[$group_key])) {
      continue;
    }

    // Insert a section header when entering a new section.
    if (!empty($section_map[$group_key]) && $section_map[$group_key] !== $current_section) {
      $current_section = $section_map[$group_key];
      $header_key = 'section_header_' . preg_replace('/[^a-z0-9]+/', '_', strtolower($current_section));
      $form[$header_key] = array(
        '#type'   => 'markup',
        '#markup' => '<h3 class="design-tokens-section-header">' . check_plain($current_section) . '</h3>',
        '#weight' => $group_weight++,
      );
    }

    // Support both simple string labels and arrays with label + description.
    if (is_array($group_def)) {
      $label = $group_def['label'];
      $description = isset($group_def['description']) ? $group_def['description'] : '';
    }
    else {
      $label = $group_def;
      $description = '';
    }

    $form[$group_key] = array(
      '#type' => 'fieldset',
      '#title' => $label,
      '#collapsible' => TRUE,
      '#collapsed' => !$first_group,
      '#weight' => $group_weight++,
    );
    if ($description) {
      $form[$group_key]['_description'] = array(
        '#type' => 'markup',
        '#markup' => '<p class="design-tokens-group-description">' . $description . '</p>',
        '#weight' => -100,
      );
    }

    $form[$group_key]['_reset'] = array(
      '#type' => 'markup',
      '#markup' => '<button type="button" class="design-tokens-reset-section button button-secondary" data-group="' . check_plain($group_key) . '">' . t('Reset to defaults') . '</button>',
      '#weight' => 100,
    );

    $first_group = FALSE;

    foreach ($grouped[$group_key] as $token_name => $token) {
      $form[$group_key][$token_name] = design_tokens_build_field(
        $token_name, $token, $current_values, $types
      );
    }
  }

  // Render ungrouped tokens.
  if (!empty($ungrouped)) {
    $form['ungrouped'] = array(
      '#type' => 'fieldset',
      '#title' => t('Other'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    foreach ($ungrouped as $token_name => $token) {
      $form['ungrouped'][$token_name] = design_tokens_build_field(
        $token_name, $token, $current_values, $types
      );
    }
  }

  // For theme developers: list the tokens whose default comes from another
  // theme (a base theme), so a value missing from a subtheme's variables file
  // can be spotted. Collapsed, and left out when there is nothing to list.
  $themes = list_themes();
  $theme_label = isset($themes[$theme_name]->info['name']) ? $themes[$theme_name]->info['name'] : $theme_name;
  $inherited_rows = array();
  $source_themes = array();
  $example = '';
  foreach ($tokens as $token_name => $token) {
    if (empty($token['default_source']) || $token['default_source']['theme'] === $theme_name) {
      continue;
    }
    $source = $token['default_source'];
    $source_label = isset($themes[$source['theme']]->info['name']) ? $themes[$source['theme']]->info['name'] : $source['theme'];
    $source_themes[$source_label] = $source_label;
    if (!$example) {
      $example = '--' . $token_name . ': ' . $token['default'] . ';';
    }
    // Many labels only make sense inside their group ("Background"), so
    // show the group too.
    $token_label = isset($token['label']) ? $token['label'] : $token_name;
    if (!empty($token['group']) && isset($groups[$token['group']])) {
      $group_def = $groups[$token['group']];
      $token_label = (is_array($group_def) ? $group_def['label'] : $group_def) . ': ' . $token_label;
    }
    $inherited_rows[] = array(
      check_plain($token_label) . '<br><code>--' . check_plain($token_name) . '</code>',
      '<code>' . check_plain($token['default']) . '</code>',
      t('@theme, <code>@file</code>', array('@theme' => $source_label, '@file' => $source['file'])),
    );
  }
  if ($inherited_rows) {
    $variables_files = !empty($info['variables_file']) ? (array) $info['variables_file'] : array();
    $own_file = $variables_files ? end($variables_files) : 'css/variables.css';
    $form['default_sources'] = array(
      '#type' => 'markup',
      '#markup' => '<details class="design-tokens-default-sources">'
        . '<summary style="cursor: pointer;">' . t('Theme developers: Where the defaults come from') . '</summary>'
        . '<p style="margin-top: 0.5em;">' . t('Site Architects can change colors and fonts without this section.') . '</p>'
        . '<p>' . t('%theme is built on %base. A theme like this only needs to set the values it wants to change; everything else is taken from the theme it is built on. The tokens below take their default value from %base, not from %theme.', array('%theme' => $theme_label, '%base' => implode(', ', $source_themes))) . '</p>'
        . '<p>' . t('That is usually intended. If one of them should have its own value in %theme, add it to %theme\'s variables file (<code>@file</code>) with the value you want, for example <code>@example</code>. Then clear the caches.', array('%theme' => $theme_label, '@file' => $own_file, '@example' => $example)) . '</p>'
        . '<p>' . t('These are only the defaults: they are used by <em>Reset to defaults</em> and when the theme is first enabled. Values already saved on this page are not affected.') . '</p>'
        . theme('table', array(
          'header' => array(t('Token'), t('Default'), t('Comes from')),
          'rows' => $inherited_rows,
        ))
        . '</details>',
      // Just above the buttons, which core weights at 100.
      '#weight' => 99,
    );
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save tokens'),
  );
  $form['actions']['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset all to defaults'),
    '#submit' => array('design_tokens_settings_form_reset'),
    '#limit_validation_errors' => array(),
  );

  // Live preview — fixed panel toggled by a button, independent of admin layout.
  $preview_url = url('<front>', array('absolute' => TRUE));
  $form['preview'] = array(
    '#type' => 'markup',
    '#markup' => '<button type="button" id="design-tokens-preview-toggle" class="button">' . t('Show preview') . '</button>'
      . '<div id="design-tokens-preview-panel" hidden>'
      . '<div class="design-tokens-preview-header">'
      . '<span>' . t('Live preview') . '</span>'
      . '<button type="button" id="design-tokens-preview-close">&times;</button>'
      . '</div>'
      . '<iframe id="design-tokens-preview-iframe" src="' . check_plain($preview_url) . '" title="' . t('Live preview') . '"></iframe>'
      . '</div>',
    '#weight' => -199,
  );

  $form['#attached']['js'][] = backdrop_get_path('module', 'design_tokens') . '/js/design-tokens-admin.js';
  $form['#attached']['css'][] = backdrop_get_path('module', 'design_tokens') . '/css/design-tokens-admin.css';

  // Auto-load a theme-specific admin JS file if the theme provides one.
  // Themes can create js/design-tokens-theme.js to customize preview behavior
  // for computed or derived CSS properties (e.g. auto-calculated text colors).
  // Base themes' files load first, so a subtheme can add to them.
  foreach (design_tokens_get_theme_chain($theme_name) as $chain_theme) {
    $theme_js = backdrop_get_path('theme', $chain_theme) . '/js/design-tokens-theme.js';
    if (file_exists(BACKDROP_ROOT . '/' . $theme_js)) {
      $form['#attached']['js'][] = $theme_js;
    }
  }

  return $form;
}

/**
 * Builds a form field for a single token by delegating to the type's callback.
 *
 * @param string $token_name
 * @param array $token
 *   Token definition from tokens.inc.
 * @param array $current_values
 *   Current saved/default values keyed by token name.
 * @param array $types
 *   Registered token types from design_tokens_get_types().
 *
 * @return array
 *   A form element.
 */
function design_tokens_build_field($token_name, array $token, array $current_values, array $types) {
  $type = isset($token['type']) ? $token['type'] : '';

  // Delegate to the sub-module's field callback if available.
  if ($type && isset($types[$type]['field callback'])) {
    $callback = $types[$type]['field callback'];
    if (function_exists($callback)) {
      return $callback($token_name, $token, $current_values);
    }
  }

  // Fallback: plain text field for unknown/unhandled types.
  return array(
    '#type' => 'textfield',
    '#title' => isset($token['label']) ? $token['label'] : $token_name,
    '#default_value' => isset($current_values[$token_name]) ? $current_values[$token_name] : '',
    '#attributes' => array('data-token-name' => $token_name),
  );
}

/**
 * Form submit handler — saves token values.
 */
function design_tokens_settings_form_submit($form, &$form_state) {
  $theme_name = $form['#theme_name'];
  $info = design_tokens_load_info($theme_name);
  $tokens = isset($info['tokens']) ? $info['tokens'] : array();
  $schemes = isset($info['schemes']) ? $info['schemes'] : array();

  $values = array();
  foreach ($tokens as $token_name => $token) {
    $group = isset($token['group']) ? $token['group'] : 'ungrouped';
    if (isset($form_state['values'][$token_name])) {
      $values[$token_name] = $form_state['values'][$token_name];
    }
    elseif (isset($form_state['values'][$group][$token_name])) {
      $values[$token_name] = $form_state['values'][$group][$token_name];
    }
    // An empty field for a calculated token means "Automatic": save the
    // calculation, so the value keeps following the tokens it comes from.
    if (!empty($token['calculated']) && isset($values[$token_name]) && trim($values[$token_name]) === '') {
      $values[$token_name] = $token['default'];
    }
  }

  design_tokens_save_values($theme_name, $values);

  // Determine the active scheme to save. Start with the selector value, then
  // verify by comparing submitted token values against the scheme definition.
  // If any token differs from the scheme, the values are Custom — even if the
  // JS did not reset the selector (e.g. due to a page quirk or manual typing).
  $scheme = isset($form_state['values']['scheme']) ? $form_state['values']['scheme'] : '';
  if ($scheme && isset($schemes[$scheme]['tokens'])) {
    foreach ($schemes[$scheme]['tokens'] as $token_name => $scheme_value) {
      if (isset($values[$token_name]) && $values[$token_name] !== $scheme_value) {
        $scheme = '';
        break;
      }
    }
  }
  design_tokens_save_scheme($theme_name, $scheme);

  backdrop_set_message(t('Token settings saved.'));
}

/**
 * Form submit handler for "Reset all to defaults": asks for confirmation.
 */
function design_tokens_settings_form_reset($form, &$form_state) {
  $form_state['redirect'] = 'admin/appearance/tokens/' . $form['#theme_name'] . '/reset';
}

/**
 * Form constructor: confirms resetting all of a theme's tokens.
 *
 * @param string $theme_name
 *   The machine name of the theme.
 *
 * @see design_tokens_reset_confirm_form_submit()
 */
function design_tokens_reset_confirm_form($form, &$form_state, $theme_name) {
  $themes = list_themes();
  $theme_label = isset($themes[$theme_name]->info['name']) ? $themes[$theme_name]->info['name'] : $theme_name;
  $form['#theme_name'] = $theme_name;

  return confirm_form($form,
    t('Reset all %theme tokens to their defaults?', array('%theme' => $theme_label)),
    'admin/appearance/tokens/' . $theme_name,
    t('Every color, font and other value on the %theme Design Tokens page will be replaced with the theme\'s default. Changes made on that page will be lost. This cannot be undone.', array('%theme' => $theme_label)),
    t('Reset all to defaults'),
    t('Cancel')
  );
}

/**
 * Form submit handler for design_tokens_reset_confirm_form().
 *
 * Saves the theme's current defaults as the token values, instead of deleting
 * the saved values. With no saved values, the site would follow the theme's
 * defaults live, so a later theme update that changes a default would change
 * this site too. Backdrop's rule is that new defaults only reach new sites.
 */
function design_tokens_reset_confirm_form_submit($form, &$form_state) {
  $theme_name = $form['#theme_name'];
  $info = design_tokens_load_info($theme_name);
  $defaults = array();
  if (!empty($info['tokens'])) {
    foreach ($info['tokens'] as $token_name => $token) {
      if (isset($token['default'])) {
        $defaults[$token_name] = $token['default'];
      }
    }
  }
  design_tokens_save_values($theme_name, $defaults);
  design_tokens_save_scheme($theme_name, '');
  backdrop_set_message(t('All tokens were reset to their defaults.'));
  $form_state['redirect'] = 'admin/appearance/tokens/' . $theme_name;
}
