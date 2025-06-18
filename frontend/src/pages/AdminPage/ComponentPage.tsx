// src/pages/AdminPage/ComponentsTable.tsx
import React, { useCallback, useEffect, useState } from "react";
import { Table, Spinner, Alert, Card, Button, Form } from "react-bootstrap";
import { fetcher } from "../../api/fetcher";

interface Component {
  comp_id: number;
  gh_id: number;
  name: string;
  role: string;       // 'sensor' или 'actuator'
  subtype: string;    // 'temperature', 'humidity', 'fan' и т.д.
  created_at: string;
  updated_at: string;
}

const ComponentsTable: React.FC = () => {
  const [components, setComponents] = useState<Component[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const [editId, setEditId] = useState<number | null>(null);
  const [draft, setDraft] = useState<Partial<Component>>({});
  const [addingRow, setAddingRow] = useState(false);
  const [isOpen, setIsOpen] = useState(true);
  const handleToggleClick = useCallback(() => setIsOpen(prev => !prev), []);

  // Загрузка списка компонентов
  const load = async () => {
    setLoading(true);
    try {
      const data = await fetcher<Component[]>("/api/Components");
      setComponents(data);
      setError(null);
    } catch {
      setError("Ошибка загрузки компонентов");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    load();
  }, []);

  // Сохранение (создание или обновление)
  const handleSave = async () => {
    if (draft.gh_id == null || !draft.name || !draft.role || !draft.subtype) {
      alert("Заполните все поля: gh_id, name, role, subtype");
      return;
    }
    try {
      if (addingRow) {
        const created = await fetcher<Component>("/api/Components", {
          method: "POST",
          body: JSON.stringify(draft),
        });
        setComponents((prev) => [...prev, created]);
      } else if (editId != null) {
        await fetcher(`/api/Components/${editId}`, {
          method: "PUT",
          body: JSON.stringify(draft),
        });
        setComponents((prev) =>
          prev.map((c) =>
            c.comp_id === editId ? ({ ...c, ...draft } as Component) : c
          )
        );
      }
    } catch {
      alert("Ошибка при сохранении");
    } finally {
      cancelEdit();
    }
  };

  // Удаление
  const handleDelete = async (id: number) => {
    if (!window.confirm("Удалить компонент?")) return;
    try {
      await fetcher(`/api/Component/${id}`, { method: "DELETE" });
      setComponents((prev) => prev.filter((c) => c.comp_id !== id));
    } catch {
      alert("Ошибка при удалении");
    }
  };

  const startEdit = (c: Component) => {
    setEditId(c.comp_id);
    setDraft({ gh_id: c.gh_id, name: c.name, role: c.role, subtype: c.subtype });
  };

  const startAdd = () => {
    setAddingRow(true);
    setDraft({ gh_id: 0, name: "", role: "", subtype: "" });
  };

  const cancelEdit = () => {
    setEditId(null);
    setAddingRow(false);
    setDraft({});
  };


  const renderRow = (c: Component) =>
    editId === c.comp_id ? (
      <tr key={c.comp_id} className="align-middle">
        <td>{c.comp_id}</td>
        <td>
          <Form.Control
            size="sm"
            type="number"
            value={draft.gh_id}
            onChange={(e) => setDraft({ ...draft, gh_id: Number(e.target.value) })}
          />
        </td>
        <td>
          <Form.Control
            size="sm"
            value={draft.name}
            onChange={(e) => setDraft({ ...draft, name: e.target.value })}
          />
        </td>
        <td>
          <Form.Select
            size="sm"
            value={draft.role}
            onChange={(e) => setDraft({ ...draft, role: e.target.value })}
          >
            <option value="">Выберите роль</option>
            <option value="sensor">sensor</option>
            <option value="actuator">actuator</option>
          </Form.Select>
        </td>
        <td>
          <Form.Control
            size="sm"
            value={draft.subtype}
            onChange={(e) => setDraft({ ...draft, subtype: e.target.value })}
          />
        </td>
        <td>{c.created_at}</td>
        <td>{c.updated_at}</td>
        <td>
          <Button size="sm" variant="success" onClick={handleSave}>✔️</Button>{" "}
          <Button size="sm" variant="outline-secondary" onClick={cancelEdit}>✖️</Button>
        </td>
      </tr>
    ) : (
      <tr key={c.comp_id} className="align-middle">
        <td>{c.comp_id}</td>
        <td>{c.gh_id}</td>
        <td>{c.name}</td>
        <td>{c.role}</td>
        <td>{c.subtype}</td>
        <td>{c.created_at}</td>
        <td>{c.updated_at}</td>
        <td>
          <Button size="sm" variant="outline-primary" onClick={() => startEdit(c)}>✏️</Button>{" "}
          <Button size="sm" variant="outline-danger" onClick={() => handleDelete(c.comp_id)}>🗑️</Button>
        </td>
      </tr>
    );

  return (
    <Card className="shadow-sm">
      <Card.Header className="d-flex justify-content-between align-items-center">
        <h3 className="mb-0">Компоненты</h3>
        <div>
          <Button
            size="sm"
            variant="outline-secondary"
            onClick={() => setIsOpen(!isOpen)}
            className="me-2"
          >
            {isOpen ? 'Скрыть' : 'Показать'}
          </Button>
          {!addingRow && editId == null && (
            <Button size="sm" onClick={startAdd}>+ Добавить</Button>
          )}
        </div>
      </Card.Header>
      <Card.Body className="p-0">
        {loading ? (
          <div className="text-center py-4"><Spinner /></div>
        ) : error ? (
          <Alert variant="danger" className="m-3">{error}</Alert>
        ) : (
          isOpen && (
            <Table hover responsive className="mb-0">
              <thead className="table-success">
                <tr>
                  <th>ID</th>
                  <th>GH_ID</th>
                  <th>Name</th>
                  <th>Role</th>
                  <th>Subtype</th>
                  <th>Created At</th>
                  <th>Updated At</th>
                  <th>Actions</th>
                </tr>
              </thead>
              <tbody>
                {components.map(renderRow)}

                {addingRow && (
                  <tr className="align-middle">
                    <td>—</td>
                    <td>
                      <Form.Control
                        size="sm"
                        type="number"
                        placeholder="GH_ID"
                        value={draft.gh_id}
                        onChange={(e) => setDraft({ ...draft, gh_id: Number(e.target.value) })}
                      />
                    </td>
                    <td>
                      <Form.Control
                        size="sm"
                        placeholder="Name"
                        value={draft.name}
                        onChange={(e) => setDraft({ ...draft, name: e.target.value })}
                      />
                    </td>
                    <td>
                      <Form.Select
                        size="sm"
                        value={draft.role}
                        onChange={(e) => setDraft({ ...draft, role: e.target.value })}
                      >
                        <option value="">Выберите роль</option>
                        <option value="sensor">sensor</option>
                        <option value="actuator">actuator</option>
                      </Form.Select>
                    </td>
                    <td>
                      <Form.Control
                        size="sm"
                        placeholder="Subtype"
                        value={draft.subtype}
                        onChange={(e) => setDraft({ ...draft, subtype: e.target.value })}
                      />
                    </td>
                    <td>—</td>
                    <td>—</td>
                    <td>
                      <Button size="sm" variant="success" onClick={handleSave}>✔️</Button>{" "}
                      <Button size="sm" variant="outline-secondary" onClick={cancelEdit}>✖️</Button>
                    </td>
                  </tr>
                )}
              </tbody>
            </Table>
          )
        )}
      </Card.Body>
    </Card>
  );
};

export default ComponentsTable;
