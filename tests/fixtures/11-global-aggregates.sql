SELECT COUNT(*) AS employee_count,
       MIN(salary) AS min_salary,
       MAX(salary) AS max_salary
FROM employees;
