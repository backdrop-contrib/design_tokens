/**
 * @file
 * Design Tokens Font — admin field enhancement.
 *
 * Inserts a preset select dropdown and font preview for each font token field.
 * The original text field remains always visible so the user can see (and edit)
 * the actual CSS value. Google Fonts are loaded on demand when selected.
 */
(function ($) {
  'use strict';

  /**
   * Dynamically injects a Google Fonts stylesheet if not already loaded.
   */
  function loadGoogleFont(googleFontSpec) {
    var id = 'design-tokens-gf-' + googleFontSpec.replace(/[^a-zA-Z0-9]/g, '-');
    if (document.getElementById(id)) {
      return;
    }
    var link = document.createElement('link');
    link.id = id;
    link.rel = 'stylesheet';
    link.href = 'https://fonts.googleapis.com/css2?family=' + googleFontSpec + '&display=swap';
    document.head.appendChild(link);
  }

  Backdrop.behaviors.designTokensFont = {
    attach: function (context, settings) {
      var s = settings.designTokensFont || {};
      var presets     = s.presets     || [];
      var selectLabel = s.selectLabel || '— Choose a font —';
      var previewText = s.previewText || 'The quick brown fox jumps over the lazy dog.';

      // Build a flat map: CSS value → google_font spec (for on-demand loading).
      var googleFontsMap = {};
      $.each(presets, function (i, group) {
        $.each(group.fonts, function (j, font) {
          if (font.google_font) {
            googleFontsMap[font.value] = font.google_font;
          }
        });
      });

      $('.design-tokens-font-field', context).once('design-tokens-font', function () {
        var $field   = $(this);
        var $wrapper = $field.closest('.form-item');
        var current  = $field.val();

        /* ---- Build the preset select ------------------------------------ */
        var $select = $('<select class="design-tokens-font-select form-select">');
        $select.append($('<option>').val('').text(selectLabel));

        var matchesPreset = false;

        $.each(presets, function (i, group) {
          var $group = $('<optgroup>').attr('label', group.label);
          $.each(group.fonts, function (j, font) {
            var $opt = $('<option>').val(font.value).text(font.label);
            if (font.value === current) {
              $opt.prop('selected', true);
              matchesPreset = true;
              if (font.google_font) {
                loadGoogleFont(font.google_font);
              }
            }
            $group.append($opt);
          });
          $select.append($group);
        });

        /* ---- Build the preview ------------------------------------------ */
        var $preview = $('<div class="design-tokens-font-preview">').text(previewText);
        $preview.css('font-family', current || 'inherit');

        /* ---- Insert into DOM -------------------------------------------- */
        $select.insertBefore($field);
        $wrapper.append($preview);

        /* ---- Event handlers --------------------------------------------- */

        // Select → update text field, preview, and iframe.
        $select.on('change', function () {
          var val = $(this).val();
          if (!val) {
            return;
          }
          $field.val(val);
          $preview.css('font-family', val);

          if (googleFontsMap[val]) {
            loadGoogleFont(googleFontsMap[val]);
            if (Backdrop.designTokens && Backdrop.designTokens.loadFontInPreview) {
              Backdrop.designTokens.loadFontInPreview(googleFontsMap[val]);
            }
          }
          if (Backdrop.designTokens && Backdrop.designTokens.updatePreview) {
            Backdrop.designTokens.updatePreview($field.data('token-name'), val);
          }
        });

        // Text field → sync select, preview, and iframe.
        $field.on('input change', function () {
          var val = $(this).val();
          $preview.css('font-family', val || 'inherit');

          // Sync select if value matches a preset.
          var matched = false;
          $select.find('option').each(function () {
            if ($(this).val() === val) {
              $(this).prop('selected', true);
              matched = true;
            }
          });
          if (!matched) {
            $select.val('');
          }

          if (Backdrop.designTokens && Backdrop.designTokens.updatePreview) {
            Backdrop.designTokens.updatePreview($field.data('token-name'), val);
          }
        });
      });
    }
  };

})(jQuery);
