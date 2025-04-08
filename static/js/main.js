// 初始化脚本
document.addEventListener('DOMContentLoaded', function() {
    // 初始化 WebSocket 连接
    initWebSocket();
    
    // 加载初始日志数据
    fetchLogs();
    
    // 加载统计数据
    fetchStats();
    
    // 设置定时刷新统计数据
    setInterval(fetchStats, 5000);
    
    // 添加事件监听
    document.getElementById('refresh-logs').addEventListener('click', fetchLogs);
    document.getElementById('clear-filters').addEventListener('click', clearFilters);
    
    document.getElementById('search-input').addEventListener('input', filterLogs);
    document.getElementById('level-filter').addEventListener('change', filterLogs);
    document.getElementById('type-filter').addEventListener('change', filterLogs);
});

// 获取日志数据
function fetchLogs() {
    fetch('/api/logs')
        .then(response => response.json())
        .then(logs => {
            displayLogs(logs);
        })
        .catch(error => {
            console.error('Error fetching logs:', error);
        });
}

// 获取统计数据
function fetchStats() {
    fetch('/api/stats')
        .then(response => response.json())
        .then(stats => {
            document.getElementById('total-logs').textContent = stats.totalLogs;
            document.getElementById('warning-logs').textContent = stats.warningCount;
            document.getElementById('error-logs').textContent = stats.errorCount;
            document.getElementById('client-count').textContent = stats.clientCount;
        })
        .catch(error => {
            console.error('Error fetching stats:', error);
        });
}

// 显示日志数据
function displayLogs(logs) {
    const logsBody = document.getElementById('logs-body');
    logsBody.innerHTML = '';
    
    logs.forEach(log => {
        const row = document.createElement('tr');
        row.className = 'log-entry';
        row.dataset.level = log.level;
        row.dataset.type = getLogType(log.message);
        
        row.innerHTML = `
            <td>${log.timestamp}</td>
            <td><div class="log-level ${log.level}">${log.level}</div></td>
            <td>${log.message}</td>
            <td>${getLogType(log.message)}</td>
        `;
        
        logsBody.appendChild(row);
    });
    
    // 应用当前筛选条件
    filterLogs();
}

// 获取日志类型
function getLogType(message) {
    if (message.includes('User') || message.includes('login') || message.includes('Password')) {
        return 'auth';
    } else if (message.includes('Database') || message.includes('Table') || message.includes('Transaction')) {
        return 'database';
    } else if (message.includes('Request') || message.includes('Response') || message.includes('Connection')) {
        return 'network';
    } else {
        return 'system';
    }
}

// 筛选日志
function filterLogs() {
    const searchText = document.getElementById('search-input').value.toLowerCase();
    const levelFilter = document.getElementById('level-filter').value;
    const typeFilter = document.getElementById('type-filter').value;
    
    const rows = document.querySelectorAll('.log-entry');
    
    rows.forEach(row => {
        const level = row.dataset.level;
        const type = row.dataset.type;
        const text = row.textContent.toLowerCase();
        
        const matchesSearch = searchText === '' || text.includes(searchText);
        const matchesLevel = levelFilter === 'all' || level === levelFilter;
        const matchesType = typeFilter === 'all' || type === typeFilter;
        
        if (matchesSearch && matchesLevel && matchesType) {
            row.style.display = '';
        } else {
            row.style.display = 'none';
        }
    });
}

// 清除所有筛选条件
function clearFilters() {
    document.getElementById('search-input').value = '';
    document.getElementById('level-filter').value = 'all';
    document.getElementById('type-filter').value = 'all';
    
    filterLogs();
}