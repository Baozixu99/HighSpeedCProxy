#!/bin/bash

# 创建持久化网络命名空间并移动网卡脚本
# 需要root权限执行，错误时立即停止

set -e  # 任何命令失败时立即退出脚本

# 默认配置
DEFAULT_NETNS="ns1"
DEFAULT_INTERFACE="veth0"
DEFAULT_IP_CIDR="192.168.1.100/24"
DEFAULT_GATEWAY="192.168.1.1"

# 解析命令行参数
NETNS_NAME=${1:-$DEFAULT_NETNS}
INTERFACE=${2:-$DEFAULT_INTERFACE}

echo "操作开始: 创建网络命名空间 [$NETNS_NAME] 并移动网卡 [$INTERFACE]"

# 检查网卡是否存在
if ! ip link show dev "$INTERFACE" > /dev/null 2>&1; then
    echo "错误: 网卡 [$INTERFACE] 不存在"
    exit 1
fi


# 创建持久化网络命名空间
echo "1. 创建命名空间 [$NETNS_NAME]"
ip netns add "$NETNS_NAME" || {
    echo "错误: 创建命名空间失败"
    exit 2
}

# 移动网卡
echo "2. 移动网卡 [$INTERFACE] 到命名空间 [$NETNS_NAME]"
ip link set "$INTERFACE" netns "$NETNS_NAME" || {
    echo "错误: 移动网卡失败"
    exit 3
}

# 配置网络
echo "3. 配置命名空间内网络"
ip netns exec "$NETNS_NAME" ip link set lo up
ip netns exec "$NETNS_NAME" ip link set "$INTERFACE" up
ip netns exec "$NETNS_NAME" ip addr add $DEFAULT_IP_CIDR dev "$INTERFACE"
ip netns exec "$NETNS_NAME" ip route add default via $DEFAULT_GATEWAY

# 显示结果
echo "操作完成:"
echo "命名空间列表:"
ip netns list
echo "网卡 [$INTERFACE] 配置:"
ip netns exec "$NETNS_NAME" ip addr show dev "$INTERFACE" 

echo "使用说明:"
echo "操作命令: ip netns exec $NETNS_NAME [command]"
echo "网络命名空间 [$NETNS_NAME] 已成功设置"
