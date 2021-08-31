/* Copyright (C) 2007-2008 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "migration/qemu-file.h"
#include "sysemu/char.h"
#include "hw/android/goldfish/device.h"
#include "hw/android/goldfish/vmem.h"
#include "hw/hw.h"

enum {
    TTY_PUT_CHAR       = 0x00,
    TTY_BYTES_READY    = 0x04,
    TTY_CMD            = 0x08,

    TTY_DATA_PTR       = 0x10,
    TTY_DATA_LEN       = 0x14,
    TTY_DATA_PTR_HIGH  = 0x18,

    /* Only used by kernel 3.10+ */
    TTY_VERSION        = 0x20,

    TTY_CMD_INT_DISABLE    = 0,
    TTY_CMD_INT_ENABLE     = 1,
    TTY_CMD_WRITE_BUFFER   = 2,
    TTY_CMD_READ_BUFFER    = 3,
};

struct tty_state {
    struct goldfish_device dev;
    CharDriverState *cs;
    uint64_t ptr;
    uint32_t ptr_len;
    uint32_t ready;
    uint8_t data[128];
    uint32_t data_count;
};

#define  TTY_DEVICE_VERSION 0
#define  GOLDFISH_TTY_SAVE_VERSION  2

#define  DEBUG 0

/* Set to 1 to debug i/o register reads/writes */
#define DEBUG_REGS  0

#if DEBUG >= 1
#  define D(...)  fprintf(stderr, __VA_ARGS__)
#else
#  define D(...)  (void)0
#endif

#define E(...)  cpu_abort(cpu_single_env, __VA_ARGS__)

static void  goldfish_tty_save(QEMUFile*  f, void*  opaque)
{
    struct tty_state*  s = opaque;

    qemu_put_be64( f, s->ptr );
    qemu_put_be32( f, s->ptr_len );
    qemu_put_byte( f, s->ready );
    qemu_put_byte( f, s->data_count );
    qemu_put_buffer( f, s->data, s->data_count );
}

static int  goldfish_tty_load(QEMUFile*  f, void*  opaque, int  version_id)
{
    struct tty_state*  s = opaque;

    if ((version_id != GOLDFISH_TTY_SAVE_VERSION) && 
        (version_id != (GOLDFISH_TTY_SAVE_VERSION - 1))) {
        return -1;
    }
    if (version_id == (GOLDFISH_TTY_SAVE_VERSION - 1)) {
        s->ptr    = (uint64_t)qemu_get_be32(f);
    } else {
        s->ptr    = qemu_get_be64(f);
    }
    s->ptr_len    = qemu_get_be32(f);
    s->ready      = qemu_get_byte(f);
    s->data_count = qemu_get_byte(f);
    qemu_get_buffer(f, s->data, s->data_count);

    return 0;
}

static uint32_t goldfish_tty_read(void *opaque, hwaddr offset)
{
    struct tty_state *s = (struct tty_state *)opaque;

    D("goldfish_tty_read %" HWADDR_PRIx "\n", offset);

    switch (offset) {
        case TTY_BYTES_READY:
            return s->data_count;
        case TTY_VERSION:
            return TTY_DEVICE_VERSION;
    default:
        E("goldfish_tty_read: Bad offset %" HWADDR_PRIx "\n", offset);
        return 0;
    }
}

static void goldfish_tty_write(void *opaque, hwaddr offset, uint32_t value)
{
    struct tty_state *s = (struct tty_state *)opaque;

    D("goldfish_tty_write %" HWADDR_PRIx " %x\n", offset, value);

    switch(offset) {
        case TTY_PUT_CHAR: {
            uint8_t ch = value;
            if(s->cs)
                qemu_chr_write(s->cs, &ch, 1);
        } break;

        case TTY_CMD:
            switch(value) {
                case TTY_CMD_INT_DISABLE:
                    if(s->ready) {
                        if(s->data_count > 0)
                            goldfish_device_set_irq(&s->dev, 0, 0);
                        s->ready = 0;
                    }
                    break;

                case TTY_CMD_INT_ENABLE:
                    if(!s->ready) {
                        if(s->data_count > 0)
                            goldfish_device_set_irq(&s->dev, 0, 1);
                        s->ready = 1;
                    }
                    break;

                case TTY_CMD_WRITE_BUFFER:
                    if(s->cs) {
                        int len;
                        target_ulong  buf;

                        buf = s->ptr;
                        len = s->ptr_len;

                        while (len) {
                            char   temp[64];
                            int    to_write = sizeof(temp);
                            if (to_write > len)
                                to_write = len;

                            safe_memory_rw_debug(current_cpu, buf, (uint8_t*)temp, to_write, 0);
                            qemu_chr_write(s->cs, (const uint8_t*)temp, to_write);
                            buf += to_write;
                            len -= to_write;
                        }
                        D("goldfish_tty_write: got %d bytes from %llx\n", s->ptr_len, (unsigned long long)s->ptr);
                    }
                    break;

                case TTY_CMD_READ_BUFFER:
                    if(s->ptr_len > s->data_count)
                        E("goldfish_tty_write: reading more data than available %d %d\n", s->ptr_len, s->data_count);
                    safe_memory_rw_debug(current_cpu, s->ptr, s->data, s->ptr_len,1);
                    D("goldfish_tty_write: read %d bytes to %llx\n", s->ptr_len, (unsigned long long)s->ptr);
                    if(s->data_count > s->ptr_len)
                        memmove(s->data, s->data + s->ptr_len, s->data_count - s->ptr_len);
                    s->data_count -= s->ptr_len;
                    if(s->data_count == 0 && s->ready)
                        goldfish_device_set_irq(&s->dev, 0, 0);
                    break;

                default:
                    E("goldfish_tty_write: Bad command %x\n", value);
            };
            break;

        case TTY_DATA_PTR:
            uint64_set_low(&s->ptr, value);
            break;

        case TTY_DATA_PTR_HIGH:
            uint64_set_high(&s->ptr, value);
            break;

        case TTY_DATA_LEN:
            s->ptr_len = value;
            break;

        default:
            E("goldfish_tty_write: Bad offset %" HWADDR_PRIx "\n", offset);
    }
}

static int tty_can_receive(void *opaque)
{
    struct tty_state *s = opaque;

    return (sizeof(s->data) - s->data_count);
}

static void tty_receive(void *opaque, const uint8_t *buf, int size)
{
    struct tty_state *s = opaque;

    memcpy(s->data + s->data_count, buf, size);
    s->data_count += size;
    if(s->data_count > 0 && s->ready)
        goldfish_device_set_irq(&s->dev, 0, 1);
}

static CPUReadMemoryFunc *goldfish_tty_readfn[] = {
    goldfish_tty_read,
    goldfish_tty_read,
    goldfish_tty_read
};

static CPUWriteMemoryFunc *goldfish_tty_writefn[] = {
    goldfish_tty_write,
    goldfish_tty_write,
    goldfish_tty_write
};

int goldfish_tty_add(CharDriverState *cs, int id, uint32_t base, int irq)
{
    int ret;
    struct tty_state *s;
    static int  instance_id = 0;

    s = g_malloc0(sizeof(*s));
    s->dev.name = "goldfish_tty";
    s->dev.id = id;
    s->dev.base = base;
    s->dev.size = 0x1000;
    s->dev.irq = irq;
    s->dev.irq_count = 1;
    s->cs = cs;

    if(cs) {
        qemu_chr_add_handlers(cs, tty_can_receive, tty_receive, NULL, s);
    }

    ret = goldfish_device_add(&s->dev, goldfish_tty_readfn, goldfish_tty_writefn, s);
    if(ret) {
        g_free(s);
    } else {
        register_savevm(NULL,
                        "goldfish_tty",
                        instance_id++,
                        GOLDFISH_TTY_SAVE_VERSION,
                        goldfish_tty_save,
                        goldfish_tty_load,
                        s);
    }
    return ret;
}

