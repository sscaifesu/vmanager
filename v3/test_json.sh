#!/bin/bash
JSON='{"data":{"memory":"4096","net0":"virtio=BC:24:11:9F:E0:D5,bridge=vmbr0","serial0":"socket","bootdisk":"scsi0","machine":"q35","boot":"order=scsi0","smbios1":"uuid=7bb32af1-8084-4d53-9110-212a795dc7ba","ide2":"local:iso/seed-111.iso,media=cdrom,size=380K","scsihw":"virtio-scsi-pci","scsi0":"vmdata-1:vm-111-disk-0,size=2252M","agent":"enabled=1","vmgenid":"87526c2a-ab5c-4bf2-ae53-032e08037c65","ostype":"l26","meta":"creation-qemu=10.0.2,ctime=1759586713","vga":"serial0","digest":"c752912ca294814e0b7c18e223b7b0e73971d0dd","cores":2,"name":"srv-web-25c201"}}'

echo "$JSON" | grep -oP '"(scsi|ide|sata|virtio)\d+":\s*"[^"]*"'
