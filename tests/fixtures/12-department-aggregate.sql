SELECT location, COUNT(*) AS department_count
FROM departments
GROUP BY location
ORDER BY location;
