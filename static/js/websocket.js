let websocket = null;
let isConnected = false;
let reconnectAttempts = 0;
let maxReconnectAttempts = 5;
let reconnectDelay = 3000;

function initWebSocket() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;
    
    console.log('正在连接WebSocket:', wsUrl);
    updateConnectionStatus('连接中...', 'connecting');
    
    try {
        websocket = new WebSocket(wsUrl);
        
        websocket.onopen = function() {
            isConnected = true;
            reconnectAttempts = 0;
            updateConnectionStatus('已连接', 'connected');
            console.log('WebSocket 连接已建立');
        };
        
        websocket.onclose = function(event) {
            isConnected = false;
            updateConnectionStatus('已断开', 'disconnected');
            console.log('WebSocket 连接已关闭', event);
            
            // 自动重连
            attemptReconnect();
        };
        
        websocket.onmessage = function(event) {
            console.log('收到WebSocket消息:', event.data);
            handleWebSocketMessage(event.data);
        };
        
        websocket.onerror = function(error) {
            console.error('WebSocket错误:', error);
            updateConnectionStatus('连接错误', 'error');
        };
        
    } catch (error) {
        console.error('WebSocket创建失败:', error);
        updateConnectionStatus('连接失败', 'error');
    }
}

function handleWebSocketMessage(data) {
    try {
        const message = JSON.parse(data);
        console.log('解析后的消息:', message);
        
        if (message.type === 'log_update') {
            // 处理日志更新消息
            addLogEntryToTable(message);
            updateStatCounter(message.level);
            console.log('日志更新处理完成');
        } else if (message.type === 'stats_update') {
            // 处理统计更新消息
            updateStatsDisplay(message);
            console.log('统计更新处理完成');
        } else {
            console.log('ℹ未知消息类型:', message.type);
        }
        
    } catch (error) {
        console.error('消息解析失败:', error);
        console.error('原始数据:', data);
    }
}

function attemptReconnect() {
    if (reconnectAttempts < maxReconnectAttempts) {
        reconnectAttempts++;
        updateConnectionStatus(`重连中... (${reconnectAttempts}/${maxReconnectAttempts})`, 'reconnecting');
        
        setTimeout(() => {
            console.log(`尝试重连 (${reconnectAttempts}/${maxReconnectAttempts})`);
            initWebSocket();
        }, reconnectDelay);
        
        // 递增重连延迟
        reconnectDelay = Math.min(reconnectDelay * 1.5, 30000);
    } else {
        updateConnectionStatus('连接失败', 'failed');
        console.error('超过最大重连次数，停止重连');
    }
}

function updateConnectionStatus(text, status) {
    const statusElement = document.getElementById('connection-status');
    if (statusElement) {
        statusElement.textContent = text;
        statusElement.className = `connection-status ${status}`;
    }
}

// 发送WebSocket消息的辅助函数
function sendWebSocketMessage(message) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify(message));
        return true;
    } else {
        console.warn('WebSocket未连接, 无法发送消息');
        return false;
    }
}

// 增加请求-响应模式的 WebSocket API
const pendingRequests = new Map();
let requestIdCounter = 0;

function sendWebSocketRequest(type, data = {}, timeout = 5000) {
    return new Promise((resolve, reject) => {
        if (!websocket || websocket.readyState !== WebSocket.OPEN) {
            reject(new Error('WebSocket未连接'));
            return;
        }

        const requestId = ++requestIdCounter;
        const message = {
            type: 'request',
            requestType: type,
            requestId: requestId,
            data: data,
            timestamp: new Date().toISOString()
        };

        // 设置超时
        const timeoutId = setTimeout(() => {
            if (pendingRequests.has(requestId)) {
                pendingRequests.delete(requestId);
                reject(new Error(`WebSocket请求超时: ${type}`));
            }
        }, timeout);

        // 存储请求
        pendingRequests.set(requestId, { resolve, reject, timeoutId });

        // 发送请求
        websocket.send(JSON.stringify(message));
        console.log(`WebSocket请求已发送: ${type}`, message);
    });
}

function handleWebSocketMessage(data) {
    try {
        const message = JSON.parse(data);
        console.log('解析后的消息:', message);
        
        // 处理响应消息
        if (message.type === 'response' && message.requestId) {
            const pending = pendingRequests.get(message.requestId);
            if (pending) {
                clearTimeout(pending.timeoutId);
                pendingRequests.delete(message.requestId);
                
                if (message.success) {
                    pending.resolve(message.data);
                } else {
                    pending.reject(new Error(message.error || '请求失败'));
                }
                return;
            }
        }
        
        // 处理推送消息
        if (message.type === 'log_update') {
            addLogEntryToTable(message);
            updateStatCounter(message.level);
            console.log('日志更新处理完成');
        } else if (message.type === 'stats_update') {
            updateStatsDisplay(message);
            console.log('统计更新处理完成');
        } else {
            console.log('ℹ未知消息类型:', message.type);
        }
        
    } catch (error) {
        console.error('消息解析失败:', error);
        console.error('原始数据:', data);
    }
}

// 导出WebSocket API函数
window.wsAPI = {
    getLogs: (limit = 100, offset = 0) => sendWebSocketRequest('get_logs', { limit, offset }),
    getStats: () => sendWebSocketRequest('get_stats'),
    clearLogs: () => sendWebSocketRequest('clear_logs'),
    getLogsByLevel: (level) => sendWebSocketRequest('get_logs_by_level', { level }),
    downloadLogs: (format) => sendWebSocketRequest('download_logs', { format })
};