// Global Variables
let isPeriodic = false;
let isDeviceOn = false;
let isMqttConnected = false;
let isFirebaseConnected = false;
let temperatureData = [];
let humidityData = [];
let statusQueue = [];
let maxDataPoints = 50;
let maxStatusItems = 5;
let mqttClient = null;
let firebaseDb = null;

// Current values for display
let currentTemp = null;
let currentHumi = null;

// State synchronization management
let stateSync = {
  lastKnownState: null,
  syncInProgress: false,
  syncRequested: false, // Track if we already requested sync
  syncRetryCount: 0,
  maxSyncRetries: 1, // Reduce retries (ESP32 will auto-publish on reconnect)
  deviceOffLock: false,
  deviceOffLockTimeout: null,
  lastSyncMessage: "", // Track duplicate messages
  ignoreNextPeriodicSync: false, // Flag to ignore periodic state from ESP32
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
  url: "ws://127.0.0.1:8083/mqtt",
  topics: {
    // Command topics
    stm32Command: "datalogger/stm32/command",
    relayControl: "datalogger/esp32/relay/control",
    systemState: "datalogger/esp32/system/state",

    // JSON data topics (recommended)
    singleData: "datalogger/stm32/single/data",
    periodicData: "datalogger/stm32/periodic/data",
  },
};

// Enhanced Chart configurations
const chartTempConfig = {
  type: "line",
  data: {
    labels: [],
    datasets: [
      {
        label: "Temperature (°C)",
        data: [],
        borderColor: "rgba(255, 99, 132, 1)",
        backgroundColor: "rgba(255, 99, 132, 0.1)",
        tension: 0.4,
        pointRadius: 4,
        pointHoverRadius: 8,
        borderWidth: 3,
        fill: true,
        pointBackgroundColor: "rgba(255, 99, 132, 1)",
        pointBorderColor: "white",
        pointBorderWidth: 2,
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
        display: true,
        position: "top",
        labels: {
          color: "#2c3e50",
          font: { size: 12, weight: "bold" },
          usePointStyle: true,
          pointStyle: "circle",
        },
      },
      tooltip: {
        backgroundColor: "rgba(255, 99, 132, 0.9)",
        titleColor: "white",
        bodyColor: "white",
        cornerRadius: 8,
        displayColors: false,
        callbacks: {
          label: function (context) {
            return `Temperature: ${context.parsed.y.toFixed(1)}°C`;
          },
        },
      },
    },
    scales: {
      y: {
        beginAtZero: false,
        grid: {
          color: "rgba(255, 99, 132, 0.1)",
          drawBorder: false,
        },
        ticks: {
          color: "#2c3e50",
          font: { size: 11, weight: "bold" },
          callback: function (value) {
            return value.toFixed(1) + "°C";
          },
        },
      },
      x: {
        grid: {
          color: "rgba(255, 99, 132, 0.1)",
          drawBorder: false,
        },
        ticks: {
          color: "#2c3e50",
          font: { size: 10 },
          maxTicksLimit: 8,
        },
      },
    },
    animation: {
      duration: 750,
      easing: "easeInOutQuart",
    },
  },
};

// Humidity Chart Configuration
const chartHumiConfig = {
  type: "line",
  data: {
    labels: [],
    datasets: [
      {
        label: "Humidity (%)",
        data: [],
        borderColor: "rgba(54, 162, 235, 1)",
        backgroundColor: "rgba(54, 162, 235, 0.1)",
        tension: 0.4,
        pointRadius: 4,
        pointHoverRadius: 8,
        borderWidth: 3,
        fill: true,
        pointBackgroundColor: "rgba(54, 162, 235, 1)",
        pointBorderColor: "white",
        pointBorderWidth: 2,
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
        display: true,
        position: "top",
        labels: {
          color: "#2c3e50",
          font: { size: 12, weight: "bold" },
          usePointStyle: true,
          pointStyle: "circle",
        },
      },
      tooltip: {
        backgroundColor: "rgba(54, 162, 235, 0.9)",
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
        grid: {
          color: "rgba(54, 162, 235, 0.1)",
          drawBorder: false,
        },
        ticks: {
          color: "#2c3e50",
          font: { size: 11, weight: "bold" },
          callback: function (value) {
            return value.toFixed(1) + "%";
          },
        },
      },
      x: {
        grid: {
          color: "rgba(54, 162, 235, 0.1)",
          drawBorder: false,
        },
        ticks: {
          color: "#2c3e50",
          font: { size: 10 },
          maxTicksLimit: 8,
        },
      },
    },
    animation: {
      duration: 750,
      easing: "easeInOutQuart",
    },
  },
};

// Initialize charts
let chart1, chart2;

// Firebase Functions
function initializeFirebase() {
  if (
    !FIREBASE_CONFIG.apiKey ||
    !FIREBASE_CONFIG.databaseURL ||
    !FIREBASE_CONFIG.projectId
  ) {
    addStatus("[WARNING] Firebase configuration incomplete", "WARNING");
    return false;
  }

  try {
    if (firebase.apps.length === 0) {
      firebase.initializeApp(FIREBASE_CONFIG);
    }
    firebaseDb = firebase.database();

    firebaseDb.ref(".info/connected").on("value", function (snapshot) {
      if (snapshot.val() === true) {
        firebaseDb
          .ref("test/connection")
          .set({
            timestamp: Date.now(),
            message: "Connection test",
          })
          .then(() => {
            updateFirebaseStatus(true);
            addStatus("Firebase write permissions verified", "FIREBASE");
            firebaseDb.ref("test").remove();
          })
          .catch((error) => {
            updateFirebaseStatus(false);
            addStatus(`Firebase permission error: ${error.code}`, "ERROR");
          });
      } else {
        updateFirebaseStatus(false);
      }
    });

    return true;
  } catch (error) {
    addStatus(`Firebase init error: ${error.message}`, "ERROR");
    return false;
  }
}

// Update Firebase connection status
function updateFirebaseStatus(connected) {
  isFirebaseConnected = connected;
  const firebaseDot = document.getElementById("firebaseDot");
  const firebaseText = document.getElementById("firebaseText");

  if (connected) {
    firebaseDot.className = "status-dot connected";
    firebaseText.textContent = "Firebase Connected";
    addStatus("Firebase database connected", "FIREBASE");
  } else {
    firebaseDot.className = "status-dot disconnected";
    firebaseText.textContent = "Firebase Disconnected";
    addStatus("Firebase database disconnected", "FIREBASE");
  }
}

// Save data to Firebase
function saveToFirebase(type, value, timestamp) {
  if (!isFirebaseConnected || !firebaseDb) return;

  const data = {
    value: value,
    timestamp: timestamp,
    date: new Date(timestamp).toISOString(),
    source: isPeriodic ? "periodic" : "single",
  };

  firebaseDb
    .ref(`sht31/${type}/${timestamp}`)
    .set(data)
    .then(() => {
      console.log(`Saved ${type}: ${value} to Firebase`);
    })
    .catch((error) => {
      addStatus(`Firebase save error: ${error.message}`, "ERROR");
    });
}

// Load historical data from Firebase
function loadHistoricalData() {
  if (!isFirebaseConnected || !firebaseDb) {
    addStatus("Firebase not connected", "ERROR");
    return;
  }

  addStatus("Loading historical data...", "FIREBASE");
  clearChartData();

  // Load temperature data
  firebaseDb
    .ref("sht31/temperature")
    .limitToLast(maxDataPoints)
    .once("value", (snapshot) => {
      const tempData = snapshot.val();
      if (tempData) {
        const tempArray = Object.values(tempData);
        tempArray.forEach((item) => {
          const timestamp = new Date(item.timestamp).toLocaleTimeString(
            "en-US",
            {
              hour12: false,
              hour: "2-digit",
              minute: "2-digit",
              second: "2-digit",
            }
          );

          if (chart1) {
            chart1.data.labels.push(timestamp);
            chart1.data.datasets[0].data.push(item.value);
          }
          temperatureData.push(item.value);
        });

        if (chart1) {
          chart1.update("none");
          updateTempStats();
        }
      }
    });

  // Load humidity data
  firebaseDb
    .ref("sht31/humidity")
    .limitToLast(maxDataPoints)
    .once("value", (snapshot) => {
      const humiData = snapshot.val();
      if (humiData) {
        const humiArray = Object.values(humiData);
        humiArray.forEach((item) => {
          const timestamp = new Date(item.timestamp).toLocaleTimeString(
            "en-US",
            {
              hour12: false,
              hour: "2-digit",
              minute: "2-digit",
              second: "2-digit",
            }
          );

          if (chart2) {
            chart2.data.labels.push(timestamp);
            chart2.data.datasets[0].data.push(item.value);
          }
          humidityData.push(item.value);
        });

        if (chart2) {
          chart2.update("none");
          updateHumiStats();
        }
      }
      addStatus("Historical data loaded successfully", "FIREBASE");
    });
}

// Clear chart data
function clearChartData() {
  if (chart1) {
    chart1.data.labels = [];
    chart1.data.datasets[0].data = [];
    chart1.update("none");
  }

  if (chart2) {
    chart2.data.labels = [];
    chart2.data.datasets[0].data = [];
    chart2.update("none");
  }

  temperatureData = [];
  humidityData = [];

  updateTempStats();
  updateHumiStats();
  addStatus("Chart data cleared", "INFO");
}

// Statistics calculation functions
function updateTempStats() {
  const tempStats = document.getElementById("tempStats");
  if (tempStats) {
    if (temperatureData.length > 0) {
      const min = Math.min(...temperatureData).toFixed(1);
      const max = Math.max(...temperatureData).toFixed(1);
      const avg = (
        temperatureData.reduce((a, b) => a + b, 0) / temperatureData.length
      ).toFixed(1);
      tempStats.textContent = `Min: ${min}°C | Max: ${max}°C | Avg: ${avg}°C`;
    } else {
      tempStats.textContent = "Min: -- | Max: -- | Avg: --";
    }
  }
}

// Statistics calculation functions
function updateHumiStats() {
  const humiStats = document.getElementById("humiStats");
  if (humiStats) {
    if (humidityData.length > 0) {
      const min = Math.min(...humidityData).toFixed(1);
      const max = Math.max(...humidityData).toFixed(1);
      const avg = (
        humidityData.reduce((a, b) => a + b, 0) / humidityData.length
      ).toFixed(1);
      humiStats.textContent = `Min: ${min}% | Max: ${max}% | Avg: ${avg}%`;
    } else {
      humiStats.textContent = "Min: -- | Max: -- | Avg: --";
    }
  }
}

// Device OFF lock management
function setDeviceOffLock(duration = 1500) {
  // Clear existing timeout if any
  if (stateSync.deviceOffLockTimeout) {
    clearTimeout(stateSync.deviceOffLockTimeout);
  }

  stateSync.deviceOffLock = true;
  addStatus(`Device OFF lock activated for ${duration}ms`, "SYNC");

  stateSync.deviceOffLockTimeout = setTimeout(() => {
    stateSync.deviceOffLock = false;
    stateSync.deviceOffLockTimeout = null;
    addStatus("Device OFF lock released", "SYNC");
  }, duration);
}

// State synchronization functions
function requestStateSync() {
  if (!isMqttConnected) {
    console.log("Cannot sync: MQTT not connected");
    return;
  }

  // Only request once per connection
  if (stateSync.syncRequested) {
    console.log("State sync already requested for this connection");
    return;
  }

  if (stateSync.syncInProgress) {
    console.log("Cannot sync: Sync already in progress");
    return;
  }

  if (stateSync.syncRetryCount >= stateSync.maxSyncRetries) {
    addStatus("Waiting for ESP32 state broadcast...", "SYNC");
    return;
  }

  stateSync.syncInProgress = true;
  stateSync.syncRequested = true; // Mark that we've requested sync
  stateSync.syncRetryCount++;

  console.log("Requesting state sync from ESP32 (once per connection)...");
  addStatus("Requesting system state from ESP32...", "SYNC");

  // Request current state from ESP32 (only once)
  publishMQTT(MQTT_CONFIG.topics.systemState, "REQUEST");

  // Reset sync state after timeout
  setTimeout(() => {
    if (stateSync.syncInProgress) {
      stateSync.syncInProgress = false;
      addStatus("Using retained state or defaults", "INFO");
      console.log("State sync complete (timeout)");
    }
  }, 2000);
}

// Parse state message from ESP32
function parseStateMessage(stateData) {
  try {
    // Parse JSON state message from ESP32
    const state = JSON.parse(stateData);

    return {
      device: state.device === "ON",
      periodic: state.periodic === "ON",
      // Timestamp removed - not meaningful for state sync
    };
  } catch (error) {
    console.log("Error parsing state message:", error, "Data:", stateData);
    return null;
  }
}

// Synchronize UI with hardware state
function syncUIWithHardwareState(parsedState) {
  if (!parsedState) return;

  // Create a unique identifier for this sync message
  const syncId = `${parsedState.device ? "ON" : "OFF"}_${
    parsedState.periodic ? "ON" : "OFF"
  }_${parsedState.rate}`;

  // FIXED: Prevent duplicate sync messages from spamming
  if (stateSync.lastSyncMessage === syncId) {
    console.log("Duplicate sync message ignored:", syncId);
    return;
  }
  stateSync.lastSyncMessage = syncId;

  // FIXED: Only skip sync for device ON transitions when lock is active
  // Allow device OFF sync to proceed normally
  if (
    stateSync.deviceOffLock &&
    parsedState.device &&
    isDeviceOn !== parsedState.device
  ) {
    addStatus("State sync skipped - device ON transition locked", "SYNC");
    return;
  }

  const previousDeviceState = isDeviceOn;
  const previousPeriodicState = isPeriodic;
  let stateChanged = false;

  // CRITICAL: Handle device state change first
  if (isDeviceOn !== parsedState.device) {
    isDeviceOn = parsedState.device;
    const deviceBtn = document.getElementById("deviceBtn");
    if (deviceBtn) {
      deviceBtn.textContent = isDeviceOn ? "DEVICE ON" : "DEVICE OFF";
      deviceBtn.classList.toggle("on", isDeviceOn);
    }
    stateChanged = true;

    // FIXED: Reset current values immediately when device goes OFF via sync
    if (!isDeviceOn) {
      currentTemp = null;
      currentHumi = null;
      updateCurrentDisplay();
      addStatus("Current values reset (device OFF via sync)", "SYNC");
    }
  }

  // Handle periodic mode - NEVER auto-start, only sync UI
  if (!isDeviceOn) {
    // Device is OFF -> periodic MUST be OFF
    if (isPeriodic) {
      console.log("State sync: Device is OFF, forcing periodic mode OFF");
      isPeriodic = false;
      const periodicBtn = document.getElementById("periodicBtn");
      const stopBtn = document.getElementById("stopBtn");
      if (periodicBtn) periodicBtn.style.display = "block";
      if (stopBtn) stopBtn.style.display = "none";
      stateChanged = true;

      // Send stop command if needed
      if (previousPeriodicState) {
        publishMQTT(MQTT_CONFIG.topics.stm32Command, "PERIODIC OFF");
        addStatus("Periodic mode force-stopped (device OFF)", "SYNC");
      }
    }
  } else {
    // Device is ON
    // CRITICAL: Only update UI, NEVER send commands automatically
    // Periodic should ONLY start/stop via user button clicks
    if (isPeriodic !== parsedState.periodic) {
      isPeriodic = parsedState.periodic;
      const periodicBtn = document.getElementById("periodicBtn");
      const stopBtn = document.getElementById("stopBtn");

      if (isPeriodic) {
        if (periodicBtn) periodicBtn.style.display = "none";
        if (stopBtn) stopBtn.style.display = "block";
      } else {
        if (periodicBtn) periodicBtn.style.display = "block";
        if (stopBtn) stopBtn.style.display = "none";
      }
      stateChanged = true;

      console.log(
        `Periodic state synced from ESP32: ${isPeriodic ? "ON" : "OFF"}`
      );
    }
  }

  // Update frame rate
  // Frame rate tracking removed - not needed for new API

  // Only log if something actually changed
  if (stateChanged) {
    stateSync.lastKnownState = parsedState;
    addStatus(
      `State synchronized: Device=${
        parsedState.device ? "ON" : "OFF"
      }, Periodic=${isPeriodic ? "ON" : "OFF"}`,
      "SYNC"
    );
  }

  stateSync.syncInProgress = false;
  stateSync.syncRetryCount = 0;
}

// MQTT Functions
function updateConnectionStatus(connected) {
  isMqttConnected = connected;
  const statusDot = document.getElementById("statusDot");
  const statusText = document.getElementById("statusText");

  if (connected) {
    statusDot.className = "status-dot connected";
    statusText.textContent = "MQTT Connected";
    addStatus("MQTT broker connected", "MQTT");

    // Request state sync after connection
    setTimeout(() => {
      requestStateSync();
    }, 1000);
  } else {
    statusDot.className = "status-dot disconnected";
    statusText.textContent = "MQTT Disconnected";
    addStatus("MQTT broker disconnected", "MQTT");

    // Reset sync state
    stateSync.syncInProgress = false;
    stateSync.syncRetryCount = 0;
    stateSync.lastSyncMessage = "";
  }
}

// Publish MQTT message
function publishMQTT(topic, message) {
  if (mqttClient && isMqttConnected) {
    console.log("Publishing MQTT:", topic, "=", message);
    mqttClient.publish(topic, message, { qos: 0, retain: false }, (err) => {
      if (err) {
        console.error("Publish error:", err);
        addStatus(`Publish failed: ${err.message}`, "ERROR");
      } else {
        console.log("Publish success:", topic);
      }
    });
  } else {
    console.error("Cannot publish: MQTT not ready", {
      mqttClient: !!mqttClient,
      connected: isMqttConnected,
    });
    addStatus("Cannot publish: MQTT not connected", "ERROR");
  }
}

// Connect to MQTT broker
function connectMQTT() {
  try {
    MQTT_CONFIG.url = `ws://${MQTT_CONFIG.host}:${MQTT_CONFIG.port}${MQTT_CONFIG.path}`;

    if (mqttClient) {
      mqttClient.end(true);
      mqttClient = null;
    }

    addStatus("Connecting to MQTT...", "MQTT");

    const connectOptions = {
      clientId: "WebClient_" + Math.random().toString(16).substring(2, 8),
      reconnectPeriod: 2000,
      clean: true,
      connectTimeout: 10000,
      keepalive: 30,
      protocolVersion: 4,
      protocolId: "MQTT",
    };

    if (MQTT_CONFIG.username) {
      connectOptions.username = MQTT_CONFIG.username;
      if (MQTT_CONFIG.password) {
        connectOptions.password = MQTT_CONFIG.password;
      }
    }

    mqttClient = mqtt.connect(MQTT_CONFIG.url, connectOptions);

    mqttClient.on("connect", (connack) => {
      console.log("MQTT Connected:", connack);
      updateConnectionStatus(true);

      // Reset sync flags on new connection
      stateSync.syncRequested = false;
      stateSync.syncInProgress = false;
      stateSync.syncRetryCount = 0;

      // Subscribe to JSON data topics only
      const topics = [
        MQTT_CONFIG.topics.singleData,
        MQTT_CONFIG.topics.periodicData,
        MQTT_CONFIG.topics.systemState,
      ];

      mqttClient.subscribe(topics, { qos: 0 }, (err) => {
        if (err) {
          console.error("Subscribe error:", err);
          addStatus("MQTT subscribe failed: " + err.message, "ERROR");
        } else {
          console.log("Subscribed to topics:", topics);
          addStatus("Subscribed to all topics", "MQTT");
          addStatus("JSON data format active", "INFO");

          // Request state sync ONCE after successful subscription
          console.log("Scheduling state sync request (once per connection)...");
          setTimeout(() => {
            requestStateSync();
          }, 1000);
        }
      });
    });

    mqttClient.on("reconnect", () => {
      addStatus("MQTT reconnecting...", "MQTT");
      // Reset sync flags on reconnect
      stateSync.syncRequested = false;
      stateSync.syncRetryCount = 0;
    });

    mqttClient.on("error", (e) => {
      console.error("MQTT Error:", e);
      updateConnectionStatus(false);

      let errorMsg = "Connection error";
      if (e.code === 5) {
        errorMsg = "Not authorized - check credentials";
      } else if (e.code === 4) {
        errorMsg = "Bad username or password";
      } else if (e.message) {
        errorMsg = e.message;
      }

      addStatus("MQTT error: " + errorMsg, "ERROR");
    });

    mqttClient.on("offline", () => {
      updateConnectionStatus(false);
    });

    mqttClient.on("close", () => {
      updateConnectionStatus(false);
    });

    mqttClient.on("message", (topic, payload) => {
      const text = payload.toString();
      console.log("MQTT Message:", topic, text);

      // FIXED: Handle state synchronization messages
      if (topic === MQTT_CONFIG.topics.systemState) {
        const parsedState = parseStateMessage(text);
        if (parsedState) {
          syncUIWithHardwareState(parsedState);
        }
        return;
      }

      // Handle JSON data topics (recommended)
      if (
        topic === MQTT_CONFIG.topics.singleData ||
        topic === MQTT_CONFIG.topics.periodicData
      ) {
        try {
          const jsonData = JSON.parse(text);
          const timestamp = jsonData.timestamp * 1000 || Date.now(); // Convert Unix to milliseconds
          const isPeriodic = jsonData.mode === "PERIODIC";

          // Check for sensor failure
          if (jsonData.temperature === 0.0 && jsonData.humidity === 0.0) {
            addStatus("⚠️ Sensor hardware failed (disconnected)", "WARNING");
          }

          // Check for RTC failure
          if (jsonData.timestamp === 0) {
            addStatus("⚠️ RTC failed (using local time)", "WARNING");
          }

          // Process temperature
          if (jsonData.temperature !== undefined) {
            const modeLabel = isPeriodic ? "Periodic" : "Single";
            addStatus(
              `${modeLabel} temp: ${jsonData.temperature.toFixed(2)}°C`,
              isPeriodic ? "DATA" : "SINGLE"
            );
            pushTemperature(jsonData.temperature, isPeriodic, timestamp);
          }

          // Process humidity
          if (jsonData.humidity !== undefined) {
            const modeLabel = isPeriodic ? "Periodic" : "Single";
            addStatus(
              `${modeLabel} humi: ${jsonData.humidity.toFixed(2)}%`,
              isPeriodic ? "DATA" : "SINGLE"
            );
            pushHumidity(jsonData.humidity, isPeriodic, timestamp);
          }

          return;
        } catch (e) {
          console.error("JSON parse error:", e);
          addStatus("Failed to parse JSON data: " + text, "ERROR");
        }
      }
    });
  } catch (e) {
    console.error("MQTT Init Error:", e);
    updateConnectionStatus(false);
    addStatus("MQTT init failed: " + e.message, "ERROR");
  }
}

// Modal Functions
function openModal() {
  document.getElementById("mqttModal").style.display = "flex";
  document.getElementById("mqttIp").value = MQTT_CONFIG.host;
  document.getElementById("mqttPort").value = MQTT_CONFIG.port;
  document.getElementById("mqttPath").value = MQTT_CONFIG.path;
  document.getElementById("mqttUser").value = MQTT_CONFIG.username;
  document.getElementById("mqttPass").value = MQTT_CONFIG.password;
  document.getElementById("firebaseUrl").value = FIREBASE_CONFIG.databaseURL;
  document.getElementById("firebaseApiKey").value = FIREBASE_CONFIG.apiKey;
  document.getElementById("firebaseProject").value = FIREBASE_CONFIG.projectId;
}

// Close modal
function closeModal() {
  document.getElementById("mqttModal").style.display = "none";
}

// Save settings and connect
function saveAndConnect() {
  const ip = document.getElementById("mqttIp").value.trim();
  const port = document.getElementById("mqttPort").value.trim();
  const path = document.getElementById("mqttPath").value.trim();
  const username = document.getElementById("mqttUser").value.trim();
  const password = document.getElementById("mqttPass").value.trim();

  const firebaseUrl = document.getElementById("firebaseUrl").value.trim();
  const firebaseApiKey = document.getElementById("firebaseApiKey").value.trim();
  const firebaseProject = document
    .getElementById("firebaseProject")
    .value.trim();

  if (!ip || !port) {
    addStatus("Please enter valid IP and port", "ERROR");
    return;
  }

  MQTT_CONFIG.host = ip;
  MQTT_CONFIG.port = port;
  MQTT_CONFIG.path = path || "/mqtt";
  MQTT_CONFIG.username = username;
  MQTT_CONFIG.password = password;

  FIREBASE_CONFIG.databaseURL = firebaseUrl;
  FIREBASE_CONFIG.apiKey = firebaseApiKey;
  FIREBASE_CONFIG.projectId = firebaseProject;

  closeModal();
  addStatus(`Connecting to ${ip}:${port}${MQTT_CONFIG.path}...`, "MQTT");

  if (username) {
    addStatus(`Using authentication: ${username}`, "MQTT");
  }

  connectMQTT();

  if (firebaseUrl && firebaseApiKey && firebaseProject) {
    addStatus("Initializing Firebase...", "FIREBASE");
    initializeFirebase();
  }
}

// Device validation function
function validateDeviceState(operation) {
  if (!isDeviceOn) {
    addStatus(`[WARNING] Turn on device first`, "WARNING");
    return false;
  }
  if (!isMqttConnected) {
    addStatus("MQTT not connected", "ERROR");
    return false;
  }
  return true;
}

// Enhanced helper functions with Firebase integration
function pushTemperature(
  newTemp,
  isPeriodicData = false,
  timestamp = Date.now()
) {
  currentTemp = newTemp;
  updateCurrentDisplay();

  // Save to Firebase if enabled and is periodic data
  if (isFirebaseConnected && isPeriodicData) {
    saveToFirebase("temperature", newTemp, timestamp);
  }

  // Only update chart if in periodic mode AND the data is from periodic source
  if (isPeriodicData && isPeriodic && chart1) {
    const timeString = new Date(timestamp).toLocaleTimeString("en-US", {
      hour12: false,
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
    });

    chart1.data.labels.push(timeString);
    chart1.data.datasets[0].data.push(newTemp);

    if (chart1.data.labels.length > maxDataPoints) {
      chart1.data.labels.shift();
      chart1.data.datasets[0].data.shift();
    }
    chart1.update("none");

    // Store data for statistics
    temperatureData.push(newTemp);
    if (temperatureData.length > maxDataPoints) {
      temperatureData.shift();
    }
    updateTempStats();
  }
}

// Enhanced helper functions with Firebase integration
function pushHumidity(newHumi, isPeriodicData = false, timestamp = Date.now()) {
  currentHumi = newHumi;
  updateCurrentDisplay();

  // Save to Firebase if enabled and is periodic data
  if (isFirebaseConnected && isPeriodicData) {
    saveToFirebase("humidity", newHumi, timestamp);
  }

  // Only update chart if in periodic mode AND the data is from periodic source
  if (isPeriodicData && isPeriodic && chart2) {
    const timeString = new Date(timestamp).toLocaleTimeString("en-US", {
      hour12: false,
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
    });

    chart2.data.labels.push(timeString);
    chart2.data.datasets[0].data.push(newHumi);

    if (chart2.data.labels.length > maxDataPoints) {
      chart2.data.labels.shift();
      chart2.data.datasets[0].data.shift();
    }
    chart2.update("none");

    // Store data for statistics
    humidityData.push(newHumi);
    if (humidityData.length > maxDataPoints) {
      humidityData.shift();
    }
    updateHumiStats();
  }
}

// Update combined display
function updateCurrentDisplay() {
  const el = document.getElementById("currentDisplay");
  if (el) {
    // Show -- when device is OFF or values are null
    if (!isDeviceOn || currentTemp === null || currentHumi === null) {
      el.textContent = `Current: --°C & --% RH`;
    } else {
      el.textContent = `Current: ${currentTemp.toFixed(
        1
      )}°C & ${currentHumi.toFixed(1)}% RH`;
    }
    el.classList.add("pulse");
    setTimeout(() => el.classList.remove("pulse"), 800);
  }
}

// Status management with enhanced performance
function addStatus(message, type = "INFO") {
  const timestamp = new Date().toLocaleTimeString("en-US", {
    hour12: false,
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  });
  const statusItem = {
    text: `[${type}] ${timestamp}: ${message}`,
    type: type,
  };

  statusQueue.push(statusItem);
  if (statusQueue.length > maxStatusItems) {
    statusQueue.shift();
  }

  requestAnimationFrame(updateStatusDisplay);
}

// Update status display
function updateStatusDisplay() {
  const statusDisplay = document.getElementById("statusDisplay");
  if (statusDisplay) {
    statusDisplay.innerHTML = statusQueue
      .map((item) => {
        let className = "status-item";
        if (item.type === "WARNING") {
          className += " status-warning";
        } else if (item.type === "ERROR") {
          className += " status-error";
        } else if (item.type === "MQTT") {
          className += " status-mqtt";
        } else if (item.type === "FIREBASE") {
          className += " status-firebase";
        } else if (item.type === "SYNC") {
          className += " status-sync";
        }
        return `<div class="${className}">${item.text}</div>`;
      })
      .join("");
    statusDisplay.scrollTop = statusDisplay.scrollHeight;
  }
}

// Control Functions
function startPeriodicMode() {
  if (!validateDeviceState("periodic mode")) return;

  publishMQTT(MQTT_CONFIG.topics.stm32Command, "PERIODIC ON");

  // Update UI immediately (will be synced with hardware via state sync)
  isPeriodic = true;
  document.getElementById("stopBtn").style.display = "block";
  document.getElementById("periodicBtn").style.display = "none";

  addStatus("Started periodic mode", "START");
}

function stopPeriodicMode() {
  if (!isMqttConnected) {
    addStatus("MQTT not connected", "ERROR");
    return;
  }

  publishMQTT(MQTT_CONFIG.topics.stm32Command, "PERIODIC OFF");

  // Update UI immediately (will be synced with hardware via state sync)
  isPeriodic = false;
  document.getElementById("stopBtn").style.display = "none";
  document.getElementById("periodicBtn").style.display = "block";

  addStatus("Stopped periodic mode", "STOP");
}

// Single read function
function singleRead() {
  if (!validateDeviceState("single read")) return;

  publishMQTT(MQTT_CONFIG.topics.stm32Command, "SINGLE");
  addStatus("Single read command sent", "SINGLE");
}

// Toggle device ON/OFF
function toggleDevice() {
  if (!isMqttConnected) {
    addStatus("MQTT not connected", "ERROR");
    return;
  }

  const command = isDeviceOn ? "RELAY OFF" : "RELAY ON";
  const willBeDeviceOn = !isDeviceOn; // Store the future state

  publishMQTT(MQTT_CONFIG.topics.relayControl, command);

  // Update UI state immediately
  isDeviceOn = willBeDeviceOn;
  const deviceBtn = document.getElementById("deviceBtn");
  if (deviceBtn) {
    deviceBtn.textContent = isDeviceOn ? "DEVICE ON" : "DEVICE OFF";
    deviceBtn.classList.toggle("on", isDeviceOn);
  }

  // CRITICAL: Handle periodic mode BEFORE any async operations
  // This ensures periodic mode is stopped immediately when device is turned off
  if (!isDeviceOn && isPeriodic) {
    console.log("Device turned OFF - stopping periodic mode immediately");
    isPeriodic = false; // Set flag immediately to prevent race conditions

    // Update UI immediately
    const periodicBtn = document.getElementById("periodicBtn");
    const stopBtn = document.getElementById("stopBtn");
    if (periodicBtn) periodicBtn.style.display = "block";
    if (stopBtn) stopBtn.style.display = "none";

    // Send MQTT command to stop periodic mode
    publishMQTT(MQTT_CONFIG.topics.stm32Command, "PERIODIC OFF");
    addStatus("Periodic mode stopped (device OFF)", "STOP");
  }

  // Reset current values when device is turned off
  if (!isDeviceOn) {
    currentTemp = null;
    currentHumi = null;
    updateCurrentDisplay();
    addStatus("Device turned OFF - current values reset", "POWER");

    // Prevent state sync from interfering during device OFF transition
    setDeviceOffLock(2000); // 2 second lock

    // CRITICAL: Publish state update to inform other clients and ESP32
    const stateMsg = JSON.stringify({
      device: "OFF",
      periodic: "OFF",
    });
    publishMQTT(MQTT_CONFIG.topics.systemState, stateMsg);
  } else {
    // Device turned ON - do NOT auto-request values or start periodic
    addStatus("Device turned ON", "POWER");

    // CRITICAL: Publish state update (device=ON, periodic stays as-is)
    const stateMsg = JSON.stringify({
      device: "ON",
      periodic: isPeriodic ? "ON" : "OFF",
    });
    publishMQTT(MQTT_CONFIG.topics.systemState, stateMsg);
  }

  addStatus(`Device command sent: ${command}`, "POWER");
}

// Set time function - sends current browser time to STM32
function setTime() {
  if (!validateDeviceState("set time")) return;

  const now = new Date();
  const timestamp = Math.floor(now.getTime() / 1000); // Unix timestamp in seconds

  const command = `SET TIME ${timestamp}`;
  publishMQTT(MQTT_CONFIG.topics.stm32Command, command);

  addStatus(`Time set to: ${now.toLocaleString()} (${timestamp})`, "SETTING");
}

// Open interval modal
function openIntervalModal() {
  // Validate device state first
  if (!validateDeviceState("set interval")) return;

  const modal = document.getElementById("intervalModal");
  if (!modal) return;

  modal.style.display = "flex";

  // Initialize pickers
  initializeTimePicker();

  // Set default values if opening fresh
  if (!pickerInitialized) {
    selectedMinute = 0;
    selectedSecond = 5;
  }
  updatePickerSelection();
}

// Close interval modal
function closeIntervalModal() {
  const modal = document.getElementById("intervalModal");
  if (!modal) return;
  modal.style.display = "none";
}

// Initialize time picker with values
let selectedMinute = 0;
let selectedSecond = 5;
let pickerInitialized = false;

// Initialize time picker
function initializeTimePicker() {
  const minutePicker = document.getElementById("minutePicker");
  const secondPicker = document.getElementById("secondPicker");

  if (!minutePicker || !secondPicker) return;

  // Only initialize once
  if (pickerInitialized) {
    scrollToValue(minutePicker, selectedMinute);
    scrollToValue(secondPicker, selectedSecond);
    return;
  }

  // Clear existing items
  minutePicker.innerHTML = "";
  secondPicker.innerHTML = "";

  // Add padding items at top and bottom for scroll centering
  for (let i = 0; i < 3; i++) {
    minutePicker.appendChild(createPickerItem("", true));
    secondPicker.appendChild(createPickerItem("", true));
  }

  // Populate minutes (0-60)
  for (let i = 0; i <= 60; i++) {
    const item = createPickerItem(i, false, "minute");
    minutePicker.appendChild(item);
  }

  // Populate seconds (0-59)
  for (let i = 0; i <= 59; i++) {
    const item = createPickerItem(i, false, "second");
    secondPicker.appendChild(item);
  }

  // Add padding items at bottom
  for (let i = 0; i < 3; i++) {
    minutePicker.appendChild(createPickerItem("", true));
    secondPicker.appendChild(createPickerItem("", true));
  }

  // Add scroll listeners only once
  minutePicker.removeEventListener("scroll", handlePickerScroll);
  secondPicker.removeEventListener("scroll", handlePickerScroll);
  minutePicker.addEventListener("scroll", handlePickerScroll);
  secondPicker.addEventListener("scroll", handlePickerScroll);

  pickerInitialized = true;

  // Scroll to initial positions
  setTimeout(() => {
    scrollToValue(minutePicker, selectedMinute);
    scrollToValue(secondPicker, selectedSecond);
  }, 50);
}

// Create picker item
function createPickerItem(value, isPadding, type) {
  const div = document.createElement("div");
  div.className = "picker-item";
  if (!isPadding) {
    div.textContent = String(value).padStart(2, "0");
    div.dataset.value = value;
    div.dataset.type = type;
    div.addEventListener("click", function () {
      if (type === "minute") {
        selectedMinute = parseInt(value);
        scrollToValue(document.getElementById("minutePicker"), selectedMinute);
      } else {
        selectedSecond = parseInt(value);
        scrollToValue(document.getElementById("secondPicker"), selectedSecond);
      }

      setTimeout(() => {
        updatePickerSelection();
      }, 100);
    });
  }
  return div;
}

// Scroll picker to specific value
function scrollToValue(picker, value) {
  if (!picker) return;

  const items = picker.querySelectorAll(".picker-item[data-value]");
  const targetItem = Array.from(items).find(
    (item) => parseInt(item.dataset.value) === value
  );
  if (targetItem) {
    const scrollTop =
      targetItem.offsetTop -
      picker.offsetHeight / 2 +
      targetItem.offsetHeight / 2;
    picker.scrollTo({ top: scrollTop, behavior: "smooth" });
  }
}

// Handle picker scroll event
function handlePickerScroll(event) {
  const picker = event.currentTarget;
  if (!picker) return;

  // Debounce scroll event
  clearTimeout(picker.scrollTimeout);
  picker.scrollTimeout = setTimeout(() => {
    updatePickerSelection();
    snapToClosest(picker);
  }, 50);
}

function snapToClosest(picker) {
  if (!picker) return;

  const items = picker.querySelectorAll(".picker-item[data-value]");
  if (items.length === 0) return;

  const pickerCenter = picker.scrollTop + picker.offsetHeight / 2;
  let closestItem = null;
  let closestDistance = Infinity;

  items.forEach((item) => {
    const itemCenter = item.offsetTop + item.offsetHeight / 2;
    const distance = Math.abs(itemCenter - pickerCenter);

    if (distance < closestDistance) {
      closestDistance = distance;
      closestItem = item;
    }
  });

  if (closestItem && closestDistance > 3) {
    const scrollTop =
      closestItem.offsetTop -
      picker.offsetHeight / 2 +
      closestItem.offsetHeight / 2;
    picker.scrollTo({ top: scrollTop, behavior: "smooth" });
  }
}

function updatePickerSelection() {
  updatePickerHighlight("minutePicker", selectedMinute);
  updatePickerHighlight("secondPicker", selectedSecond);
}

function updatePickerHighlight(pickerId, selectedValue) {
  const picker = document.getElementById(pickerId);
  if (!picker) return;

  const items = picker.querySelectorAll(".picker-item[data-value]");
  if (items.length === 0) return;

  // Find center item based on scroll position
  const pickerCenter = picker.scrollTop + picker.offsetHeight / 2;
  let closestItem = null;
  let closestDistance = Infinity;

  items.forEach((item) => {
    const itemCenter = item.offsetTop + item.offsetHeight / 2;
    const distance = Math.abs(itemCenter - pickerCenter);

    if (distance < closestDistance) {
      closestDistance = distance;
      closestItem = item;
    }

    item.classList.remove("selected");
  });

  if (closestItem) {
    closestItem.classList.add("selected");
    const type = closestItem.dataset.type;
    const value = parseInt(closestItem.dataset.value);

    if (type === "minute") {
      selectedMinute = value;
    } else {
      selectedSecond = value;
    }
  }
}

// Set periodic interval function
function setPeriodicInterval() {
  // Double-check device state (in case it changed while modal was open)
  if (!validateDeviceState("set interval")) {
    closeIntervalModal();
    return;
  }

  const totalSeconds = selectedMinute * 60 + selectedSecond;

  if (totalSeconds === 0) {
    addStatus("Invalid interval! Must be at least 1 second", "ERROR");
    return;
  }

  const command = `SET PERIODIC INTERVAL ${totalSeconds}`;
  publishMQTT(MQTT_CONFIG.topics.stm32Command, command);

  let timeStr = "";
  if (selectedMinute > 0) {
    timeStr += `${selectedMinute} minute${selectedMinute > 1 ? "s" : ""}`;
  }
  if (selectedSecond > 0) {
    if (timeStr) timeStr += " ";
    timeStr += `${selectedSecond} second${selectedSecond > 1 ? "s" : ""}`;
  }

  addStatus(`Periodic interval set to ${timeStr}`, "SETTING");
  closeIntervalModal();
}

// Resize handler for charts
function handleResize() {
  if (chart1) chart1.resize();
  if (chart2) chart2.resize();
}

// Page initialization
function initializePage() {
  // Reset all states to default
  isPeriodic = false;
  isDeviceOn = false;
  currentTemp = null;
  currentHumi = null;
  stateSync.syncInProgress = false;
  stateSync.syncRetryCount = 0;
  stateSync.lastKnownState = null;
  stateSync.lastSyncMessage = "";
  stateSync.deviceOffLock = false;

  // Clear any existing timeouts
  if (stateSync.deviceOffLockTimeout) {
    clearTimeout(stateSync.deviceOffLockTimeout);
    stateSync.deviceOffLockTimeout = null;
  }

  // Reset UI to default state
  const deviceBtn = document.getElementById("deviceBtn");
  if (deviceBtn) {
    deviceBtn.textContent = "DEVICE OFF";
    deviceBtn.classList.remove("on");
  }

  const periodicBtn = document.getElementById("periodicBtn");
  const stopBtn = document.getElementById("stopBtn");
  if (periodicBtn) periodicBtn.style.display = "block";
  if (stopBtn) stopBtn.style.display = "none";

  // Initialize current display
  updateCurrentDisplay();

  addStatus("System initialized - waiting for hardware sync...", "INIT");
}

// Event listeners
document.addEventListener("DOMContentLoaded", function () {
  // Initialize page
  initializePage();

  // Initialize charts
  const ctx1 = document.getElementById("tempChart1");
  const ctx2 = document.getElementById("tempChart2");

  if (ctx1) chart1 = new Chart(ctx1.getContext("2d"), chartTempConfig);
  if (ctx2) chart2 = new Chart(ctx2.getContext("2d"), chartHumiConfig);

  // Modal event listeners
  document.getElementById("settingsBtn").addEventListener("click", openModal);
  document.getElementById("cancelBtn").addEventListener("click", closeModal);
  document.getElementById("saveBtn").addEventListener("click", saveAndConnect);

  // Close modal when clicking outside
  document.getElementById("mqttModal").addEventListener("click", function (e) {
    if (e.target === this) closeModal();
  });

  // Control buttons
  const periodicBtn = document.getElementById("periodicBtn");
  const stopBtn = document.getElementById("stopBtn");
  const singleBtn = document.getElementById("singleBtn");
  const deviceBtn = document.getElementById("deviceBtn");
  const setTimeBtn = document.getElementById("setTimeBtn");
  const setIntervalBtn = document.getElementById("setIntervalBtn");
  const intervalCancelBtn = document.getElementById("intervalCancelBtn");
  const intervalSetBtn = document.getElementById("intervalSetBtn");
  const loadDataBtn = document.getElementById("loadDataBtn");
  const clearDataBtn = document.getElementById("clearDataBtn");

  if (periodicBtn) periodicBtn.addEventListener("click", startPeriodicMode);
  if (stopBtn) stopBtn.addEventListener("click", stopPeriodicMode);
  if (singleBtn) singleBtn.addEventListener("click", singleRead);
  if (deviceBtn) deviceBtn.addEventListener("click", toggleDevice);
  if (setTimeBtn) setTimeBtn.addEventListener("click", setTime);
  if (setIntervalBtn)
    setIntervalBtn.addEventListener("click", openIntervalModal);
  if (intervalCancelBtn)
    intervalCancelBtn.addEventListener("click", closeIntervalModal);
  if (intervalSetBtn)
    intervalSetBtn.addEventListener("click", setPeriodicInterval);
  if (loadDataBtn) loadDataBtn.addEventListener("click", loadHistoricalData);
  if (clearDataBtn) clearDataBtn.addEventListener("click", clearChartData);

  // Close interval modal when clicking outside
  document
    .getElementById("intervalModal")
    .addEventListener("click", function (e) {
      if (e.target === this) closeIntervalModal();
    });

  // Window resize handler
  window.addEventListener("resize", handleResize);

  // Initialize status
  setTimeout(() => {
    addStatus("SHT31 Monitor ready", "INIT");
    addStatus("Configure MQTT broker to connect", "INFO");
    addStatus("[WARNING] Firebase needed for data persistence", "WARNING");
    updateConnectionStatus(false);
    updateFirebaseStatus(false);
  }, 500);

  // Auto-connect MQTT
  setTimeout(() => {
    connectMQTT();
  }, 1000);

  // Start clock update
  updateClock();
  setInterval(updateClock, 1000);
});

// Clock update function
function updateClock() {
  const now = new Date();

  // Format time HH:MM:SS
  const hours = String(now.getHours()).padStart(2, "0");
  const minutes = String(now.getMinutes()).padStart(2, "0");
  const seconds = String(now.getSeconds()).padStart(2, "0");
  const timeString = `${hours}:${minutes}:${seconds}`;

  // Format date YYYY/MM/DD (simple format)
  const year = now.getFullYear();
  const month = String(now.getMonth() + 1).padStart(2, "0");
  const day = String(now.getDate()).padStart(2, "0");
  const dateString = `${year}/${month}/${day}`;

  // Update DOM
  const timeEl = document.getElementById("currentTime");
  const dateEl = document.getElementById("currentDate");

  if (timeEl) timeEl.textContent = timeString;
  if (dateEl) dateEl.textContent = dateString;
}
