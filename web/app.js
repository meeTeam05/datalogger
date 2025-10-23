// Initialize Feather Icons
feather.replace();

// ====================================================================
// GLOBAL VARIABLES (from script.js)
// ====================================================================
let isPeriodic = false;
let isDeviceOn = false;
let isMqttConnected = false;
let isFirebaseConnected = false;
let temperatureData = [];
let humidityData = [];
let logBuffer = [];
let maxDataPoints = 50;
let maxLogsInMemory = 100;
let mqttClient = null;
let firebaseDb = null;
let currentTemp = null;
let currentHumi = null;
// Last reading timestamp (ms since epoch) sourced from device payload
let lastReadingTimestamp = null;

let logFilterType = "ALL";

// Data Management cache (MUST be declared BEFORE any functions that use it)
let filteredDataCache = [];

// Live Data table
let liveDataBuffer = [];
const maxLiveDataEntries = 100;

// Component health tracking
const componentHealth = {
  SHT31: {
    healthy: false,
    lastUpdate: null,
    lastTemp: null,
    lastHumi: null,
    errorCount: 0,
  },
  DS3231: {
    healthy: false,
    lastSync: null,
    failCount: 0,
  },
};

// Time picker state
let timePickerYear = new Date().getFullYear();
let timePickerMonth = new Date().getMonth();
let timePickerDay = new Date().getDate();
let timePickerHour = new Date().getHours();
let timePickerMinute = new Date().getMinutes();
let timePickerSecond = new Date().getSeconds();

// Device/system time emulation (updated after SET TIME)
// deviceClockMs: epoch ms set on device; deviceClockSetAtMs: local time when we set it
// Displayed time = deviceClockMs + (Date.now() - deviceClockSetAtMs)
let deviceClockMs = null;
let deviceClockSetAtMs = null;
// Interval ID for the footer System Time ticker; must be declared before any usage to avoid TDZ
let clockIntervalId = null;

// Interval picker state
let selectedMinute = 0;
let selectedSecond = 5;

// State synchronization management
let stateSync = {
  lastKnownState: null,
  syncInProgress: false,
  syncRequested: false,
  syncRetryCount: 0,
  maxSyncRetries: 1,
  deviceOffLock: false,
  deviceOffLockTimeout: null,
  lastSyncMessage: "",
  ignoreNextPeriodicSync: false,
};

// Firebase Configuration
let FIREBASE_CONFIG = {
  apiKey: "AIzaSyAxEhTb1cNHTwmVWh4vbpA5MZSF0Vf0l0U",
  databaseURL: "https://datalogger-8c5d5-default-rtdb.firebaseio.com/",
  projectId: "datalogger-8c5d5",
};

// MQTT Configuration
const MQTT_CONFIG = {
  host: "127.0.0.1",
  port: 8083,
  path: "/mqtt",
  username: "DataLogger",
  password: "datalogger",
  clientId: "web_client_1",
  url: "ws://127.0.0.1:8083/mqtt",
  topics: {
    stm32Command: "datalogger/stm32/command",
    relayControl: "datalogger/esp32/relay/control",
    systemState: "datalogger/esp32/system/state",
    singleData: "datalogger/stm32/single/data",
    periodicData: "datalogger/stm32/periodic/data",
  },
};

// Charts
let tempChart, humiChart;
let chartInitRetryCount = 0;
const maxChartInitRetries = 10;

// ====================================================================
// NAVIGATION
// ====================================================================
document.querySelectorAll(".menu-item").forEach((item) => {
  item.addEventListener("click", function () {
    const page = this.dataset.page;

    // Update active menu
    document
      .querySelectorAll(".menu-item")
      .forEach((m) => m.classList.remove("active"));
    this.classList.add("active");

    // Update active page
    document
      .querySelectorAll(".page-section")
      .forEach((p) => p.classList.remove("active"));
    document.getElementById("page-" + page).classList.add("active");

    // Initialize page-specific components
    if (page === "time") {
      renderCalendar();
      updateTimeDisplay();
      bindCalendarButtons();
      // Ensure time buttons are bound when user navigates here
      if (typeof attachTimeButtons === "function") attachTimeButtons();
    } else if (page === "livedata") {
      renderLiveDataTable();
      bindLiveDataButtons();
    } else if (page === "data") {
      bindDataManagementButtons();
      // Auto-load data when opening page
      applyDataFilters();
    } else if (page === "logs") {
      bindLogsButtons();
      renderFullConsole();
    } else if (page === "config") {
      loadSettingsPage();
    }

    // Re-initialize feather icons
    if (typeof feather !== "undefined") feather.replace();
  });
});

// ====================================================================
// LOGGING SYSTEM
// ====================================================================
function addStatus(message, type = "INFO") {
  const timestamp = Date.now();
  const logEntry = {
    type: type,
    message: message,
    timestamp: timestamp,
    date_iso: new Date(timestamp).toISOString(),
  };

  // Add to buffer (FIFO)
  logBuffer.unshift(logEntry);
  if (logBuffer.length > maxLogsInMemory) {
    logBuffer.pop();
  }

  // Update console displays
  updateConsoleDisplay(logEntry);

  console.log(`[${type}] ${message}`);
}

function updateConsoleDisplay(logEntry) {
  const time = new Date(logEntry.timestamp).toLocaleTimeString("en-US", {
    hour12: false,
  });
  const line = `<div class="log-line log-${logEntry.type.toLowerCase()}">[${
    logEntry.type
  }] ${time}: ${logEntry.message}</div>`;

  // Update preview (last 5)
  const preview = document.getElementById("consolePreview");
  if (preview) {
    preview.innerHTML = line + preview.innerHTML;
    const lines = preview.querySelectorAll(".log-line");
    if (lines.length > 5) {
      lines[lines.length - 1].remove();
    }
  }

  // Update full logs if on logs page
  const logsSection = document.getElementById("page-logs");
  if (logsSection && logsSection.classList.contains("active")) {
    renderFullConsole();
  }
}

// ====================================================================
// FIREBASE INITIALIZATION
// ====================================================================
function initializeFirebase() {
  if (
    !FIREBASE_CONFIG.apiKey ||
    !FIREBASE_CONFIG.databaseURL ||
    !FIREBASE_CONFIG.projectId
  ) {
    addStatus(
      " Firebase configuration incomplete - skipping initialization",
      "WARNING"
    );
    addStatus(
      `   API Key: ${FIREBASE_CONFIG.apiKey ? " Set" : " Missing"}`,
      "INFO"
    );
    addStatus(
      `   Database URL: ${FIREBASE_CONFIG.databaseURL ? " Set" : " Missing"}`,
      "INFO"
    );
    addStatus(
      `   Project ID: ${FIREBASE_CONFIG.projectId ? " Set" : " Missing"}`,
      "INFO"
    );
    updateFirebaseStatus(false);
    return false;
  }

  addStatus(" Initializing Firebase...", "FIREBASE");
  addStatus(`   Project: ${FIREBASE_CONFIG.projectId}`, "INFO");
  addStatus(`   Database: ${FIREBASE_CONFIG.databaseURL}`, "INFO");

  try {
    // Remove existing app if present (for reconnect)
    if (firebase.apps.length > 0) {
      addStatus("Removing existing Firebase app...", "INFO");
      firebase
        .app()
        .delete()
        .then(() => {
          addStatus("Existing Firebase app removed", "INFO");
        });
    }

    firebase.initializeApp(FIREBASE_CONFIG);
    firebaseDb = firebase.database();

    addStatus("Firebase app initialized", "FIREBASE");

    firebaseDb.ref(".info/connected").on("value", function (snapshot) {
      if (snapshot.val() === true) {
        addStatus(" Firebase connected - testing permissions...", "FIREBASE");

        firebaseDb
          .ref("test/connection")
          .set({
            timestamp: Date.now(),
            message: "Connection test",
          })
          .then(() => {
            updateFirebaseStatus(true);
            addStatus(" Firebase write permissions verified", "FIREBASE");
            firebaseDb.ref("test").remove();
          })
          .catch((error) => {
            updateFirebaseStatus(false);
            addStatus(` Firebase permission error: ${error.code}`, "ERROR");
            addStatus(`   Message: ${error.message}`, "ERROR");
            addStatus(`   Check Firebase Rules and authentication`, "ERROR");
          });
      } else {
        updateFirebaseStatus(false);
        addStatus(" Firebase connection lost", "WARNING");
      }
    });

    return true;
  } catch (error) {
    addStatus(` Firebase init exception: ${error.message}`, "ERROR");
    addStatus(`   Stack: ${error.stack}`, "ERROR");
    updateFirebaseStatus(false);
    return false;
  }
}

function updateFirebaseStatus(connected) {
  isFirebaseConnected = connected;
  const badge = document.getElementById("firebaseStatus");
  const dot = document.getElementById("firebaseDot");
  const text = document.getElementById("firebaseText");

  if (connected) {
    badge.className = "status-badge connected";
    dot.className = "status-dot connected";
    text.textContent = "Firebase Connected";
    addStatus("Firebase database connected", "FIREBASE");
  } else {
    badge.className = "status-badge disconnected";
    dot.className = "status-dot disconnected";
    text.textContent = "Firebase Disconnected";
    addStatus("Firebase database disconnected", "FIREBASE");
  }
}

// ====================================================================
// MQTT INITIALIZATION
// ====================================================================
function initializeMQTT() {
  const url = `ws://${MQTT_CONFIG.host}:${MQTT_CONFIG.port}${MQTT_CONFIG.path}`;

  addStatus(` Connecting to MQTT broker...`, "MQTT");
  addStatus(`   Host: ${MQTT_CONFIG.host}`, "INFO");
  addStatus(`   Port: ${MQTT_CONFIG.port}`, "INFO");
  addStatus(`   Path: ${MQTT_CONFIG.path}`, "INFO");
  addStatus(`   Client ID: ${MQTT_CONFIG.clientId}`, "INFO");
  addStatus(`   Full URL: ${url}`, "INFO");

  try {
    mqttClient = mqtt.connect(url, {
      clientId: MQTT_CONFIG.clientId,
      username: MQTT_CONFIG.username,
      password: MQTT_CONFIG.password,
      clean: true,
      connectTimeout: 4000,
      reconnectPeriod: 2000,
    });

    mqttClient.on("connect", function () {
      updateMQTTStatus(true);
      addStatus(" MQTT broker connected successfully!", "MQTT");

      // Subscribe to topics
      const topics = [
        MQTT_CONFIG.topics.systemState,
        MQTT_CONFIG.topics.singleData,
        MQTT_CONFIG.topics.periodicData,
      ];

      topics.forEach((topic) => {
        mqttClient.subscribe(topic, { qos: 1 }, (err) => {
          if (err) {
            addStatus(
              ` Failed to subscribe to ${topic}: ${err.message}`,
              "ERROR"
            );
          } else {
            addStatus(` Subscribed to ${topic}`, "MQTT");
          }
        });
      });

      // Request state sync
      setTimeout(() => {
        requestStateSync();
      }, 500);
    });

    mqttClient.on("message", handleMQTTMessage);

    mqttClient.on("error", function (error) {
      addStatus(` MQTT error: ${error.message}`, "ERROR");
      addStatus(
        `   Check if broker is running at ${MQTT_CONFIG.host}:${MQTT_CONFIG.port}`,
        "ERROR"
      );
      addStatus(
        `   Verify network connectivity and firewall settings`,
        "ERROR"
      );
      updateMQTTStatus(false);
    });

    mqttClient.on("offline", function () {
      addStatus(" MQTT client offline - attempting reconnect...", "WARNING");
      updateMQTTStatus(false);
    });

    mqttClient.on("reconnect", function () {
      addStatus(" MQTT reconnecting...", "MQTT");
    });

    mqttClient.on("close", function () {
      updateMQTTStatus(false);
      addStatus(" MQTT connection closed", "MQTT");
    });
  } catch (error) {
    addStatus(` MQTT init exception: ${error.message}`, "ERROR");
    addStatus(`   Stack: ${error.stack}`, "ERROR");
    updateMQTTStatus(false);
  }
}

function updateMQTTStatus(connected) {
  isMqttConnected = connected;
  const badge = document.getElementById("mqttStatus");
  const dot = document.getElementById("mqttDot");
  const text = document.getElementById("mqttText");

  if (connected) {
    badge.className = "status-badge connected";
    dot.className = "status-dot connected";
    text.textContent = "MQTT Connected";
  } else {
    badge.className = "status-badge disconnected";
    dot.className = "status-dot disconnected";
    text.textContent = "MQTT Disconnected";
  }
}

function handleMQTTMessage(topic, payload) {
  const text = payload.toString();
  console.log(
    `[MQTT] Message received - Topic: ${topic}, Payload: ${text.substring(
      0,
      100
    )}`
  );

  // System state sync
  if (topic === MQTT_CONFIG.topics.systemState) {
    const parsedState = parseStateMessage(text);
    if (parsedState) {
      syncUIWithHardwareState(parsedState);
    }
    return;
  }

  // Data topics
  if (
    topic === MQTT_CONFIG.topics.singleData ||
    topic === MQTT_CONFIG.topics.periodicData
  ) {
    try {
      const jsonData = JSON.parse(text);
      const timestamp =
        jsonData.timestamp * 1000 - 7 * 3600 * 1000 || Date.now();
      // Derive mode from topic to avoid unreliable payloads
      const isPeriodicTopic = topic === MQTT_CONFIG.topics.periodicData;
      const isSingleTopic = topic === MQTT_CONFIG.topics.singleData;
      const mode = isPeriodicTopic ? "periodic" : "single";

      // Only treat periodic messages when user actually enabled PERIODIC and device is ON
      const isActiveMeasurement =
        isSingleTopic || (isPeriodicTopic && isDeviceOn && isPeriodic);

      if (isPeriodicTopic && !isActiveMeasurement) {
        console.log(
          `[DATA] Ignored periodic message (deviceOn=${isDeviceOn}, isPeriodic=${isPeriodic})`
        );
        return; // ignore completely: no warnings, no firebase, no charts
      }

      // Check for sensor/RTC failures
      const sensorFailed =
        jsonData.temperature === 0.0 && jsonData.humidity === 0.0;
      const rtcFailed = jsonData.timestamp === 0;

      // Only surface warnings when measurement is active
      if (isActiveMeasurement) {
        if (sensorFailed) {
          addStatus(
            "SHT31 sensor hardware failed (disconnected or wiring issue)",
            "ERROR"
          );
          addStatus("Sensor hardware failed (disconnected)", "WARNING");
        }
        if (rtcFailed) {
          addStatus(
            "DS3231 RTC module failed (time sync error or I2C communication failure)",
            "ERROR"
          );
          addStatus("RTC failed (using local time)", "WARNING");
        }
      }

      // Update current values only when active
      if (isActiveMeasurement) {
        currentTemp = jsonData.temperature;
        currentHumi = jsonData.humidity;
        lastReadingTimestamp = timestamp;
        updateCurrentDisplay();
      }

      // Save to Firebase only when active
      if (isActiveMeasurement) {
        saveToFirebaseSimple({
          temp: jsonData.temperature,
          humi: jsonData.humidity,
          mode: mode,
          sensor: "SHT31",
          time: jsonData.timestamp || 0,
          device: "ESP32_01",
        });
      } else {
        console.log("[DATA] Skipped Firebase save (inactive)");
      }

      // Update charts only when active and valid values
      if (isPeriodicTopic && isActiveMeasurement && !sensorFailed) {
        console.log(
          `[CHART] Pushing data: temp=${jsonData.temperature}, humi=${
            jsonData.humidity
          }, time=${new Date(timestamp).toLocaleTimeString()}`
        );
        pushTemperature(jsonData.temperature, true, timestamp);
        pushHumidity(jsonData.humidity, true, timestamp);
        console.log(
          `[CHART] Data pushed. TempData length: ${temperatureData.length}, HumiData length: ${humidityData.length}`
        );
      } else {
        console.log(
          `[CHART] Skipped: mode=${mode}, sensorFailed=${sensorFailed}`
        );
      }

      // Update component health
      if (isActiveMeasurement) {
        updateComponentHealth("SHT31", !sensorFailed);
        updateComponentHealth("DS3231", !rtcFailed);
      }

      // Add to Live Data table only when active
      if (isActiveMeasurement) {
        addToLiveDataTable({
          time: timestamp,
          temp: jsonData.temperature,
          humi: jsonData.humidity,
          mode: mode,
          status: sensorFailed || rtcFailed ? "error" : "success",
        });
      }
    } catch (e) {
      addStatus(`JSON parse error: ${e.message}`, "ERROR");
    }
  }
}

// ====================================================================
// STATE SYNCHRONIZATION (Keep original logic)
// ====================================================================
function requestStateSync() {
  if (!isMqttConnected || !mqttClient) return;

  addStatus("Requesting state sync...", "SYNC");
  mqttClient.publish(MQTT_CONFIG.topics.systemState, "REQUEST", { qos: 1 });
}

function parseStateMessage(message) {
  try {
    // Try JSON format first (from ESP32)
    const state = JSON.parse(message);
    return {
      device: state.device === "ON",
      periodic: state.periodic === "ON",
    };
  } catch (e) {
    // Fallback to text format
    const deviceMatch = message.match(/device[:\s]+(ON|OFF)/i);
    const periodicMatch = message.match(/periodic[:\s]+(ON|OFF)/i);

    if (deviceMatch || periodicMatch) {
      return {
        device: deviceMatch
          ? deviceMatch[1].toUpperCase() === "ON"
          : isDeviceOn,
        periodic: periodicMatch
          ? periodicMatch[1].toUpperCase() === "ON"
          : isPeriodic,
      };
    }
  }

  return null;
}

function syncUIWithHardwareState(state) {
  isDeviceOn = state.device;
  isPeriodic = state.periodic;

  // Update Device button - show CURRENT state
  const deviceBtn = document.getElementById("deviceBtn");
  const deviceBtnText = document.getElementById("deviceBtnText");
  if (isDeviceOn) {
    deviceBtn.className = "btn btn-success";
    deviceBtnText.textContent = "Device ON";
  } else {
    deviceBtn.className = "btn btn-secondary";
    deviceBtnText.textContent = "Device OFF";
  }

  // Update Periodic button - show CURRENT state
  const periodicBtn = document.getElementById("periodicBtn");
  const periodicBtnText = document.getElementById("periodicBtnText");
  if (isPeriodic) {
    periodicBtn.className = "btn btn-success";
    periodicBtnText.textContent = "Periodic ON";
  } else {
    periodicBtn.className = "btn btn-secondary";
    periodicBtnText.textContent = "Periodic OFF";
  }

  addStatus(
    `State synced: Device=${state.device ? "ON" : "OFF"}, Periodic=${
      state.periodic ? "ON" : "OFF"
    }`,
    "SYNC"
  );
}

// ====================================================================
// CURRENT DISPLAY
// ====================================================================
function updateCurrentDisplay() {
  document.getElementById("currentTemp").textContent =
    currentTemp !== null ? currentTemp.toFixed(1) : "--";
  document.getElementById("currentHumi").textContent =
    currentHumi !== null ? currentHumi.toFixed(1) : "--";

  // Prefer the device-provided timestamp if available
  const timeStr = lastReadingTimestamp
    ? new Date(lastReadingTimestamp).toLocaleTimeString("en-US", {
        hour12: false,
      })
    : "--:--:--";
  document.getElementById("lastUpdate").textContent = `Updated at ${timeStr}`;
}

// ====================================================================
// FIREBASE SAVE (New structure)
// ====================================================================
function saveToFirebaseSimple(data) {
  if (!isFirebaseConnected || !firebaseDb) return;

  // Use device timestamp to create date key, not browser time
  const deviceTimestamp = data.time
    ? (data.time - 7 * 3600) * 1000
    : Date.now();
  const dateStr = new Date(deviceTimestamp).toISOString().split("T")[0];
  const id = Date.now().toString();

  const record = {
    temp: data.temp ?? null,
    humi: data.humi ?? null,
    mode: data.mode ?? "single",
    sensor: data.sensor ?? "SHT31",
    status: data.temp === 0 || data.humi === 0 ? "error" : "success",
    error:
      data.temp === 0 && data.humi === 0
        ? "sensor_fail"
        : data.time === 0
        ? "rtc_fail"
        : null,
    time: data.time ? data.time - 7 * 3600 : Math.floor(Date.now() / 1000),
    device: data.device ?? "ESP32_01",
    created_at: Date.now(),
  };

  firebaseDb
    .ref(`readings/${dateStr}/${id}`)
    .set(record)
    .then(() => {
      addStatus(
        `Firebase saved: ${record.sensor} (${record.status})`,
        "FIREBASE"
      );
    })
    .catch((err) => {
      addStatus(`Firebase error: ${err.message}`, "ERROR");
    });
}

// ====================================================================
// CHARTS
// ====================================================================
// Helper to (re)try chart initialization on-demand
function ensureChartsInitialized() {
  if (tempChart && humiChart) return; // already ready
  console.warn(
    "[CHART] ensureChartsInitialized(): charts not ready, invoking initializeCharts()"
  );
  try {
    initializeCharts();
  } catch (e) {
    console.error("[CHART] ensureChartsInitialized() failed:", e);
  }
}

function initializeCharts() {
  console.log(
    `[CHART] Attempting to initialize charts (attempt ${
      chartInitRetryCount + 1
    }/${maxChartInitRetries})...`
  );

  const tempCanvas = document.getElementById("tempChart");
  const humiCanvas = document.getElementById("humiChart");

  console.log("[CHART] Canvas elements:", {
    tempCanvas: !!tempCanvas,
    humiCanvas: !!humiCanvas,
  });

  if (!tempCanvas || !humiCanvas) {
    chartInitRetryCount++;
    if (chartInitRetryCount < maxChartInitRetries) {
      console.warn(
        `[CHART] Canvas elements not found! Retrying in 500ms... (${chartInitRetryCount}/${maxChartInitRetries})`
      );
      setTimeout(initializeCharts, 500);
    } else {
      console.error(
        "[CHART] FAILED: Canvas elements not found after maximum retries!"
      );
      addStatus("ERROR: Chart canvas elements not found", "ERROR");
    }
    return;
  }

  const tempCtx = tempCanvas.getContext("2d");
  const humiCtx = humiCanvas.getContext("2d");

  console.log("[CHART] Canvas contexts:", {
    tempCtx: !!tempCtx,
    humiCtx: !!humiCtx,
  });

  if (!tempCtx || !humiCtx) {
    chartInitRetryCount++;
    if (chartInitRetryCount < maxChartInitRetries) {
      console.warn(
        `[CHART] Cannot get 2D context! Retrying in 500ms... (${chartInitRetryCount}/${maxChartInitRetries})`
      );
      setTimeout(initializeCharts, 500);
    } else {
      console.error(
        "[CHART] FAILED: Cannot get 2D context after maximum retries!"
      );
      addStatus("ERROR: Cannot get chart 2D context", "ERROR");
    }
    return;
  }

  // Check if Chart.js is loaded
  if (typeof Chart === "undefined") {
    console.error("[CHART] Chart.js library is not loaded!");
    addStatus("ERROR: Chart.js library not loaded", "ERROR");
    return;
  }

  console.log("[CHART] Starting chart initialization with Chart.js...");

  try {
    tempChart = new Chart(tempCtx, {
      type: "line",
      data: {
        labels: [],
        datasets: [
          {
            label: "Temperature (C)",
            data: [],
            borderColor: "#06B6D4",
            backgroundColor: "rgba(6, 182, 212, 0.1)",
            tension: 0.4,
            pointRadius: 2,
            pointHoverRadius: 6,
            borderWidth: 2,
            fill: true,
            pointBackgroundColor: "#06B6D4",
            pointBorderColor: "white",
            pointBorderWidth: 1,
          },
        ],
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        interaction: {
          intersect: false,
          mode: "index",
        },
        plugins: {
          legend: {
            display: false,
          },
          tooltip: {
            backgroundColor: "rgba(6, 182, 212, 0.9)",
            titleColor: "white",
            bodyColor: "white",
            cornerRadius: 8,
            displayColors: false,
            callbacks: {
              label: function (context) {
                return `Temperature: ${context.parsed.y.toFixed(1)}C`;
              },
            },
          },
        },
        scales: {
          y: {
            beginAtZero: false,
            suggestedMin: 15,
            suggestedMax: 40,
            grid: {
              color: "rgba(6, 182, 212, 0.1)",
            },
            ticks: {
              callback: function (value) {
                return value.toFixed(1) + "C";
              },
            },
          },
          x: {
            grid: {
              color: "rgba(6, 182, 212, 0.1)",
            },
            ticks: {
              maxTicksLimit: 8,
            },
          },
        },
        animation: {
          duration: 750,
          easing: "easeInOutQuart",
        },
      },
    });

    humiChart = new Chart(humiCtx, {
      type: "line",
      data: {
        labels: [],
        datasets: [
          {
            label: "Humidity (%)",
            data: [],
            borderColor: "#22D3EE",
            backgroundColor: "rgba(34, 211, 238, 0.1)",
            tension: 0.4,
            pointRadius: 2,
            pointHoverRadius: 6,
            borderWidth: 2,
            fill: true,
            pointBackgroundColor: "#22D3EE",
            pointBorderColor: "white",
            pointBorderWidth: 1,
          },
        ],
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        interaction: {
          intersect: false,
          mode: "index",
        },
        plugins: {
          legend: {
            display: false,
          },
          tooltip: {
            backgroundColor: "rgba(34, 211, 238, 0.9)",
            titleColor: "white",
            bodyColor: "white",
            cornerRadius: 8,
            displayColors: false,
            callbacks: {
              label: function (context) {
                return `Humidity: ${context.parsed.y.toFixed(1)}%`;
              },
            },
          },
        },
        scales: {
          y: {
            beginAtZero: false,
            suggestedMin: 40,
            suggestedMax: 90,
            grid: {
              color: "rgba(34, 211, 238, 0.1)",
            },
            ticks: {
              callback: function (value) {
                return value.toFixed(1) + "%";
              },
            },
          },
          x: {
            grid: {
              color: "rgba(34, 211, 238, 0.1)",
            },
            ticks: {
              maxTicksLimit: 8,
            },
          },
        },
        animation: {
          duration: 750,
          easing: "easeInOutQuart",
        },
      },
    });

    console.log("[CHART] Charts initialized successfully", {
      tempChart: !!tempChart,
      humiChart: !!humiChart,
      existingTempData: temperatureData.length,
      existingHumiData: humidityData.length,
    });

    // If there's existing data, render it now
    if (temperatureData.length > 0) {
      console.log("[CHART] Rendering existing temperature data...");
      tempChart.data.labels = temperatureData.map((d) => d.time);
      tempChart.data.datasets[0].data = temperatureData.map((d) => d.value);
      tempChart.update("none");
      updateChartStats("temp");
    }

    if (humidityData.length > 0) {
      console.log("[CHART] Rendering existing humidity data...");
      humiChart.data.labels = humidityData.map((d) => d.time);
      humiChart.data.datasets[0].data = humidityData.map((d) => d.value);
      humiChart.update("none");
      updateChartStats("humi");
    }

    addStatus("Charts initialized successfully", "INFO");
  } catch (error) {
    console.error("[CHART] Error initializing charts:", error);
    addStatus(`Chart initialization error: ${error.message}`, "ERROR");
  }
}

function pushTemperature(value, update = true, timestamp = null) {
  // Lazy init in case initial boot missed DOM timing
  if (!tempChart || !humiChart) ensureChartsInitialized();
  const time = timestamp
    ? new Date(timestamp).toLocaleTimeString("en-US", { hour12: false })
    : new Date().toLocaleTimeString("en-US", { hour12: false });

  temperatureData.push({ value, time });

  if (temperatureData.length > maxDataPoints) {
    temperatureData.shift();
  }

  console.log(
    `[pushTemperature] Value: ${value}, Update: ${update}, TempChart exists: ${!!tempChart}, Array length: ${
      temperatureData.length
    }`
  );

  if (update) {
    if (tempChart) {
      tempChart.data.labels = temperatureData.map((d) => d.time);
      tempChart.data.datasets[0].data = temperatureData.map((d) => d.value);
      tempChart.update("active");
      updateChartStats("temp");
      console.log(
        `[pushTemperature] Chart updated with ${temperatureData.length} points`
      );
    } else {
      console.warn(
        "[pushTemperature] TempChart not initialized yet - data saved, will render when ready"
      );
    }
  }
}

function pushHumidity(value, update = true, timestamp = null) {
  // Lazy init in case initial boot missed DOM timing
  if (!tempChart || !humiChart) ensureChartsInitialized();
  const time = timestamp
    ? new Date(timestamp).toLocaleTimeString("en-US", { hour12: false })
    : new Date().toLocaleTimeString("en-US", { hour12: false });

  humidityData.push({ value, time });

  if (humidityData.length > maxDataPoints) {
    humidityData.shift();
  }

  console.log(
    `[pushHumidity] Value: ${value}, Update: ${update}, HumiChart exists: ${!!humiChart}, Array length: ${
      humidityData.length
    }`
  );

  if (update) {
    if (humiChart) {
      humiChart.data.labels = humidityData.map((d) => d.time);
      humiChart.data.datasets[0].data = humidityData.map((d) => d.value);
      humiChart.update("active");
      updateChartStats("humi");
      console.log(
        `[pushHumidity] Chart updated with ${humidityData.length} points`
      );
    } else {
      console.warn(
        "[pushHumidity] HumiChart not initialized yet - data saved, will render when ready"
      );
    }
  }
}

function updateChartStats(type) {
  const data = type === "temp" ? temperatureData : humidityData;
  if (data.length === 0) return;

  const values = data.map((d) => d.value);
  const min = Math.min(...values);
  const max = Math.max(...values);
  const avg = values.reduce((a, b) => a + b, 0) / values.length;

  document.getElementById(`${type}Min`).textContent = min.toFixed(1);
  document.getElementById(`${type}Max`).textContent = max.toFixed(1);
  document.getElementById(`${type}Avg`).textContent = avg.toFixed(1);
}

function clearChartData() {
  temperatureData = [];
  humidityData = [];

  if (tempChart) {
    tempChart.data.labels = [];
    tempChart.data.datasets[0].data = [];
    tempChart.update();
  }

  if (humiChart) {
    humiChart.data.labels = [];
    humiChart.data.datasets[0].data = [];
    humiChart.update();
  }

  document.getElementById("tempMin").textContent = "--";
  document.getElementById("tempMax").textContent = "--";
  document.getElementById("tempAvg").textContent = "--";
  document.getElementById("humiMin").textContent = "--";
  document.getElementById("humiMax").textContent = "--";
  document.getElementById("humiAvg").textContent = "--";

  addStatus("Charts cleared", "INFO");
}

// ====================================================================
// BUTTON HANDLERS
// ====================================================================
// ====================================================================
// BUTTON EVENT LISTENERS
// ====================================================================
document.getElementById("deviceBtn").addEventListener("click", function () {
  if (!isMqttConnected) {
    addStatus("MQTT not connected", "ERROR");
    return;
  }

  // Toggle device state
  const command = isDeviceOn ? "RELAY OFF" : "RELAY ON";
  const willBeDeviceOn = !isDeviceOn;

  // Publish MQTT command
  mqttClient.publish(MQTT_CONFIG.topics.relayControl, command, { qos: 1 });

  // Update state immediately
  isDeviceOn = willBeDeviceOn;

  // Update UI - show CURRENT state
  const deviceBtn = document.getElementById("deviceBtn");
  const deviceBtnText = document.getElementById("deviceBtnText");
  if (isDeviceOn) {
    deviceBtn.className = "btn btn-success";
    deviceBtnText.textContent = "Device ON";
  } else {
    deviceBtn.className = "btn btn-secondary";
    deviceBtnText.textContent = "Device OFF";
  }

  // If turning OFF, stop periodic mode immediately
  if (!isDeviceOn && isPeriodic) {
    isPeriodic = false;
    const periodicBtn = document.getElementById("periodicBtn");
    const periodicBtnText = document.getElementById("periodicBtnText");
    if (periodicBtn) periodicBtn.className = "btn btn-secondary";
    if (periodicBtnText) periodicBtnText.textContent = "Periodic OFF";

    mqttClient.publish(MQTT_CONFIG.topics.stm32Command, "PERIODIC OFF", {
      qos: 1,
    });
    addStatus("Periodic mode stopped (device OFF)", "SYNC");
  }

  // Reset current values when device OFF
  if (!isDeviceOn) {
    currentTemp = null;
    currentHumi = null;
    document.getElementById("currentTemp").textContent = "--";
    document.getElementById("currentHumi").textContent = "--";
    document.getElementById("lastUpdate").textContent = "Updated at --:--:--";
  }

  addStatus(`Device ${command} sent`, "MQTT");
});

document.getElementById("periodicBtn").addEventListener("click", function () {
  if (!isMqttConnected) {
    addStatus("MQTT not connected", "ERROR");
    return;
  }

  if (!isDeviceOn) {
    addStatus("Device must be ON first", "WARNING");
    return;
  }

  // Toggle periodic state
  const willBePeriodic = !isPeriodic;
  const command = willBePeriodic ? "PERIODIC ON" : "PERIODIC OFF";

  // Publish MQTT command
  mqttClient.publish(MQTT_CONFIG.topics.stm32Command, command, { qos: 1 });

  // Update state immediately
  isPeriodic = willBePeriodic;

  // Update UI - show CURRENT state
  const periodicBtn = document.getElementById("periodicBtn");
  const periodicBtnText = document.getElementById("periodicBtnText");
  if (isPeriodic) {
    periodicBtn.className = "btn btn-success";
    periodicBtnText.textContent = "Periodic ON";
  } else {
    periodicBtn.className = "btn btn-secondary";
    periodicBtnText.textContent = "Periodic OFF";
  }

  addStatus(`Periodic ${isPeriodic ? "ON" : "OFF"} command sent`, "MQTT");
});

document.getElementById("singleBtn").addEventListener("click", function () {
  if (!isMqttConnected) {
    addStatus("MQTT not connected", "ERROR");
    return;
  }

  if (!isDeviceOn) {
    addStatus("Device must be ON first", "WARNING");
    return;
  }

  mqttClient.publish(MQTT_CONFIG.topics.stm32Command, "SINGLE", { qos: 1 });
  addStatus("Single read command sent", "MQTT");
});

document.getElementById("intervalBtn").addEventListener("click", function () {
  if (!isMqttConnected) {
    addStatus("MQTT not connected", "ERROR");
    return;
  }

  if (!isDeviceOn) {
    addStatus("Device must be ON first", "WARNING");
    return;
  }

  openIntervalModal();
});

document
  .getElementById("loadHistoryBtn")
  .addEventListener("click", function () {
    loadHistoryFromFirebase();
  });

function loadHistoryFromFirebase() {
  if (!isFirebaseConnected || !firebaseDb) {
    addStatus("Firebase not connected. Cannot load history.", "ERROR");
    return;
  }

  addStatus("Loading history from Firebase...", "INFO");

  // Get last 24 hours of data
  const now = new Date();
  const yesterday = new Date(now.getTime() - 24 * 60 * 60 * 1000);

  const dateRange = [];
  let currentDate = new Date(yesterday);
  while (currentDate <= now) {
    const dateStr = currentDate.toISOString().split("T")[0];
    dateRange.push(dateStr);
    currentDate.setDate(currentDate.getDate() + 1);
  }

  let totalRecords = 0;
  const promises = dateRange.map((date) => {
    return firebaseDb
      .ref(`readings/${date}`)
      .once("value")
      .then((snapshot) => {
        const dayData = snapshot.val() || {};
        return Object.entries(dayData).map(([id, record]) => ({
          id,
          date,
          ...record,
        }));
      })
      .catch((err) => {
        console.error(`Error loading ${date}:`, err);
        return [];
      });
  });

  Promise.all(promises)
    .then((results) => {
      const allData = results.flat();
      totalRecords = allData.length;

      if (totalRecords === 0) {
        addStatus("No historical data found in last 24 hours", "WARNING");
        return;
      }

      // Sort by timestamp (newest first)
      allData.sort((a, b) => b.timestamp - a.timestamp);

      // Take last 50 points for charts
      const recentData = allData.slice(0, 50).reverse();

      // Clear existing chart data
      temperatureData = [];
      humidityData = [];

      // Populate charts
      recentData.forEach((record) => {
        if (record.temp !== undefined && record.humi !== undefined) {
          pushTemperature(record.temp, false, record.timestamp * 1000);
          pushHumidity(record.humi, false, record.timestamp * 1000);
        }
      });

      // Update charts
      if (tempChart) {
        tempChart.data.labels = temperatureData.map((d) => d.time);
        tempChart.data.datasets[0].data = temperatureData.map((d) => d.value);
        tempChart.update("active");
        updateChartStats("temp");
      }

      if (humiChart) {
        humiChart.data.labels = humidityData.map((d) => d.time);
        humiChart.data.datasets[0].data = humidityData.map((d) => d.value);
        humiChart.update("active");
        updateChartStats("humi");
      }

      addStatus(
        `Loaded ${totalRecords} records (last 24h), showing ${recentData.length} on charts`,
        "INFO"
      );
    })
    .catch((err) => {
      addStatus(`Firebase load error: ${err.message}`, "ERROR");
    });
}

document
  .getElementById("clearChartsBtn")
  .addEventListener("click", function () {
    clearChartData();
  });

// ====================================================================
// LIVE DATA TABLE
// ====================================================================
function addToLiveDataTable(data) {
  liveDataBuffer.unshift(data);

  if (liveDataBuffer.length > maxLiveDataEntries) {
    liveDataBuffer.pop();
  }

  renderLiveDataTable();
}

function renderLiveDataTable() {
  const tbody = document.getElementById("liveDataTable");
  const filter = document.getElementById("liveDataFilter")?.value || "all";

  let filtered = liveDataBuffer;
  if (filter === "success") {
    filtered = liveDataBuffer.filter((d) => d.status === "success");
  } else if (filter === "error") {
    filtered = liveDataBuffer.filter((d) => d.status === "error");
  }

  if (filtered.length === 0) {
    tbody.innerHTML =
      '<tr><td colspan="6" style="padding: 2rem; text-align: center; color: var(--text-muted);">No data available</td></tr>';
    document.getElementById("liveDataCount").textContent = "0";
    return;
  }

  tbody.innerHTML = filtered
    .map((data, idx) => {
      const time = new Date(data.time).toLocaleTimeString("en-US", {
        hour12: false,
      });
      const statusClass =
        data.status === "success" ? "connected" : "disconnected";
      const statusText = data.status === "success" ? " Success" : " Error";

      return `
                    <tr style="border-bottom: 1px solid var(--border-color);">
                        <td style="padding: 0.75rem;">${idx + 1}</td>
                        <td style="padding: 0.75rem;">${time}</td>
                        <td style="padding: 0.75rem;">${data.temp.toFixed(
                          1
                        )}</td>
                        <td style="padding: 0.75rem;">${data.humi.toFixed(
                          1
                        )}</td>
                        <td style="padding: 0.75rem; text-transform: capitalize;">${
                          data.mode
                        }</td>
                        <td style="padding: 0.75rem;">
                            <span class="status-badge ${statusClass}" style="font-size: 0.75rem; padding: 0.25rem 0.5rem;">
                                ${statusText}
                            </span>
                        </td>
                    </tr>
                `;
    })
    .join("");

  document.getElementById("liveDataCount").textContent = filtered.length;

  // Re-initialize feather icons
  if (typeof feather !== "undefined") feather.replace();
}

document
  .getElementById("liveDataFilter")
  ?.addEventListener("change", renderLiveDataTable);

// Function to bind Live Data buttons
function bindLiveDataButtons() {
  const clearBtn = document.getElementById("clearLiveDataBtn");
  const exportBtn = document.getElementById("exportLiveDataBtn");

  if (clearBtn) {
    // Remove old listener by cloning
    const newClearBtn = clearBtn.cloneNode(true);
    clearBtn.parentNode.replaceChild(newClearBtn, clearBtn);

    newClearBtn.addEventListener("click", function () {
      if (confirm("Clear all live data?")) {
        liveDataBuffer = [];
        renderLiveDataTable();
        addStatus("Live data cleared", "INFO");
      }
    });
  }

  if (exportBtn) {
    // Remove old listener by cloning
    const newExportBtn = exportBtn.cloneNode(true);
    exportBtn.parentNode.replaceChild(newExportBtn, exportBtn);

    newExportBtn.addEventListener("click", function () {
      if (liveDataBuffer.length === 0) {
        addStatus("No data to export", "WARNING");
        return;
      }

      const csv = [
        ["#", "Time", "Temperature (C)", "Humidity (%)", "Mode", "Status"].join(
          ","
        ),
        ...liveDataBuffer.map((d, idx) => {
          const time = new Date(d.time).toLocaleTimeString("en-US", {
            hour12: false,
          });
          return [idx + 1, time, d.temp, d.humi, d.mode, d.status].join(",");
        }),
      ].join("\n");

      const blob = new Blob([csv], { type: "text/csv" });
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");
      a.href = url;
      a.download = `livedata_${Date.now()}.csv`;
      a.click();

      addStatus("Live data exported", "INFO");
    });
  }
}

// ====================================================================
// COMPONENT HEALTH MONITORING
// ====================================================================
function updateComponentHealth(component, isHealthy) {
  const health = componentHealth[component];
  if (!health) return;

  health.healthy = isHealthy;
  health.lastUpdate = Date.now();

  if (!isHealthy) {
    health.errorCount = (health.errorCount || 0) + 1;
  } else {
    health.errorCount = 0;
  }

  updateComponentCard(component);
}

function updateComponentCard(component) {
  const card = document.getElementById(`component-${component}`);
  if (!card) return;

  const health = componentHealth[component];
  const badge = card.querySelector(".status-badge");

  if (component === "SHT31") {
    if (health.healthy) {
      badge.className = "status-badge connected";
      badge.innerHTML =
        '<span class="status-dot connected"></span><span>Running</span>';

      document.getElementById("sht31-temp").textContent =
        currentTemp?.toFixed(1) + "C" || "--";
      document.getElementById("sht31-humi").textContent =
        currentHumi?.toFixed(1) + "%" || "--";
      document.getElementById("sht31-time").textContent = getTimeAgo(
        health.lastUpdate
      );
    } else {
      badge.className = "status-badge disconnected";
      badge.innerHTML =
        '<span class="status-dot disconnected"></span><span>Error</span>';
      document.getElementById("sht31-temp").textContent = "Sensor Failed";
      document.getElementById("sht31-humi").textContent = "Check Wiring";
      document.getElementById("sht31-time").textContent = "Error";
    }
  } else if (component === "DS3231") {
    if (health.healthy) {
      badge.className = "status-badge connected";
      badge.innerHTML =
        '<span class="status-dot connected"></span><span>Running</span>';
      document.getElementById("rtc-time").textContent = getTimeAgo(
        health.lastSync || health.lastUpdate
      );
      document.getElementById("rtc-status").textContent = "Operating normally";
    } else {
      badge.className = "status-badge disconnected";
      badge.innerHTML =
        '<span class="status-dot disconnected"></span><span>Error</span>';
      document.getElementById("rtc-time").textContent = "RTC Failed";
      document.getElementById("rtc-status").textContent = "Time sync failed";
    }
  }
}

function getTimeAgo(timestamp) {
  if (!timestamp) return "Never";
  const seconds = Math.floor((Date.now() - timestamp) / 1000);
  if (seconds < 60) return `${seconds} seconds ago`;
  const minutes = Math.floor(seconds / 60);
  if (minutes < 60) return `${minutes} minutes ago`;
  const hours = Math.floor(minutes / 60);
  return `${hours} hours ago`;
}

// ====================================================================
// TIME SETTINGS
// ====================================================================
function renderCalendar() {
  const calendar = document.getElementById("calendar");
  if (!calendar) return;

  const year = timePickerYear;
  const month = timePickerMonth;

  // Update month/year display
  const monthNames = [
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December",
  ];
  document.getElementById(
    "monthYear"
  ).textContent = `${monthNames[month]} ${year}`;

  // Clear calendar
  calendar.innerHTML = "";

  // Add day headers
  const dayHeaders = ["Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"];
  dayHeaders.forEach((day) => {
    const div = document.createElement("div");
    div.textContent = day;
    div.style.fontWeight = "600";
    div.style.padding = "0.5rem";
    div.style.color = "var(--text-muted)";
    calendar.appendChild(div);
  });

  // Get first day of month and number of days
  const firstDay = new Date(year, month, 1).getDay();
  const daysInMonth = new Date(year, month + 1, 0).getDate();

  // Add empty cells for days before month starts
  for (let i = 0; i < firstDay; i++) {
    const div = document.createElement("div");
    div.style.padding = "0.5rem";
    calendar.appendChild(div);
  }

  // Add day cells
  for (let day = 1; day <= daysInMonth; day++) {
    const div = document.createElement("div");
    div.textContent = day;
    div.style.padding = "0.5rem";
    div.style.cursor = "pointer";
    div.style.borderRadius = "0.25rem";
    div.style.transition = "all 0.2s";

    if (
      day === timePickerDay &&
      month === timePickerMonth &&
      year === timePickerYear
    ) {
      div.style.backgroundColor = "var(--primary)";
      div.style.color = "white";
      div.style.fontWeight = "700";
    }

    div.addEventListener("click", function () {
      timePickerDay = day;
      renderCalendar();
    });

    div.addEventListener("mouseenter", function () {
      if (
        !(
          day === timePickerDay &&
          month === timePickerMonth &&
          year === timePickerYear
        )
      ) {
        div.style.backgroundColor = "var(--bg-tertiary)";
      }
    });

    div.addEventListener("mouseleave", function () {
      if (
        !(
          day === timePickerDay &&
          month === timePickerMonth &&
          year === timePickerYear
        )
      ) {
        div.style.backgroundColor = "";
      }
    });

    calendar.appendChild(div);
  }
}

// Function to bind calendar navigation buttons
function bindCalendarButtons() {
  const prevBtn = document.getElementById("prevMonth");
  const nextBtn = document.getElementById("nextMonth");

  console.log("[TIME] Binding calendar buttons:", {
    prevBtn: !!prevBtn,
    nextBtn: !!nextBtn,
  });

  if (prevBtn) {
    const newPrevBtn = prevBtn.cloneNode(true);
    prevBtn.parentNode.replaceChild(newPrevBtn, prevBtn);

    newPrevBtn.addEventListener("click", function () {
      console.log("[TIME] prevMonth clicked");
      timePickerMonth--;
      if (timePickerMonth < 0) {
        timePickerMonth = 11;
        timePickerYear--;
      }
      console.log("[TIME] New month/year:", {
        month: timePickerMonth,
        year: timePickerYear,
      });
      renderCalendar();
    });
  }

  if (nextBtn) {
    const newNextBtn = nextBtn.cloneNode(true);
    nextBtn.parentNode.replaceChild(newNextBtn, nextBtn);

    newNextBtn.addEventListener("click", function () {
      console.log("[TIME] nextMonth clicked");
      timePickerMonth++;
      if (timePickerMonth > 11) {
        timePickerMonth = 0;
        timePickerYear++;
      }
      console.log("[TIME] New month/year:", {
        month: timePickerMonth,
        year: timePickerYear,
      });
      renderCalendar();
    });
  }
}

// Global handler for inline onclick buttons in index.html
function adjustTime(unit, delta) {
  console.log("[TIME] adjustTime called:", { unit, delta });
  if (unit === "hour") {
    timePickerHour += delta;
    if (timePickerHour < 0) timePickerHour = 23;
    if (timePickerHour > 23) timePickerHour = 0;
  } else if (unit === "min") {
    timePickerMinute += delta;
    if (timePickerMinute < 0) timePickerMinute = 59;
    if (timePickerMinute > 59) timePickerMinute = 0;
  } else if (unit === "sec") {
    timePickerSecond += delta;
    if (timePickerSecond < 0) timePickerSecond = 59;
    if (timePickerSecond > 59) timePickerSecond = 0;
  }

  console.log("[TIME] Updated values:", {
    hour: timePickerHour,
    min: timePickerMinute,
    sec: timePickerSecond,
  });
  updateTimeDisplay();
}
// Also expose on window for completeness
window.adjustTime = adjustTime;

function updateTimeDisplay() {
  document.getElementById("displayHour").textContent = String(
    timePickerHour
  ).padStart(2, "0");
  document.getElementById("displayMin").textContent = String(
    timePickerMinute
  ).padStart(2, "0");
  document.getElementById("displaySec").textContent = String(
    timePickerSecond
  ).padStart(2, "0");
}

// Robust binding for Time Settings buttons
function attachTimeButtons() {
  const syncBtn = document.getElementById("syncTimeBtn");
  const manualBtn = document.getElementById("setManualTimeBtn");
  console.log("[TIME] attachTimeButtons()", {
    hasSyncBtn: !!syncBtn,
    hasManualBtn: !!manualBtn,
  });

  if (syncBtn && !syncBtn.__bound) {
    syncBtn.addEventListener("click", onSyncTimeClick);
    syncBtn.__bound = true;
  }
  if (manualBtn && !manualBtn.__bound) {
    manualBtn.addEventListener("click", onManualTimeClick);
    manualBtn.__bound = true;
  }
}

// Delegated fallback in case DOM is re-rendered or binding missed
document.addEventListener(
  "click",
  (e) => {
    const syncEl = e.target.closest && e.target.closest("#syncTimeBtn");
    const manualEl = e.target.closest && e.target.closest("#setManualTimeBtn");
    if (syncEl) return onSyncTimeClick(e);
    if (manualEl) return onManualTimeClick(e);
  },
  true
);

function onSyncTimeClick() {
  console.log("[TIME] onSyncTimeClick() called");
  const now = new Date();
  const timestamp = Math.floor(now.getTime() / 1000);

  console.log("[TIME] Sync from internet:", {
    now: now.toISOString(),
    timestamp,
  });

  // Update time picker UI
  timePickerYear = now.getFullYear();
  timePickerMonth = now.getMonth();
  timePickerDay = now.getDate();
  timePickerHour = now.getHours();
  timePickerMinute = now.getMinutes();
  timePickerSecond = now.getSeconds();

  renderCalendar();
  updateTimeDisplay();

  // Send to device (add 7 hours offset for Vietnam timezone UTC+7)
  const timestampWithOffset = timestamp + 7 * 3600; // Add 7 hours in seconds
  const command = `SET TIME ${timestampWithOffset}`;
  if (isMqttConnected && mqttClient) {
    mqttClient.publish(MQTT_CONFIG.topics.stm32Command, command, { qos: 1 });
    addStatus(
      `Time synced from internet: ${now.toLocaleString()} (UTC+7 sent to device)`,
      "SYNC"
    );
    console.log(
      "[TIME] MQTT command sent:",
      command,
      "(original:",
      timestamp,
      "+ 7h offset)"
    );
  } else {
    addStatus("MQTT not connected - cannot send SET TIME", "ERROR");
    console.warn("[TIME] MQTT not connected, command not sent");
  }

  // Update local device/system time emulation regardless, so UI reflects chosen time
  deviceClockMs = timestamp * 1000;
  deviceClockSetAtMs = Date.now();
  console.log("[TIME] Device clock set:", {
    deviceClockMs,
    deviceClockSetAtMs,
  });
  startDeviceClockTicker();
}

function onManualTimeClick() {
  console.log("[TIME] onManualTimeClick() called");
  const date = new Date(
    timePickerYear,
    timePickerMonth,
    timePickerDay,
    timePickerHour,
    timePickerMinute,
    timePickerSecond
  );
  const timestamp = Math.floor(date.getTime() / 1000);

  console.log("[TIME] Manual time set:", {
    date: date.toISOString(),
    timestamp,
  });

  // Send to device (add 7 hours offset for Vietnam timezone UTC+7)
  const timestampWithOffset = timestamp + 7 * 3600; // Add 7 hours in seconds
  const command = `SET TIME ${timestampWithOffset}`;
  if (isMqttConnected && mqttClient) {
    mqttClient.publish(MQTT_CONFIG.topics.stm32Command, command, { qos: 1 });
    addStatus(
      `Manual time set: ${date.toLocaleString()} (UTC+7 sent to device)`,
      "SETTING"
    );
    console.log(
      "[TIME] MQTT command sent:",
      command,
      "(original:",
      timestamp,
      "+ 7h offset)"
    );
  } else {
    addStatus("MQTT not connected - cannot send SET TIME", "ERROR");
    console.warn("[TIME] MQTT not connected, command not sent");
  }

  // Update local device/system time emulation to reflect manual selection
  deviceClockMs = timestamp * 1000;
  deviceClockSetAtMs = Date.now();
  console.log("[TIME] Device clock set:", {
    deviceClockMs,
    deviceClockSetAtMs,
  });
  startDeviceClockTicker();
}

// Initial binding
attachTimeButtons();

// Update time display every second (picker clock)
setInterval(updateTimeDisplay, 1000);

// Start device clock ticker (footer). Will show --:--:-- until first time set
startDeviceClockTicker();

// ====================================================================
// DATA MANAGEMENT
// ====================================================================

function getDateRange(startDate, endDate) {
  const dates = [];
  const start = new Date(startDate);
  const end = new Date(endDate);

  for (let d = new Date(start); d <= end; d.setDate(d.getDate() + 1)) {
    dates.push(d.toISOString().split("T")[0]);
  }

  return dates;
}

function applyDataFilters() {
  if (!isFirebaseConnected || !firebaseDb) {
    addStatus("Firebase not connected", "ERROR");
    return;
  }

  const filters = {
    dateFrom: document.getElementById("filterDateFrom").value || "2024-01-01",
    dateTo:
      document.getElementById("filterDateTo").value ||
      new Date().toISOString().split("T")[0],
    mode: document.getElementById("filterMode").value,
    status: document.getElementById("filterStatus").value,
    tempMin:
      parseFloat(document.getElementById("filterTempMin").value) || -Infinity,
    tempMax:
      parseFloat(document.getElementById("filterTempMax").value) || Infinity,
    humiMin:
      parseFloat(document.getElementById("filterHumiMin").value) || -Infinity,
    humiMax:
      parseFloat(document.getElementById("filterHumiMax").value) || Infinity,
    sortBy: document.getElementById("filterSort").value,
  };

  addStatus("Loading filtered data from Firebase...", "INFO");

  const dateRange = getDateRange(filters.dateFrom, filters.dateTo);
  let allData = [];

  const promises = dateRange.map((date) => {
    return firebaseDb
      .ref(`readings/${date}`)
      .once("value")
      .then((snapshot) => {
        const dayData = snapshot.val() || {};
        return Object.entries(dayData).map(([id, record]) => ({
          id,
          date,
          ...record,
        }));
      })
      .catch((err) => {
        console.error(`Error loading ${date}:`, err);
        return [];
      });
  });

  Promise.all(promises)
    .then((results) => {
      allData = results.flat();

      // Apply filters
      let filtered = allData.filter((record) => {
        if (filters.mode && record.mode !== filters.mode) return false;
        if (filters.status && record.status !== filters.status) return false;
        if (record.temp < filters.tempMin || record.temp > filters.tempMax)
          return false;
        if (record.humi < filters.humiMin || record.humi > filters.humiMax)
          return false;
        return true;
      });

      // Apply sorting
      filtered = applySorting(filtered, filters.sortBy);

      // Cache and render
      filteredDataCache = filtered;
      renderDataManagementTable(filtered);
      updateDataStatistics(filtered);

      addStatus(
        `Loaded ${filtered.length} records from ${allData.length} total`,
        "INFO"
      );
    })
    .catch((err) => {
      addStatus(`Load error: ${err.message}`, "ERROR");
    });
}

function applySorting(data, sortBy) {
  const [field, order] = sortBy.split("-");

  return data.sort((a, b) => {
    let valA, valB;

    if (field === "time") {
      valA = a.created_at;
      valB = b.created_at;
    } else if (field === "temp") {
      valA = a.temp;
      valB = b.temp;
    } else if (field === "humi") {
      valA = a.humi;
      valB = b.humi;
    }

    return order === "asc" ? valA - valB : valB - valA;
  });
}

function renderDataManagementTable(data) {
  const tbody = document.getElementById("dataManagementTable");

  if (!data || data.length === 0) {
    tbody.innerHTML =
      '<tr><td colspan="6" style="padding: 2rem; text-align: center; color: var(--text-muted);">No matching data found</td></tr>';
    document.getElementById("dataManagementCount").textContent = "0";
    return;
  }

  tbody.innerHTML = data
    .map((record, idx) => {
      // Use device timestamp (time field) instead of created_at
      const timestamp = record.time ? record.time * 1000 : record.created_at;
      const date = new Date(timestamp);
      const dateStr = date.toLocaleDateString();
      const timeStr = date.toLocaleTimeString("en-US", { hour12: false });
      const statusClass =
        record.status === "success" ? "connected" : "disconnected";
      const statusText = record.status === "success" ? " Success" : " Error";

      return `
                    <tr style="border-bottom: 1px solid var(--border-color);">
                        <td style="padding: 0.75rem;">${idx + 1}</td>
                        <td style="padding: 0.75rem;">${dateStr} ${timeStr}</td>
                        <td style="padding: 0.75rem;">${
                          record.temp?.toFixed(1) || "--"
                        }</td>
                        <td style="padding: 0.75rem;">${
                          record.humi?.toFixed(1) || "--"
                        }</td>
                        <td style="padding: 0.75rem; text-transform: capitalize;">${
                          record.mode || "--"
                        }</td>
                        <td style="padding: 0.75rem;">
                            <span class="status-badge ${statusClass}" style="font-size: 0.75rem; padding: 0.25rem 0.5rem;">
                                ${statusText}
                            </span>
                        </td>
                    </tr>
                `;
    })
    .join("");

  document.getElementById("dataManagementCount").textContent = data.length;

  if (typeof feather !== "undefined") feather.replace();
}

function updateDataStatistics(data) {
  if (!data || data.length === 0) {
    document.getElementById("statsTotal").textContent = "0";
    document.getElementById("statsSuccess").textContent = "--";
    document.getElementById("statsAvgTemp").textContent = "--";
    document.getElementById("statsAvgHumi").textContent = "--";
    return;
  }

  const successCount = data.filter((d) => d.status === "success").length;
  const successRate = ((successCount / data.length) * 100).toFixed(1);

  const temps = data
    .filter((d) => d.temp !== null && d.temp !== 0)
    .map((d) => d.temp);
  const humis = data
    .filter((d) => d.humi !== null && d.humi !== 0)
    .map((d) => d.humi);

  const avgTemp =
    temps.length > 0
      ? (temps.reduce((a, b) => a + b, 0) / temps.length).toFixed(1)
      : "--";
  const avgHumi =
    humis.length > 0
      ? (humis.reduce((a, b) => a + b, 0) / humis.length).toFixed(1)
      : "--";

  document.getElementById("statsTotal").textContent = data.length;
  document.getElementById("statsSuccess").textContent = successRate + "%";
  document.getElementById("statsAvgTemp").textContent =
    avgTemp + (avgTemp !== "--" ? "C" : "");
  document.getElementById("statsAvgHumi").textContent =
    avgHumi + (avgHumi !== "--" ? "%" : "");
}

function exportFilteredData() {
  if (!filteredDataCache || filteredDataCache.length === 0) {
    addStatus("No data to export. Apply filters first.", "WARNING");
    return;
  }

  const headers = [
    "#",
    "Date",
    "Time",
    "Temperature (C)",
    "Humidity (%)",
    "Mode",
    "Status",
    "Sensor",
    "Device",
  ];
  const rows = filteredDataCache.map((record, idx) => {
    const date = new Date(record.created_at);
    return [
      idx + 1,
      date.toLocaleDateString(),
      date.toLocaleTimeString("en-US", { hour12: false }),
      record.temp,
      record.humi,
      record.mode,
      record.status,
      record.sensor || "SHT31",
      record.device || "ESP32_01",
    ];
  });

  const csv = [headers.join(","), ...rows.map((row) => row.join(","))].join(
    "\n"
  );

  const blob = new Blob([csv], { type: "text/csv" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = `datalogger_export_${Date.now()}.csv`;
  a.click();

  addStatus(`Exported ${filteredDataCache.length} records to CSV`, "INFO");
}

function resetDataFilters() {
  document.getElementById("filterDateFrom").value = "";
  document.getElementById("filterDateTo").value = "";
  document.getElementById("filterMode").value = "";
  document.getElementById("filterStatus").value = "";
  document.getElementById("filterTempMin").value = "";
  document.getElementById("filterTempMax").value = "";
  document.getElementById("filterHumiMin").value = "";
  document.getElementById("filterHumiMax").value = "";
  document.getElementById("filterSort").value = "time-desc";

  filteredDataCache = [];

  document.getElementById("dataManagementTable").innerHTML =
    '<tr><td colspan="6" style="padding: 2rem; text-align: center; color: var(--text-muted);">No data loaded. Click "Apply Filters" to load data from Firebase.</td></tr>';
  document.getElementById("dataManagementCount").textContent = "0";

  updateDataStatistics([]);

  addStatus("Filters reset", "INFO");
}

// Function to bind Data Management buttons
function bindDataManagementButtons() {
  console.log("[DATA] Binding Data Management buttons...");

  // Define setQuickFilter locally
  function setQuickFilter(type) {
    const today = new Date();
    const startInput = document.getElementById("filterDateFrom");
    const endInput = document.getElementById("filterDateTo");

    if (!startInput || !endInput) {
      console.error("[DATA] Date inputs not found!");
      return;
    }

    let startDate = new Date();
    const endDate = new Date();

    switch (type) {
      case "today":
        startDate = today;
        break;
      case "week":
        startDate.setDate(today.getDate() - 7);
        break;
      case "month":
        startDate.setMonth(today.getMonth() - 1);
        break;
      case "all":
        startDate = new Date(2020, 0, 1);
        break;
    }

    startInput.value = startDate.toISOString().split("T")[0];
    endInput.value = endDate.toISOString().split("T")[0];

    console.log(`[DATA] Quick filter "${type}" set dates:`, {
      from: startInput.value,
      to: endInput.value,
    });
  }

  // Quick filter buttons
  const todayBtn = document.getElementById("quickFilterToday");
  const weekBtn = document.getElementById("quickFilterWeek");
  const monthBtn = document.getElementById("quickFilterMonth");
  const allBtn = document.getElementById("quickFilterAll");

  console.log("[DATA] Quick filter buttons found:", {
    today: !!todayBtn,
    week: !!weekBtn,
    month: !!monthBtn,
    all: !!allBtn,
  });

  if (todayBtn) {
    const newBtn = todayBtn.cloneNode(true);
    todayBtn.parentNode.replaceChild(newBtn, todayBtn);
    newBtn.addEventListener("click", () => {
      console.log("[DATA] Quick filter: Today clicked");
      setQuickFilter("today");
      applyDataFilters();
    });
  } else {
    console.warn("[DATA] Today button not found!");
  }

  if (weekBtn) {
    const newBtn = weekBtn.cloneNode(true);
    weekBtn.parentNode.replaceChild(newBtn, weekBtn);
    newBtn.addEventListener("click", () => {
      console.log("[DATA] Quick filter: Week clicked");
      setQuickFilter("week");
      applyDataFilters();
    });
  } else {
    console.warn("[DATA] Week button not found!");
  }

  if (monthBtn) {
    const newBtn = monthBtn.cloneNode(true);
    monthBtn.parentNode.replaceChild(newBtn, monthBtn);
    newBtn.addEventListener("click", () => {
      console.log("[DATA] Quick filter: Month clicked");
      setQuickFilter("month");
      applyDataFilters();
    });
  } else {
    console.warn("[DATA] Month button not found!");
  }

  if (allBtn) {
    const newBtn = allBtn.cloneNode(true);
    allBtn.parentNode.replaceChild(newBtn, allBtn);
    newBtn.addEventListener("click", () => {
      console.log("[DATA] Quick filter: All Time clicked");
      setQuickFilter("all");
      applyDataFilters();
    });
  } else {
    console.warn("[DATA] All Time button not found!");
  }

  // Action buttons
  const applyBtn = document.getElementById("applyFiltersBtn");
  const resetBtn = document.getElementById("resetFiltersBtn");
  const exportBtn = document.getElementById("exportDataBtn");

  console.log("[DATA] Action buttons found:", {
    apply: !!applyBtn,
    reset: !!resetBtn,
    export: !!exportBtn,
  });

  if (applyBtn) {
    const newBtn = applyBtn.cloneNode(true);
    applyBtn.parentNode.replaceChild(newBtn, applyBtn);
    newBtn.addEventListener("click", () => {
      console.log("[DATA] Apply filters clicked");
      applyDataFilters();
    });
  } else {
    console.warn("[DATA] Apply button not found!");
  }

  if (resetBtn) {
    const newBtn = resetBtn.cloneNode(true);
    resetBtn.parentNode.replaceChild(newBtn, resetBtn);
    newBtn.addEventListener("click", () => {
      console.log("[DATA] Reset filters clicked");
      resetDataFilters();
    });
  } else {
    console.warn("[DATA] Reset button not found!");
  }

  if (exportBtn) {
    const newBtn = exportBtn.cloneNode(true);
    exportBtn.parentNode.replaceChild(newBtn, exportBtn);
    newBtn.addEventListener("click", () => {
      console.log("[DATA] Export data clicked");
      exportFilteredData();
    });
  } else {
    console.warn("[DATA] Export button not found!");
  }

  console.log("[DATA] All buttons bound successfully");
}

// ====================================================================
// LOGS PAGE ENHANCEMENT
// ====================================================================

/**
 * Render full console with filtering
 */
function renderFullConsole() {
  const consoleEl = document.getElementById("consoleFull");
  if (!consoleEl) {
    console.warn("[LOGS] consoleFull element not found");
    return;
  }

  const searchTerm =
    document.getElementById("logSearchInput")?.value.toLowerCase() || "";

  // Apply filters
  let filteredLogs = logBuffer.filter((log) => {
    // Type filter
    if (logFilterType !== "ALL" && log.type !== logFilterType) return false;

    // Search filter
    if (searchTerm && !log.message.toLowerCase().includes(searchTerm))
      return false;

    return true;
  });

  // Update count
  const countEl = document.getElementById("logCount");
  if (countEl) {
    countEl.textContent = filteredLogs.length;
  }

  // Empty state
  if (filteredLogs.length === 0) {
    consoleEl.innerHTML =
      '<div style="color: var(--text-muted); text-align: center; padding: 2rem;">No logs found</div>';
    return;
  }

  // Color mapping for log types
  const colors = {
    INFO: "#3B82F6",
    WARNING: "#F59E0B",
    ERROR: "#EF4444",
    MQTT: "#8B5CF6",
    FIREBASE: "#F97316",
    SYNC: "#06B6D4",
    SETTING: "#14B8A6",
  };

  // Render logs (newest first)
  consoleEl.innerHTML = filteredLogs
    .map((log) => {
      const time = new Date(log.timestamp).toLocaleTimeString("en-US", {
        hour12: false,
      });
      const color = colors[log.type] || "#10B981";

      return `<div class="log-line log-${log.type.toLowerCase()}" style="color: ${color};">
                    <strong>[${time}]</strong> <strong style="display: inline-block; min-width: 80px;">[${
        log.type
      }]</strong> ${log.message}
                </div>`;
    })
    .join("");

  // Auto-scroll to bottom for new logs
  consoleEl.scrollTop = consoleEl.scrollHeight;
}

/**
 * Filter logs by type
 */
function filterLogs(type) {
  console.log("[LOGS] Filtering by type:", type);
  logFilterType = type;

  // Update button styles
  document.querySelectorAll(".filter-btn").forEach((btn) => {
    if (btn.dataset.filter === type) {
      btn.className = "btn btn-primary filter-btn active";
    } else {
      btn.className = "btn btn-secondary filter-btn";
    }
  });

  renderFullConsole();
}

/**
 * Clear all logs with confirmation
 */
function clearLogs() {
  if (
    !confirm(
      "[WARNING] Are you sure you want to clear ALL logs? This action cannot be undone."
    )
  ) {
    return;
  }

  console.log("[LOGS] Clearing all logs...");

  // Clear main buffer
  logBuffer = [];

  // Clear preview console on dashboard
  const preview = document.getElementById("consolePreview");
  if (preview) {
    preview.innerHTML =
      '<div style="color: var(--text-muted); text-align: center; padding: 1rem;">No logs yet</div>';
  }

  // Re-render full console
  renderFullConsole();

  // Add confirmation log
  addStatus("All logs cleared by user", "INFO");
}

/**
 * Export logs to CSV
 */
function exportLogs() {
  if (logBuffer.length === 0) {
    addStatus("No logs to export", "WARNING");
    alert("[WARNING] No logs available to export!");
    return;
  }

  console.log("[LOGS] Exporting", logBuffer.length, "logs...");

  // CSV headers
  const headers = ["Timestamp", "Date/Time", "Type", "Message"];

  // CSV rows
  const rows = logBuffer.map((log) => {
    const date = new Date(log.timestamp);
    const dateStr = date.toLocaleDateString();
    const timeStr = date.toLocaleTimeString("en-US", { hour12: false });

    return [
      log.timestamp,
      `${dateStr} ${timeStr}`,
      log.type,
      `"${log.message.replace(/"/g, '""')}"`, // Escape quotes
    ].join(",");
  });

  // Combine
  const csv = [headers.join(","), ...rows].join("\n");

  // Create blob and download
  const blob = new Blob([csv], { type: "text/csv;charset=utf-8;" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = `datalogger_logs_${Date.now()}.csv`;
  link.click();
  URL.revokeObjectURL(url);

  addStatus(`Exported ${logBuffer.length} logs to CSV`, "INFO");
}

/**
 * Bind all logs page event listeners
 */
function bindLogsButtons() {
  console.log("[LOGS] Binding buttons...");

  // Filter buttons
  document.querySelectorAll(".filter-btn").forEach((btn) => {
    // Remove old listeners by cloning
    const newBtn = btn.cloneNode(true);
    btn.parentNode.replaceChild(newBtn, btn);

    newBtn.addEventListener("click", function () {
      filterLogs(this.dataset.filter);
    });
  });

  // Clear button
  const clearBtn = document.getElementById("clearLogsBtn");
  if (clearBtn) {
    const newClearBtn = clearBtn.cloneNode(true);
    clearBtn.parentNode.replaceChild(newClearBtn, clearBtn);
    newClearBtn.addEventListener("click", clearLogs);
  }

  // Export button
  const exportBtn = document.getElementById("exportLogsBtn");
  if (exportBtn) {
    const newExportBtn = exportBtn.cloneNode(true);
    exportBtn.parentNode.replaceChild(newExportBtn, exportBtn);
    newExportBtn.addEventListener("click", exportLogs);
  }

  // Search input with debounce
  const searchInput = document.getElementById("logSearchInput");
  if (searchInput) {
    let searchTimeout;
    const newSearchInput = searchInput.cloneNode(true);
    searchInput.parentNode.replaceChild(newSearchInput, searchInput);

    newSearchInput.addEventListener("input", function () {
      clearTimeout(searchTimeout);
      searchTimeout = setTimeout(() => {
        renderFullConsole();
      }, 300); // Debounce 300ms
    });
  }

  console.log("[LOGS] All buttons bound successfully");

  // Re-initialize feather icons
  if (typeof feather !== "undefined") {
    feather.replace();
  }
}

// ====================================================================
// SETTINGS PAGE
// ====================================================================
function loadSettingsPage() {
  // Load MQTT settings
  document.getElementById("mqttHost").value = MQTT_CONFIG.host;
  document.getElementById("mqttPort").value = MQTT_CONFIG.port;
  document.getElementById("mqttPath").value = MQTT_CONFIG.path;
  document.getElementById("mqttUser").value = MQTT_CONFIG.username;
  document.getElementById("mqttPass").value = MQTT_CONFIG.password;
  document.getElementById("mqttClientId").value = MQTT_CONFIG.clientId;

  // Load Firebase settings
  document.getElementById("firebaseApiKey").value = FIREBASE_CONFIG.apiKey;
  document.getElementById("firebaseProjectId").value =
    FIREBASE_CONFIG.projectId;
  document.getElementById("firebaseDbUrl").value = FIREBASE_CONFIG.databaseURL;

  // Load display preferences (from localStorage or defaults)
  document.getElementById("tempUnit").value =
    localStorage.getItem("tempUnit") || "C";
  document.getElementById("timeFormat").value =
    localStorage.getItem("timeFormat") || "24h";
  document.getElementById("dateFormat").value =
    localStorage.getItem("dateFormat") || "DD/MM/YYYY";

  // Load chart settings
  document.getElementById("chartMaxPoints").value =
    localStorage.getItem("chartMaxPoints") || "50";
  document.getElementById("chartUpdateInterval").value =
    localStorage.getItem("chartUpdateInterval") || "1";
  document.getElementById("chartSkipErrors").checked =
    localStorage.getItem("chartSkipErrors") === "true";

  // Load data settings
  document.getElementById("dataDefaultInterval").value =
    localStorage.getItem("dataDefaultInterval") || "5";
  document.getElementById("dataRetentionDays").value =
    localStorage.getItem("dataRetentionDays") || "30";
  document.getElementById("dataAutoSave").checked =
    localStorage.getItem("dataAutoSave") !== "false";
}

function saveSettings() {
  addStatus("Saving settings...", "INFO");

  // Validate inputs
  const host = document.getElementById("mqttHost").value.trim();
  const port = parseInt(document.getElementById("mqttPort").value);
  const clientId = document.getElementById("mqttClientId").value.trim();
  const path = document.getElementById("mqttPath").value.trim() || "/mqtt";
  const username =
    document.getElementById("mqttUser").value.trim() || "DataLogger";
  const password =
    document.getElementById("mqttPass").value.trim() || "datalogger";

  // Validate MQTT settings
  if (!host) {
    alert(" Error: MQTT Host cannot be empty!");
    addStatus("Settings validation failed: Empty MQTT host", "ERROR");
    return;
  }
  if (!port || port < 1 || port > 65535) {
    alert(" Error: MQTT Port must be between 1-65535!");
    addStatus("Settings validation failed: Invalid MQTT port", "ERROR");
    return;
  }
  if (!clientId) {
    alert(" Error: Client ID cannot be empty!");
    addStatus("Settings validation failed: Empty client ID", "ERROR");
    return;
  }

  // Validate Firebase settings
  const apiKey = document.getElementById("firebaseApiKey").value.trim();
  const projectId = document.getElementById("firebaseProjectId").value.trim();
  const databaseURL = document.getElementById("firebaseDbUrl").value.trim();

  if (!apiKey || !projectId || !databaseURL) {
    alert(
      " Warning: Firebase settings are incomplete. Firebase features will be disabled."
    );
    addStatus("Settings saved with incomplete Firebase config", "WARNING");
  }

  // Update MQTT config object
  MQTT_CONFIG.host = host;
  MQTT_CONFIG.port = port;
  MQTT_CONFIG.path = path;
  MQTT_CONFIG.username = username;
  MQTT_CONFIG.password = password;
  MQTT_CONFIG.clientId = clientId;
  MQTT_CONFIG.url = `ws://${host}:${port}${path}`;

  // Update Firebase config object
  FIREBASE_CONFIG.apiKey = apiKey;
  FIREBASE_CONFIG.projectId = projectId;
  FIREBASE_CONFIG.databaseURL = databaseURL;

  // Save config objects to localStorage
  localStorage.setItem("datalogger_mqtt_config", JSON.stringify(MQTT_CONFIG));
  localStorage.setItem(
    "datalogger_firebase_config",
    JSON.stringify(FIREBASE_CONFIG)
  );

  // Save display preferences
  localStorage.setItem("tempUnit", document.getElementById("tempUnit").value);
  localStorage.setItem(
    "timeFormat",
    document.getElementById("timeFormat").value
  );
  localStorage.setItem(
    "dateFormat",
    document.getElementById("dateFormat").value
  );

  // Save chart settings
  localStorage.setItem(
    "chartMaxPoints",
    document.getElementById("chartMaxPoints").value
  );
  localStorage.setItem(
    "chartUpdateInterval",
    document.getElementById("chartUpdateInterval").value
  );
  localStorage.setItem(
    "chartSkipErrors",
    document.getElementById("chartSkipErrors").checked
  );

  // Save data settings
  localStorage.setItem(
    "dataDefaultInterval",
    document.getElementById("dataDefaultInterval").value
  );
  localStorage.setItem(
    "dataRetentionDays",
    document.getElementById("dataRetentionDays").value
  );
  localStorage.setItem(
    "dataAutoSave",
    document.getElementById("dataAutoSave").checked
  );

  addStatus(" Settings saved to localStorage", "SUCCESS");

  // Reconnect MQTT with diagnostic
  addStatus(" Reconnecting MQTT broker...", "INFO");
  if (mqttClient && mqttClient.connected) {
    addStatus("Disconnecting existing MQTT connection...", "INFO");
    mqttClient.end(true);
  }

  // Small delay to ensure clean disconnect
  setTimeout(() => {
    addStatus(
      `Connecting to ws://${host}:${port}${path} as ${clientId}...`,
      "INFO"
    );
    initializeMQTT();
  }, 500);

  // Reconnect Firebase with diagnostic
  if (apiKey && projectId && databaseURL) {
    addStatus(" Reconnecting Firebase...", "INFO");
    setTimeout(() => {
      initializeFirebase();
    }, 1000);
  }

  alert(
    " Settings saved!\n\n Reconnecting to MQTT and Firebase...\nCheck the System Console for connection status."
  );
}

function exportSettings() {
  const settings = {
    mqtt: {
      host: MQTT_CONFIG.host,
      port: MQTT_CONFIG.port,
      clientId: MQTT_CONFIG.clientId,
    },
    firebase: {
      apiKey: FIREBASE_CONFIG.apiKey,
      projectId: FIREBASE_CONFIG.projectId,
      databaseURL: FIREBASE_CONFIG.databaseURL,
    },
    display: {
      tempUnit: localStorage.getItem("tempUnit") || "C",
      timeFormat: localStorage.getItem("timeFormat") || "24h",
      dateFormat: localStorage.getItem("dateFormat") || "DD/MM/YYYY",
    },
    chart: {
      maxPoints: localStorage.getItem("chartMaxPoints") || "50",
      updateInterval: localStorage.getItem("chartUpdateInterval") || "1",
      skipErrors: localStorage.getItem("chartSkipErrors") === "true",
    },
    data: {
      defaultInterval: localStorage.getItem("dataDefaultInterval") || "5",
      retentionDays: localStorage.getItem("dataRetentionDays") || "30",
      autoSave: localStorage.getItem("dataAutoSave") !== "false",
    },
  };

  const json = JSON.stringify(settings, null, 2);
  const blob = new Blob([json], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = `datalogger_settings_${Date.now()}.json`;
  a.click();

  addStatus("Settings exported to JSON", "INFO");
}

function importSettings() {
  const input = document.createElement("input");
  input.type = "file";
  input.accept = ".json";

  input.onchange = (e) => {
    const file = e.target.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (event) => {
      try {
        const settings = JSON.parse(event.target.result);

        // Apply MQTT settings
        if (settings.mqtt) {
          document.getElementById("mqttHost").value = settings.mqtt.host;
          document.getElementById("mqttPort").value = settings.mqtt.port;
          document.getElementById("mqttClientId").value =
            settings.mqtt.clientId;
        }

        // Apply Firebase settings
        if (settings.firebase) {
          document.getElementById("firebaseApiKey").value =
            settings.firebase.apiKey;
          document.getElementById("firebaseProjectId").value =
            settings.firebase.projectId;
          document.getElementById("firebaseDbUrl").value =
            settings.firebase.databaseURL;
        }

        // Apply display preferences
        if (settings.display) {
          document.getElementById("tempUnit").value = settings.display.tempUnit;
          document.getElementById("timeFormat").value =
            settings.display.timeFormat;
          document.getElementById("dateFormat").value =
            settings.display.dateFormat;
        }

        // Apply chart settings
        if (settings.chart) {
          document.getElementById("chartMaxPoints").value =
            settings.chart.maxPoints;
          document.getElementById("chartUpdateInterval").value =
            settings.chart.updateInterval;
          document.getElementById("chartSkipErrors").checked =
            settings.chart.skipErrors;
        }

        // Apply data settings
        if (settings.data) {
          document.getElementById("dataDefaultInterval").value =
            settings.data.defaultInterval;
          document.getElementById("dataRetentionDays").value =
            settings.data.retentionDays;
          document.getElementById("dataAutoSave").checked =
            settings.data.autoSave;
        }

        addStatus(
          "Settings imported successfully. Click Save to apply.",
          "INFO"
        );
        alert('Settings imported! Click "Save Settings" to apply them.');
      } catch (err) {
        addStatus(`Import error: ${err.message}`, "ERROR");
        alert("Failed to import settings. Invalid JSON file.");
      }
    };
    reader.readAsText(file);
  };

  input.click();
}

function restoreDefaults() {
  if (
    !confirm(
      "Are you sure you want to restore all settings to defaults? This will clear your MQTT and Firebase configuration."
    )
  )
    return;

  // Clear all localStorage
  localStorage.clear();

  // Reset form to defaults
  document.getElementById("mqttHost").value = "";
  document.getElementById("mqttPort").value = "9001";
  document.getElementById("mqttClientId").value = "web_client_1";

  document.getElementById("firebaseApiKey").value = "";
  document.getElementById("firebaseProjectId").value = "";
  document.getElementById("firebaseDbUrl").value = "";

  document.getElementById("tempUnit").value = "C";
  document.getElementById("timeFormat").value = "24h";
  document.getElementById("dateFormat").value = "DD/MM/YYYY";

  document.getElementById("chartMaxPoints").value = "50";
  document.getElementById("chartUpdateInterval").value = "1";
  document.getElementById("chartSkipErrors").checked = false;

  document.getElementById("dataDefaultInterval").value = "5";
  document.getElementById("dataRetentionDays").value = "30";
  document.getElementById("dataAutoSave").checked = true;

  addStatus("Settings restored to defaults", "INFO");
  alert("Settings restored to defaults! Reload the page to apply changes.");
}

// ====================================================================
// INTERVAL PICKER
// ====================================================================

function openIntervalModal() {
  console.log("[INTERVAL] Opening modal with:", {
    selectedMinute,
    selectedSecond,
  });
  document.getElementById("intervalModal").classList.add("active");
  updateIntervalDisplay();

  // Bind buttons EVERY TIME modal opens (to ensure DOM elements exist)
  bindIntervalButtons();
}

function closeIntervalModal() {
  console.log("[INTERVAL] Closing modal");
  document.getElementById("intervalModal").classList.remove("active");
}

function adjustInterval(type, delta) {
  console.log("[INTERVAL] Adjust:", {
    type,
    delta,
    before: { selectedMinute, selectedSecond },
  });
  if (type === "minute") {
    selectedMinute += delta;
    if (selectedMinute < 0) selectedMinute = 60;
    if (selectedMinute > 60) selectedMinute = 0;
  } else if (type === "second") {
    selectedSecond += delta;
    if (selectedSecond < 0) selectedSecond = 59;
    if (selectedSecond > 59) selectedSecond = 0;
  }
  console.log("[INTERVAL] After adjust:", { selectedMinute, selectedSecond });
  updateIntervalDisplay();
}

function updateIntervalDisplay() {
  document.getElementById("minuteDisplay").textContent = String(
    selectedMinute
  ).padStart(2, "0");
  document.getElementById("secondDisplay").textContent = String(
    selectedSecond
  ).padStart(2, "0");
  console.log("[INTERVAL] Display updated:", {
    selectedMinute,
    selectedSecond,
  });
}

function setPeriodicInterval() {
  const totalSeconds = selectedMinute * 60 + selectedSecond;
  console.log("[INTERVAL] Setting interval:", {
    selectedMinute,
    selectedSecond,
    totalSeconds,
  });

  if (totalSeconds < 1) {
    addStatus("Interval must be at least 1 second", "ERROR");
    return;
  }

  if (!isMqttConnected) {
    addStatus("MQTT not connected", "ERROR");
    return;
  }

  const command = `SET PERIODIC INTERVAL ${totalSeconds}`;
  console.log(
    "[INTERVAL] Publishing command:",
    command,
    "to",
    MQTT_CONFIG.topics.stm32Command
  );
  mqttClient.publish(MQTT_CONFIG.topics.stm32Command, command, { qos: 1 });

  addStatus(
    `Interval set to ${selectedMinute}m ${selectedSecond}s (${totalSeconds}s)`,
    "SETTING"
  );
  closeIntervalModal();
}

// Interval modal button handlers
function bindIntervalButtons() {
  console.log("[INTERVAL] Binding buttons...");

  // Get all button elements
  const minuteUp = document.getElementById("minuteUpBtn");
  const minuteDown = document.getElementById("minuteDownBtn");
  const secondUp = document.getElementById("secondUpBtn");
  const secondDown = document.getElementById("secondDownBtn");
  const cancelBtn = document.getElementById("intervalCancelBtn");
  const setBtn = document.getElementById("intervalSetBtn");

  console.log("[INTERVAL] Found buttons:", {
    minuteUp: !!minuteUp,
    minuteDown: !!minuteDown,
    secondUp: !!secondUp,
    secondDown: !!secondDown,
    cancelBtn: !!cancelBtn,
    setBtn: !!setBtn,
  });

  // Remove old listeners by cloning and replacing
  if (minuteUp) {
    const newMinuteUp = minuteUp.cloneNode(true);
    minuteUp.parentNode.replaceChild(newMinuteUp, minuteUp);
    newMinuteUp.addEventListener("click", () => {
      console.log("[INTERVAL] Minute UP clicked");
      adjustInterval("minute", 1);
    });
  }

  if (minuteDown) {
    const newMinuteDown = minuteDown.cloneNode(true);
    minuteDown.parentNode.replaceChild(newMinuteDown, minuteDown);
    newMinuteDown.addEventListener("click", () => {
      console.log("[INTERVAL] Minute DOWN clicked");
      adjustInterval("minute", -1);
    });
  }

  if (secondUp) {
    const newSecondUp = secondUp.cloneNode(true);
    secondUp.parentNode.replaceChild(newSecondUp, secondUp);
    newSecondUp.addEventListener("click", () => {
      console.log("[INTERVAL] Second UP clicked");
      adjustInterval("second", 1);
    });
  }

  if (secondDown) {
    const newSecondDown = secondDown.cloneNode(true);
    secondDown.parentNode.replaceChild(newSecondDown, secondDown);
    newSecondDown.addEventListener("click", () => {
      console.log("[INTERVAL] Second DOWN clicked");
      adjustInterval("second", -1);
    });
  }

  if (cancelBtn) {
    const newCancelBtn = cancelBtn.cloneNode(true);
    cancelBtn.parentNode.replaceChild(newCancelBtn, cancelBtn);
    newCancelBtn.addEventListener("click", () => {
      console.log("[INTERVAL] Cancel clicked");
      closeIntervalModal();
    });
  }

  if (setBtn) {
    const newSetBtn = setBtn.cloneNode(true);
    setBtn.parentNode.replaceChild(newSetBtn, setBtn);
    newSetBtn.addEventListener("click", () => {
      console.log("[INTERVAL] Set clicked");
      setPeriodicInterval();
    });
  }

  console.log("[INTERVAL] All buttons bound successfully");
}

// ====================================================================
// FOOTER CLOCK
function updateFooterClock() {
  let display;
  if (deviceClockMs !== null && deviceClockSetAtMs !== null) {
    const elapsed = Date.now() - deviceClockSetAtMs;
    const currentDeviceTime = deviceClockMs + elapsed;
    display = new Date(currentDeviceTime).toLocaleTimeString("en-US", {
      hour12: false,
    });

    // Debug: log every 10 seconds to verify it's updating
    if (
      Math.floor(elapsed / 1000) % 10 === 0 &&
      elapsed > 0 &&
      elapsed < 1000
    ) {
      console.log("[CLOCK] Device time updating:", {
        deviceClockMs,
        deviceClockSetAtMs,
        elapsed,
        currentDeviceTime,
        display,
      });
    }
  } else {
    display = "--:--:--";
  }
  const el = document.getElementById("footerTime");
  if (el) el.textContent = display;
}

function startDeviceClockTicker() {
  if (clockIntervalId) clearInterval(clockIntervalId);
  clockIntervalId = setInterval(updateFooterClock, 1000);
  updateFooterClock();
}

// ====================================================================
// INITIALIZATION
// ====================================================================

// Function to run initialization tasks
function runInitialization() {
  addStatus("DataLogger Monitor starting...", "INFO");

  // Load saved config from localStorage
  const savedMQTT = localStorage.getItem("datalogger_mqtt_config");
  const savedFirebase = localStorage.getItem("datalogger_firebase_config");

  if (savedMQTT) {
    const config = JSON.parse(savedMQTT);
    MQTT_CONFIG.host = config.host || MQTT_CONFIG.host;
    MQTT_CONFIG.port = config.port || MQTT_CONFIG.port;
    MQTT_CONFIG.path = config.path || MQTT_CONFIG.path;
    MQTT_CONFIG.username = config.username || MQTT_CONFIG.username;
    MQTT_CONFIG.password = config.password || MQTT_CONFIG.password;
    MQTT_CONFIG.clientId = config.clientId || MQTT_CONFIG.clientId;
    MQTT_CONFIG.url = `ws://${MQTT_CONFIG.host}:${MQTT_CONFIG.port}${MQTT_CONFIG.path}`;

    // Update form fields
    document.getElementById("mqttIp").value = MQTT_CONFIG.host;
    document.getElementById("mqttPort").value = MQTT_CONFIG.port;
    document.getElementById("mqttPath").value = MQTT_CONFIG.path;
    document.getElementById("mqttUser").value = MQTT_CONFIG.username;
    document.getElementById("mqttPass").value = MQTT_CONFIG.password;
    document.getElementById("mqttClientId").value = MQTT_CONFIG.clientId;
  }

  if (savedFirebase) {
    const config = JSON.parse(savedFirebase);
    FIREBASE_CONFIG.databaseURL =
      config.databaseURL || FIREBASE_CONFIG.databaseURL;
    FIREBASE_CONFIG.apiKey = config.apiKey || FIREBASE_CONFIG.apiKey;
    FIREBASE_CONFIG.projectId = config.projectId || FIREBASE_CONFIG.projectId;

    // Update form fields
    document.getElementById("firebaseUrl").value = FIREBASE_CONFIG.databaseURL;
    document.getElementById("firebaseApiKey").value = FIREBASE_CONFIG.apiKey;
    document.getElementById("firebaseProject").value =
      FIREBASE_CONFIG.projectId;
  }

  // Initialize charts FIRST - CRITICAL for rendering
  console.log("[INIT] Calling initializeCharts()");
  initializeCharts();
  addStatus("Charts initialized", "INFO");

  // Initialize Firebase
  initializeFirebase();

  // Initialize MQTT
  initializeMQTT();

  // Update footer clock immediately
  updateFooterClock();

  addStatus("System ready", "INFO");
}

// Run initialization when DOM is ready
if (document.readyState === "loading") {
  // DOM is still loading
  window.addEventListener("DOMContentLoaded", runInitialization);
} else {
  // DOM is already loaded (script loaded after DOM)
  console.log("[INIT] DOM already loaded, running initialization immediately");
  runInitialization();
}
