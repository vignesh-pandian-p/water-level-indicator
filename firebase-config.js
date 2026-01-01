// Firebase configuration - REPLACE WITH YOUR OWN CONFIG
const firebaseConfig = {
  apiKey: "AIzaSyDBdgInWH3ieLGfiELSznGOjnC2nBquTaM",
  authDomain: "water-level-indicator-201f4.firebaseapp.com",
  databaseURL: "https://water-level-indicator-201f4-default-rtdb.firebaseio.com",
  projectId: "water-level-indicator-201f4",
  storageBucket: "water-level-indicator-201f4.firebasestorage.app",
  messagingSenderId: "194581425604",
  appId: "1:194581425604:web:bfb9f203d7570286834891",
  measurementId: "G-G751KXXGM5"
};

// Initialize Firebase
firebase.initializeApp(firebaseConfig);

// Get a reference to the database service
const database = firebase.database();
