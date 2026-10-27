#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "e1000_dev.h"

#define TX_RING_SIZE 16
static struct tx_desc tx_ring[TX_RING_SIZE] __attribute__((aligned(16)));

#define RX_RING_SIZE 16
static struct rx_desc rx_ring[RX_RING_SIZE] __attribute__((aligned(16)));

// remember where the e1000's registers live.
static volatile uint32 *regs;

struct spinlock e1000_lock;

// called by pci_init().
// xregs is the memory address at which the
// e1000's registers are mapped.
// this code loosely follows the initialization directions
// in Chapter 14 of Intel's Software Developer's Manual.
void
e1000_init(uint32 *xregs)
{
  int i;

  initlock(&e1000_lock, "e1000");

  regs = xregs;

  // Reset the device
  regs[E1000_IMS] = 0; // disable interrupts
  regs[E1000_CTL] |= E1000_CTL_RST;
  regs[E1000_IMS] = 0; // redisable interrupts
  __sync_synchronize();

  // [E1000 14.5] Transmit initialization
  memset(tx_ring, 0, sizeof(tx_ring));
  for (i = 0; i < TX_RING_SIZE; i++) {
    tx_ring[i].status = E1000_TXD_STAT_DD;
    // MT: Question: what is txd stat dd?
    tx_ring[i].addr = 0;
  }
  regs[E1000_TDBAL] = (uint64) tx_ring;
  // MT: Question: TDBAL represents the start address of tx_ring?
  // Answer: Yes
  if(sizeof(tx_ring) % 128 != 0)
    panic("e1000");
  // MT: Question: why check sizeof(tx_ring) % 128?
  // Answer: e1000's requirement
  regs[E1000_TDLEN] = sizeof(tx_ring);
  regs[E1000_TDH] = regs[E1000_TDT] = 0;
  // MT:
  // TDH — Transmit Descriptor Head: the index of the next descriptor the e1000 will process.
  // The device advances it as it transmits.
  //
  // TDT — Transmit Descriptor Tail: the boundary up to which the driver has supplied descriptors.
  // The driver advances it to notify the device of new work.

  // [E1000 14.4] Receive initialization
  memset(rx_ring, 0, sizeof(rx_ring));
  for (i = 0; i < RX_RING_SIZE; i++) {
    rx_ring[i].addr = (uint64) kalloc();
    // MT: Question: why kalloc() here?  Tx's descriptor's address just points some are of RAM? It address is set by kernel?
    // But Rx's d's address need to be allocated in advance, because hardware need to write into memo through DMA? But why not just use
    // Global array? why kalloc()? Because we should not reserve space in advance, for we may not use the corresponding hardware?
    //
    // Answer: can use global array, but, buffer allocated by kalloc() will be used by other hardware,
    // We can just move the ownship to other device and kalloc() a new buffer to rx_ring[i].addr
    if (!rx_ring[i].addr)
      panic("e1000");
  }
  regs[E1000_RDBAL] = (uint64) rx_ring;
  if(sizeof(rx_ring) % 128 != 0)
    panic("e1000");
  regs[E1000_RDH] = 0;
  regs[E1000_RDT] = RX_RING_SIZE - 1;
  regs[E1000_RDLEN] = sizeof(rx_ring);

  // filter by qemu's MAC address, 52:54:00:12:34:56
  regs[E1000_RA] = 0x12005452;
  // MT: THis is RAL(Receive Address Low), the former 4 bytes of mac.
  regs[E1000_RA+1] = 0x5634 | (1<<31);
  // MT: This is RAH(Receive Address High), last 2 bytes of mac, and control bits.

  // multicast table
  // MT: this disable all multicast
  for (int i = 0; i < 4096/32; i++)
    regs[E1000_MTA + i] = 0;

  // transmitter control bits.
  regs[E1000_TCTL] = E1000_TCTL_EN |  // enable
    E1000_TCTL_PSP |                  // pad short packets
    (0x10 << E1000_TCTL_CT_SHIFT) |   // collision stuff
    (0x40 << E1000_TCTL_COLD_SHIFT);
  regs[E1000_TIPG] = 10 | (8<<10) | (6<<20); // inter-pkt gap

  // receiver control bits.
  regs[E1000_RCTL] = E1000_RCTL_EN | // enable receiver
    E1000_RCTL_BAM |                 // enable broadcast
    E1000_RCTL_SZ_2048 |             // 2048-byte rx buffers
    E1000_RCTL_SECRC;                // strip CRC
  
  // ask e1000 for receive interrupts.
  regs[E1000_RDTR] = 0; // interrupt after every received packet (no timer)
  regs[E1000_RADV] = 0; // interrupt after every packet (no timer)
  regs[E1000_IMS] = (1 << 7); // RXDW -- Receiver Descriptor Write Back
}

int
e1000_transmit(char *buf, int len)
{
  //
  // Your code here.
  //
  // buf contains an ethernet frame; program it into
  // the TX descriptor ring so that the e1000 sends it. Stash
  // a pointer so that it can be freed after send completes.
  //
  // return 0 on success.
  // return -1 on failure (e.g., there is no descriptor available)
  // so that the caller knows to free buf.
  //

  // net code
  int index;
  index = regs[E1000_TDT];
  // check if rx_ring is overflowing
  if((tx_ring[index].status & E1000_TXD_STAT_DD) == 0) {
    printf("DEBUG: tx_ring is overflowing\n");
    return -1;
  }

  // kfree() last buffer that was transmitted from this descriptor
  acquire(&e1000_lock);
  if(tx_ring[index].addr)
    kfree((char *)tx_ring[index].addr);

  // fill in descriptor
  tx_ring[index].addr = (uint64)buf;
  tx_ring[index].length = len;
  tx_ring[index].status = 0;

  // file in cmd register
  tx_ring[index].cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS;

  // update the ring position
  regs[E1000_TDT] = (regs[E1000_TDT] + 1) % TX_RING_SIZE;
  release(&e1000_lock);

  return 0;
}

static void
e1000_recv(void)
{
  //
  // Your code here.
  //
  // Check for packets that have arrived from the e1000
  // Create and deliver a buf for each packet (using net_rx()).
  //

  // net code
  int index;
  index = (regs[E1000_RDT] + 1) % RX_RING_SIZE;

  // check if a new packt is available
  while (1) {
    acquire(&e1000_lock);
    if((rx_ring[index].status & E1000_TXD_STAT_DD) == 1) {
      // deliver the packet buffer to the network stack
      net_rx((char *)rx_ring[index].addr, rx_ring[index].length);

      // allocate a new buffer using kalloc
      rx_ring[index].addr = (uint64) kalloc();
      rx_ring->length = 0;
      rx_ring[index].status = 0;

      // update E1000_RDT
      regs[E1000_RDT] = (regs[E1000_RDT] + 1) % RX_RING_SIZE;
    } else {
      break;
    }
  release(&e1000_lock);
  }
}

void
e1000_intr(void)
{
  // tell the e1000 we've seen this interrupt;
  // without this the e1000 won't raise any
  // further interrupts.
  regs[E1000_ICR] = 0xffffffff;

  e1000_recv();
}
