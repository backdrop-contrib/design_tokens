(function () {

  'use strict';

  /**
   * Listens for CSS variable updates sent from the Theme Tokens admin form
   * via postMessage and applies them instantly to the document root.
   *
   * This script is loaded on all front-end pages for themes that support
   * Theme Tokens. It is a no-op unless the page is inside a preview iframe.
   */
  window.addEventListener('message', function (event) {
    // Security: only accept messages from the same origin.
    if (event.origin !== window.location.origin) {
      return;
    }

    var data = event.data;
    if (!data || data.type !== 'themeTokensUpdate') {
      return;
    }

    if (data.token && typeof data.value !== 'undefined') {
      document.documentElement.style.setProperty(data.token, data.value);
    }
  });

}());
