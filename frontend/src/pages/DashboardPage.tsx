import React, { useEffect, useState } from "react";
import {
  Spinner,
  Alert,
  Card,
  Form,
  Button,
  Collapse,
  Row,
  Col,
  Table,
} from "react-bootstrap";
import { fetcher } from "../api/fetcher";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from "recharts";

// --- Interfaces ---
interface Metric {
  metric_id: number;
  gh_id: number;
  ts: string;
  subtype: string;
  value: number;
}

interface Component {
  comp_id: number;
  gh_id: number;
  name: string;
  role: string;
  subtype: string;
  created_at?: string;
  updated_at?: string;
}

interface Rule {
  rule_id: number;
  gh_id: number;
  name: string;
  enabled: boolean;
}

interface Greenhouse {
  gh_id: number;
  name: string;
}

const subtypeIcons: Record<string, string> = {
  temperature: "fas fa-thermometer-half",
  humidity: "fas fa-tint",
  co2: "fas fa-cloud",
  light: "fas fa-sun",
};

const DashboardPage: React.FC = () => {
  // State
  const [greenhouses, setGreenhouses] = useState<Greenhouse[]>([]);
  const [ghId, setGhId] = useState<number | null>(null);

  const [components, setComponents] = useState<Component[]>([]);
  const [subtypes, setSubtypes] = useState<string[]>([]);
  const [selectedSubtype, setSelectedSubtype] = useState<string>("");

  const [latestMetrics, setLatestMetrics] = useState<Metric[]>([]);
  const [showLatest, setShowLatest] = useState<boolean>(
    () => localStorage.getItem("showLatest") !== "false"
  );
  const [loadingLatest, setLoadingLatest] = useState(false);
  const [errorLatest, setErrorLatest] = useState<string | null>(null);

  const [metrics, setMetrics] = useState<Metric[]>([]);
  const [loadingMetrics, setLoadingMetrics] = useState(false);
  const [errorMetrics, setErrorMetrics] = useState<string | null>(null);

  const [rules, setRules] = useState<Rule[]>([]);
  const [loadingRules, setLoadingRules] = useState(false);
  const [errorRules, setErrorRules] = useState<string | null>(null);

  const [from, setFrom] = useState<string>(
    new Date(Date.now() - 7 * 24 * 3600 * 1000).toISOString().slice(0, 10)
  );
  const [to, setTo] = useState<string>(new Date().toISOString().slice(0, 10));

  // Load greenhouses
  useEffect(() => {
    const loadGH = async () => {
      try {
        const data = await fetcher<Greenhouse[]>('/api/greenhouses');
        setGreenhouses(data);
        if (data.length > 0) setGhId(data[0].gh_id);
      } catch {
        // handle error if needed
      }
    };
    loadGH();
  }, []);

  // Load components and subtypes when ghId changes
  useEffect(() => {
    if (ghId === null) return;
    const loadComponents = async () => {
      try {
        const data = await fetcher<Component[]>(
          `/api/components?gh_id=${ghId}&role=sensor`
        );
        setComponents(data);
        // Filter out "Time" subtype (case-insensitive)
        const rawTypes = Array.from(new Set(data.map((c) => c.subtype)));
        const filtered = rawTypes.filter(
          (t) => t.toLowerCase() !== 'time'
        );
        setSubtypes(filtered);
        setSelectedSubtype(filtered[0] || "");
      } catch {
        // handle error
      }
    };
    loadComponents();
    localStorage.setItem("showLatest", showLatest.toString());
  }, [ghId, showLatest]);

  // Load latest metrics
  useEffect(() => {
    if (ghId === null || subtypes.length === 0) return;
    const loadLatest = async () => {
      setLoadingLatest(true);
      setErrorLatest(null);
      try {
        const promises = subtypes.map((st) =>
          fetcher<Metric>(
            `/api/metrics/latest?gh_id=${ghId}&subtype=${st}`
          )
        );
        const results = await Promise.all(promises);
        setLatestMetrics(results);
      } catch {
        setErrorLatest('Не удалось загрузить последние метрики');
      } finally {
        setLoadingLatest(false);
      }
    };
    loadLatest();
  }, [ghId, subtypes]);

  // Load historical metrics
  const loadMetrics = async () => {
    if (ghId === null || !selectedSubtype) return;
    setLoadingMetrics(true);
    setErrorMetrics(null);

    try {
      const params = new URLSearchParams({
        gh_id: ghId.toString(),
        subtype: selectedSubtype,
        from,
        to,
        limit: '1000',
      });
      const data = await fetcher<Metric[]>(
        `/api/metrics?${params.toString()}`
      );
      setMetrics(data);
    } catch {
      setErrorMetrics('Ошибка загрузки метрик');
    } finally {
      setLoadingMetrics(false);
    }
  };

  useEffect(() => {
    loadMetrics();
  }, [ghId, selectedSubtype, from, to]);

  // Load rules
  useEffect(() => {
    if (ghId === null) return;
    const loadRules = async () => {
      setLoadingRules(true);
      setErrorRules(null);
      try {
        const data = await fetcher<Rule[]>(
          `/api/greenhouses/${ghId}/rules`
        );
        setRules(data);
      } catch {
        setErrorRules('Ошибка загрузки правил');
      } finally {
        setLoadingRules(false);
      }
    };
    loadRules();
  }, [ghId]);

  const toggleRule = async (rule: Rule) => {
    try {
      await fetcher(
        `/api/rules/${rule.rule_id}/toggle`,
        {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ enabled: !rule.enabled }),
        }
      );
      setRules((prev) =>
        prev.map((r) =>
          r.rule_id === rule.rule_id ? { ...r, enabled: !r.enabled } : r
        )
      );
    } catch {
      // handle error
    }
  };

  const chartData = metrics.map((m) => ({
    time: new Date(m.ts).toLocaleString(),
    value: m.value,
  }));

  return (
    <Card className="p-3">
      <h1>Панель пользователя</h1>

      {/* Выбор теплицы */}
      <Form className="mb-3 d-flex align-items-end" style={{ gap: '1rem' }}>
        <Form.Group controlId="ghSelect">
          <Form.Label>Теплица</Form.Label>
          <Form.Select
            value={ghId ?? undefined}
            onChange={(e) => setGhId(Number(e.target.value))}
          >
            {greenhouses.map((gh) => (
              <option key={gh.gh_id} value={gh.gh_id}>
                {gh.name} (ID: {gh.gh_id})
              </option>
            ))}
          </Form.Select>
        </Form.Group>
      </Form>

      {/* Последние метрики */}
      <Button
        onClick={() => setShowLatest((prev) => !prev)}
        aria-controls="latest-collapse"
        className="mb-3"
      >
        {showLatest ? 'Скрыть' : 'Показать'} последние метрики
      </Button>
      <Collapse in={showLatest}>
        <div id="latest-collapse">
          {loadingLatest ? (
            <div className="text-center py-4"><Spinner animation="border" /></div>
          ) : errorLatest ? (
            <Alert variant="danger">{errorLatest}</Alert>
          ) : (
            <Row xs={2} md={4} className="g-3">
              {latestMetrics.map((m) => (
                <Col key={m.subtype}>
                  <Card className="text-center p-2">
                    <i className={`${subtypeIcons[m.subtype]} fa-2x`}></i>
                    <h4 className="mt-2">{m.value.toFixed(1)}</h4>
                    <small>{m.subtype}</small>
                  </Card>
                </Col>
              ))}
            </Row>
          )}
        </div>
      </Collapse>

      {/* Графики */}
      <Form
        className="mb-3 d-flex flex-wrap align-items-end"
        style={{ gap: '1rem' }}
        onSubmit={(e) => { e.preventDefault(); loadMetrics(); }}
      >
        <Form.Group controlId="subtypeSelect" style={{ minWidth: '200px' }}>
          <Form.Label>Тип метрики</Form.Label>
          <Form.Select
            value={selectedSubtype}
            onChange={(e) => setSelectedSubtype(e.target.value)}
          >
            {subtypes.map((st) => (
              <option key={st} value={st}>{st}</option>
            ))}
          </Form.Select>
        </Form.Group>
        <Form.Group controlId="fromDate">
          <Form.Label>Дата с</Form.Label>
          <Form.Control type="date" value={from} onChange={(e) => setFrom(e.target.value)} />
        </Form.Group>
        <Form.Group controlId="toDate">
          <Form.Label>Дата по</Form.Label>
          <Form.Control type="date" value={to} onChange={(e) => setTo(e.target.value)} />
        </Form.Group>
        <Button type="submit">Обновить график</Button>
      </Form>
      {loadingMetrics ? (
        <div className="text-center py-4"><Spinner animation="border" /></div>
      ) : errorMetrics ? (
        <Alert variant="danger">{errorMetrics}</Alert>
      ) : chartData.length === 0 ? (
        <Alert variant="info">Данные не найдены</Alert>
      ) : (
        <ResponsiveContainer width="100%" height={300}>
          <LineChart data={chartData} margin={{ top: 20, right: 30, left: 0, bottom: 5 }}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="time" tick={{ fontSize: 12 }} />
            <YAxis />
            <Tooltip />
            <Legend />
            <Line type="monotone" dataKey="value" dot={false} name={selectedSubtype} />
          </LineChart>
        </ResponsiveContainer>
      )}

      {/* Правила */}
      <h3 className="mt-4">Правила</h3>
      {loadingRules ? (
        <Spinner animation="border" />
      ) : errorRules ? (
        <Alert variant="danger">{errorRules}</Alert>
      ) : (
        <Table bordered hover>
          <thead>
            <tr><th>Описание</th><th>Статус</th></tr>
          </thead>
          <tbody>
            {rules.map((r) => (
              <tr key={r.rule_id}>
                <td>{r.name}</td>
                <td>
                  <Button
                    variant={r.enabled ? 'success' : 'outline-secondary'}
                    size="sm"
                    onClick={() => toggleRule(r)}
                  >{r.enabled ? 'Включено' : 'Выключено'}</Button>
                </td>
              </tr>
            ))}
          </tbody>
        </Table>
      )}
    </Card>
  );
};

export default DashboardPage;
