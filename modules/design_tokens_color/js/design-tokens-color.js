(function ($) {

  'use strict';

  // --- WCAG contrast ratio helpers ---

  /**
   * Converts a 6-digit hex color string to relative luminance (WCAG 2.1).
   *
   * @param {string} hex - e.g. '#6e0e0a'
   * @return {number} Relative luminance between 0 and 1.
   */
  function hexToLuminance(hex) {
    return [1, 3, 5]
      .map(function (i) { return parseInt(hex.slice(i, i + 2), 16) / 255; })
      .map(function (c) { return c <= 0.03928 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4); })
      .reduce(function (sum, c, i) { return sum + c * [0.2126, 0.7152, 0.0722][i]; }, 0);
  }

  /**
   * Returns the WCAG contrast ratio between two hex colors.
   *
   * @param {string} hex1
   * @param {string} hex2
   * @return {number} Contrast ratio, e.g. 4.5.
   */
  function contrastRatio(hex1, hex2) {
    var l1 = hexToLuminance(hex1);
    var l2 = hexToLuminance(hex2);
    return (Math.max(l1, l2) + 0.05) / (Math.min(l1, l2) + 0.05);
  }

  /**
   * Scans all contrast badges and updates the indicator dot on each group
   * fieldset legend. Red = any Fail; orange = AA Large only; none = all pass.
   *
   * Called after every badge update so the dots stay in sync with live edits.
   */
  function updateGroupIndicators() {
    $('fieldset.collapsible').each(function () {
      var $fieldset = $(this);
      var $badges = $fieldset.find('.dt-contrast-badge');
      var hasFail  = $badges.hasClass('dt-contrast--fail');
      var hasLarge = $badges.hasClass('dt-contrast--large');

      var $legend = $fieldset.find('> legend .fieldset-legend').first();
      var $dot = $legend.find('.dt-group-indicator');

      if (hasFail || hasLarge) {
        if (!$dot.length) {
          $dot = $('<span>').addClass('dt-group-indicator');
          // Insert immediately after the fieldset title link so the dot sits
          // right beside the label text. Fall back to appending if collapse.js
          // hasn't run yet and the <a> doesn't exist.
          var $titleLink = $legend.find('a.fieldset-title');
          if ($titleLink.length) {
            $titleLink.after($dot);
          }
          else {
            $legend.append($dot);
          }
        }
        if (hasFail) {
          $dot
            .removeClass('dt-group-indicator--large')
            .addClass('dt-group-indicator--fail')
            .attr('title', 'One or more color pairs in this group fail WCAG AA contrast.');
        }
        else {
          $dot
            .removeClass('dt-group-indicator--fail')
            .addClass('dt-group-indicator--large')
            .attr('title', 'One or more color pairs in this group pass only for large text (AA Large).');
        }
      }
      else {
        $dot.remove();
      }
    });
  }

  /**
   * Applies a contrast rating level and text to a badge element.
   *
   * @param {jQuery} $badge
   * @param {number} ratio
   */
  function applyContrastBadge($badge, ratio) {
    var ratioText = ratio.toFixed(1) + ':1';
    var level, modifier, title;

    if (ratio >= 7) {
      level = 'AAA';
      modifier = 'dt-contrast--aaa';
      title = 'Excellent contrast (' + ratioText + '). Passes WCAG AAA — suitable for all text.';
    }
    else if (ratio >= 4.5) {
      level = 'AA';
      modifier = 'dt-contrast--aa';
      title = 'Good contrast (' + ratioText + '). Passes WCAG AA — suitable for normal body text.';
    }
    else if (ratio >= 3) {
      level = 'AA Large';
      modifier = 'dt-contrast--large';
      title = 'Marginal contrast (' + ratioText + '). Passes only for large text (18pt+) or bold text (14pt+). Not suitable for body text.';
    }
    else {
      level = 'Fail';
      modifier = 'dt-contrast--fail';
      title = 'Insufficient contrast (' + ratioText + '). Fails all WCAG thresholds — this color combination should be changed.';
    }

    $badge
      .text(ratioText + ' ' + level)
      .attr('title', title)
      .removeClass('dt-contrast--aaa dt-contrast--aa dt-contrast--large dt-contrast--fail')
      .addClass(modifier);

    updateGroupIndicators();
  }

  /**
   * Updates the contrast badge on a background-only color field.
   *
   * Used for tokens with a 'contrast_with' hint (e.g. color-card-bg,
   * color-block-N). The value 'auto' picks whichever of color-text-default
   * or color-text-inverted has better contrast with the background — matching
   * what the theme actually renders. A named token looks up that specific field.
   *
   * @param {jQuery} $bgField - The background color field.
   */
  function updateBgContrastBadge($bgField) {
    var $badge = $bgField.siblings('.dt-contrast-badge');
    if (!$badge.length) {
      return;
    }

    var bgHex = $bgField.val().toLowerCase();
    if (!/^#[0-9a-f]{6}$/.test(bgHex)) {
      $badge.text('').removeClass('dt-contrast--aaa dt-contrast--aa dt-contrast--large dt-contrast--fail');
      return;
    }

    var contrastWith = $bgField.data('contrast-with');
    var textHex;

    if (contrastWith === 'auto') {
      // Use whichever auto-text token has better contrast — this matches the
      // theme's luminance-based auto-selection logic.
      var $defaultField = $('[data-token-name="color-text-default"]');
      var $invertedField = $('[data-token-name="color-text-inverted"]');
      if (!$defaultField.length || !$invertedField.length) {
        return;
      }
      var defaultHex = $defaultField.val().toLowerCase();
      var invertedHex = $invertedField.val().toLowerCase();
      if (!/^#[0-9a-f]{6}$/.test(defaultHex) || !/^#[0-9a-f]{6}$/.test(invertedHex)) {
        return;
      }
      var ratioDefault = contrastRatio(bgHex, defaultHex);
      var ratioInverted = contrastRatio(bgHex, invertedHex);
      textHex = ratioDefault >= ratioInverted ? defaultHex : invertedHex;
    }
    else {
      var $namedField = $('[data-token-name="' + contrastWith + '"]');
      if (!$namedField.length) {
        return;
      }
      textHex = $namedField.val().toLowerCase();
      if (!/^#[0-9a-f]{6}$/.test(textHex)) {
        return;
      }
    }

    applyContrastBadge($badge, contrastRatio(bgHex, textHex));
  }

  /**
   * Updates the contrast badge next to a text-type color field.
   *
   * Looks up the companion background field by stripping the '-text' suffix
   * from the token name, calculates the contrast ratio between the two current
   * values, and updates the badge's text and CSS modifier class.
   *
   * @param {jQuery} $textField - The color field for the text token.
   */
  function updateContrastBadge($textField) {
    var $badge = $textField.siblings('.dt-contrast-badge');
    if (!$badge.length) {
      return;
    }

    // Derive the companion background field. Try '{base}-bg' first (e.g.
    // 'button-text' → 'button-bg'), then fall back to just '{base}' (e.g.
    // 'color-primary-text' → 'color-primary').
    var baseName = $textField.data('token-name').replace(/-text$/, '');
    var $bgField = $('[data-token-name="' + baseName + '-bg"]');
    if (!$bgField.length) {
      $bgField = $('[data-token-name="' + baseName + '"]');
    }
    if (!$bgField.length) {
      return;
    }

    var textHex = $textField.val().toLowerCase();
    var bgHex = $bgField.val().toLowerCase();

    if (!/^#[0-9a-f]{6}$/.test(textHex) || !/^#[0-9a-f]{6}$/.test(bgHex)) {
      $badge.text('').removeClass('dt-contrast--aaa dt-contrast--aa dt-contrast--large dt-contrast--fail');
      return;
    }

    applyContrastBadge($badge, contrastRatio(textHex, bgHex));
  }

  Backdrop.behaviors.designTokensColor = {
    attach: function (context, settings) {
      $('.design-tokens-color-field', context).once('design-tokens-color').each(function () {
        var $text = $(this);
        var tokenName = $text.data('token-name');
        var currentVal = $text.val();
        var isTextToken = /-text$/.test(tokenName);

        // Append a native color picker input after the text field.
        var $picker = $('<input>')
          .attr('type', 'color')
          .attr('aria-label', $text.attr('aria-label') || $text.attr('title') || '')
          .addClass('design-tokens-color-picker');

        if (/^#[0-9a-f]{6}$/.test(currentVal)) {
          $picker.val(currentVal);
        }

        $text.after($picker);

        var isBgWithHint = !!$text.data('contrast-with');

        // Add a contrast badge after the picker for text tokens and for
        // background-only tokens that declare a contrast_with hint.
        if (isTextToken) {
          $picker.after($('<span>').addClass('dt-contrast-badge'));
          updateContrastBadge($text);
        }
        else if (isBgWithHint) {
          $picker.after($('<span>').addClass('dt-contrast-badge'));
          updateBgContrastBadge($text);
        }

        // Picker → text field.
        $picker.on('input change', function () {
          var value = $picker.val();
          $text.val(value);
          Backdrop.designTokens.updatePreview(tokenName, value);
          if (isTextToken) {
            updateContrastBadge($text);
            // If this is an auto-text token, refresh all bg badges that use 'auto'.
            if (tokenName === 'color-text-default' || tokenName === 'color-text-inverted') {
              $('[data-contrast-with="auto"]').each(function () {
                updateBgContrastBadge($(this));
              });
            }
          }
          else if (isBgWithHint) {
            updateBgContrastBadge($text);
          }
          else {
            // Paired background changed — update the companion text field's badge.
            // Strip '-bg' suffix if present so 'button-bg' finds 'button-text'.
            var baseName = tokenName.replace(/-bg$/, '');
            var $companion = $('[data-token-name="' + baseName + '-text"]');
            if ($companion.length) {
              updateContrastBadge($companion);
            }
          }
        });

        // Text field → picker (and badge).
        $text.on('input change', function () {
          var value = $text.val().toLowerCase();
          if (/^#[0-9a-f]{6}$/.test(value)) {
            $picker.val(value);
          }
          Backdrop.designTokens.updatePreview(tokenName, value);
          if (isTextToken) {
            updateContrastBadge($text);
            if (tokenName === 'color-text-default' || tokenName === 'color-text-inverted') {
              $('[data-contrast-with="auto"]').each(function () {
                updateBgContrastBadge($(this));
              });
            }
          }
          else if (isBgWithHint) {
            updateBgContrastBadge($text);
          }
          else {
            var baseName = tokenName.replace(/-bg$/, '');
            var $companion = $('[data-token-name="' + baseName + '-text"]');
            if ($companion.length) {
              updateContrastBadge($companion);
            }
          }
        });
      });
    }
  };

}(jQuery));
