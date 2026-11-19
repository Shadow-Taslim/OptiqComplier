SELECT d.dept_name, COUNT(e.id) AS employee_count
FROM departments d
JOIN employees e ON e.dept_id = d.id
GROUP BY d.dept_name
ORDER BY d.dept_name;
