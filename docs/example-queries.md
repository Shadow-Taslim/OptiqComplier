# Supported Example SQL Queries

These 12 queries define the initial supported SQL behavior and serve as acceptance fixtures for later compiler and execution tests. They use the `sample-v1` CSV data in `data/`.

## Conventions

- SQL keywords are case-insensitive; table and column names use the spelling in the schemas.
- Each request contains one statement. A trailing semicolon is optional.
- String literals use single quotes. Table aliases and qualified names such as `e.name` are supported.
- `ORDER BY` determines row order where shown. Without it, insertion order is the default for these fixtures.
- Decimal values are displayed with two digits after the decimal point. `AVG` values are shown rounded to two decimals.
- Initial expression support includes comparison operators, arithmetic on numeric constants, `AND`, and `OR`.
- Initial aggregates are `COUNT`, `AVG`, `SUM`, `MIN`, and `MAX`.

## 1. Ordered projection

```sql
SELECT name, salary
FROM employees
ORDER BY salary DESC;
```

Expected result — 8 rows:

| name | salary |
| --- | ---: |
| Farid Hassan | 105000.00 |
| Aisha Rahman | 95000.00 |
| Chloe Davis | 78000.00 |
| Elena Fox | 71000.00 |
| Grace Kim | 68000.00 |
| Ben Carter | 62000.00 |
| Daniel Evans | 55000.00 |
| Henry Lee | 47000.00 |

## 2. Equality filter

```sql
SELECT id, name
FROM employees
WHERE dept_id = 1
ORDER BY id;
```

Expected result — 3 rows:

| id | name |
| ---: | --- |
| 1 | Aisha Rahman |
| 3 | Chloe Davis |
| 6 | Farid Hassan |

## 3. Constant arithmetic in a predicate

```sql
SELECT name, salary
FROM employees
WHERE salary > 50000 + 20000
ORDER BY salary DESC;
```

The predicate folds to `salary > 70000`. Expected result — 4 rows:

| name | salary |
| --- | ---: |
| Farid Hassan | 105000.00 |
| Aisha Rahman | 95000.00 |
| Chloe Davis | 78000.00 |
| Elena Fox | 71000.00 |

## 4. Inner join with a pushed-down filter

```sql
SELECT e.name, d.dept_name
FROM employees e
JOIN departments d ON e.dept_id = d.id
WHERE d.location = 'HQ'
ORDER BY e.id;
```

Expected result — 5 rows:

| name | dept_name |
| --- | --- |
| Aisha Rahman | Engineering |
| Chloe Davis | Engineering |
| Daniel Evans | Sales |
| Farid Hassan | Engineering |
| Henry Lee | Sales |

## 5. Grouped count

```sql
SELECT d.dept_name, COUNT(e.id) AS employee_count
FROM departments d
JOIN employees e ON e.dept_id = d.id
GROUP BY d.dept_name
ORDER BY d.dept_name;
```

Expected result — 4 rows:

| dept_name | employee_count |
| --- | ---: |
| Engineering | 3 |
| Human Resources | 1 |
| Marketing | 2 |
| Sales | 2 |

## 6. Grouped average

```sql
SELECT dept_id, AVG(salary) AS avg_salary
FROM employees
GROUP BY dept_id
ORDER BY dept_id;
```

Expected result — 4 rows:

| dept_id | avg_salary |
| ---: | ---: |
| 1 | 92666.67 |
| 2 | 66500.00 |
| 3 | 51000.00 |
| 4 | 68000.00 |

## 7. Filter, join, group, and sum

```sql
SELECT d.dept_name, SUM(e.salary) AS total_salary
FROM employees e
JOIN departments d ON e.dept_id = d.id
WHERE d.location = 'HQ'
GROUP BY d.dept_name
ORDER BY d.dept_name;
```

Expected result — 2 rows:

| dept_name | total_salary |
| --- | ---: |
| Engineering | 278000.00 |
| Sales | 102000.00 |

## 8. Conjunctive range filter

```sql
SELECT name, salary
FROM employees
WHERE salary >= 60000 AND salary <= 80000
ORDER BY name;
```

Expected result — 4 rows:

| name | salary |
| --- | ---: |
| Ben Carter | 62000.00 |
| Chloe Davis | 78000.00 |
| Elena Fox | 71000.00 |
| Grace Kim | 68000.00 |

## 9. Disjunctive filter

```sql
SELECT name, dept_id
FROM employees
WHERE dept_id = 2 OR dept_id = 4
ORDER BY id;
```

Expected result — 3 rows:

| name | dept_id |
| --- | ---: |
| Ben Carter | 2 |
| Elena Fox | 2 |
| Grace Kim | 4 |

## 10. Date ordering with limit

```sql
SELECT name, hire_date
FROM employees
ORDER BY hire_date DESC
LIMIT 3;
```

Expected result — 3 rows:

| name | hire_date |
| --- | --- |
| Grace Kim | 2025-01-06 |
| Henry Lee | 2024-06-25 |
| Chloe Davis | 2024-03-10 |

## 11. Global aggregates

```sql
SELECT COUNT(*) AS employee_count,
       MIN(salary) AS min_salary,
       MAX(salary) AS max_salary
FROM employees;
```

Expected result — 1 row:

| employee_count | min_salary | max_salary |
| ---: | ---: | ---: |
| 8 | 47000.00 | 105000.00 |

## 12. Aggregate over the second table

```sql
SELECT location, COUNT(*) AS department_count
FROM departments
GROUP BY location
ORDER BY location;
```

Expected result — 3 rows:

| location | department_count |
| --- | ---: |
| HQ | 2 |
| NY | 1 |
| Remote | 1 |
