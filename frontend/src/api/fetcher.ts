// src/api/fetcher.ts
const API_URL = "http://localhost:8080";

export async function fetcher<T>(
  input: string,
  options: RequestInit = {}
): Promise<T> {
  const token = localStorage.getItem("token");
  const headers = {
    "Content-Type": "application/json",
    ...(token && { Authorization: `Bearer ${token}` }),
    ...options.headers,
  };
  console.log("a")
  const res = await fetch(`${API_URL}${input}`, { ...options, headers });
  console.log("b")
  if (res.status === 401) {
    localStorage.removeItem("token");
    window.location.href = "/login";
  }

  if (!res.ok) {
    throw new Error(await res.text());
  }

  return res.json();
}
