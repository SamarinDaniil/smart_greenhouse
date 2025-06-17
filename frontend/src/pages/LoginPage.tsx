import { useState } from "react";
import { useAuth } from "../context/AuthContext";
import { Container, Form, Button, Alert } from "react-bootstrap";

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
    } catch (err) {
      setError("Ошибка входа");
    }
  };

  return (
    <Container
      fluid
      className="min-vh-100 d-flex align-items-center justify-content-center"
      style={{
        background: "linear-gradient(135deg, #0f172a 0%, #000 100%)",
        padding: "20px"
      }}
    >
      <Form
        onSubmit={onSubmit}
        className="bg-white p-4 rounded-3 shadow"
        style={{ width: "100%", maxWidth: "400px" }}
      >
        <h1 className="text-center mb-4">Вход</h1>

        {error && <Alert variant="danger" className="text-center">{error}</Alert>}

        <Form.Group className="mb-3 position-relative">
          <Form.Control
            type="text"
            placeholder="Сложное имя пользователя..."
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            required
            className="p-3 rounded-3 ps-5" // Добавлен отступ слева
          />
          <i className="bi bi-person position-absolute" style={{
            top: "50%",
            left: "16px",
            transform: "translateY(-50%)",
            opacity: 0.5
          }}></i>
        </Form.Group>

        <Form.Group className="mb-4 position-relative">
          <Form.Control
            type="password"
            placeholder="Сложный пароль..."
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            required
            className="p-3 rounded-3 ps-5" // Добавлен отступ слева
          />
          <i className="bi bi-lock position-absolute" style={{
            top: "50%",
            left: "16px",
            transform: "translateY(-50%)",
            opacity: 0.5
          }}></i>
        </Form.Group>

        <Button
          variant="primary"
          type="submit"
          className="w-100 p-3 fw-bold rounded-3"
        >
          ВОЙТИ
        </Button>
      </Form>
    </Container>
  );
}