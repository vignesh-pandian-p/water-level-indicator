// DOM Elements
const waterLevel = document.getElementById('waterLevel');
const levelPercentage = document.getElementById('levelPercentage');
const volumeDisplay = document.getElementById('volumeDisplay');
const motorStatus = document.getElementById('motorStatus');
const motorSubtext = document.getElementById('motorSubtext');
const motorBar = document.getElementById('motorBar');
const motorIconWrapper = document.getElementById('motorIconWrapper');
const systemHealth = document.getElementById('systemHealth');
const healthSubtext = document.getElementById('healthSubtext');
const distanceValue = document.getElementById('distanceValue');
const capacityPercent = document.getElementById('capacityPercent');
const connectionStatus = document.getElementById('connectionStatus');
const connectionSubtext = document.getElementById('connectionSubtext');
const connectionBar = document.getElementById('connectionBar');
const connectionIconWrapper = document.getElementById('connectionIconWrapper');
const temperatureValue = document.getElementById('temperatureValue');
const humidityValue = document.getElementById('humidityValue');
const tdsValue = document.getElementById('tdsValue');
const turbidityValue = document.getElementById('turbidityValue');
const lastUpdateTime = document.getElementById('lastUpdateTime');

// Tank configuration (from your receiver code)
const TANK_CONFIG = {
    EMPTY_DISTANCE: 138,  // cm - distance when tank is empty
    FULL_DISTANCE: 44,    // cm - distance when tank is full
    TANK_HEIGHT: 94,      // cm - actual tank height (138 - 44)
    MAX_VOLUME: 1500      // liters - adjust based on your tank
};

// Calculate water level percentage from distance
function calculateWaterLevel(distance) {
    const levelPercent = ((TANK_CONFIG.EMPTY_DISTANCE - distance) * 100.0) / TANK_CONFIG.TANK_HEIGHT;
    return Math.max(0, Math.min(100, levelPercent)); // Clamp between 0-100
}

// Calculate volume in liters
function calculateVolume(percentage) {
    return Math.round((percentage / 100) * TANK_CONFIG.MAX_VOLUME);
}

// Format timestamp
function formatTimestamp(timestamp) {
    if (!timestamp) return 'Just now';
    const date = new Date(timestamp);
    const now = new Date();
    const diffMs = now - date;
    const diffSecs = Math.floor(diffMs / 1000);
    const diffMins = Math.floor(diffSecs / 60);

    if (diffSecs < 10) {
        return 'Just now';
    } else if (diffSecs < 60) {
        return `${diffSecs}s ago`;
    } else if (diffMins < 60) {
        return `${diffMins}m ago`;
    } else {
        return date.toLocaleTimeString();
    }
}

// Update UI with new data
function updateUI(data) {
    if (!data) {
        console.log('No data available');
        return;
    }

    const { distance, motorState: motor, timestamp } = data;

    // Calculate water level percentage
    const waterLevelPercent = calculateWaterLevel(distance);
    const volume = calculateVolume(waterLevelPercent);

    // Update water level visualization
    waterLevel.style.height = `${waterLevelPercent}%`;
    levelPercentage.textContent = `${waterLevelPercent.toFixed(0)}%`;
    volumeDisplay.textContent = volume;

    // Update capacity card
    capacityPercent.textContent = waterLevelPercent.toFixed(0);

    // Update distance card
    distanceValue.textContent = distance.toFixed(1);

    // Update motor status
    motorStatus.textContent = motor;
    if (motor === 'ON') {
        motorSubtext.textContent = 'Running';
        motorBar.classList.add('active');
        motorIconWrapper.classList.add('active');
    } else {
        motorSubtext.textContent = 'Standby';
        motorBar.classList.remove('active');
        motorIconWrapper.classList.remove('active');
    }

    // Update system health based on water level
    if (waterLevelPercent > 75) {
        systemHealth.textContent = 'Excellent';
        healthSubtext.textContent = 'Optimal';
    } else if (waterLevelPercent > 50) {
        systemHealth.textContent = 'Good';
        healthSubtext.textContent = 'Stable';
    } else if (waterLevelPercent > 25) {
        systemHealth.textContent = 'Fair';
        healthSubtext.textContent = 'Monitor';
    } else {
        systemHealth.textContent = 'Low';
        healthSubtext.textContent = 'Alert';
    }

    // Update last update time
    lastUpdateTime.textContent = formatTimestamp(timestamp);

    // Update connection status
    connectionStatus.textContent = 'Connected';
    connectionSubtext.textContent = 'Online';
    connectionBar.classList.add('connected');
    connectionIconWrapper.classList.add('connected');

    // Update environmental sensors (if available)
    if (data.temperature !== undefined) {
        temperatureValue.textContent = data.temperature.toFixed(1);
    }

    if (data.humidity !== undefined) {
        humidityValue.textContent = data.humidity.toFixed(1);
    }

    if (data.tds !== undefined) {
        tdsValue.textContent = Math.round(data.tds);
    }

    if (data.turbidity !== undefined) {
        turbidityValue.textContent = data.turbidity.toFixed(2);
    }

    // Refresh Lucide icons (in case any were dynamically updated)
    if (typeof lucide !== 'undefined') {
        lucide.createIcons();
    }
}

// Handle connection state
function handleConnectionState(isConnected) {
    if (isConnected) {
        connectionStatus.textContent = 'Connected';
        connectionSubtext.textContent = 'Online';
        connectionBar.classList.add('connected');
        connectionIconWrapper.classList.add('connected');
        console.log('Connected to Firebase');
    } else {
        connectionStatus.textContent = 'Disconnected';
        connectionSubtext.textContent = 'Offline';
        connectionBar.classList.remove('connected');
        connectionIconWrapper.classList.remove('connected');
        console.log('Disconnected from Firebase');
    }
}

// Initialize Firebase listeners
function initializeFirebase() {
    try {
        // Reference to the water level data
        const waterLevelRef = database.ref('waterLevel');

        // Listen for connection state
        const connectedRef = database.ref('.info/connected');
        connectedRef.on('value', (snapshot) => {
            handleConnectionState(snapshot.val() === true);
        });

        // Listen for real-time updates
        waterLevelRef.on('value', (snapshot) => {
            const data = snapshot.val();
            console.log('Received data:', data);
            updateUI(data);
        }, (error) => {
            console.error('Error reading data:', error);
            connectionStatus.textContent = 'Error';
            connectionSubtext.textContent = error.message;
        });

        console.log('Firebase listeners initialized');
    } catch (error) {
        console.error('Error initializing Firebase:', error);
        connectionStatus.textContent = 'Config Error';
        connectionSubtext.textContent = 'Check setup';
    }
}

// Update "time ago" every 10 seconds
setInterval(() => {
    database.ref('waterLevel/timestamp').once('value', (snapshot) => {
        const timestamp = snapshot.val();
        lastUpdateTime.textContent = formatTimestamp(timestamp);
    });
}, 10000);

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    console.log('Initializing Water Level Dashboard...');
    initializeFirebase();
});

// Handle page visibility changes
document.addEventListener('visibilitychange', () => {
    if (!document.hidden) {
        console.log('Page visible, refreshing data...');
        database.ref('waterLevel').once('value', (snapshot) => {
            updateUI(snapshot.val());
        });
    }
});
