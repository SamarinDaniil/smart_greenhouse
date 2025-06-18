// src/pages/AdminPage/RuleTable.tsx
import React, { useCallback, useEffect, useState } from "react";
import {
  Card, Table, Spinner, Alert, Button, Form, Badge,
} from "react-bootstrap";
import { fetcher } from "../../api/fetcher";

/* ---------- Типы ---------- */
interface Rule {
  rule_id: number;
  gh_id: number;
  name: string;
  from_comp_id: number;
  to_comp_id: number;
  kind: "time" | "threshold";
  operator_?: string;
  threshold?: number;
  time_spec?: string;
  enabled: boolean;
  created_at: string;
  updated_at: string;
}

interface Greenhouse { gh_id: number; name: string; }
interface Component { comp_id: number; name: string; role: "sensor" | "actuator"; subtype: string; }

const RuleTable: React.FC = () => {
  /* ---------- State ---------- */
  const [greenhouses, setGreenhouses] = useState<Greenhouse[]>([]);
  const [ghId, setGhId] = useState<number | null>(null);

  const [rules, setRules] = useState<Rule[]>([]);
  const [sensors, setSensors] = useState<Component[]>([]);
  const [actuators, setActuators] = useState<Component[]>([]);

  const [loadingGH, setLoadingGH] = useState(true);
  const [loadingRules, setLoadingRules] = useState(false);
  const [error, setError] = useState<string | null>(null);

  /* ---------- CRUD draft ---------- */
  const [editingId, setEditingId] = useState<number | null>(null);
  const [adding, setAdding] = useState(false);
  const [draft, setDraft] = useState<Partial<Rule>>({});
  const [isOpen, setIsOpen] = useState(true);

  const handleToggleClick = useCallback(() => setIsOpen(prev => !prev), []);

  /* ---------- Init: список теплиц ---------- */
  useEffect(() => {
    (async () => {
      try {
        const gh = await fetcher<Greenhouse[]>("/api/greenhouses");
        setGreenhouses(gh);
        if (gh.length) setGhId(gh[0].gh_id);
      } catch { 
        setError("Не удалось загрузить теплицы"); 
      } finally { 
        setLoadingGH(false); 
      }
    })();
  }, []);

  /* ---------- При смене gh_id: подгружаем правила и компоненты ---------- */
  useEffect(() => {
    if (ghId == null) return;
    const load = async () => {
      setLoadingRules(true); 
      setError(null);
      try {
        const [rulesData, sensorsData, actuatorsData] = await Promise.all([
          fetcher<Rule[]>(`/api/greenhouses/${ghId}/rules`),
          fetcher<Component[]>(`/api/Components?gh_id=${ghId}&role=sensor`),
          fetcher<Component[]>(`/api/Components?gh_id=${ghId}&role=actuator`),
        ]);
        setRules(rulesData);
        setSensors(sensorsData);
        setActuators(actuatorsData);
      } catch { 
        setError("Ошибка загрузки правил / компонентов"); 
      } finally { 
        setLoadingRules(false); 
      }
    };
    load();
  }, [ghId]);

  /* ---------- Helpers ---------- */
  const resetDraft = () => { 
    setDraft({}); 
    setEditingId(null); 
    setAdding(false); 
  };

  const validateDraft = () => {
    if (!draft.name || !draft.kind || !draft.to_comp_id) return false;
    if (draft.kind === "threshold" && (!draft.from_comp_id || !draft.operator_ || draft.threshold == null))
      return false;
    if (draft.kind === "time" && !draft.time_spec) return false;
    return true;
  };

  const autoTimeSensor = () => {
    const sensorTime = sensors.find((s) => s.subtype === "Time");
    return sensorTime ? sensorTime.comp_id : 0;
  };

  /* ---------- CRUD Actions ---------- */
  const saveRule = async () => {
    if (!validateDraft()) { 
      alert("Заполните все обязательные поля"); 
      return; 
    }

    try {
      if (adding) {
        const body = { ...draft, gh_id: ghId, enabled: true };
        const created = await fetcher<Rule>("/api/rules", { 
          method: "POST", 
          body: JSON.stringify(body) 
        });
        setRules((r) => [...r, created]);
      } else if (editingId != null) {
        await fetcher(`/api/rules/${editingId}`, { 
          method: "PUT", 
          body: JSON.stringify(draft) 
        });
        setRules((r) => r.map((ru) => (ru.rule_id === editingId ? { ...ru, ...draft } as Rule : ru)));
      }
    } catch { 
      alert("Ошибка сохранения"); 
    } finally { 
      resetDraft(); 
    }
  };

  const deleteRule = async (id: number) => {
    if (!window.confirm("Удалить правило?")) return;
    try { 
      await fetcher(`/api/rules/${id}`, { method: "DELETE" }); 
      setRules((r) => r.filter((ru) => ru.rule_id !== id));
    } catch { 
      alert("Ошибка удаления"); 
    }
  };

  const toggleRule = async (id: number, enabled: boolean) => {
    try {
      await fetcher(`/api/rules/${id}/toggle`, { 
        method: "POST", 
        body: JSON.stringify({ enabled: !enabled }) 
      });
      setRules((r) => r.map((ru) => (ru.rule_id === id ? { ...ru, enabled: !enabled } : ru)));
    } catch { 
      alert("Ошибка переключения"); 
    }
  };

  const startEdit = (r: Rule) => {
    setEditingId(r.rule_id);
    setDraft(r);
  };

  const startAdd = () => {
    setAdding(true);
    setDraft({ 
      kind: "time", 
      gh_id: ghId??undefined, 
      from_comp_id: autoTimeSensor(), 
      enabled: true,
      name: "",
      time_spec: ""
    });
  };

  /* ---------- UI helpers ---------- */
  const compName = (id: number) =>
    [...sensors, ...actuators].find((c) => c.comp_id === id)?.name || id;

  const operatorOptions = ["<", "<=", "==", ">=", ">"];

  const renderRow = (r: Rule) => 
    editingId === r.rule_id ? (
      <tr key={r.rule_id} className="align-middle">
        <td>{r.rule_id}</td>
        <td>
          <Form.Control 
            size="sm"
            value={draft.name || ""} 
            onChange={(e) => setDraft({ ...draft, name: e.target.value })} 
          />
        </td>
        <td>
          <Form.Select
            size="sm"
            value={draft.kind}
            onChange={(e) => {
              const k = e.target.value as "time" | "threshold";
              setDraft(k === "time"
                ? { ...draft, kind: k, operator_: undefined, threshold: undefined, time_spec: draft.time_spec || "", from_comp_id: autoTimeSensor() }
                : { ...draft, kind: k, time_spec: undefined, operator_: "<", threshold: draft.threshold ?? 0 });
            }}>
            <option value="time">time</option>
            <option value="threshold">threshold</option>
          </Form.Select>
        </td>
        <td>
          {draft.kind === "threshold" ? (
            <Form.Select
              size="sm"
              value={draft.from_comp_id}
              onChange={(e) => setDraft({ ...draft, from_comp_id: Number(e.target.value) })}
            >
              <option value="">— сенсор —</option>
              {sensors.map((s) => <option key={s.comp_id} value={s.comp_id}>{s.name}</option>)}
            </Form.Select>
          ) : (
            <span className="text-muted">{compName(draft.from_comp_id!)}</span>
          )}
        </td>
        <td>
          {draft.kind === "threshold" ? (
            <Form.Select
              size="sm"
              value={draft.operator_}
              onChange={(e) => setDraft({ ...draft, operator_: e.target.value })}
            >
              {operatorOptions.map((op) => <option key={op} value={op}>{op}</option>)}
            </Form.Select>
          ) : "—"}
        </td>
        <td>
          {draft.kind === "threshold" ? (
            <Form.Control
              size="sm"
              type="number"
              value={draft.threshold ?? ""}
              onChange={(e) => setDraft({ ...draft, threshold: Number(e.target.value) })}
            />
          ) : (
            <Form.Control
              size="sm"
              placeholder="HH:MM или ISO дата"
              value={draft.time_spec || ""}
              onChange={(e) => setDraft({ ...draft, time_spec: e.target.value })}
            />
          )}
        </td>
        <td>
          <Form.Select
            size="sm"
            value={draft.to_comp_id}
            onChange={(e) => setDraft({ ...draft, to_comp_id: Number(e.target.value) })}
          >
            <option value="">— актюатор —</option>
            {actuators.map((a) => <option key={a.comp_id} value={a.comp_id}>{a.name}</option>)}
          </Form.Select>
        </td>
        <td>
          <Form.Check
            type="switch"
            checked={draft.enabled}
            onChange={(e) => setDraft({ ...draft, enabled: e.target.checked })}
          />
        </td>
        <td>{r.created_at}</td>
        <td>{r.updated_at}</td>
        <td>
          <Button size="sm" variant="success" onClick={saveRule}>✔️</Button>{" "}
          <Button size="sm" variant="outline-secondary" onClick={resetDraft}>✖️</Button>
        </td>
      </tr>
    ) : (
      <tr key={r.rule_id} className="align-middle">
        <td>{r.rule_id}</td>
        <td>{r.name}</td>
        <td>{r.kind}</td>
        <td>{compName(r.from_comp_id)}</td>
        <td>{r.operator_ || "—"}</td>
        <td>{r.kind === "time" ? r.time_spec : r.threshold}</td>
        <td>{compName(r.to_comp_id)}</td>
        <td>
          <Badge
            bg={r.enabled ? "success" : "secondary"}
            style={{ cursor: "pointer" }}
            onClick={() => toggleRule(r.rule_id, r.enabled)}
          >
            {r.enabled ? "ON" : "OFF"}
          </Badge>
        </td>
        <td>{r.created_at}</td>
        <td>{r.updated_at}</td>
        <td>
          <Button size="sm" variant="outline-primary" onClick={() => startEdit(r)}>✏️</Button>{" "}
          <Button size="sm" variant="outline-danger" onClick={() => deleteRule(r.rule_id)}>🗑️</Button>
        </td>
      </tr>
    );

  /* ---------- JSX ---------- */
  return (
    <Card className="shadow-sm">
       <Card.Header className="d-flex justify-content-between align-items-center">
        <h3 className="mb-0">Правила</h3>
        <div>
          <Button
            size="sm"
            variant="outline-secondary"
            onClick={handleToggleClick}
            className="me-2"
          >
            {isOpen ? 'Скрыть' : 'Показать'}
          </Button>
          {!adding && editingId == null && ghId && (
            <Button size="sm" onClick={startAdd}>+ Добавить</Button>
          )}
        </div>
      </Card.Header>

      <Card.Body className="p-0">
        {/* Выбор теплицы */}
        <div className="p-3 border-bottom">
          <Form.Select
            style={{ maxWidth: 300 }}
            value={ghId ?? undefined}
            onChange={(e) => setGhId(Number(e.target.value))}
          >
            {greenhouses.map((g) => (
              <option key={g.gh_id} value={g.gh_id}>{g.name} (ID:{g.gh_id})</option>
            ))}
          </Form.Select>
        </div>

        {loadingGH || loadingRules ? (
          <div className="text-center py-4"><Spinner /></div>
        ) : error ? (
          <Alert variant="danger" className="m-3">{error}</Alert>
        ) : (
          isOpen && (
            <Table hover responsive className="mb-0">
              <thead className="table-success">
                <tr>
                  <th>ID</th>
                  <th>Имя</th>
                  <th>Тип</th>
                  <th>FROM</th>
                  <th>OP</th>
                  <th>Threshold/Time</th>
                  <th>TO</th>
                  <th>⚡</th>
                  <th>Created At</th>
                  <th>Updated At</th>
                  <th>Actions</th>
                </tr>
              </thead>
              <tbody>
                {rules.map(renderRow)}

                {adding && (
                  <tr className="align-middle">
                    <td>—</td>
                    <td>
                      <Form.Control 
                        size="sm"
                        placeholder="Имя правила"
                        value={draft.name || ""} 
                        onChange={(e) => setDraft({ ...draft, name: e.target.value })} 
                      />
                    </td>
                    <td>
                      <Form.Select
                        size="sm"
                        value={draft.kind}
                        onChange={(e) => {
                          const k = e.target.value as "time" | "threshold";
                          setDraft(k === "time"
                            ? { ...draft, kind: k, from_comp_id: autoTimeSensor(), operator_: undefined, threshold: undefined, time_spec: draft.time_spec || "" }
                            : { ...draft, kind: k, operator_: "<", threshold: 0, time_spec: undefined });
                        }}>
                        <option value="time">time</option>
                        <option value="threshold">threshold</option>
                      </Form.Select>
                    </td>
                    <td>
                      {draft.kind === "threshold" ? (
                        <Form.Select 
                          size="sm"
                          value={draft.from_comp_id} 
                          onChange={(e) => setDraft({ ...draft, from_comp_id: Number(e.target.value) })}
                        >
                          <option value="">— сенсор —</option>
                          {sensors.map((s) => <option key={s.comp_id} value={s.comp_id}>{s.name}</option>)}
                        </Form.Select>
                      ) : (
                        <span className="text-muted">{compName(draft.from_comp_id!)}</span>
                      )}
                    </td>
                    <td>
                      {draft.kind === "threshold" ? (
                        <Form.Select 
                          size="sm"
                          value={draft.operator_} 
                          onChange={(e) => setDraft({ ...draft, operator_: e.target.value })}
                        >
                          {operatorOptions.map((op) => <option key={op} value={op}>{op}</option>)}
                        </Form.Select>
                      ) : "—"}
                    </td>
                    <td>
                      {draft.kind === "threshold" ? (
                        <Form.Control 
                          size="sm"
                          type="number" 
                          placeholder="Пороговое значение"
                          value={draft.threshold ?? ""} 
                          onChange={(e) => setDraft({ ...draft, threshold: Number(e.target.value) })} 
                        />
                      ) : (
                        <Form.Control 
                          size="sm"
                          placeholder="HH:MM или ISO дата" 
                          value={draft.time_spec || ""} 
                          onChange={(e) => setDraft({ ...draft, time_spec: e.target.value })} 
                        />
                      )}
                    </td>
                    <td>
                      <Form.Select 
                        size="sm"
                        value={draft.to_comp_id} 
                        onChange={(e) => setDraft({ ...draft, to_comp_id: Number(e.target.value) })}
                      >
                        <option value="">— актюатор —</option>
                        {actuators.map((a) => <option key={a.comp_id} value={a.comp_id}>{a.name}</option>)}
                      </Form.Select>
                    </td>
                    <td>—</td>
                    <td>—</td>
                    <td>—</td>
                    <td>
                      <Button size="sm" variant="success" onClick={saveRule}>✔️</Button>{" "}
                      <Button size="sm" variant="outline-secondary" onClick={resetDraft}>✖️</Button>
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

export default RuleTable;