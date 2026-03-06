(function ($) {

  'use strict';

  Backdrop.behaviors.themeTokensAdmin = {
    attach: function (context, settings) {

      // Preview panel toggle.
      $('#theme-tokens-preview-toggle', context).once('theme-tokens-toggle').on('click', function () {
        var $panel = $('#theme-tokens-preview-panel');
        var isHidden = $panel.attr('hidden') !== undefined;
        if (isHidden) {
          $panel.removeAttr('hidden');
          $(this).text(Backdrop.t('Hide preview'));
        }
        else {
          $panel.attr('hidden', '');
          $(this).text(Backdrop.t('Show preview'));
        }
      });

      // Close button inside the panel.
      $('#theme-tokens-preview-close', context).once('theme-tokens-close').on('click', function () {
        $('#theme-tokens-preview-panel').attr('hidden', '');
        $('#theme-tokens-preview-toggle').text(Backdrop.t('Show preview'));
      });

      // Scheme selector — populate all token fields when a preset is chosen.
      $('#theme-tokens-scheme-select', context).once('theme-tokens-scheme').on('change', function () {
        var scheme_key = $(this).val();
        if (!scheme_key) {
          return;
        }
        var schemes = (settings.themeTokens && settings.themeTokens.schemes) ? settings.themeTokens.schemes : {};
        var scheme = schemes[scheme_key];
        if (!scheme || !scheme.tokens) {
          return;
        }
        $.each(scheme.tokens, function (token_name, value) {
          var $field = $('[data-token-name="' + token_name + '"]');
          if ($field.length) {
            $field.val(value).trigger('change');
          }
        });
      });

      // Initialize live preview iframe.
      Backdrop.themeTokens.initPreview(context);
    }
  };

  /**
   * Shared Theme Tokens preview utilities.
   */
  Backdrop.themeTokens = Backdrop.themeTokens || {

    /**
     * Initializes the preview iframe — replays all current values on load.
     */
    initPreview: function (context) {
      var $iframe = $('#theme-tokens-preview-iframe', context);
      if (!$iframe.length) {
        return;
      }

      $iframe.on('load', function () {
        $('[data-token-name]').each(function () {
          var $field = $(this);
          Backdrop.themeTokens.updatePreview($field.data('token-name'), $field.val());
        });
      });
    },

    /**
     * Sends a token update to the preview iframe via postMessage.
     *
     * @param {string} tokenName  Token name (without --).
     * @param {string} value      New value.
     */
    updatePreview: function (tokenName, value) {
      var $iframe = $('#theme-tokens-preview-iframe');
      if (!$iframe.length || !$iframe[0].contentWindow) {
        return;
      }
      $iframe[0].contentWindow.postMessage({
        type: 'themeTokensUpdate',
        token: '--' + tokenName,
        value: value
      }, window.location.origin);
    }
  };

}(jQuery));
