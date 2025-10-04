// FoundryEngine Web Library
// Provides JavaScript bindings and utilities for the Web platform

mergeInto(LibraryManager.library, {
  // Platform detection and initialization
  $FOUNDRY_ENGINE_WEB: {
    canvas: null,
    context: null,
    isInitialized: false,
    animationId: 0,
    lastFrameTime: 0,
    frameCount: 0,
    fps: 0,
    debugMode: false,

    // Initialize the Web platform
    init: function() {
      console.log('FoundryEngine Web Library initializing...');

      // Get canvas element
      this.canvas = document.getElementById('canvas');
      if (!this.canvas) {
        console.error('Canvas element not found!');
        return false;
      }

      // Initialize WebGL context
      this.context = this.canvas.getContext('webgl2') || this.canvas.getContext('webgl');
      if (!this.context) {
        console.error('WebGL context not available!');
        return false;
      }

      // Set canvas size
      this.updateCanvasSize();

      // Setup event handlers
      this.setupEventHandlers();

      // Setup resize observer
      this.setupResizeObserver();

      this.isInitialized = true;
      console.log('FoundryEngine Web Library initialized successfully!');
      return true;
    },

    // Update canvas size based on display
    updateCanvasSize: function() {
      if (!this.canvas) return;

      const devicePixelRatio = window.devicePixelRatio || 1;
      const rect = this.canvas.getBoundingClientRect();

      this.canvas.width = rect.width * devicePixelRatio;
      this.canvas.height = rect.height * devicePixelRatio;

      // Notify the engine about size change
      if (typeof _onCanvasResize !== 'undefined') {
        _onCanvasResize(this.canvas.width, this.canvas.height);
      }
    },

    // Setup event handlers
    setupEventHandlers: function() {
      if (!this.canvas) return;

      // Keyboard events
      document.addEventListener('keydown', this.onKeyDown.bind(this));
      document.addEventListener('keyup', this.onKeyUp.bind(this));

      // Mouse events
      this.canvas.addEventListener('mousedown', this.onMouseDown.bind(this));
      this.canvas.addEventListener('mouseup', this.onMouseUp.bind(this));
      this.canvas.addEventListener('mousemove', this.onMouseMove.bind(this));
      this.canvas.addEventListener('wheel', this.onMouseWheel.bind(this));

      // Touch events
      this.canvas.addEventListener('touchstart', this.onTouchStart.bind(this));
      this.canvas.addEventListener('touchend', this.onTouchEnd.bind(this));
      this.canvas.addEventListener('touchmove', this.onTouchMove.bind(this));

      // Context menu
      this.canvas.addEventListener('contextmenu', function(e) {
        e.preventDefault();
      });

      // Focus events
      this.canvas.addEventListener('focus', this.onFocus.bind(this));
      this.canvas.addEventListener('blur', this.onBlur.bind(this));

      // Visibility change
      document.addEventListener('visibilitychange', this.onVisibilityChange.bind(this));

      // Fullscreen change
      document.addEventListener('fullscreenchange', this.onFullscreenChange.bind(this));
    },

    // Setup resize observer
    setupResizeObserver: function() {
      if (typeof ResizeObserver !== 'undefined') {
        const resizeObserver = new ResizeObserver(entries => {
          this.updateCanvasSize();
        });
        resizeObserver.observe(this.canvas);
      } else {
        // Fallback for older browsers
        window.addEventListener('resize', this.updateCanvasSize.bind(this));
      }
    },

    // Keyboard event handlers
    onKeyDown: function(event) {
      if (typeof _onKeyEvent !== 'undefined') {
        _onKeyEvent(event.keyCode, 1, event.repeat ? 1 : 0);
      }
      event.preventDefault();
    },

    onKeyUp: function(event) {
      if (typeof _onKeyEvent !== 'undefined') {
        _onKeyEvent(event.keyCode, 0, 0);
      }
      event.preventDefault();
    },

    // Mouse event handlers
    onMouseDown: function(event) {
      if (typeof _onMouseEvent !== 'undefined') {
        _onMouseEvent(event.button, 1, event.clientX, event.clientY);
      }
      event.preventDefault();
    },

    onMouseUp: function(event) {
      if (typeof _onMouseEvent !== 'undefined') {
        _onMouseEvent(event.button, 0, event.clientX, event.clientY);
      }
      event.preventDefault();
    },

    onMouseMove: function(event) {
      if (typeof _onMouseMove !== 'undefined') {
        _onMouseMove(event.clientX, event.clientY);
      }
    },

    onMouseWheel: function(event) {
      if (typeof _onMouseWheel !== 'undefined') {
        _onMouseWheel(event.deltaX, event.deltaY);
      }
      event.preventDefault();
    },

    // Touch event handlers
    onTouchStart: function(event) {
      event.preventDefault();

      for (let i = 0; i < event.touches.length; i++) {
        const touch = event.touches[i];
        if (typeof _onTouchEvent !== 'undefined') {
          _onTouchEvent(touch.identifier, touch.clientX, touch.clientY, 1);
        }
      }
    },

    onTouchEnd: function(event) {
      event.preventDefault();

      for (let i = 0; i < event.changedTouches.length; i++) {
        const touch = event.changedTouches[i];
        if (typeof _onTouchEvent !== 'undefined') {
          _onTouchEvent(touch.identifier, touch.clientX, touch.clientY, 0);
        }
      }
    },

    onTouchMove: function(event) {
      event.preventDefault();

      for (let i = 0; i < event.touches.length; i++) {
        const touch = event.touches[i];
        if (typeof _onTouchEvent !== 'undefined') {
          _onTouchEvent(touch.identifier, touch.clientX, touch.clientY, 2);
        }
      }
    },

    // Focus event handlers
    onFocus: function(event) {
      if (typeof _onFocusEvent !== 'undefined') {
        _onFocusEvent(1);
      }
    },

    onBlur: function(event) {
      if (typeof _onFocusEvent !== 'undefined') {
        _onFocusEvent(0);
      }
    },

    // Visibility change handler
    onVisibilityChange: function(event) {
      if (typeof _onVisibilityChange !== 'undefined') {
        _onVisibilityChange(document.hidden ? 0 : 1);
      }
    },

    // Fullscreen change handler
    onFullscreenChange: function(event) {
      if (typeof _onFullscreenChange !== 'undefined') {
        _onFullscreenChange(document.fullscreenElement ? 1 : 0);
      }
    },

    // Start the main loop
    startMainLoop: function() {
      if (this.animationId !== 0) return;

      const loop = function(currentTime) {
        // Calculate FPS
        if (this.lastFrameTime !== 0) {
          const deltaTime = currentTime - this.lastFrameTime;
          this.frameCount++;
          if (deltaTime >= 1000) {
            this.fps = Math.round((this.frameCount * 1000) / deltaTime);
            this.frameCount = 0;
            this.lastFrameTime = currentTime;
          }
        } else {
          this.lastFrameTime = currentTime;
        }

        // Call engine update
        if (typeof _mainLoop !== 'undefined') {
          _mainLoop();
        }

        this.animationId = requestAnimationFrame(loop);
      }.bind(this);

      this.animationId = requestAnimationFrame(loop);
    },

    // Stop the main loop
    stopMainLoop: function() {
      if (this.animationId !== 0) {
        cancelAnimationFrame(this.animationId);
        this.animationId = 0;
      }
    },

    // Get current FPS
    getFPS: function() {
      return this.fps;
    },

    // Get canvas dimensions
    getCanvasWidth: function() {
      return this.canvas ? this.canvas.width : 0;
    },

    getCanvasHeight: function() {
      return this.canvas ? this.canvas.height : 0;
    },

    // Get device pixel ratio
    getDevicePixelRatio: function() {
      return window.devicePixelRatio || 1;
    },

    // Get WebGL context
    getWebGLContext: function() {
      return this.context;
    },

    // Set debug mode
    setDebugMode: function(debug) {
      this.debugMode = debug;
    },

    // Log debug message
    logDebug: function(message) {
      if (this.debugMode) {
        // Sanitize message to prevent XSS
        const sanitizedMessage = this.sanitizeString(message);
        console.log('[FoundryEngine Debug] ' + sanitizedMessage);
      }
    },

    // Log error message
    logError: function(message) {
      // Sanitize message to prevent XSS
      const sanitizedMessage = this.sanitizeString(message);
      console.error('[FoundryEngine Error] ' + sanitizedMessage);
    },

    // Log warning message
    logWarning: function(message) {
      // Sanitize message to prevent XSS
      const sanitizedMessage = this.sanitizeString(message);
      console.warn('[FoundryEngine Warning] ' + sanitizedMessage);
    },

    // Sanitize string to prevent XSS
    sanitizeString: function(str) {
      if (typeof str !== 'string') return '';
      return str.replace(/[<>"'&]/g, function(match) {
        const escapeMap = {
          '<': '&lt;',
          '>': '&gt;',
          '"': '&quot;',
          "'": '&#x27;',
          '&': '&amp;'
        };
        return escapeMap[match];
      });
    }
  },

  // Web-specific functions
  foundry_engine_web_init: function() {
    return FOUNDRY_ENGINE_WEB.init();
  },

  foundry_engine_web_start_main_loop: function() {
    FOUNDRY_ENGINE_WEB.startMainLoop();
  },

  foundry_engine_web_stop_main_loop: function() {
    FOUNDRY_ENGINE_WEB.stopMainLoop();
  },

  foundry_engine_web_get_fps: function() {
    return FOUNDRY_ENGINE_WEB.getFPS();
  },

  foundry_engine_web_get_canvas_width: function() {
    return FOUNDRY_ENGINE_WEB.getCanvasWidth();
  },

  foundry_engine_web_get_canvas_height: function() {
    return FOUNDRY_ENGINE_WEB.getCanvasHeight();
  },

  foundry_engine_web_get_device_pixel_ratio: function() {
    return FOUNDRY_ENGINE_WEB.getDevicePixelRatio();
  },

  foundry_engine_web_set_debug_mode: function(debug) {
    FOUNDRY_ENGINE_WEB.setDebugMode(debug);
  },

  foundry_engine_web_log_debug: function(messagePtr) {
    const message = UTF8ToString(messagePtr);
    FOUNDRY_ENGINE_WEB.logDebug(message);
  },

  foundry_engine_web_log_error: function(messagePtr) {
    const message = UTF8ToString(messagePtr);
    FOUNDRY_ENGINE_WEB.logError(message);
  },

  foundry_engine_web_log_warning: function(messagePtr) {
    const message = UTF8ToString(messagePtr);
    FOUNDRY_ENGINE_WEB.logWarning(message);
  },

  // WebGL functions
  foundry_engine_webgl_get_context: function() {
    return FOUNDRY_ENGINE_WEB.getWebGLContext();
  },

  // Web Audio functions
  foundry_engine_webaudio_create_context: function() {
    try {
      if (typeof AudioContext !== 'undefined') {
        return new AudioContext();
      } else if (typeof webkitAudioContext !== 'undefined') {
        return new webkitAudioContext();
      }
      return 0;
    } catch (e) {
      console.error('Failed to create Web Audio context:', e);
      return 0;
    }
  },

  foundry_engine_webaudio_load_buffer: function(contextPtr, urlPtr, bufferNamePtr) {
    const context = contextPtr;
    const url = UTF8ToString(urlPtr);
    const bufferName = UTF8ToString(bufferNamePtr);

    if (!context) return 0;

    const request = new XMLHttpRequest();
    request.open('GET', url, true);
    request.responseType = 'arraybuffer';

    request.onload = function() {
      context.decodeAudioData(request.response, function(buffer) {
        // Store buffer in a global registry
        if (typeof window.foundryAudioBuffers === 'undefined') {
          window.foundryAudioBuffers = {};
        }
        window.foundryAudioBuffers[bufferName] = buffer;

        if (typeof _onAudioBufferLoaded !== 'undefined') {
          _onAudioBufferLoaded(bufferNamePtr);
        }
      }, function(error) {
        console.error('Failed to decode audio data:', error);
        if (typeof _onAudioBufferError !== 'undefined') {
          _onAudioBufferError(bufferNamePtr);
        }
      });
    };

    request.onerror = function() {
      console.error('Failed to load audio file:', url);
      if (typeof _onAudioBufferError !== 'undefined') {
        _onAudioBufferError(bufferNamePtr);
      }
    };

    request.send();
    return 1;
  },

  foundry_engine_webaudio_play_buffer: function(bufferNamePtr, loop) {
    const bufferName = UTF8ToString(bufferNamePtr);

    if (typeof window.foundryAudioBuffers === 'undefined') {
      console.error('No audio buffers loaded');
      return 0;
    }

    const buffer = window.foundryAudioBuffers[bufferName];
    if (!buffer) {
      console.error('Audio buffer not found:', bufferName);
      return 0;
    }

    // Create audio context if needed
    if (typeof window.foundryAudioContext === 'undefined') {
      try {
        window.foundryAudioContext = new (window.AudioContext || window.webkitAudioContext)();
      } catch (e) {
        console.error('Failed to create audio context:', e);
        return 0;
      }
    }

    const context = window.foundryAudioContext;
    const source = context.createBufferSource();
    source.buffer = buffer;
    source.loop = loop ? true : false;

    // Connect to destination
    source.connect(context.destination);

    // Start playing
    source.start(0);
    return 1;
  },

  // WebRTC functions
  foundry_engine_webrtc_create_peer_connection: function() {
    try {
      const configuration = {
        iceServers: [
          { urls: 'stun:stun.l.google.com:19302' },
          { urls: 'stun:stun1.l.google.com:19302' }
        ]
      };

      const peerConnection = new RTCPeerConnection(configuration);

      // Setup event handlers
      peerConnection.onicecandidate = function(event) {
        if (event.candidate && typeof _onIceCandidate !== 'undefined') {
          const candidateStr = JSON.stringify(event.candidate);
          const candidatePtr = allocate(intArrayFromString(candidateStr), ALLOC_STACK);
          _onIceCandidate(candidatePtr);
        }
      };

      peerConnection.onconnectionstatechange = function(event) {
        if (typeof _onConnectionStateChange !== 'undefined') {
          _onConnectionStateChange(peerConnection.connectionState);
        }
      };

      peerConnection.ondatachannel = function(event) {
        const dataChannel = event.channel;
        setupDataChannel(dataChannel);
      };

      return peerConnection;
    } catch (e) {
      console.error('Failed to create peer connection:', e);
      return 0;
    }
  },

  foundry_engine_webrtc_create_data_channel: function(peerConnectionPtr, labelPtr) {
    const peerConnection = peerConnectionPtr;
    const label = UTF8ToString(labelPtr);

    if (!peerConnection) return 0;

    try {
      const dataChannel = peerConnection.createDataChannel(label, {
        ordered: true,
        maxPacketLifeTime: 3000
      });

      setupDataChannel(dataChannel);
      return dataChannel;
    } catch (e) {
      console.error('Failed to create data channel:', e);
      return 0;
    }
  },

  foundry_engine_webrtc_send_message: function(dataChannelPtr, messagePtr) {
    const dataChannel = dataChannelPtr;
    const message = UTF8ToString(messagePtr);

    if (!dataChannel || dataChannel.readyState !== 'open') {
      return 0;
    }

    try {
      dataChannel.send(message);
      return 1;
    } catch (e) {
      console.error('Failed to send message:', e);
      return 0;
    }
  },

  // Setup data channel event handlers
  setupDataChannel: function(dataChannel) {
    dataChannel.onopen = function(event) {
      if (typeof _onDataChannelOpen !== 'undefined') {
        _onDataChannelOpen();
      }
    };

    dataChannel.onclose = function(event) {
      if (typeof _onDataChannelClose !== 'undefined') {
        _onDataChannelClose();
      }
    };

    dataChannel.onmessage = function(event) {
      if (typeof _onDataChannelMessage !== 'undefined') {
        const messagePtr = allocate(intArrayFromString(event.data), ALLOC_STACK);
        _onDataChannelMessage(messagePtr);
      }
    };
  },

  // IndexedDB functions
  foundry_engine_indexeddb_open: function(dbNamePtr, version) {
    const dbName = UTF8ToString(dbNamePtr);

    return new Promise(function(resolve, reject) {
      const request = indexedDB.open(dbName, version);

      request.onerror = function(event) {
        console.error('Failed to open IndexedDB:', request.error);
        resolve(0);
      };

      request.onsuccess = function(event) {
        const db = event.target.result;
        resolve(db);
      };

      request.onupgradeneeded = function(event) {
        const db = event.target.result;

        // Create object stores if they don't exist
        if (!db.objectStoreNames.contains('gameData')) {
          const store = db.createObjectStore('gameData', { keyPath: 'key' });
          store.createIndex('key', 'key', { unique: true });
        }

        if (!db.objectStoreNames.contains('settings')) {
          const store = db.createObjectStore('settings', { keyPath: 'key' });
          store.createIndex('key', 'key', { unique: true });
        }

        if (!db.objectStoreNames.contains('achievements')) {
          const store = db.createObjectStore('achievements', { keyPath: 'id' });
          store.createIndex('id', 'id', { unique: true });
        }
      };
    });
  },

  foundry_engine_indexeddb_get: function(dbPtr, storeNamePtr, keyPtr) {
    const db = dbPtr;
    const storeName = UTF8ToString(storeNamePtr);
    const key = UTF8ToString(keyPtr);

    return new Promise(function(resolve, reject) {
      const transaction = db.transaction([storeName], 'readonly');
      const store = transaction.objectStore(storeName);
      const request = store.get(key);

      request.onerror = function(event) {
        console.error('Failed to get data from IndexedDB:', request.error);
        resolve(0);
      };

      request.onsuccess = function(event) {
        const result = event.target.result;
        if (result) {
          const dataStr = JSON.stringify(result);
          const dataPtr = allocate(intArrayFromString(dataStr), ALLOC_STACK);
          resolve(dataPtr);
        } else {
          resolve(0);
        }
      };
    });
  },

  foundry_engine_indexeddb_put: function(dbPtr, storeNamePtr, keyPtr, dataPtr) {
    const db = dbPtr;
    const storeName = UTF8ToString(storeNamePtr);
    const key = UTF8ToString(keyPtr);
    const data = UTF8ToString(dataPtr);

    return new Promise(function(resolve, reject) {
      const transaction = db.transaction([storeName], 'readwrite');
      const store = transaction.objectStore(storeName);

      const dataObj = JSON.parse(data);
      dataObj.key = key;

      const request = store.put(dataObj);

      request.onerror = function(event) {
        console.error('Failed to put data to IndexedDB:', request.error);
        resolve(0);
      };

      request.onsuccess = function(event) {
        resolve(1);
      };
    });
  },

  foundry_engine_indexeddb_delete: function(dbPtr, storeNamePtr, keyPtr) {
    const db = dbPtr;
    const storeName = UTF8ToString(storeNamePtr);
    const key = UTF8ToString(keyPtr);

    return new Promise(function(resolve, reject) {
      const transaction = db.transaction([storeName], 'readwrite');
      const store = transaction.objectStore(storeName);
      const request = store.delete(key);

      request.onerror = function(event) {
        console.error('Failed to delete data from IndexedDB:', request.error);
        resolve(0);
      };

      request.onsuccess = function(event) {
        resolve(1);
      };
    });
  },

  // Service Worker functions
  foundry_engine_service_worker_register: function(scriptUrlPtr) {
    const scriptUrl = UTF8ToString(scriptUrlPtr);

    if ('serviceWorker' in navigator) {
      return navigator.serviceWorker.register(scriptUrl).then(function(registration) {
        console.log('Service Worker registered successfully:', registration);
        return 1;
      }).catch(function(error) {
        console.error('Service Worker registration failed:', error);
        return 0;
      });
    }

    return 0;
  },

  foundry_engine_service_worker_unregister: function() {
    if ('serviceWorker' in navigator) {
      return navigator.serviceWorker.getRegistration().then(function(registration) {
        if (registration) {
          return registration.unregister().then(function(success) {
            console.log('Service Worker unregistered:', success);
            return success ? 1 : 0;
          });
        }
        return 0;
      }).catch(function(error) {
        console.error('Service Worker unregistration failed:', error);
        return 0;
      });
    }

    return 0;
  },

  // Push notification functions
  foundry_engine_push_notification_request_permission: function() {
    if ('Notification' in window && 'serviceWorker' in navigator) {
      return Notification.requestPermission().then(function(permission) {
        return permission === 'granted' ? 1 : 0;
      });
    }
    return 0;
  },

  foundry_engine_push_notification_show: function(titlePtr, bodyPtr, iconPtr) {
    const title = FOUNDRY_ENGINE_WEB.sanitizeString(UTF8ToString(titlePtr));
    const body = FOUNDRY_ENGINE_WEB.sanitizeString(UTF8ToString(bodyPtr));
    const icon = FOUNDRY_ENGINE_WEB.sanitizeString(UTF8ToString(iconPtr));

    if ('Notification' in window && Notification.permission === 'granted') {
      const options = {
        body: body,
        icon: icon || '/icon.png',
        badge: '/badge.png',
        tag: 'foundry-engine-notification'
      };

      const notification = new Notification(title, options);

      notification.onclick = function() {
        window.focus();
        notification.close();
      };

      return 1;
    }

    return 0;
  },

  // Local storage functions
  foundry_engine_local_storage_set_item: function(keyPtr, valuePtr) {
    const key = UTF8ToString(keyPtr);
    const value = UTF8ToString(valuePtr);

    try {
      localStorage.setItem(key, value);
      return 1;
    } catch (e) {
      console.error('Failed to set localStorage item:', e);
      return 0;
    }
  },

  foundry_engine_local_storage_get_item: function(keyPtr) {
    const key = UTF8ToString(keyPtr);

    try {
      const value = localStorage.getItem(key);
      if (value !== null) {
        const valuePtr = allocate(intArrayFromString(value), ALLOC_STACK);
        return valuePtr;
      }
    } catch (e) {
      console.error('Failed to get localStorage item:', e);
    }

    return 0;
  },

  foundry_engine_local_storage_remove_item: function(keyPtr) {
    const key = UTF8ToString(keyPtr);

    try {
      localStorage.removeItem(key);
      return 1;
    } catch (e) {
      console.error('Failed to remove localStorage item:', e);
      return 0;
    }
  },

  // Device information functions
  foundry_engine_device_get_user_agent: function() {
    const userAgent = navigator.userAgent;
    const userAgentPtr = allocate(intArrayFromString(userAgent), ALLOC_STACK);
    return userAgentPtr;
  },

  foundry_engine_device_get_platform: function() {
    const platform = navigator.platform;
    const platformPtr = allocate(intArrayFromString(platform), ALLOC_STACK);
    return platformPtr;
  },

  foundry_engine_device_get_language: function() {
    const language = navigator.language || navigator.userLanguage;
    const languagePtr = allocate(intArrayFromString(language), ALLOC_STACK);
    return languagePtr;
  },

  foundry_engine_device_get_cookie_enabled: function() {
    return navigator.cookieEnabled ? 1 : 0;
  },

  foundry_engine_device_get_online_status: function() {
    return navigator.onLine ? 1 : 0;
  },

  foundry_engine_device_get_screen_width: function() {
    return screen.width;
  },

  foundry_engine_device_get_screen_height: function() {
    return screen.height;
  },

  foundry_engine_device_get_screen_color_depth: function() {
    return screen.colorDepth;
  },

  foundry_engine_device_get_screen_pixel_depth: function() {
    return screen.pixelDepth;
  },

  // Performance monitoring functions
  foundry_engine_performance_get_memory_usage: function() {
    if ('memory' in performance) {
      return performance.memory.usedJSHeapSize;
    }
    return 0;
  },

  foundry_engine_performance_get_total_memory: function() {
    if ('memory' in performance) {
      return performance.memory.totalJSHeapSize;
    }
    return 0;
  },

  foundry_engine_performance_get_memory_limit: function() {
    if ('memory' in performance) {
      return performance.memory.jsHeapSizeLimit;
    }
    return 0;
  },

  foundry_engine_performance_get_timing: function() {
    const timing = performance.timing;
    const navigationStart = timing.navigationStart;

    const timingData = {
      navigationStart: navigationStart,
      unloadEventStart: timing.unloadEventStart - navigationStart,
      unloadEventEnd: timing.unloadEventEnd - navigationStart,
      redirectStart: timing.redirectStart - navigationStart,
      redirectEnd: timing.redirectEnd - navigationStart,
      fetchStart: timing.fetchStart - navigationStart,
      domainLookupStart: timing.domainLookupStart - navigationStart,
      domainLookupEnd: timing.domainLookupEnd - navigationStart,
      connectStart: timing.connectStart - navigationStart,
      connectEnd: timing.connectEnd - navigationStart,
      secureConnectionStart: timing.secureConnectionStart - navigationStart,
      requestStart: timing.requestStart - navigationStart,
      responseStart: timing.responseStart - navigationStart,
      responseEnd: timing.responseEnd - navigationStart,
      domLoading: timing.domLoading - navigationStart,
      domInteractive: timing.domInteractive - navigationStart,
      domContentLoadedEventStart: timing.domContentLoadedEventStart - navigationStart,
      domContentLoadedEventEnd: timing.domContentLoadedEventEnd - navigationStart,
      domComplete: timing.domComplete - navigationStart,
      loadEventStart: timing.loadEventStart - navigationStart,
      loadEventEnd: timing.loadEventEnd - navigationStart
    };

    const timingStr = JSON.stringify(timingData);
    const timingPtr = allocate(intArrayFromString(timingStr), ALLOC_STACK);
    return timingPtr;
  },

  foundry_engine_performance_mark: function(namePtr) {
    const name = UTF8ToString(namePtr);
    performance.mark(name);
    return 1;
  },

  foundry_engine_performance_measure: function(namePtr, startMarkPtr, endMarkPtr) {
    const name = UTF8ToString(namePtr);
    const startMark = UTF8ToString(startMarkPtr);
    const endMark = UTF8ToString(endMarkPtr);

    try {
      performance.measure(name, startMark, endMark);
      return 1;
    } catch (e) {
      console.error('Failed to create performance measure:', e);
      return 0;
    }
  },

  foundry_engine_performance_get_entries: function() {
    const entries = performance.getEntriesByType('measure');
    const entriesStr = JSON.stringify(entries);
    const entriesPtr = allocate(intArrayFromString(entriesStr), ALLOC_STACK);
    return entriesPtr;
  }
});

// Auto-initialize when the module loads
Module['onRuntimeInitialized'] = function() {
  console.log('FoundryEngine WebAssembly runtime initialized');

  // Initialize the Web platform
  if (_foundry_engine_web_init && _foundry_engine_web_init()) {
    console.log('FoundryEngine Web platform initialized successfully');

    // Start the main loop
    if (_foundry_engine_web_start_main_loop) {
      _foundry_engine_web_start_main_loop();
    }
  } else {
    console.error('Failed to initialize FoundryEngine Web platform');
  }
};

// Cleanup when the module is destroyed
Module['onExit'] = function() {
  console.log('FoundryEngine WebAssembly runtime exiting');

  // Stop the main loop
  if (_foundry_engine_web_stop_main_loop) {
    _foundry_engine_web_stop_main_loop();
  }
};

console.log('FoundryEngine Web Library loaded successfully!');
