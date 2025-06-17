import { NavLink } from "react-router-dom";
import { useAuth } from "../context/AuthContext";
import "bootstrap/dist/css/bootstrap.min.css";

export default function Navbar() {
    const { role, logout } = useAuth();

    return (
        <nav className="navbar navbar-expand-lg navbar-light bg-light shadow-sm">
            <div className="container-fluid">
                <NavLink className="navbar-brand" to="/">
                    🌿 Smart Greenhouse
                </NavLink>

                <div className="collapse navbar-collapse justify-content-end">
                    <ul className="navbar-nav">
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

                                {role === "admin" && (
                                    <li className="nav-item">
                                        <NavLink to="/admin" className="nav-link">
                                            Админ-панель
                                        </NavLink>
                                    </li>
                                )}

                                <li className="nav-item">
                                    <button className="btn btn-outline-danger ms-3" onClick={logout}>
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
