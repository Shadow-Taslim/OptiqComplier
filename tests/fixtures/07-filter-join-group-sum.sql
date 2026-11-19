SELECT d.dept_name, SUM(e.salary) AS total_salary
FROM employees e
JOIN departments d ON e.dept_id = d.id
WHERE d.location = 'HQ'
GROUP BY d.dept_name
ORDER BY d.dept_name;
