let websocket;
let isConnected = false;
const connectionStatusElement = document.getElementById('connection-status');

function initWebSocket() {
    // 使用当前主机和端口，自动构建 WebSocket URL
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;
    
    websocket = new WebSocket(wsUrl);
    
    websocket.onopen = function() {
        isConnected = true;
        connectionStatusElement.textContent = '已连接';
        connectionStatusElement.className = 'connection-status connected';
        console.log('WebSocket 连接已建立');
    };
    
    websocket.onclose = function() {
        isConnected = false;
        connectionStatusElement.textContent = '已断开';
        connectionStatusElement.className = 'connection-status disconnected';
        console.log('WebSocket 连接已关闭');
        
        // 尝试在断开后重新连接
        setTimeout(initWebSocket, 3000);
    };
    
    websocket.onmessage = function(event) {
        try {
            const log = JSON.parse(event.data);
            addLogEntry(log);
            updateStatCounter(log.level);
        } catch (e) {
            console.error('Error processing WebSocket message:', e);
        }
    };
    
    websocket.onerror = function(error) {
        console.error('WebSocket 错误:', error);
    };
}

function addLogEntry(log) {
    const logsBody = document.getElementById('logs-body');
    
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
    
    // 添加到表格最前面
    if (logsBody.firstChild) {
        logsBody.insertBefore(row, logsBody.firstChild);
    } else {
        logsBody.appendChild(row);
    }
    
    // 限制显示的日志数量
    const maxLogs = 100;
    while (logsBody.children.length > maxLogs) {
        logsBody.removeChild(logsBody.lastChild);
    }
    
    // 应用当前筛选条件
    filterLogs();
}

function updateStatCounter(level) {
    // 更新总日志计数
    const totalElement = document.getElementById('total-logs');
    totalElement.textContent = Number(totalElement.textContent) + 1;
    
    // 更新特定级别的计数
    if (level === 'WARNING') {
        const warningElement = document.getElementById('warning-logs');
        warningElement.textContent = Number(warningElement.textContent) + 1;
    } else if (level === 'ERROR' || level === 'FATAL') {
        const errorElement = document.getElementById('error-logs');
        errorElement.textContent = Number(errorElement.textContent) + 1;
    }
}