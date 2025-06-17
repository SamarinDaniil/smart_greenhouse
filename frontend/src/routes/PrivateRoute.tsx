// src/routes/PrivateRoute.tsx
import { Navigate, Outlet } from "react-router-dom";
import { useAuth } from "../context/AuthContext";
import { Role } from "../context/AuthContext";

interface PrivateRouteProps {
  allowedRoles?: Role[];
}

export const PrivateRoute: React.FC<PrivateRouteProps> = ({ allowedRoles }) => {
  const { role } = useAuth();

  if (!role) {
    return <Navigate to="/login" replace />;
  }
  // password1234 - for test
  // 2) Роль авторизована, но не входит в allowedRoles → на «домашнюю» страницу роли
  if (allowedRoles && !allowedRoles.includes(role)) {
    const home = role === "admin" ? "/admin" : "/dashboard";
    return <Navigate to={home} replace />;
  }

  // 3) Всё ок — отрисовываем вложенный маршрут
  return <Outlet />;
};
