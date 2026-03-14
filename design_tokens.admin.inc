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

  $form['#theme_name'] = $theme_name;

  // Link to theme settings for options not managed by Design Tokens.
  $settings_url = url('admin/appearance/settings/' . $theme_name);
  $form['settings_link'] = array(
    '#type' => 'markup',
    '#markup' => '<p class="design-tokens-nav-link">' . t('For other theme options, see <a href="!url">theme settings</a>.', array('!url' => $settings_url)) . '</p>',
    '#weight' => -250,
  );

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

  // Render grouped tokens as fieldsets. Groups are assigned sequential
  // negative weights so they render in tokens.inc declaration order. The first
  // group gets the most negative weight and is expanded; the rest are collapsed.
  // Groups can be either a simple string label or an array with 'label' and
  // optional 'description' keys.
  $first_group = TRUE;
  $group_weight = -10;
  foreach ($groups as $group_key => $group_def) {
    if (empty($grouped[$group_key])) {
      continue;
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

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save tokens'),
  );
  $form['actions']['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset to defaults'),
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
    '#weight' => -100,
  );

  $form['#attached']['js'][] = backdrop_get_path('module', 'design_tokens') . '/js/design-tokens-admin.js';
  $form['#attached']['css'][] = backdrop_get_path('module', 'design_tokens') . '/css/design-tokens-admin.css';

  // Auto-load a theme-specific admin JS file if the theme provides one.
  // Themes can create js/design-tokens-theme.js to customize preview behavior
  // for computed or derived CSS properties (e.g. auto-calculated text colors).
  $theme_js = backdrop_get_path('theme', $theme_name) . '/js/design-tokens-theme.js';
  if (file_exists(BACKDROP_ROOT . '/' . $theme_js)) {
    $form['#attached']['js'][] = $theme_js;
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
 * Form submit handler — resets tokens to defaults.
 */
function design_tokens_settings_form_reset($form, &$form_state) {
  $theme_name = $form['#theme_name'];
  design_tokens_save_values($theme_name, array());
  design_tokens_save_scheme($theme_name, '');
  backdrop_set_message(t('Token settings reset to defaults.'));
}
