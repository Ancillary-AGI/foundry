// FoundryEngine Web Loading Script
// Handles loading screen, progress updates, and initialization

(function() {
  'use strict';

  // Loading state
  let loadingState = {
    progress: 0,
    status: 'Initializing...',
    details: {
      platform: 'Web',
      webgl: 'Detecting...',
      memory: 'Calculating...'
    },
    tips: [
      'Loading game engine...',
      'Initializing WebGL context...',
      'Setting up audio system...',
      'Loading game assets...',
      'Preparing input handlers...',
      'Almost ready...'
    ],
    currentTip: 0,
    tipInterval: null
  };

  // DOM elements
  let elements = {};

  // Initialize loading system
  function init() {
    console.log('FoundryEngine Loading System initializing...');

    // Get DOM elements
    elements = {
      loadingScreen: document.getElementById('loading-screen'),
      progressFill: document.getElementById('progress-fill'),
      progressText: document.getElementById('progress-text'),
      platformInfo: document.getElementById('platform-info'),
      webglInfo: document.getElementById('webgl-info'),
      memoryInfo: document.getElementById('memory-info'),
      loadingTip: document.getElementById('loading-tip')
    };

    // Check if all elements exist
    const missingElements = Object.keys(elements).filter(key => !elements[key]);
    if (missingElements.length > 0) {
      console.warn('Missing loading elements:', missingElements);
    }

    // Detect platform capabilities
    detectCapabilities();

    // Start tip rotation
    startTipRotation();

    // Setup event listeners
    setupEventListeners();

    console.log('FoundryEngine Loading System initialized');
  }

  // Detect platform capabilities
  function detectCapabilities() {
    // Detect WebGL support
    const canvas = document.createElement('canvas');
    const gl = canvas.getContext('webgl2') || canvas.getContext('webgl') || canvas.getContext('experimental-webgl');

    if (gl) {
      const debugInfo = gl.getExtension('WEBGL_debug_renderer_info');
      let renderer = 'Unknown';

      if (debugInfo) {
        renderer = gl.getParameter(debugInfo.UNMASKED_RENDERER_WEBGL) || 'Unknown';
      }

      loadingState.details.webgl = gl.getParameter(gl.VERSION) + ' (' + renderer + ')';
    } else {
      loadingState.details.webgl = 'Not supported';
    }

    // Detect memory information
    if ('memory' in performance) {
      const memory = performance.memory;
      const usedMB = Math.round(memory.usedJSHeapSize / 1048576);
      const totalMB = Math.round(memory.totalJSHeapSize / 1048576);
      const limitMB = Math.round(memory.jsHeapSizeLimit / 1048576);
      loadingState.details.memory = usedMB + 'MB / ' + totalMB + 'MB (Limit: ' + limitMB + 'MB)';
    } else {
      loadingState.details.memory = 'Unknown';
    }

    // Update display
    updateDisplay();
  }

  // Start tip rotation
  function startTipRotation() {
    if (loadingState.tipInterval) {
      clearInterval(loadingState.tipInterval);
    }

    loadingState.tipInterval = setInterval(() => {
      loadingState.currentTip = (loadingState.currentTip + 1) % loadingState.tips.length;
      updateDisplay();
    }, 2000);
  }

  // Setup event listeners
  function setupEventListeners() {
    // Listen for engine progress events
    window.addEventListener('foundryengine-progress', handleProgressEvent);
    window.addEventListener('foundryengine-loaded', handleLoadedEvent);
    window.addEventListener('foundryengine-error', handleErrorEvent);

    // Handle visibility change
    document.addEventListener('visibilitychange', handleVisibilityChange);
  }

  // Handle progress events from the engine
  function handleProgressEvent(event) {
    const detail = event.detail || {};
    updateProgress(detail.progress || 0, detail.status, detail.details);
  }

  // Handle loaded event
  function handleLoadedEvent(event) {
    console.log('FoundryEngine loaded successfully');
    hideLoadingScreen();
  }

  // Handle error event
  function handleErrorEvent(event) {
    const error = event.detail || {};
    console.error('FoundryEngine loading error:', error);
    showError(error.message || 'Failed to load game engine');
  }

  // Handle visibility change
  function handleVisibilityChange() {
    if (document.hidden) {
      // Pause loading animations when tab is hidden
      if (loadingState.tipInterval) {
        clearInterval(loadingState.tipInterval);
        loadingState.tipInterval = null;
      }
    } else {
      // Resume loading animations when tab becomes visible
      startTipRotation();
    }
  }

  // Update loading progress
  function updateProgress(progress, status, details) {
    loadingState.progress = Math.max(0, Math.min(100, progress || 0));
    loadingState.status = status || loadingState.status;

    if (details) {
      Object.assign(loadingState.details, details);
    }

    updateDisplay();
  }

  // Update display elements
  function updateDisplay() {
    // Update progress bar
    if (elements.progressFill) {
      elements.progressFill.style.width = loadingState.progress + '%';
    }

    // Update progress text
    if (elements.progressText) {
      elements.progressText.textContent = loadingState.status;
    }

    // Update detail information
    if (elements.platformInfo) {
      elements.platformInfo.textContent = loadingState.details.platform;
    }

    if (elements.webglInfo) {
      elements.webglInfo.textContent = loadingState.details.webgl;
    }

    if (elements.memoryInfo) {
      elements.memoryInfo.textContent = loadingState.details.memory;
    }

    // Update loading tip
    if (elements.loadingTip) {
      elements.loadingTip.textContent = loadingState.tips[loadingState.currentTip];
    }
  }

  // Hide loading screen
  function hideLoadingScreen() {
    if (loadingState.tipInterval) {
      clearInterval(loadingState.tipInterval);
      loadingState.tipInterval = null;
    }

    if (elements.loadingScreen) {
      elements.loadingScreen.classList.add('hidden');

      // Remove from DOM after animation
      setTimeout(() => {
        if (elements.loadingScreen.parentNode) {
          elements.loadingScreen.parentNode.removeChild(elements.loadingScreen);
        }
      }, 500);
    }

    console.log('Loading screen hidden');
  }

  // Show error message
  function showError(message) {
    console.error('Loading error:', message);

    // Update loading text to show error
    if (elements.progressText) {
      elements.progressText.textContent = 'Error loading game';
    }

    if (elements.loadingTip) {
      elements.loadingTip.textContent = message;
    }

    // Show error modal if available
    const errorModal = document.getElementById('error-modal');
    const errorMessage = document.getElementById('error-message');

    if (errorModal && errorMessage) {
      errorMessage.textContent = message;
      errorModal.classList.remove('hidden');
    }
  }

  // Public API
  window.FoundryEngineLoading = {
    init: init,
    updateProgress: updateProgress,
    hideLoadingScreen: hideLoadingScreen,
    showError: showError,
    getProgress: () => loadingState.progress,
    getStatus: () => loadingState.status
  };

  // Auto-initialize when DOM is ready
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

  console.log('FoundryEngine Loading Script loaded successfully!');

})();
