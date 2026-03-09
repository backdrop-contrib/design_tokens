(function () {

  'use strict';

  /**
   * Listens for messages from the Design Tokens admin form and acts on them.
   *
   * Handles two message types:
   *  - designTokensUpdate:   applies a CSS variable change to :root instantly.
   *  - designTokensLoadFont: injects a Google Fonts <link> into the iframe <head>
   *                          so the updated font renders without a page reload.
   *
   * This script is loaded on all front-end pages for themes that support
   * Design Tokens. It is a no-op unless the page is inside a preview iframe.
   */
  window.addEventListener('message', function (event) {
    // Security: only accept messages from the same origin.
    if (event.origin !== window.location.origin) {
      return;
    }

    var data = event.data;
    if (!data) {
      return;
    }

    // Apply a CSS custom property update.
    if (data.type === 'designTokensUpdate' && data.token && typeof data.value !== 'undefined') {
      document.documentElement.style.setProperty(data.token, data.value);
    }

    // Load a Google Font on demand.
    if (data.type === 'designTokensLoadFont' && data.googleFont) {
      var id = 'design-tokens-gf-' + data.googleFont.replace(/[^a-zA-Z0-9]/g, '-');
      if (!document.getElementById(id)) {
        var link = document.createElement('link');
        link.id = id;
        link.rel = 'stylesheet';
        link.href = 'https://fonts.googleapis.com/css2?family=' + data.googleFont + '&display=swap';
        document.head.appendChild(link);
      }
    }
  });

}());
