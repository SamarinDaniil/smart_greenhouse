import React, { useEffect, useState } from "react";
import { Spinner, Alert, Card, Form, Button } from "react-bootstrap";
import { fetcher } from "../api/fetcher";
import {
  LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer,
} from "recharts";

interface Metric {
  metric_id: number;
  gh_id: number;
  subtype: string;
  value: number;
  timestamp: string;
}

interface Greenhouse {
  gh_id: number;
  name: string;
  location?: string;
  created_at?: string;
  updated_at?: string;
}

const DashboardPage: React.FC = () => {
  const [metrics, setMetrics] = useState<Metric[]>([]);
  const [greenhouses, setGreenhouses] = useState<Greenhouse[]>([]);
  const [loading, setLoading] = useState(false);
  const [loadingGH, setLoadingGH] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [errorGH, setErrorGH] = useState<string | null>(null);

  const [ghId, setGhId] = useState<number | null>(null);
  const [subtype, setSubtype] = useState<string>("temperature");
  const [from, setFrom] = useState<string>(new Date(Date.now() - 7 * 24 * 3600 * 1000).toISOString().slice(0, 10));
  const [to, setTo] = useState<string>(new Date().toISOString().slice(0, 10));

  // Загрузка списка теплиц
  useEffect(() => {
    const loadGreenhouses = async () => {
      setLoadingGH(true);
      setErrorGH(null);
      try {
        const data = await fetcher<Greenhouse[]>("/api/greenhouses");
        setGreenhouses(data);
        if (data.length > 0) setGhId(data[0].gh_id); // по умолчанию первая теплица
      } catch {
        setErrorGH("Ошибка загрузки списка теплиц");
      } finally {
        setLoadingGH(false);
      }
    };
    loadGreenhouses();
  }, []);

  // Загрузка метрик
  const loadMetrics = async () => {
    if (ghId === null) return;
    setLoading(true);
    setError(null);
    try {
      const params = new URLSearchParams({
        gh_id: ghId.toString(),
        subtype,
        from,
        to,
        limit: "1000",
      });
      const data = await fetcher<Metric[]>(`/api/metrics?${params.toString()}`);
      setMetrics(data);
    } catch {
      setError("Ошибка загрузки метрик");
      setMetrics([]);
    } finally {
      setLoading(false);
    }
  };

  // Автозагрузка метрик при смене ghId, subtype, from, to
  useEffect(() => {
    if (ghId !== null) {
      loadMetrics();
    }
  }, [ghId, subtype, from, to]);

  const onFilterSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    loadMetrics();
  };

  const chartData = metrics.map((m) => ({
    time: new Date(m.timestamp).toLocaleString(),
    value: m.value,
  }));

  return (
    <Card className="p-3">
      <h1>Панель пользователя</h1>

      {loadingGH ? (
        <div className="text-center py-4"><Spinner animation="border" /></div>
      ) : errorGH ? (
        <Alert variant="danger">{errorGH}</Alert>
      ) : (
        <Form className="mb-3 d-flex flex-wrap align-items-end" onSubmit={onFilterSubmit} style={{ gap: "1rem" }}>
          <Form.Group controlId="ghId" style={{ minWidth: "150px" }}>
            <Form.Label>Теплица</Form.Label>
            <Form.Select value={ghId ?? undefined} onChange={(e) => setGhId(Number(e.target.value))}>
              {greenhouses.map((gh) => (
                <option key={gh.gh_id} value={gh.gh_id}>
                  {gh.name} (ID: {gh.gh_id})
                </option>
              ))}
            </Form.Select>
          </Form.Group>

          <Form.Group controlId="subtype" style={{ minWidth: "150px" }}>
            <Form.Label>Тип метрики</Form.Label>
            <Form.Select value={subtype} onChange={(e) => setSubtype(e.target.value)}>
              <option value="temperature">Температура</option>
              <option value="humidity">Влажность</option>
              <option value="co2">CO2</option>
              <option value="light">Освещенность</option>
            </Form.Select>
          </Form.Group>

          <Form.Group controlId="from" style={{ minWidth: "150px" }}>
            <Form.Label>Дата с</Form.Label>
            <Form.Control type="date" value={from} onChange={(e) => setFrom(e.target.value)} />
          </Form.Group>

          <Form.Group controlId="to" style={{ minWidth: "150px" }}>
            <Form.Label>Дата по</Form.Label>
            <Form.Control type="date" value={to} onChange={(e) => setTo(e.target.value)} />
          </Form.Group>

          <Button type="submit" style={{ height: "38px" }}>Обновить</Button>
        </Form>
      )}

      {loading ? (
        <div className="text-center py-4"><Spinner animation="border" /></div>
      ) : error ? (
        <Alert variant="danger">{error}</Alert>
      ) : chartData.length === 0 ? (
        <Alert variant="info">Данные не найдены</Alert>
      ) : (
        <ResponsiveContainer width="100%" height={400}>
          <LineChart
            data={chartData}
            margin={{ top: 20, right: 30, left: 0, bottom: 5 }}
          >
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="time" tick={{ fontSize: 12 }} />
            <YAxis />
            <Tooltip />
            <Legend />
            <Line
              type="monotone"
              dataKey="value"
              stroke="#8884d8"
              dot={false}
              name={subtype}
            />
          </LineChart>
        </ResponsiveContainer>
      )}
    </Card>
  );
};

export default DashboardPage;
