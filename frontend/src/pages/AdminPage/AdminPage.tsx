// src/pages/AdminPage.tsx
import React from "react";
import { Container, Row, Col, Table, Card } from "react-bootstrap";
import { JSX } from "react/jsx-runtime";
import UsersTable from "./UsersTable";
import GreenhousesTable from "./GreenhouseTable";

const AdminPage: React.FC = () => {
  // Пока статичные данные (можно заменить на useEffect + fetch позже)

  const greenhouses = [
    { id: 1, name: "Теплица #1", location: "Север" },
    { id: 2, name: "Теплица #2", location: "Юг" },
  ];

  const components = [
    { id: 1, name: "Датчик температуры", type: "Sensor" },
    { id: 2, name: "Полив", type: "Actuator" },
  ];

  const rules = [
    { id: 1, name: "Автополив при < 20°C", active: true },
    { id: 2, name: "Тревога при > 35°C", active: false },
  ];

  const renderTable = (title: string, headers: string[], rows: JSX.Element[]) => (
    <Card className="mb-4 shadow-sm">
      <Card.Body>
        <Card.Title>{title}</Card.Title>
        <Table striped bordered hover responsive>
          <thead>
            <tr>{headers.map((h, idx) => <th key={idx}>{h}</th>)}</tr>
          </thead>
          <tbody>{rows}</tbody>
        </Table>
      </Card.Body>
    </Card>
  );

  return (
    <Container className="py-4">
      <h1 className="mb-4">Админ-панель</h1>

      {/* Users */}
      <UsersTable />

      {/* Greenhouses */}
      <GreenhousesTable />

      {/* Components */}
      {renderTable(
        "Компоненты",
        ["ID", "Название", "Тип"],
        components.map((comp) => (
          <tr key={comp.id}>
            <td>{comp.id}</td>
            <td>{comp.name}</td>
            <td>{comp.type}</td>
          </tr>
        ))
      )}

      {/* Rules */}
      {renderTable(
        "Правила",
        ["ID", "Название", "Активно"],
        rules.map((rule) => (
          <tr key={rule.id}>
            <td>{rule.id}</td>
            <td>{rule.name}</td>
            <td>{rule.active ? "Да" : "Нет"}</td>
          </tr>
        ))
      )}
    </Container>
  );
};

export default AdminPage;
