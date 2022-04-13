/**
 * コードの引用元: https://github.com/brenns10/sos
 */

#include "virtio.h"
#include "printf.h"
#include "mm.h"
#include "pmalloc.h"

struct virtqueue* virtq_create(uint32_t len){
    int i;
    uint32_t page_virt;
    struct virtqueue* virtq;

    uint32_t memsize = vring_size(len, 4096);


    printf("memsieze for queue len %d 2 %d\n", len, vring_size(len, 4096));

    printf("memsieze for queue %d\n", memsize);

    if(memsize > PAGE_SIZE){
        printf("virtq_create: error, too big for a page\n");
        //return NULL;
    }
    page_virt = (uint32_t) pmalloc();
    pmalloc();
    virtq = (struct virtqueue*) page_virt;
    virtq->phys = (void*) page_virt;
    virtq->len = len;
    virtq->ring_mem = pmalloc();
    pmalloc();
    vring_init(&virtq->ring, len, virtq->ring_mem, 4096);


    virtq->ring.avail->idx = 0;
    virtq->ring.used->idx = 0;
    virtq->seen_used = virtq->ring.used->idx;
    virtq->free_desc = 0;

    for(i = 0; i < len; i++){
        virtq->ring.desc[i].next = i + 1;
    }
    printf("4!");

    return virtq;
}

uint32_t virtq_alloc_desc(struct virtqueue* virtq, void* addr){
    uint32_t desc = virtq->free_desc;
    uint32_t next = virtq->ring.desc[desc].next;
    if(desc == virtq->len) printf("ERROR: ran out of virtqueue descriptors\n");
    virtq->free_desc = next;

    virtq->ring.desc[desc].addr = (uint64_t) addr;
    return desc;
}

void virtq_free_desc(struct virtqueue* virtq, uint32_t desc){
    virtq->ring.desc[desc].next = virtq->free_desc;
    virtq->free_desc = desc;
}

void virtq_add_to_device(volatile virtio_regs* regs, struct virtqueue* virtq,
                         uint32_t queue_sel){
    WRITE32(regs->QueueSel, queue_sel);
    mb();
    WRITE32(regs->QueueNum, virtq->len);
    WRITE32(regs->QueueDescLow,
            virtq->phys + ((void*) virtq->ring.desc - (void*) virtq));
    WRITE32(regs->QueueDescHigh, 0);
    WRITE32(regs->QueueAvailLow,
            virtq->phys + ((void*) virtq->ring.avail - (void*) virtq));
    WRITE32(regs->QueueAvailHigh, 0);
    WRITE32(regs->QueueUsedLow,
            virtq->phys + ((void*) virtq->ring.used - (void*) virtq));
    WRITE32(regs->QueueUsedHigh, 0);
    mb();
    WRITE32(regs->QueueReady, 1);
}

void virtq_show(struct virtqueue* virtq){
    int count = 0;
    uint32_t i = virtq->free_desc;
    printf("Current free_desc: %u, len=%u\n", virtq->free_desc, virtq->len);
    while(i != virtq->len && count++ <= virtq->len){
        printf("  next: %u -> %u\n", i, virtq->ring.desc[i].next);
        i = virtq->ring.desc[i].next;
    }
    if(count > virtq->len){
        printf("Overflowed descriptors?\n");
    }
}

void virtio_check_capabilities(virtio_regs* regs, struct virtio_cap* caps,
                               uint32_t n, char* whom){
    uint32_t i;
    uint32_t bank = 0;
    uint32_t driver = 0;
    uint32_t device;

    WRITE32(regs->DeviceFeaturesSel, bank);
    mb();
    device = READ32(regs->DeviceFeatures);

    for(i = 0; i < n; i++){
        if(caps[i].bit / 32 != bank){
            /* Time to write our selected bits for this bank */
            WRITE32(regs->DriverFeaturesSel, bank);
            mb();
            WRITE32(regs->DriverFeatures, driver);
            if(device){
                /*printf("%s: device supports unknown bits"
                       " 0x%x in bank %u\n", whom, device,
                   bank);*/
            }
            /* Now we set these variables for next time. */
            bank = caps[i].bit / 32;
            WRITE32(regs->DeviceFeaturesSel, bank);
            mb();
            device = READ32(regs->DeviceFeatures);
        }
        if(device & (1 << caps[i].bit)){
            if(caps[i].support){
                driver |= (1 << caps[i].bit);
            }else{
                /*printf("virtio supports unsupported option %s
                   "
                       "(%s)\n",
                       caps[i].name, caps[i].help);*/
            }
            /* clear this from device now */
            device &= ~(1 << caps[i].bit);
        }
    }
    /* Time to write our selected bits for this bank */
    printf("bank: %d, driver: %d\n", bank, driver);

    WRITE32(regs->DriverFeaturesSel, bank);
    mb();
    WRITE32(regs->DriverFeatures, driver);
    if(device){
        /*printf("%s: device supports unknown bits"
               " 0x%x in bank %u\n", whom, device, bank);*/
    }
}

static int virtio_dev_init(uint32_t virt, uint32_t intid){
    virtio_regs* regs = (virtio_regs*) virt;

    if(READ32(regs->MagicValue) != VIRTIO_MAGIC){
        printf("error: virtio at 0x%x had wrong magic value 0x%x, "
               "expected 0x%x\n",
               virt, regs->MagicValue, VIRTIO_MAGIC);
        return -1;
    }
    if(READ32(regs->Version) != VIRTIO_VERSION){
        printf("error: virtio at 0x%x had wrong version 0x%x, expected "
               "0x%x\n",
               virt, regs->Version, VIRTIO_VERSION);
        return -1;
    }
    if(READ32(regs->DeviceID) == 0){
        /*On QEMU, this is pretty common, don't print a message */
        /*printf("warn: virtio at 0x%x has DeviceID=0, skipping\n",
         * virt);*/
        return -1;
    }

    /* First step of initialization: reset */
    WRITE32(regs->Status, 0);
    mb();
    /* Hello there, I see you */
    WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_ACKNOWLEDGE);
    mb();

    /* Hello, I am a driver for you */
    WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_DRIVER);
    mb();

    switch(READ32(regs->DeviceID)){
        case VIRTIO_DEV_BLK:
            //return virtio_blk_init(regs, intid);
        case VIRTIO_DEV_NET:
            return virtio_net_init(regs, intid);
        default:
            printf("unsupported virtio device ID 0x%x\n",
                   READ32(regs->DeviceID));
    }
    return 0;
}

void virtio_init(void){

    virtio_dev_init(0x0a000000L, 48);
}