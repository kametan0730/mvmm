/**
 * コードの引用元: https://github.com/brenns10/sos
 */

#ifndef MVMM_IRQ_VIRTIO_H
#define MVMM_IRQ_VIRTIO_H

#include "types.h"
#include "virtio_vring.h"

typedef _Bool bool;
typedef unsigned long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

#define nelem(x) (sizeof(x) / sizeof(x[0]))


#define VIRTIO_NET_IRQ  48

#define PAGE_SIZE PAGESIZE

#define mb()                __asm__ __volatile__("dsb sy")
#define isb()               __asm__ __volatile__("isb")

#define WRITE32(_reg, _val)                                                    \
    do {                                                                   \
        register uint32_t __myval__ = (_val);                          \
        *(volatile uint32_t *)&(_reg) = __myval__;                     \
    } while (0)
#define READ32(_reg) (*(volatile uint32_t *)&(_reg))
#define READ64(_reg) (*(volatile uint64_t *)&(_reg))


#define VIRTIO_MAGIC   0x74726976
#define VIRTIO_VERSION 0x2
#define VIRTIO_DEV_NET 0x1
#define VIRTIO_DEV_BLK 0x2
// #define wrap(x, len)   ((x) & ~(len))
#define wrap(x, len) ((x) % (len))
/*
 * See Section 4.2.2 of VIRTIO 1.0 Spec:
 * http://docs.oasis-open.org/virtio/virtio/v1.0/cs04/virtio-v1.0-cs04.html
 */
typedef volatile struct __attribute__((packed)){
    uint32_t MagicValue;
    uint32_t Version;
    uint32_t DeviceID;
    uint32_t VendorID;
    uint32_t DeviceFeatures;
    uint32_t DeviceFeaturesSel;
    uint32_t _reserved0[2];
    uint32_t DriverFeatures;
    uint32_t DriverFeaturesSel;
    uint32_t _reserved1[2];
    uint32_t QueueSel;
    uint32_t QueueNumMax;
    uint32_t QueueNum;
    uint32_t _reserved2[2];
    uint32_t QueueReady;
    uint32_t _reserved3[2];
    uint32_t QueueNotify;
    uint32_t _reserved4[3];
    uint32_t InterruptStatus;
    uint32_t InterruptACK;
    uint32_t _reserved5[2];
    uint32_t Status;
    uint32_t _reserved6[3];
    uint32_t QueueDescLow;
    uint32_t QueueDescHigh;
    uint32_t _reserved7[2];
    uint32_t QueueAvailLow;
    uint32_t QueueAvailHigh;
    uint32_t _reserved8[2];
    uint32_t QueueUsedLow;
    uint32_t QueueUsedHigh;
    uint32_t _reserved9[21];
    uint32_t ConfigGeneration;
    uint32_t Config[0];
} virtio_regs;

#define VIRTIO_STATUS_ACKNOWLEDGE        (1)
#define VIRTIO_STATUS_DRIVER             (2)
#define VIRTIO_STATUS_FAILED             (128)
#define VIRTIO_STATUS_FEATURES_OK        (8)
#define VIRTIO_STATUS_DRIVER_OK          (4)
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET (64)

struct virtio_cap{
    char* name;
    uint32_t bit;
    bool support;
    char* help;
};

/*
 * For simplicity, we lay out the virtqueue in contiguous memory on a single
 * page. See virtq_create for the layout and alignment requirements.
 */
struct virtqueue{
    /* Physical base address of the full data structure. */
    uint32_t phys;
    uint32_t len;
    uint32_t seen_used;
    uint32_t free_desc;

    struct vring ring;
    void** ring_mem;

    void** desc_virt;
} __attribute__((packed));


struct virtio_net_config{
    uint8_t mac[6];
#define VIRTIO_NET_S_LINK_UP  1
#define VIRTIO_NET_S_ANNOUNCE 2
    uint16_t status;
    uint16_t max_virtqueue_pairs;
} __attribute__((packed));


struct virtio_net_hdr{
#define VIRTIO_NET_HDR_F_NEEDS_CSUM    1    /* Use csum_start, csum_offset */
#define VIRTIO_NET_HDR_F_DATA_VALID    2    /* Csum is valid */
    uint8_t flags;
#define VIRTIO_NET_HDR_GSO_NONE        0    /* Not a GSO frame */
#define VIRTIO_NET_HDR_GSO_TCPV4    1    /* GSO frame, IPv4 TCP (TSO) */
#define VIRTIO_NET_HDR_GSO_UDP        3    /* GSO frame, IPv4 UDP (UFO) */
#define VIRTIO_NET_HDR_GSO_TCPV6    4    /* GSO frame, IPv6 TCP */
#define VIRTIO_NET_HDR_GSO_ECN        0x80    /* TCP has ECN set */
    uint8_t gso_type;
    uint16_t hdr_len;    /* Ethernet + IP + tcp/udp hdrs */
    uint16_t gso_size;    /* Bytes to append to hdr_len per frame */
    uint16_t csum_start;    /* Position to start checksumming from */
    uint16_t csum_offset;    /* Offset after that to place checksum */
    uint16_t num_buffers;    /* Number of merged rx buffers */
};

#define VIRTIO_NET_Q_RX 0
#define VIRTIO_NET_Q_TX 1

struct virtio_net{
    virtio_regs* regs;
    struct virtqueue* rx;
    struct virtqueue* tx;
};

/*
 * virtqueue routines
 */
struct virtqueue* virtq_create(uint32_t len);

uint32_t virtq_alloc_desc(struct virtqueue* virtq, void* addr);

void virtq_free_desc(struct virtqueue* virtq, uint32_t desc);

void virtq_add_to_device(volatile virtio_regs* regs, struct virtqueue* virtq,
                         uint32_t queue_sel);

void virtq_show(struct virtqueue* virtq);

/*
 * General purpose routines for virtio drivers
 */
void virtio_check_capabilities(virtio_regs* device, struct virtio_cap* caps,
                               uint32_t n, char* whom);

#define VIRTIO_INDP_CAPS                                                       \
    { "VIRTIO_F_RING_INDIRECT_DESC", 28, false,                            \
      "Negotiating this feature indicates that the driver can use"         \
      " descriptors with the VIRTQ_DESC_F_INDIRECT flag set, as"           \
      " described in 2.4.5.3 Indirect Descriptors." },                     \
            { "VIRTIO_F_RING_EVENT_IDX", 29, false,                        \
          "This feature enables the used_event and the avail_event "   \
          "fields"                                                     \
          " as described in 2.4.7 and 2.4.8." },                       \
            { "VIRTIO_F_VERSION_1", 32, false,                             \
          "This indicates compliance with this specification, giving " \
          "a"                                                          \
          " simple way to detect legacy devices or drivers." },


int virtio_net_init(virtio_regs* regs, uint32_t intid);

void virtio_init(void);

void virtio_net_intr();

void virtio_net_send_test();

#endif //MVMM_IRQ_VIRTIO_H
