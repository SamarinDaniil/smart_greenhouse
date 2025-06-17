// src/context/AuthContext.tsx
import React, { createContext, useContext, useState } from "react";
import { useNavigate } from "react-router-dom";
import { fetcher } from "../api/fetcher";

export type Role = "observer" | "admin" | null;

interface AuthContextType {
  role: Role;
  login: (username: string, password: string) => Promise<void>;
  logout: () => void;
}

const AuthContext = createContext<AuthContextType>({} as AuthContextType);

export const AuthProvider: React.FC<React.PropsWithChildren> = ({ children }) => {
  const navigate = useNavigate();
  const [role, setRole] = useState<Role>(() => {
    return localStorage.getItem("role") as Role;
  });

  const login = async (username: string, password: string) => {
    console.log("aa")
    const data = await fetcher<{
      role: string;
      token: string;
      expires_in: number;
      success: boolean;
      user_id: number;
    }>("/api/login", {
      method: "POST",
      body: JSON.stringify({ username, password }),
    });

    if (!data.success) {
      throw new Error("Неверный логин или пароль");
    }

    localStorage.setItem("token", data.token);
    localStorage.setItem("role", data.role);
    localStorage.setItem("user_id", String(data.user_id));
    localStorage.setItem("token_expires_in", String(data.expires_in));

    setRole(data.role as Role);

    navigate(data.role === "admin" ? "/admin" : "/dashboard", { replace: true });
  };

  const logout = () => {
    localStorage.clear();
    setRole(null);
    navigate("/login", { replace: true });
  };

  return (
    <AuthContext.Provider value={{ role, login, logout }}>
      {children}
    </AuthContext.Provider>
  );
};

export const useAuth = () => {
  const ctx = useContext(AuthContext);
  if (!ctx) throw new Error("useAuth must be used inside AuthProvider");
  return ctx;
};
