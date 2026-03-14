(function ($) {

  'use strict';

  Backdrop.behaviors.designTokensAdmin = {
    attach: function (context, settings) {

      // Collapse all / Expand all toggle.
      $('#design-tokens-collapse-all', context).once('design-tokens-collapse').on('click', function () {
        var $button = $(this);
        var $fieldsets = $('fieldset.collapsible', context);
        var allCollapsed = $fieldsets.filter(':not(.collapsed)').length === 0;

        if (allCollapsed) {
          // Expand all — click any collapsed fieldset legends.
          $fieldsets.filter('.collapsed').find('> legend a').trigger('click');
          $button.text(Backdrop.t('Collapse all'));
        }
        else {
          // Collapse all — click any expanded fieldset legends.
          $fieldsets.filter(':not(.collapsed)').find('> legend a').trigger('click');
          $button.text(Backdrop.t('Expand all'));
        }
      });

      // Preview panel toggle.
      $('#design-tokens-preview-toggle', context).once('design-tokens-toggle').on('click', function () {
        Backdrop.designTokens.togglePreview();
      });

      // Close button inside the panel.
      $('#design-tokens-preview-close', context).once('design-tokens-close').on('click', function () {
        Backdrop.designTokens.closePreview();
      });

      // Scheme selector — populate all token fields when a preset is chosen.
      // A flag prevents the field-change listener below from immediately
      // resetting the selector back to Custom during scheme application.
      var applyingScheme = false;

      $('#design-tokens-scheme-select', context).once('design-tokens-scheme').on('change', function () {
        var scheme_key = $(this).val();
        if (!scheme_key) {
          return;
        }
        var schemes = (settings.designTokens && settings.designTokens.schemes) ? settings.designTokens.schemes : {};
        var scheme = schemes[scheme_key];
        if (!scheme || !scheme.tokens) {
          return;
        }
        applyingScheme = true;
        $.each(scheme.tokens, function (token_name, value) {
          var $field = $('[data-token-name="' + token_name + '"]');
          if ($field.length) {
            $field.val(value).trigger('change');
          }
        });
        // Clear the flag after all triggered change events have fired.
        setTimeout(function () { applyingScheme = false; }, 0);
      });

      // Per-section reset — resets all token fields in a group to their defaults.
      $('.design-tokens-reset-section', context).once('design-tokens-reset-section').on('click', function () {
        var groupKey = $(this).data('group');
        var defaults = (settings.designTokens && settings.designTokens.defaults) ? settings.designTokens.defaults : {};
        var $fieldset = $(this).closest('fieldset');

        $fieldset.find('[data-token-name]').each(function () {
          var tokenName = $(this).data('token-name');
          if (defaults.hasOwnProperty(tokenName)) {
            $(this).val(defaults[tokenName]).trigger('change');
          }
        });

        // Mark the scheme selector as Custom since values may have changed.
        $('#design-tokens-scheme-select').val('');
      });

      // Reset the scheme selector to Custom when any field is manually edited.
      // This ensures the selector accurately reflects whether the current values
      // match a preset or have been customised.
      $(document, context).once('design-tokens-field-watch').on('input change', '[data-token-name]', function () {
        if (!applyingScheme) {
          $('#design-tokens-scheme-select').val('');
        }
      });

      // Initialize live preview iframe.
      Backdrop.designTokens.initPreview(context, settings);
    }
  };

  /**
   * Shared Design Tokens preview utilities.
   */
  Backdrop.designTokens = Backdrop.designTokens || {

    /**
     * Opens the preview panel, positioning it below the Backdrop toolbar.
     */
    openPreview: function () {
      var $panel = $('#design-tokens-preview-panel');
      var $toggle = $('#design-tokens-preview-toggle');

      // Position panel below the Backdrop admin toolbar.
      var toolbarHeight = $('#toolbar').outerHeight() || 0;
      $panel.css('top', toolbarHeight + 'px');

      $panel.removeAttr('hidden');
      $toggle.text(Backdrop.t('Hide preview'));
      $('body').addClass('design-tokens-preview-open');
    },

    /**
     * Closes the preview panel.
     */
    closePreview: function () {
      $('#design-tokens-preview-panel').attr('hidden', '');
      $('#design-tokens-preview-toggle').text(Backdrop.t('Show preview'));
      $('body').removeClass('design-tokens-preview-open');
    },

    /**
     * Toggles the preview panel open or closed.
     */
    togglePreview: function () {
      var isHidden = $('#design-tokens-preview-panel').attr('hidden') !== undefined;
      if (isHidden) {
        Backdrop.designTokens.openPreview();
      }
      else {
        Backdrop.designTokens.closePreview();
      }
    },

    /**
     * Initializes the preview iframe — replays all current token values on load,
     * including loading any Google Fonts needed by font tokens.
     */
    initPreview: function (context, settings) {
      var $iframe = $('#design-tokens-preview-iframe', context);
      if (!$iframe.length) {
        return;
      }

      // Build Google Fonts map from font module settings if available.
      var googleFontsMap = Backdrop.designTokens.buildGoogleFontsMap(settings);

      $iframe.on('load', function () {
        $('[data-token-name]').each(function () {
          var $field = $(this);
          var tokenName = $field.data('token-name');
          var value = $field.val();
          Backdrop.designTokens.updatePreview(tokenName, value);

          // For font tokens, also ensure the Google Font is loaded in the iframe.
          if (value && googleFontsMap[value]) {
            Backdrop.designTokens.loadFontInPreview(googleFontsMap[value]);
          }
        });
      });
    },

    /**
     * Builds a flat map of CSS font-family value → Google Fonts spec from
     * the designTokensFont settings provided by the font sub-module.
     *
     * @param {object} settings  Backdrop.settings passed from attach().
     * @return {object}
     */
    buildGoogleFontsMap: function (settings) {
      var map = {};
      var s = settings && settings.designTokensFont;
      if (!s || !s.presets) {
        return map;
      }
      $.each(s.presets, function (i, group) {
        $.each(group.fonts, function (j, font) {
          if (font.google_font) {
            map[font.value] = font.google_font;
          }
        });
      });
      return map;
    },

    /**
     * Sends a token value update to the preview iframe via postMessage.
     *
     * @param {string} tokenName  Token name without leading --.
     * @param {string} value      New CSS value.
     */
    updatePreview: function (tokenName, value) {
      var $iframe = $('#design-tokens-preview-iframe');
      if (!$iframe.length || !$iframe[0].contentWindow) {
        return;
      }
      $iframe[0].contentWindow.postMessage({
        type: 'designTokensUpdate',
        token: '--' + tokenName,
        value: value
      }, window.location.origin);
    },

    /**
     * Sends a Google Font load request to the preview iframe via postMessage.
     *
     * @param {string} googleFontSpec  Google Fonts CSS2 API family spec.
     */
    loadFontInPreview: function (googleFontSpec) {
      var $iframe = $('#design-tokens-preview-iframe');
      if (!$iframe.length || !$iframe[0].contentWindow) {
        return;
      }
      $iframe[0].contentWindow.postMessage({
        type: 'designTokensLoadFont',
        googleFont: googleFontSpec
      }, window.location.origin);
    }
  };

}(jQuery));
