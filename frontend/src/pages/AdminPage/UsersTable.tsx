import React, { useCallback, useEffect, useState } from "react";
import { Table, Spinner, Alert, Card, Button, Form, InputGroup } from "react-bootstrap";
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

  const [isOpen, setIsOpen] = useState(true);
  const handleToggleClick = useCallback(() => setIsOpen(prev => !prev), []);

  const loadUsers = async () => {
    setLoading(true);
    try {
      const data = await fetcher<User[]>('/api/users');
      setUsers(data);
    } catch {
      setError('Ошибка загрузки пользователей');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => { loadUsers(); }, []);

  const cancelEdit = () => {
    setEditId(null);
    setAddingRow(false);
    setDraft({});
  };

  const handleSave = async () => {
    if (!draft.username || !draft.role || (addingRow && !draft.password)) {
      alert("Все поля обязательны при добавлении");
      return;
    }
    try {
      if (addingRow) {
        const newUser = await fetcher<User>('/api/register', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ username: draft.username, role: draft.role, password: draft.password })
        });
        setUsers(prev => [...prev, newUser]);
      } else if (editId !== null) {
        const toSend: any = { username: draft.username, role: draft.role };
        if (draft.password) toSend.password = draft.password;
        await fetcher(`/api/users/${editId}`, {
          method: 'PUT', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(toSend)
        });
        setUsers(prev => prev.map(u => u.user_id === editId ? { ...u, ...draft } as User : u));
      }
    } catch {
      alert('Ошибка при сохранении');
    } finally {
      cancelEdit();
    }
  };

  const handleDelete = async (id: number) => {
    if (!window.confirm('Удалить пользователя?')) return;
    try {
      await fetcher(`/api/users/${id}`, { method: 'DELETE' });
      setUsers(prev => prev.filter(u => u.user_id !== id));
    } catch {
      alert('Ошибка при удалении');
    }
  };

  const startEdit = (user: User) => {
    setEditId(user.user_id);
    setDraft({ username: user.username, role: user.role, password: '' });
  };

  const startAdd = () => {
    setAddingRow(true);
    setDraft({ username: '', role: '', password: '' });
  };

  const renderRow = (user: User) => (
    <tr key={user.user_id} className="align-middle">
      {editId === user.user_id ? (
        <>  
          <td>{user.user_id}</td>
          <td><Form.Control size="sm" value={draft.username} onChange={e => setDraft({...draft, username: e.target.value})} /></td>
          <td><Form.Control size="sm" value={draft.role} onChange={e => setDraft({...draft, role: e.target.value})} /></td>
          <td><Form.Control size="sm" type="password" placeholder="••••••" value={draft.password} onChange={e => setDraft({...draft, password: e.target.value})} /></td>
          <td>
            <Button size="sm" variant="success" onClick={handleSave}>✔️</Button>{' '}
            <Button size="sm" variant="outline-secondary" onClick={cancelEdit}>✖️</Button>
          </td>
        </>
      ) : (
        <>
          <td>{user.user_id}</td>
          <td>{user.username}</td>
          <td>{user.role}</td>
          <td>—</td>
          <td>
            <Button size="sm" variant="outline-primary" onClick={() => startEdit(user)}>✏️</Button>{' '}
            <Button size="sm" variant="outline-danger" onClick={() => handleDelete(user.user_id)}>🗑️</Button>
          </td>
        </>
      )}
    </tr>
  );

  return (
    <Card className="shadow-sm">
      <Card.Header className="d-flex justify-content-between align-items-center">
        <h3 className="mb-0">Пользователи</h3>
        <div>
          <Button size="sm" variant="outline-secondary" onClick={handleToggleClick} className="me-2">
            {isOpen ? 'Скрыть' : 'Показать'}
          </Button>
          {!addingRow && editId === null && <Button size="sm" onClick={startAdd}>+ Добавить</Button>}
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
                  <th>ID</th><th>Имя</th><th>Роль</th><th>Пароль</th><th>Действия</th>
                </tr>
              </thead>
              <tbody>
                {users.map(renderRow)}
                {addingRow && (
                  <tr className="align-middle">
                    <td>—</td>
                    <td><Form.Control size="sm" placeholder="Имя" value={draft.username} onChange={e => setDraft({...draft, username: e.target.value})} /></td>
                    <td><Form.Control size="sm" placeholder="Роль" value={draft.role} onChange={e => setDraft({...draft, role: e.target.value})} /></td>
                    <td><Form.Control size="sm" type="password" placeholder="Пароль" value={draft.password} onChange={e => setDraft({...draft, password: e.target.value})} /></td>
                    <td>
                      <Button size="sm" variant="success" onClick={handleSave}>✔️</Button>{' '}
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

export default UsersTable;
