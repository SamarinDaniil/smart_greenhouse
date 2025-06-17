import React, { useEffect, useState } from "react";
import { Table, Spinner, Alert, Card, Button, Form } from "react-bootstrap";
import { fetcher } from "../../api/fetcher";

interface Greenhouse {
  gh_id: number;
  name: string;
  location: string;
  created_at: string;
  updated_at: string;
}

const GreenhousesTable: React.FC = () => {
  const [greenhouses, setGreenhouses] = useState<Greenhouse[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const [editId, setEditId] = useState<number | null>(null);
  const [draft, setDraft] = useState<Partial<Greenhouse>>({});
  const [addingRow, setAddingRow] = useState(false);

  const loadGreenhouses = async () => {
    setLoading(true);
    try {
      const data = await fetcher<Greenhouse[]>("/api/greenhouses");
      setGreenhouses(data);
      setError(null);
    } catch {
      setError("Ошибка загрузки теплиц");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadGreenhouses();
  }, []);

  const handleSave = async () => {
    if (!draft.name || !draft.location) {
      alert("Поля 'Название' и 'Локация' обязательны");
      return;
    }

    try {
      if (addingRow) {
        // Создание
        const newGH = await fetcher<Greenhouse>("/api/greenhouses", {
          method: "POST",
          body: JSON.stringify({
            name: draft.name,
            location: draft.location,
          }),
        });
        setGreenhouses((prev) => [...prev, newGH]);
      } else if (editId !== null) {
        // Обновление
        await fetcher(`/api/greenhouses/${editId}`, {
          method: "PUT",
          body: JSON.stringify({
            name: draft.name,
            location: draft.location,
          }),
        });
        setGreenhouses((prev) =>
          prev.map((gh) =>
            gh.gh_id === editId ? { ...gh, ...draft } as Greenhouse : gh
          )
        );
      }
    } catch {
      alert("Ошибка при сохранении");
    } finally {
      cancelEdit();
    }
  };

  const handleDelete = async (gh_id: number) => {
    if (!window.confirm("Вы уверены, что хотите удалить теплицу?")) return;

    try {
      await fetcher(`/api/greenhouses/${gh_id}`, { method: "DELETE" });
      setGreenhouses((prev) => prev.filter((gh) => gh.gh_id !== gh_id));
    } catch {
      alert("Ошибка при удалении");
    }
  };

  const startEdit = (gh: Greenhouse) => {
    setEditId(gh.gh_id);
    setDraft({ name: gh.name, location: gh.location });
  };

  const startAdd = () => {
    setAddingRow(true);
    setDraft({ name: "", location: "" });
  };

  const cancelEdit = () => {
    setEditId(null);
    setAddingRow(false);
    setDraft({});
  };

  const renderRow = (gh: Greenhouse) =>
    editId === gh.gh_id ? (
      <tr key={gh.gh_id}>
        <td>{gh.gh_id}</td>
        <td>
          <Form.Control
            value={draft.name || ""}
            onChange={(e) => setDraft({ ...draft, name: e.target.value })}
          />
        </td>
        <td>
          <Form.Control
            value={draft.location || ""}
            onChange={(e) => setDraft({ ...draft, location: e.target.value })}
          />
        </td>
        <td>{gh.created_at}</td>
        <td>{gh.updated_at}</td>
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
      <tr key={gh.gh_id}>
        <td>{gh.gh_id}</td>
        <td>{gh.name}</td>
        <td>{gh.location}</td>
        <td>{gh.created_at}</td>
        <td>{gh.updated_at}</td>
        <td>
          <Button
            size="sm"
            variant="outline-primary"
            onClick={() => startEdit(gh)}
          >
            ✏️
          </Button>{" "}
          <Button
            size="sm"
            variant="outline-danger"
            onClick={() => handleDelete(gh.gh_id)}
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
          Теплицы
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
                <th>Название</th>
                <th>Локация</th>
                <th>Создано</th>
                <th>Обновлено</th>
                <th>Действия</th>
              </tr>
            </thead>
            <tbody>
              {greenhouses.map(renderRow)}

              {addingRow && (
                <tr>
                  <td>—</td>
                  <td>
                    <Form.Control
                      value={draft.name || ""}
                      onChange={(e) => setDraft({ ...draft, name: e.target.value })}
                    />
                  </td>
                  <td>
                    <Form.Control
                      value={draft.location || ""}
                      onChange={(e) => setDraft({ ...draft, location: e.target.value })}
                    />
                  </td>
                  <td>—</td>
                  <td>—</td>
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

export default GreenhousesTable;
