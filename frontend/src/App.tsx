import './App.css';
import { Routes, Route, Navigate } from "react-router-dom";
import LoginPage from "./pages/LoginPage";
import DashboardPage from "./pages/DashboardPage";
import AdminPage from "./pages/AdminPage/AdminPage";
import { PrivateRoute } from './routes/PrivateRoute';

import { BrowserRouter } from "react-router-dom";
import { AuthProvider } from './context/AuthContext';
import Navbar from "./components/Navbar";

function App() {
  return (
    <BrowserRouter>
    <AuthProvider>
      
      <Navbar />
      <Routes>
        <Route path="/login" element={<LoginPage />} />

        {/* общий доступ после входа */}
        <Route element={<PrivateRoute />}>
          <Route path="/dashboard" element={<DashboardPage />} />
        </Route>

        {/* только администратор */}
        <Route element={<PrivateRoute allowedRoles={["admin"]} />}>
          <Route path="/admin" element={<AdminPage />} />
        </Route>

        <Route path="/" element={<Navigate to="/login" />} />
        <Route path="*" element={<h1>404: Not Found</h1>} />
      </Routes>
      </AuthProvider>
      
    </BrowserRouter>
)

}

export default App
