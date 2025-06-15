// 全局变量
let currentLogs = [];
let filteredLogs = [];
let logCounter = {
    total: 0,
    info: 0,
    warning: 0,
    error: 0,
    debug: 0
};

// ==================== 日志管理 ====================

function addLogEntryToTable(logData) {
    console.log('添加日志到表格:', logData);
    
    const logsBody = document.getElementById('logs-body');
    if (!logsBody) {
        console.error('找不到日志表格体元素');
        return;
    }
    
    // 移除占位符
    const placeholder = logsBody.querySelector('.log-placeholder');
    if (placeholder) {
        placeholder.remove();
    }
    
    // 创建新的日志行
    const row = document.createElement('tr');
    row.className = `log-entry log-${logData.level.toLowerCase()}`;
    row.dataset.level = logData.level;
    row.dataset.timestamp = logData.timestamp;
    
    // 格式化时间
    const timestamp = new Date(logData.timestamp).toLocaleString('zh-CN', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
    });
    
    // 确定日志类型
    const logType = determineLogType(logData.message);
    
    row.innerHTML = `
        <td class="log-time">${timestamp}</td>
        <td class="log-level">
            <span class="level-badge level-${logData.level.toLowerCase()}">${logData.level}</span>
        </td>
        <td class="log-message">${escapeHtml(logData.message)}</td>
        <td class="log-type">${logType}</td>
    `;
    
    // 插入到表格顶部（最新的在上面）
    logsBody.insertBefore(row, logsBody.firstChild);
    
    // 添加到当前日志数组
    currentLogs.unshift(logData);
    
    // 限制日志数量
    const maxLogs = 1000;
    while (logsBody.children.length > maxLogs) {
        logsBody.removeChild(logsBody.lastChild);
        currentLogs.pop();
    }
    
    // 应用当前过滤器
    applyFiltersToRow(row);
    
    // 自动滚动
    handleAutoScroll(row);
}

function updateStatCounter(level) {
    console.log('更新统计计数:', level);
    
    // 更新内存中的计数器
    logCounter.total++;
    
    switch (level.toUpperCase()) {
        case 'INFO':
            logCounter.info++;
            break;
        case 'WARNING':
        case 'WARN':
            logCounter.warning++;
            break;
        case 'ERROR':
        case 'FATAL':
            logCounter.error++;
            break;
        case 'DEBUG':
            logCounter.debug++;
            break;
    }
    
    // 更新UI显示
    updateCounterDisplay('total-logs', logCounter.total);
    updateCounterDisplay('info-count', logCounter.info);
    updateCounterDisplay('warning-count', logCounter.warning);
    updateCounterDisplay('error-count', logCounter.error);
}

function updateCounterDisplay(elementId, value) {
    const element = document.getElementById(elementId);
    if (element) {
        // 添加动画效果
        element.style.transform = 'scale(1.1)';
        element.textContent = value;
        
        setTimeout(() => {
            element.style.transform = 'scale(1)';
        }, 200);
    }
}

// ==================== 过滤和搜索 ====================

function filterLogs() {
    const searchTerm = document.getElementById('search-input').value.toLowerCase();
    const levelFilter = document.getElementById('level-filter').value;
    const typeFilter = document.getElementById('type-filter').value;
    
    console.log('应用过滤器:', { searchTerm, levelFilter, typeFilter });
    
    const rows = document.querySelectorAll('#logs-body .log-entry');
    let visibleCount = 0;
    
    rows.forEach(row => {
        const level = row.dataset.level;
        const message = row.querySelector('.log-message').textContent.toLowerCase();
        const type = row.querySelector('.log-type').textContent.toLowerCase();
        
        let shouldShow = true;
        
        // 搜索过滤
        if (searchTerm && !message.includes(searchTerm)) {
            shouldShow = false;
        }
        
        // 级别过滤
        if (levelFilter && level !== levelFilter) {
            shouldShow = false;
        }
        
        // 类型过滤
        if (typeFilter && !type.includes(typeFilter)) {
            shouldShow = false;
        }
        
        row.style.display = shouldShow ? '' : 'none';
        if (shouldShow) visibleCount++;
    });
    
    console.log(`显示 ${visibleCount} 条日志`);
}

function applyFiltersToRow(row) {
    const searchTerm = document.getElementById('search-input').value.toLowerCase();
    const levelFilter = document.getElementById('level-filter').value;
    const typeFilter = document.getElementById('type-filter').value;
    
    if (!searchTerm && !levelFilter && !typeFilter) {
        return; // 没有过滤器, 显示所有
    }
    
    const level = row.dataset.level;
    const message = row.querySelector('.log-message').textContent.toLowerCase();
    const type = row.querySelector('.log-type').textContent.toLowerCase();
    
    let shouldShow = true;
    
    if (searchTerm && !message.includes(searchTerm)) shouldShow = false;
    if (levelFilter && level !== levelFilter) shouldShow = false;
    if (typeFilter && !type.includes(typeFilter)) shouldShow = false;
    
    row.style.display = shouldShow ? '' : 'none';
}

function clearFilters() {
    document.getElementById('search-input').value = '';
    document.getElementById('level-filter').value = '';
    document.getElementById('type-filter').value = '';
    filterLogs();
}

// ==================== 工具函数 ====================

function determineLogType(message) {
    const msg = message.toLowerCase();
    
    if (msg.includes('login') || msg.includes('auth') || msg.includes('password')) {
        return '认证';
    } else if (msg.includes('database') || msg.includes('sql') || msg.includes('mysql')) {
        return '数据库';
    } else if (msg.includes('network') || msg.includes('connection') || msg.includes('socket')) {
        return '网络';
    } else if (msg.includes('system') || msg.includes('server') || msg.includes('process')) {
        return '系统';
    } else {
        return '其他';
    }
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

function handleAutoScroll(newRow) {
    const autoScroll = document.getElementById('auto-scroll');
    if (autoScroll && autoScroll.checked) {
        newRow.scrollIntoView({ 
            behavior: 'smooth', 
            block: 'nearest' 
        });
    }
}

// ==================== 工具函数 ====================

function determineLogType(message) {
    const msg = message.toLowerCase();
    
    if (msg.includes('login') || msg.includes('auth') || msg.includes('password')) {
        return '认证';
    } else if (msg.includes('database') || msg.includes('sql') || msg.includes('mysql')) {
        return '数据库';
    } else if (msg.includes('network') || msg.includes('connection') || msg.includes('socket')) {
        return '网络';
    } else if (msg.includes('system') || msg.includes('server') || msg.includes('process')) {
        return '系统';
    } else {
        return '其他';
    }
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

function handleAutoScroll(newRow) {
    const autoScroll = document.getElementById('auto-scroll');
    if (autoScroll && autoScroll.checked) {
        newRow.scrollIntoView({ 
            behavior: 'smooth', 
            block: 'nearest' 
        });
    }
}

// ==================== 统计显示更新 ====================

function updateStatsDisplay(stats) {
    console.log('更新统计显示:', stats);
    
    try {
        // 更新总日志数
        if (stats.totalLogs !== undefined) {
            updateCounterDisplay('total-logs', stats.totalLogs);
            logCounter.total = stats.totalLogs;
        }
        
        // 更新警告数
        if (stats.warningCount !== undefined) {
            updateCounterDisplay('warning-count', stats.warningCount);
            logCounter.warning = stats.warningCount;
        }
        
        // 更新错误数
        if (stats.errorCount !== undefined) {
            updateCounterDisplay('error-count', stats.errorCount);
            logCounter.error = stats.errorCount;
        }
        
        // 更新信息数
        if (stats.infoCount !== undefined) {
            updateCounterDisplay('info-count', stats.infoCount);
            logCounter.info = stats.infoCount;
        }
        
        // 更新调试数（如果存在）
        if (stats.debugCount !== undefined) {
            logCounter.debug = stats.debugCount;
        }
        
        // 更新客户端连接数
        if (stats.clientCount !== undefined) {
            updateCounterDisplay('client-count', stats.clientCount);
        }
        
        console.log('统计显示更新完成');
        
    } catch (error) {
        console.error('更新统计显示失败:', error);
    }
}

// ==================== API 调用 ====================

async function fetchLogs(limit = 100) {
    try {
        console.log('通过WebSocket获取历史日志...');
        
        // 优先使用WebSocket，降级到HTTP
        let logs;
        if (isConnected && window.wsAPI) {
            try {
                logs = await window.wsAPI.getLogs(limit);
                console.log(`WebSocket获取到 ${logs.length} 条历史日志`);
            } catch (wsError) {
                console.warn('WebSocket获取失败，使用HTTP备用:', wsError);
                logs = await fetchLogsHTTP(limit);
            }
        } else {
            logs = await fetchLogsHTTP(limit);
        }
        
        // 清空现有日志
        const logsBody = document.getElementById('logs-body');
        logsBody.innerHTML = '';
        currentLogs = [];
        
        // 重置计数器
        logCounter = { total: 0, info: 0, warning: 0, error: 0, debug: 0 };
        
        // 添加日志（按时间倒序）
        logs.reverse().forEach(log => {
            addLogEntryToTable(log);
        });
        
    } catch (error) {
        console.error('获取日志失败:', error);
        showNotification('获取日志失败: ' + error.message, 'error');
    }
}

// HTTP备用方案
async function fetchLogsHTTP(limit = 100) {
    const response = await fetch(`/api/logs?limit=${limit}`);
    if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }
    return await response.json();
}

async function fetchStats() {
    try {
        console.log('通过WebSocket获取统计信息...');
        
        let stats;
        if (isConnected && window.wsAPI) {
            try {
                stats = await window.wsAPI.getStats();
                console.log('WebSocket统计信息:', stats);
            } catch (wsError) {
                console.warn('WebSocket获取统计失败，使用HTTP备用:', wsError);
                stats = await fetchStatsHTTP();
            }
        } else {
            stats = await fetchStatsHTTP();
        }
        
        updateStatsDisplay(stats);
        
    } catch (error) {
        console.error('获取统计信息失败:', error);
    }
}

// HTTP备用方案
async function fetchStatsHTTP() {
    const response = await fetch('/api/stats');
    if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }
    return await response.json();
}

// ==================== 用户操作 ====================

function refreshLogs() {
    console.log('刷新日志...');
    showNotification('正在刷新日志...', 'info');
    fetchLogs().then(() => {
        showNotification('日志刷新完成', 'success');
    });
}

async function clearLogs() {
    if (confirm('确定要清空所有显示的日志吗？这不会影响服务器上的日志数据。')) {
        try {
            // 优先使用WebSocket
            if (isConnected && window.wsAPI) {
                await window.wsAPI.clearLogs();
                showNotification('服务器日志已清空', 'success');
            }
            
            // 清空本地显示
            const logsBody = document.getElementById('logs-body');
            logsBody.innerHTML = '<tr class="log-placeholder"><td colspan="4">日志已清空</td></tr>';
            currentLogs = [];
            
            // 重置计数器显示
            document.getElementById('total-logs').textContent = '0';
            document.getElementById('warning-count').textContent = '0';
            document.getElementById('error-count').textContent = '0';
            document.getElementById('info-count').textContent = '0';
            
            showNotification('本地日志已清空', 'success');
        } catch (error) {
            console.error('清空日志失败:', error);
            showNotification('清空日志失败: ' + error.message, 'error');
        }
    }
}

async function downloadLogFile(format) {
    try {
        console.log(`通过WebSocket下载 ${format} 格式日志...`);
        showNotification(`正在准备 ${format.toUpperCase()} 格式下载...`, 'info');
        
        // 尝试WebSocket下载
        if (isConnected && window.wsAPI) {
            try {
                const downloadData = await window.wsAPI.downloadLogs(format);
                
                // 创建下载链接
                const blob = new Blob([downloadData.content], { type: downloadData.mimeType });
                const url = window.URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = downloadData.filename || `logs_${new Date().toISOString().split('T')[0]}.${format}`;
                document.body.appendChild(a);
                a.click();
                window.URL.revokeObjectURL(url);
                document.body.removeChild(a);
                
                showNotification(`${format.toUpperCase()} 文件下载完成`, 'success');
                return;
            } catch (wsError) {
                console.warn('WebSocket下载失败，使用HTTP备用:', wsError);
            }
        }
        
        // HTTP备用方案
        const response = await fetch(`/api/download-log?type=${format}`);
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        const blob = await response.blob();
        const url = window.URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `logs_${new Date().toISOString().split('T')[0]}.${format}`;
        document.body.appendChild(a);
        a.click();
        window.URL.revokeObjectURL(url);
        document.body.removeChild(a);
        
        showNotification(`${format.toUpperCase()} 文件下载完成`, 'success');
        
    } catch (error) {
        console.error('下载失败:', error);
        showNotification('下载失败: ' + error.message, 'error');
    }
}

// 添加实时日志级别过滤
async function filterLogsByLevel(level) {
    if (isConnected && window.wsAPI) {
        try {
            const logs = await window.wsAPI.getLogsByLevel(level);
            console.log(`获取到 ${logs.length} 条 ${level} 级别日志`);
            
            // 更新显示
            const logsBody = document.getElementById('logs-body');
            logsBody.innerHTML = '';
            currentLogs = [];
            
            logs.reverse().forEach(log => {
                addLogEntryToTable(log);
            });
            
            showNotification(`已过滤显示 ${level} 级别日志`, 'info');
        } catch (error) {
            console.error('过滤日志失败:', error);
            showNotification('过滤日志失败: ' + error.message, 'error');
        }
    }
}

// ==================== 通知系统 ====================

function showNotification(message, type = 'info') {
    // 创建通知元素
    const notification = document.createElement('div');
    notification.className = `notification notification-${type}`;
    notification.textContent = message;
    
    // 添加样式
    notification.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        padding: 12px 20px;
        border-radius: 4px;
        color: white;
        font-weight: bold;
        z-index: 10000;
        transition: all 0.3s ease;
        background-color: ${type === 'success' ? '#28a745' : 
                          type === 'error' ? '#dc3545' : 
                          type === 'warning' ? '#ffc107' : '#17a2b8'};
    `;
    
    document.body.appendChild(notification);
    
    // 自动移除
    setTimeout(() => {
        notification.style.opacity = '0';
        setTimeout(() => {
            document.body.removeChild(notification);
        }, 300);
    }, 3000);
}

// ==================== 页面可见性处理 ====================

// 当页面重新获得焦点时刷新数据
document.addEventListener('visibilitychange', function() {
    if (!document.hidden && isConnected) {
        console.log('页面重新可见, 刷新数据...');
        fetchStats();
    }
});