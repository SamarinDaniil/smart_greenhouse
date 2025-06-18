// src/pages/AdminPage.tsx
import React from "react";
import { Container, Card } from "react-bootstrap";
import UsersTable from "./UsersTable";
import GreenhousesTable from "./GreenhouseTable";
import ComponentsTable from "./ComponentPage";
import RuleTable from "./RuleTable";

const AdminPage: React.FC = () => {
  return (
    <Container className="app-container py-4">
      <h1 className="mb-4">Админ-панель</h1>

      {/* Раздел пользователей */}
      <Card className="mb-5 p-sm bg-white rounded-3 shadow-sm">
        <Card.Body>
          <UsersTable />
        </Card.Body>
      </Card>

      {/* Раздел теплиц */}
      <Card className="mb-5 p-sm bg-white rounded-3 shadow-sm">
        <Card.Body>
          <GreenhousesTable />
        </Card.Body>
      </Card>

      {/* Раздел компонентов */}
      <Card className="mb-5 p-sm bg-white rounded-3 shadow-sm">
        <Card.Body>
          <ComponentsTable />
        </Card.Body>
      </Card>

      {/* Раздел правил */}
      <Card className="mb-5 p-sm bg-white rounded-3 shadow-sm">
        <Card.Body>
          <RuleTable />
        </Card.Body>
      </Card>
    </Container>
  );
};

export default AdminPage;
