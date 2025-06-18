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
  const [isOpen, setIsOpen] = useState(true);

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
        const newGH = await fetcher<Greenhouse>("/api/greenhouses", {
          method: "POST",
          body: JSON.stringify({
            name: draft.name,
            location: draft.location,
          }),
        });
        setGreenhouses((prev) => [...prev, newGH]);
      } else if (editId !== null) {
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

  const renderRow = (gh: Greenhouse) => (
    <tr key={gh.gh_id} className="align-middle">
      {editId === gh.gh_id ? (
        <>
          <td>{gh.gh_id}</td>
          <td>
            <Form.Control
              size="sm"
              value={draft.name || ""}
              onChange={(e) => setDraft({ ...draft, name: e.target.value })}
            />
          </td>
          <td>
            <Form.Control
              size="sm"
              value={draft.location || ""}
              onChange={(e) => setDraft({ ...draft, location: e.target.value })}
            />
          </td>
          <td className="d-none d-md-table-cell">—</td>
          <td className="d-none d-md-table-cell">—</td>
          <td>
            <Button size="sm" variant="success" onClick={handleSave}>✔️</Button>{' '}
            <Button size="sm" variant="outline-secondary" onClick={cancelEdit}>✖️</Button>
          </td>
        </>
      ) : (
        <>
          <td>{gh.gh_id}</td>
          <td>{gh.name}</td>
          <td>{gh.location}</td>
          <td className="d-none d-md-table-cell">{gh.created_at}</td>
          <td className="d-none d-md-table-cell">{gh.updated_at}</td>
          <td>
            <Button size="sm" variant="outline-primary" onClick={() => startEdit(gh)}>✏️</Button>{' '}
            <Button size="sm" variant="outline-danger" onClick={() => handleDelete(gh.gh_id)}>🗑️</Button>
          </td>
        </>
      )}
    </tr>
  );

  return (
    <Card className="shadow-sm">
      <Card.Header className="d-flex justify-content-between align-items-center">
        <h3 className="mb-0">Теплицы</h3>
        <div>
          <Button
            size="sm"
            variant="outline-secondary"
            onClick={() => setIsOpen(!isOpen)}
            className="me-2"
          >
            {isOpen ? "Скрыть" : "Показать"}
          </Button>
          {!addingRow && editId === null && (
            <Button size="sm" onClick={startAdd}>
              + Добавить
            </Button>
          )}
        </div>
      </Card.Header>
      <Card.Body className="p-0">
        {loading ? (
          <div className="text-center py-4">
            <Spinner animation="border" />
          </div>
        ) : error ? (
          <Alert variant="danger" className="m-3">{error}</Alert>
        ) : (
          isOpen && (
            <div className="table-responsive">
              <Table hover responsive className="mb-0">
                <thead className="table-success">
                  <tr>
                    <th>ID</th>
                    <th>Название</th>
                    <th>Локация</th>
                    <th className="d-none d-md-table-cell">Создано</th>
                    <th className="d-none d-md-table-cell">Обновлено</th>
                    <th>Действия</th>
                  </tr>
                </thead>
                <tbody>
                  {greenhouses.map(renderRow)}

                  {addingRow && (
                    <tr className="align-middle">
                      <td>—</td>
                      <td>
                        <Form.Control
                          size="sm"
                          value={draft.name || ""}
                          onChange={(e) => setDraft({ ...draft, name: e.target.value })}
                        />
                      </td>
                      <td>
                        <Form.Control
                          size="sm"
                          value={draft.location || ""}
                          onChange={(e) => setDraft({ ...draft, location: e.target.value })}
                        />
                      </td>
                      <td className="d-none d-md-table-cell">—</td>
                      <td className="d-none d-md-table-cell">—</td>
                      <td>
                        <Button size="sm" variant="success" onClick={handleSave}>✔️</Button>{' '}
                        <Button size="sm" variant="outline-secondary" onClick={cancelEdit}>✖️</Button>
                      </td>
                    </tr>
                  )}
                </tbody>
              </Table>
            </div>
          )
        )}
      </Card.Body>
    </Card>
  );
};

export default GreenhousesTable;