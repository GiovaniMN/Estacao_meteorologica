import { initializeApp } from "firebase/app";

const firebaseConfig = {
  apiKey: "AIzaSyBRk9NsauMiHqr9WOcLf6pfIoufUeGAF18",
  authDomain: "estacao-meteorologica-6e2a7.firebaseapp.com",
  databaseURL: "https://estacao-meteorologica-6e2a7-default-rtdb.firebaseio.com/",
  projectId: "estacao-meteorologica-6e2a7",
  storageBucket: "estacao-meteorologica-6e2a7.firebasestorage.app",
  messagingSenderId: "891742149854",
  appId: "1:891742149854:web:777654150d02508cccf64f"
};

const app = initializeApp(firebaseConfig);

// Export the database instance
import { getDatabase } from "firebase/database";
export const database = getDatabase(app);
