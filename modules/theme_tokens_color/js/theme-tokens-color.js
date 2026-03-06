(function ($) {

  'use strict';

  Backdrop.behaviors.themeTokensColor = {
    attach: function (context, settings) {
      $('.theme-tokens-color-field', context).once('theme-tokens-color').each(function () {
        var $text = $(this);
        var currentVal = $text.val();

        // Append a native color picker input after the text field.
        var $picker = $('<input>')
          .attr('type', 'color')
          .attr('aria-label', $text.attr('aria-label') || $text.attr('title') || '')
          .addClass('theme-tokens-color-picker');

        // Set the picker's initial value if we have a valid 6-digit hex.
        if (/^#[0-9a-f]{6}$/.test(currentVal)) {
          $picker.val(currentVal);
        }

        $text.after($picker);

        // Picker → text field.
        $picker.on('input change', function () {
          var value = $picker.val();
          $text.val(value);
          Backdrop.themeTokens.updatePreview($text.data('token-name'), value);
        });

        // Text field → picker (sync when user types a valid hex manually).
        $text.on('input change', function () {
          var value = $text.val().toLowerCase();
          if (/^#[0-9a-f]{6}$/.test(value)) {
            $picker.val(value);
          }
          Backdrop.themeTokens.updatePreview($text.data('token-name'), value);
        });
      });
    }
  };

}(jQuery));
