#!/bin/bash
# 测试 qemu-guest-agent 网络接口 API

VMID=25120
source ~/.vmanager.conf 2>/dev/null || {
    HOST="10.10.10.4"
    PORT="8006"
    NODE="pve"
    TOKEN_ID="root@pam!vmanager"
    TOKEN_SECRET="c09758b1-9c18-4b35-85ea-81a6d415734f"
}

echo "测试 VM $VMID 的网络接口 API..."
echo ""

curl -s -k "https://$HOST:$PORT/api2/json/nodes/$NODE/qemu/$VMID/agent/network-get-interfaces" \
    -H "Authorization: PVEAPIToken=$TOKEN_ID=$TOKEN_SECRET" | python3 -m json.tool

