/**
 * コードの引用元: https://github.com/brenns10/sos
 */

#include "virtio.h"
#include "printf.h"
#include "mm.h"
#include "pmalloc.h"

struct virtio_net netdev;

struct virtio_cap net_caps[] = {
        {"VIRTIO_NET_F_CSUM", 0, true,
         "Device handles packets with partial checksum. This “checksum "
         "offload” is a common feature on modern network cards."},
        {"VIRTIO_NET_F_GUEST_CSUM", 1, false,
         "Driver handles packets with partial checksum."},
        {"VIRTIO_NET_F_CTRL_GUEST_OFFLOADS", 2, false,
         "Control channel offloads reconfiguration support."},
        {"VIRTIO_NET_F_MAC", 5, true, "Device has given MAC address."},
        {"VIRTIO_NET_F_GUEST_TSO4", 7, false, "Driver can receive TSOv4."},
        {"VIRTIO_NET_F_GUEST_TSO6", 8, false, "Driver can receive TSOv6."},
        {"VIRTIO_NET_F_GUEST_ECN", 9, false,
         "Driver can receive TSO with ECN."},
        {"VIRTIO_NET_F_GUEST_UFO", 10, false, "Driver can receive UFO."},
        {"VIRTIO_NET_F_HOST_TSO4", 11, false, "Device can receive TSOv4."},
        {"VIRTIO_NET_F_HOST_TSO6", 12, false, "Device can receive TSOv6."},
        {"VIRTIO_NET_F_HOST_ECN", 13, false,
         "Device can receive TSO with ECN."},
        {"VIRTIO_NET_F_HOST_UFO", 14, false, "Device can receive UFO."},
        {"VIRTIO_NET_F_MRG_RXBUF", 15, false,
         "Driver can merge receive buffers."},
        {"VIRTIO_NET_F_STATUS", 16, true,
         "Configuration status field is available."},
        {"VIRTIO_NET_F_CTRL_VQ", 17, false, "Control channel is available."},
        {"VIRTIO_NET_F_CTRL_RX", 18, false,
         "Control channel RX mode support."},
        {"VIRTIO_NET_F_CTRL_VLAN", 19, false,
         "Control channel VLAN filtering."},
        {"VIRTIO_NET_F_GUEST_ANNOUNCE", 21, false,
         "Driver can send gratuitous packets."},
        {"VIRTIO_NET_F_MQ", 22, false,
         "Device supports multiqueue with automatic receive steering."},
        {"VIRTIO_NET_F_CTRL_MAC_ADDR", 23, false,
         "Set MAC address through control channel"},
        VIRTIO_INDP_CAPS
};


void print_net_binary(uint8_t* buffer, uint32_t len){

    for(int i = 0; i < len; ++i){
        if(0 <= buffer[i] && buffer[i] <= 0x0f){
            printf("0%x", buffer[i]);
        }else{
            printf("%x", buffer[i]);
        }
    }
    printf("\n");

}

void add_packets_to_virtqueue(int n, struct virtqueue* virtq){
    uint32_t d;
    for(int i = 0; i < n; i++){
        d = virtq_alloc_desc(virtq, pmalloc());
        virtq->ring.desc[d].len = sizeof(struct virtio_net_hdr) + 2000;
        virtq->ring.desc[d].flags = VRING_DESC_F_WRITE;
        virtq->ring.avail->ring[virtq->ring.avail->idx + i] = d;
    }
    mb();
    virtq->ring.avail->idx += n;
}

void virtio_net_send(struct virtio_net* dev, uint8_t* buffer, uint32_t len){
    uint32_t d;

    struct virtio_net_hdr* hdr = (struct virtio_net_hdr*) pmalloc();

    hdr->flags = 0;
    hdr->gso_type = VIRTIO_NET_HDR_GSO_NONE;
    hdr->hdr_len = 0;
    hdr->gso_size = 0;
    hdr->csum_start = 0;
    hdr->csum_offset = 0;
    hdr->num_buffers = 1;

    d = virtq_alloc_desc(dev->tx, (void*) hdr);

    dev->tx->ring.desc[d].len = sizeof(struct virtio_net_hdr) + len;
    dev->tx->ring.desc[d].flags = 0;

    for(int i = 0; i < len; ++i){
        (((uint8_t*) dev->tx->ring.desc[d].addr) + sizeof(struct virtio_net_hdr))[i] = buffer[i];
    }


    dev->tx->ring.avail->ring[dev->tx->ring.avail->idx] = d;
    mb();
    dev->tx->ring.avail->idx += 1;
    mb();
    WRITE32(dev->regs->QueueNotify, VIRTIO_NET_Q_TX);
}

void virtio_net_intr_tx(struct virtio_net* dev, uint32_t idx){
    printf("rcv-idx: %d\n", idx);
    uint32_t d = dev->rx->ring.used->ring[idx].id;

    printf("rcv-d: %d\n", d);

    uint32_t len = dev->rx->ring.used->ring[idx].len - sizeof(struct virtio_net_hdr);

    //struct virtio_net_hdr* hdr = (struct virtio_net_hdr*) dev->rx->desc_virt[d];

    printf("received! len %d\n", len);

    print_net_binary((uint8_t*) (dev->rx->ring.desc[d].addr) + sizeof(struct virtio_net_hdr), len);

    dev->rx->ring.avail->ring[dev->rx->ring.avail->idx] = d;
    mb();
    dev->rx->ring.avail->idx = wrap(dev->rx->ring.avail->idx + 1, dev->rx->len);

}

void virtio_net_intr_rx(struct virtio_net* dev, uint32_t idx){
    uint32_t d = dev->tx->ring.avail->ring[idx];

    printf("tx intr!!\n");

    pfree((void*) dev->tx->ring.desc[d].addr);
    virtq_free_desc(dev->tx, d);
}

void virtio_net_intr(){
    uint32_t i;
    struct virtio_net* dev = &netdev;
    uint32_t stat = READ32(dev->regs->InterruptStatus);
    printf("virtio_net intr %d\n", stat);
    WRITE32(dev->regs->InterruptACK, stat);
    printf("1: dev->rx->seen_used: %d\n", dev->rx->seen_used);
    printf("1: dev->rx->used->idx: %d\n", dev->rx->ring.used->idx);

    dev->rx->ring.used->idx = wrap(dev->rx->ring.used->idx, dev->rx->len);
    for(i = dev->rx->seen_used; i != dev->rx->ring.used->idx; i = wrap(i + 1, dev->rx->len)){
        virtio_net_intr_tx(dev, i);
    }

    printf("2: dev->rx->seen_used: %d\n", dev->rx->seen_used);
    printf("2: dev->rx->used->idx: %d\n", dev->rx->ring.used->idx);

    dev->rx->seen_used = dev->rx->ring.used->idx;


    dev->tx->ring.used->idx = wrap(dev->tx->ring.used->idx, dev->tx->len);
    for(i = dev->tx->seen_used; i != dev->tx->ring.used->idx; i = wrap(i + 1, dev->tx->len)){
        virtio_net_intr_rx(dev, i);
    }
    dev->tx->seen_used = dev->tx->ring.used->idx;

}

int virtio_net_init(virtio_regs* regs, uint32_t intid){
    volatile struct virtio_net_config* cfg = (struct virtio_net_config*) regs->Config;
    virtio_check_capabilities(regs, net_caps, nelem(net_caps), "virtio-net");

    //WRITE32(regs->DriverFeaturesSel, 1);
    //mb();
    //WRITE32(regs->DriverFeatures, 65569);


    WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_FEATURES_OK);
    mb();
    if(!(regs->Status & VIRTIO_STATUS_FEATURES_OK)){
        printf("error: virtio-net did not accept our features\n");
        return -1;
    }

    netdev.regs = regs;
    netdev.tx = virtq_create(8);
    netdev.rx = virtq_create(8);

    // maybe_init_nethdr_slab();
    add_packets_to_virtqueue(8, netdev.rx);

    virtq_add_to_device(regs, netdev.rx, VIRTIO_NET_Q_RX);
    virtq_add_to_device(regs, netdev.tx, VIRTIO_NET_Q_TX);
    printf("r4\n");

    WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_DRIVER_OK);
    mb();

    printf("virtio-net  ready!\n");


    return 0;

}

void virtio_net_send_test(){
    uint8_t buf[1000];

    for(int i = 0; i < 500; ++i){
        buf[i] = 0x00;
    }

    for(int i = 0; i < 0xff; ++i){
        buf[i] = 0xff - i;
    }

    virtio_net_send(&netdev, buf, 500);
}
