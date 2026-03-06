<?php
/**
 * @file
 * Admin pages and forms for Theme Tokens.
 */

/**
 * Overview page listing all themes that support Theme Tokens.
 */
function theme_tokens_overview_page() {
  $themes = theme_tokens_get_supported_themes();

  if (empty($themes)) {
    return '<p>' . t('No themes currently support Theme Tokens. Themes can add support by providing a <code>tokens.inc</code> file in their root directory.') . '</p>';
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
function theme_tokens_settings_form($form, &$form_state, $theme_name) {
  $info = theme_tokens_load_info($theme_name);
  if (empty($info)) {
    backdrop_set_message(t('Theme %theme does not support Theme Tokens.', array('%theme' => $theme_name)), 'error');
    return $form;
  }

  $current_values = theme_tokens_get_values($theme_name);
  $groups = isset($info['groups']) ? $info['groups'] : array();
  $tokens = isset($info['tokens']) ? $info['tokens'] : array();
  $schemes = isset($info['schemes']) ? $info['schemes'] : array();
  $types = theme_tokens_get_types();

  $form['#theme_name'] = $theme_name;

  // Preset scheme selector.
  if (!empty($schemes)) {
    $scheme_options = array('' => t('— Custom —'));
    foreach ($schemes as $scheme_key => $scheme) {
      $scheme_options[$scheme_key] = check_plain($scheme['title']);
    }

    $form['scheme'] = array(
      '#type' => 'select',
      '#title' => t('Preset scheme'),
      '#options' => $scheme_options,
      '#default_value' => '',
      '#attributes' => array('id' => 'theme-tokens-scheme-select'),
      '#attached' => array(
        'js' => array(
          array(
            'type' => 'setting',
            'data' => array(
              'themeTokens' => array(
                'schemes' => $schemes,
              ),
            ),
          ),
        ),
      ),
    );
  }

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

  // Render grouped tokens as fieldsets.
  foreach ($groups as $group_key => $group_label) {
    if (empty($grouped[$group_key])) {
      continue;
    }

    $form[$group_key] = array(
      '#type' => 'fieldset',
      '#title' => $group_label,
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );

    foreach ($grouped[$group_key] as $token_name => $token) {
      $form[$group_key][$token_name] = theme_tokens_build_field(
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
      '#collapsed' => FALSE,
    );
    foreach ($ungrouped as $token_name => $token) {
      $form['ungrouped'][$token_name] = theme_tokens_build_field(
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
    '#submit' => array('theme_tokens_settings_form_reset'),
    '#limit_validation_errors' => array(),
  );

  // Live preview — fixed panel toggled by a button, independent of admin layout.
  $preview_url = url('<front>', array('absolute' => TRUE));
  $form['preview'] = array(
    '#type' => 'markup',
    '#markup' => '<button type="button" id="theme-tokens-preview-toggle" class="button">' . t('Show preview') . '</button>'
      . '<div id="theme-tokens-preview-panel" hidden>'
      . '<div class="theme-tokens-preview-header">'
      . '<span>' . t('Live preview') . '</span>'
      . '<button type="button" id="theme-tokens-preview-close">&times;</button>'
      . '</div>'
      . '<iframe id="theme-tokens-preview-iframe" src="' . check_plain($preview_url) . '" title="' . t('Live preview') . '"></iframe>'
      . '</div>',
    '#weight' => -100,
  );

  $form['#attached']['js'][] = backdrop_get_path('module', 'theme_tokens') . '/js/theme-tokens-admin.js';
  $form['#attached']['css'][] = backdrop_get_path('module', 'theme_tokens') . '/css/theme-tokens-admin.css';

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
 *   Registered token types from theme_tokens_get_types().
 *
 * @return array
 *   A form element.
 */
function theme_tokens_build_field($token_name, array $token, array $current_values, array $types) {
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
function theme_tokens_settings_form_submit($form, &$form_state) {
  $theme_name = $form['#theme_name'];
  $info = theme_tokens_load_info($theme_name);
  $tokens = isset($info['tokens']) ? $info['tokens'] : array();

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

  theme_tokens_save_values($theme_name, $values);
  backdrop_set_message(t('Token settings saved.'));
}

/**
 * Form submit handler — resets tokens to defaults.
 */
function theme_tokens_settings_form_reset($form, &$form_state) {
  $theme_name = $form['#theme_name'];
  theme_tokens_save_values($theme_name, array());
  backdrop_set_message(t('Token settings reset to defaults.'));
}
