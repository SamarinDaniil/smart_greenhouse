// src/pages/AdminPage/RuleTable.tsx
import React, { useEffect, useState } from "react";
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
  const [greenhouses,   setGreenhouses]   = useState<Greenhouse[]>([]);
  const [ghId,          setGhId]          = useState<number | null>(null);

  const [rules,         setRules]         = useState<Rule[]>([]);
  const [sensors,       setSensors]       = useState<Component[]>([]);
  const [actuators,     setActuators]     = useState<Component[]>([]);

  const [loadingGH,     setLoadingGH]     = useState(true);
  const [loadingRules,  setLoadingRules]  = useState(false);
  const [error,         setError]         = useState<string | null>(null);

  /* ---------- CRUD draft ---------- */
  const [editingId, setEditingId] = useState<number | null>(null);
  const [adding,    setAdding]    = useState(false);
  const [draft,     setDraft]     = useState<Partial<Rule>>({});

  /* ---------- Init: список теплиц ---------- */
  useEffect(() => {
    (async () => {
      try {
        const gh = await fetcher<Greenhouse[]>("/api/greenhouses");
        setGreenhouses(gh);
        if (gh.length) setGhId(gh[0].gh_id);
      } catch { setError("Не удалось загрузить теплицы"); }
      finally { setLoadingGH(false); }
    })();
  }, []);

  /* ---------- При смене gh_id: подгружаем правила и компоненты ---------- */
  useEffect(() => {
    if (ghId == null) return;
    const load = async () => {
      setLoadingRules(true); setError(null);
      try {
        const [rulesData, sensorsData, actuatorsData] = await Promise.all([
          fetcher<Rule[]>(`/api/greenhouses/${ghId}/rules`),
          fetcher<Component[]>(`/api/Components?gh_id=${ghId}&role=sensor`),
          fetcher<Component[]>(`/api/Components?gh_id=${ghId}&role=actuator`),
        ]);
        setRules(rulesData);
        setSensors(sensorsData);
        setActuators(actuatorsData);
      } catch { setError("Ошибка загрузки правил / компонентов"); }
      finally { setLoadingRules(false); }
    };
    load();
  }, [ghId]);

  /* ---------- Helpers ---------- */
  const resetDraft = () => { setDraft({}); setEditingId(null); setAdding(false); };

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
    if (!validateDraft()) { alert("Заполните все обязательные поля"); return; }

    try {
      if (adding) {
        const body = { ...draft, gh_id: ghId, enabled: true };
        const created = await fetcher<Rule>("/api/rules", { method: "POST", body: JSON.stringify(body) });
        setRules((r) => [...r, created]);
      } else if (editingId != null) {
        await fetcher(`/api/rules/${editingId}`, { method: "PUT", body: JSON.stringify(draft) });
        setRules((r) => r.map((ru) => (ru.rule_id === editingId ? { ...ru, ...draft } as Rule : ru)));
      }
    } catch { alert("Ошибка сохранения"); }
    finally { resetDraft(); }
  };

  const deleteRule = async (id: number) => {
    if (!window.confirm("Удалить правило?")) return;
    try { await fetcher(`/api/rules/${id}`, { method: "DELETE" }); }
    catch { alert("Ошибка удаления"); return; }
    setRules((r) => r.filter((ru) => ru.rule_id !== id));
  };

  const toggleRule = async (id: number, enabled: boolean) => {
    try {
      await fetcher(`/api/rules/${id}/toggle`, { method: "POST", body: JSON.stringify({ enabled: !enabled }) });
      setRules((r) => r.map((ru) => (ru.rule_id === id ? { ...ru, enabled: !enabled } : ru)));
    } catch { alert("Ошибка переключения"); }
  };

  /* ---------- UI helpers ---------- */
  const compName = (id: number) =>
    [...sensors, ...actuators].find((c) => c.comp_id === id)?.name || id;

  const operatorOptions = ["<", "<=", "==", ">=", ">"];

  /* ---------- JSX ---------- */
  return (
    <Card className="mb-4 shadow-sm">
      <Card.Body>
        <Card.Title className="d-flex justify-content-between align-items-center">
          Правила
          {!adding && editingId == null && ghId && (
            <Button size="sm" onClick={() => { setAdding(true); setDraft({ kind: "time", gh_id: ghId, from_comp_id: autoTimeSensor(), enabled: true }); }}>
              + Добавить
            </Button>
          )}
        </Card.Title>

        {/* Выбор теплицы */}
        <Form.Select
          className="mb-3"
          style={{ maxWidth: 300 }}
          value={ghId ?? undefined}
          onChange={(e) => setGhId(Number(e.target.value))}
        >
          {greenhouses.map((g) => (
            <option key={g.gh_id} value={g.gh_id}>{g.name} (ID:{g.gh_id})</option>
          ))}
        </Form.Select>

        {loadingGH || loadingRules ? (
          <div className="text-center py-4"><Spinner animation="border" /></div>
        ) : error ? (
          <Alert variant="danger">{error}</Alert>
        ) : (
          <Table striped bordered hover responsive>
            <thead>
              <tr>
                <th>ID</th><th>Имя</th><th>Тип</th><th>FROM</th><th>OP</th><th>Threshold/Time</th><th>TO</th><th>⚡</th><th>Действия</th>
              </tr>
            </thead>
            <tbody>
              {/* существующие */}
              {rules.map((r) => (
                editingId === r.rule_id ? (
                  /* ---------- строка редактирования ---------- */
                  <tr key={r.rule_id}>
                    <td>{r.rule_id}</td>
                    <td><Form.Control value={draft.name || ""} onChange={(e) => setDraft({ ...draft, name: e.target.value })} /></td>
                    <td>
                      <Form.Select
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

                    {/* FROM comp (only for threshold) */}
                    <td>
                      {draft.kind === "threshold" ? (
                        <Form.Select
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

                    {/* Operator */}
                    <td>
                      {draft.kind === "threshold" ? (
                        <Form.Select
                          value={draft.operator_}
                          onChange={(e) => setDraft({ ...draft, operator_: e.target.value })}
                        >
                          {operatorOptions.map((op) => <option key={op} value={op}>{op}</option>)}
                        </Form.Select>
                      ) : "—"}
                    </td>

                    {/* Threshold / Time */}
                    <td>
                      {draft.kind === "threshold" ? (
                        <Form.Control
                          type="number"
                          value={draft.threshold ?? ""}
                          onChange={(e) => setDraft({ ...draft, threshold: Number(e.target.value) })}
                        />
                      ) : (
                        <Form.Control
                          placeholder="HH:MM или ISO дата"
                          value={draft.time_spec || ""}
                          onChange={(e) => setDraft({ ...draft, time_spec: e.target.value })}
                        />
                      )}
                    </td>

                    {/* TO comp */}
                    <td>
                      <Form.Select
                        value={draft.to_comp_id}
                        onChange={(e) => setDraft({ ...draft, to_comp_id: Number(e.target.value) })}
                      >
                        <option value="">— актюатор —</option>
                        {actuators.map((a) => <option key={a.comp_id} value={a.comp_id}>{a.name}</option>)}
                      </Form.Select>
                    </td>

                    {/* enabled */}
                    <td>
                      <Form.Check
                        type="switch"
                        checked={draft.enabled}
                        onChange={(e) => setDraft({ ...draft, enabled: e.target.checked })}
                      />
                    </td>

                    <td>
                      <Button size="sm" variant="success" onClick={saveRule}>Сохранить</Button>{" "}
                      <Button size="sm" variant="secondary" onClick={resetDraft}>Отмена</Button>
                    </td>
                  </tr>
                ) : (
                  /* ---------- обычная строка ---------- */
                  <tr key={r.rule_id}>
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
                    <td>
                      <Button size="sm" variant="outline-primary" onClick={() => { setEditingId(r.rule_id); setDraft(r); }}>
                        ✏️
                      </Button>{" "}
                      <Button size="sm" variant="outline-danger" onClick={() => deleteRule(r.rule_id)}>
                        🗑️
                      </Button>
                    </td>
                  </tr>
                )
              ))}

              {/* строка добавления */}
              {adding && (
                <tr>
                  <td>—</td>
                  <td>
                    <Form.Control value={draft.name || ""} onChange={(e) => setDraft({ ...draft, name: e.target.value })} />
                  </td>
                  <td>
                    <Form.Select
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

                  {/* FROM (threshold only) */}
                  <td>
                    {draft.kind === "threshold" ? (
                      <Form.Select value={draft.from_comp_id} onChange={(e) => setDraft({ ...draft, from_comp_id: Number(e.target.value) })}>
                        <option value="">— сенсор —</option>
                        {sensors.map((s) => <option key={s.comp_id} value={s.comp_id}>{s.name}</option>)}
                      </Form.Select>
                    ) : <span className="text-muted">{compName(draft.from_comp_id!)}</span>}
                  </td>

                  {/* Operator */}
                  <td>
                    {draft.kind === "threshold" ? (
                      <Form.Select value={draft.operator_} onChange={(e) => setDraft({ ...draft, operator_: e.target.value })}>
                        {operatorOptions.map((op) => <option key={op} value={op}>{op}</option>)}
                      </Form.Select>
                    ) : "—"}
                  </td>

                  {/* Threshold / Time */}
                  <td>
                    {draft.kind === "threshold" ? (
                      <Form.Control type="number" value={draft.threshold ?? ""} onChange={(e) => setDraft({ ...draft, threshold: Number(e.target.value) })} />
                    ) : (
                      <Form.Control placeholder="HH:MM или ISO дата" value={draft.time_spec || ""} onChange={(e) => setDraft({ ...draft, time_spec: e.target.value })} />
                    )}
                  </td>

                  {/* TO */}
                  <td>
                    <Form.Select value={draft.to_comp_id} onChange={(e) => setDraft({ ...draft, to_comp_id: Number(e.target.value) })}>
                      <option value="">— актюатор —</option>
                      {actuators.map((a) => <option key={a.comp_id} value={a.comp_id}>{a.name}</option>)}
                    </Form.Select>
                  </td>

                  <td>—</td>
                  <td>
                    <Button size="sm" variant="success" onClick={saveRule}>Создать</Button>{" "}
                    <Button size="sm" variant="secondary" onClick={resetDraft}>Отмена</Button>
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

export default RuleTable;
