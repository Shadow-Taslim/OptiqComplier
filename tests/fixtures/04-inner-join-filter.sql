SELECT e.name, d.dept_name
FROM employees e
JOIN departments d ON e.dept_id = d.id
WHERE d.location = 'HQ'
ORDER BY e.id;
