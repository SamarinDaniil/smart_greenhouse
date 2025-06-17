import React, { useEffect, useState } from "react";
import { Table, Spinner, Alert, Card, Button, Form } from "react-bootstrap";
import { fetcher } from "../../api/fetcher";

interface User {
  user_id: number;
  username: string;
  role: string;
}

const UsersTable: React.FC = () => {
  const [users, setUsers] = useState<User[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const [editId, setEditId] = useState<number | null>(null);
  const [draft, setDraft] = useState<Partial<User> & { password?: string }>({});
  const [addingRow, setAddingRow] = useState(false);

  const loadUsers = async () => {
    setLoading(true);
    try {
      const data = await fetcher<User[]>("/api/users");
      setUsers(data);
    } catch (err) {
      setError("Ошибка загрузки пользователей");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadUsers();
  }, []);

  const handleSave = async () => {
    if (!draft.username || !draft.role || (addingRow && !draft.password)) {
      alert("Поля 'Имя пользователя', 'Роль' и 'Пароль' (при создании) обязательны");
      return;
    }

    try {
      if (addingRow) {
        const newUser = await fetcher<User>("/api/register", {
          method: "POST",
          body: JSON.stringify({
            username: draft.username,
            role: draft.role,
            password: draft.password,
          }),
        });
        setUsers((prev) => [...prev, newUser]);
      } else if (editId !== null) {
        const bodyToSend: any = {
          username: draft.username,
          role: draft.role,
        };
        if (draft.password && draft.password.trim() !== "") {
          bodyToSend.password = draft.password;
        }
        await fetcher(`/api/users/${editId}`, {
          method: "PUT",
          body: JSON.stringify(bodyToSend),
        });
        setUsers((prev) =>
          prev.map((u) =>
            u.user_id === editId ? { ...u, ...draft } as User : u
          )
        );
      }
    } catch {
      alert("Ошибка при сохранении");
    } finally {
      cancelEdit();
    }
  };

  const handleDelete = async (user_id: number) => {
    if (!window.confirm("Вы уверены, что хотите удалить пользователя?")) return;

    try {
      await fetcher(`/api/users/${user_id}`, {
        method: "DELETE",
      });
      setUsers((prev) => prev.filter((u) => u.user_id !== user_id));
    } catch {
      alert("Ошибка при удалении пользователя");
    }
  };

  const startEdit = (user: User) => {
    setEditId(user.user_id);
    setDraft({ username: user.username, role: user.role, password: "" });
  };

  const startAdd = () => {
    setAddingRow(true);
    setDraft({ username: "", role: "", password: "" });
  };

  const cancelEdit = () => {
    setEditId(null);
    setAddingRow(false);
    setDraft({});
  };

  const renderRow = (user: User) =>
    editId === user.user_id ? (
      <tr key={user.user_id}>
        <td>{user.user_id}</td>
        <td>
          <Form.Control
            value={draft.username || ""}
            onChange={(e) => setDraft({ ...draft, username: e.target.value })}
          />
        </td>
        <td>
          <Form.Control
            value={draft.role || ""}
            onChange={(e) => setDraft({ ...draft, role: e.target.value })}
          />
        </td>
        <td>
          <Form.Control
            type="password"
            placeholder="Оставьте пустым, чтобы не менять"
            value={draft.password || ""}
            onChange={(e) => setDraft({ ...draft, password: e.target.value })}
          />
        </td>
        <td>
          <Button size="sm" variant="success" onClick={handleSave}>
            Сохранить
          </Button>{" "}
          <Button size="sm" variant="secondary" onClick={cancelEdit}>
            Отмена
          </Button>
        </td>
      </tr>
    ) : (
      <tr key={user.user_id}>
        <td>{user.user_id}</td>
        <td>{user.username}</td>
        <td>{user.role}</td>
        <td>—</td>
        <td>
          <Button
            size="sm"
            variant="outline-primary"
            onClick={() => startEdit(user)}
          >
            ✏️
          </Button>{" "}
          <Button
            size="sm"
            variant="outline-danger"
            onClick={() => handleDelete(user.user_id)}
          >
            🗑️
          </Button>
        </td>
      </tr>
    );

  return (
    <Card className="mb-4 shadow-sm">
      <Card.Body>
        <Card.Title className="d-flex justify-content-between align-items-center">
          Пользователи
          {!addingRow && editId === null && (
            <Button size="sm" onClick={startAdd}>
              + Добавить
            </Button>
          )}
        </Card.Title>

        {loading ? (
          <div className="text-center py-4">
            <Spinner animation="border" />
          </div>
        ) : error ? (
          <Alert variant="danger">{error}</Alert>
        ) : (
          <Table striped bordered hover responsive>
            <thead>
              <tr>
                <th>ID</th>
                <th>Имя пользователя</th>
                <th>Роль</th>
                <th>Пароль</th>
                <th>Действия</th>
              </tr>
            </thead>
            <tbody>
              {users.map(renderRow)}

              {addingRow && (
                <tr>
                  <td>—</td>
                  <td>
                    <Form.Control
                      value={draft.username || ""}
                      onChange={(e) =>
                        setDraft({ ...draft, username: e.target.value })
                      }
                    />
                  </td>
                  <td>
                    <Form.Control
                      value={draft.role || ""}
                      onChange={(e) =>
                        setDraft({ ...draft, role: e.target.value })
                      }
                    />
                  </td>
                  <td>
                    <Form.Control
                      type="password"
                      value={draft.password || ""}
                      onChange={(e) =>
                        setDraft({ ...draft, password: e.target.value })
                      }
                    />
                  </td>
                  <td>
                    <Button size="sm" variant="success" onClick={handleSave}>
                      Создать
                    </Button>{" "}
                    <Button size="sm" variant="secondary" onClick={cancelEdit}>
                      Отмена
                    </Button>
                  </td>
                </tr>
              )}
            </tbody>
          </Table>
        )}
      </Card.Body>
    </Card>
  );
};

export default UsersTable;
