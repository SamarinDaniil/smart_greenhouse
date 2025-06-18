import React from 'react';
import { NavLink } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';
import 'bootstrap/dist/css/bootstrap.min.css';
import 'bootstrap/dist/js/bootstrap.bundle.min';

export default function Navbar() {
  const { role, logout } = useAuth();

  return (
    <nav className="navbar navbar-expand-lg navbar-light bg-light shadow-sm">
      <div className="container-fluid app-container">
        <NavLink className="navbar-brand" to="/">
          🌿 Smart Greenhouse
        </NavLink>
        <button
          className="navbar-toggler"
          type="button"
          data-bs-toggle="collapse"
          data-bs-target="#navbarSupportedContent"
          aria-controls="navbarSupportedContent"
          aria-expanded="false"
          aria-label="Toggle navigation"
        >
          <span className="navbar-toggler-icon"></span>
        </button>

        <div className="collapse navbar-collapse" id="navbarSupportedContent">
          <ul className="navbar-nav ms-auto">
            {!role && (
              <li className="nav-item">
                <NavLink to="/login" className="nav-link">
                  Авторизация
                </NavLink>
              </li>
            )}

            {role && (
              <>
                <li className="nav-item">
                  <NavLink to="/dashboard" className="nav-link">
                    Dashboard
                  </NavLink>
                </li>

                {role === 'admin' && (
                  <li className="nav-item">
                    <NavLink to="/admin" className="nav-link">
                      Админ-панель
                    </NavLink>
                  </li>
                )}

                <li className="nav-item">
                  <button
                    className="btn btn-outline-danger ms-3"
                    onClick={logout}
                  >
                    Выйти
                  </button>
                </li>
              </>
            )}
          </ul>
        </div>
      </div>
    </nav>
  );
}