import React, { useState } from "react";
import { useAuth } from "../context/AuthContext";
import { Container, Form, Button, Alert } from "react-bootstrap";
import "../App.css"; // убедитесь, что подключен файл стилей

export default function LoginPage() {
  const { login } = useAuth();
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState<string | null>(null);

  const onSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(null);
    try {
      await login(username, password);
    } catch {
      setError("Ошибка входа");
    }
  };

  return (
    <Container fluid className="login-page d-flex align-items-center justify-content-center">
      <Form onSubmit={onSubmit} className="login-form bg-white p-sm rounded-3 shadow">
        <h1 className="text-center mb-4">Вход</h1>

        {error && <Alert variant="danger" className="text-center">{error}</Alert>}

        <Form.Group className="mb-3 position-relative">
          <i className="bi bi-person position-absolute icon-input"></i>
          <Form.Control
            type="text"
            placeholder="Имя пользователя"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            required
            className="p-3 rounded-3 ps-5"
          />
        </Form.Group>

        <Form.Group className="mb-4 position-relative">
          <i className="bi bi-lock position-absolute icon-input"></i>
          <Form.Control
            type="password"
            placeholder="Пароль"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            required
            className="p-3 rounded-3 ps-5"
          />
        </Form.Group>

        <Button variant="primary" type="submit" className="w-100 p-3 fw-bold rounded-3">
          ВОЙТИ
        </Button>
      </Form>
    </Container>
  );
}
