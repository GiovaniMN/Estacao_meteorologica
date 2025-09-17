import { initializeApp } from "firebase/app";

const firebaseConfig = {
  apiKey: "AIzaSyB-Jc3kk92NjZZHnuD5z-Ez13zrjZcanVM",
  authDomain: "estacao-metereologica-bc93c.firebaseapp.com",
  databaseURL: "https://estacao-metereologica-bc93c-default-rtdb.firebaseio.com",
  projectId: "estacao-metereologica-bc93c",
  storageBucket: "estacao-metereologica-bc93c.firebasestorage.app",
  messagingSenderId: "577500316669",
  appId: "1:577500316669:web:7555b22720a041e8184f94"
};

const app = initializeApp(firebaseConfig);

// Export the database instance
import { getDatabase } from "firebase/database";
export const database = getDatabase(app);
