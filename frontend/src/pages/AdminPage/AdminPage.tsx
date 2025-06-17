// src/pages/AdminPage.tsx
import React from "react";
import { Container, Row, Col, Table, Card } from "react-bootstrap";
import { JSX } from "react/jsx-runtime";
import UsersTable from "./UsersTable";
import GreenhousesTable from "./GreenhouseTable";
import ComponentsTable from "./ComponentPage";
import RuleTable from "./RuleTable";

const AdminPage: React.FC = () => {

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
      <ComponentsTable />

      {/* Rules */}
      <RuleTable />

    </Container>
  );
};

export default AdminPage;
