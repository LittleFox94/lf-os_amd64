#include <sd.h>
#include <vm.h>
#include <mq.h>
#include <uuid.h>
#include <flexarray.h>
#include <string.h>
#include <log.h>
#include <errno.h>
#include <scheduler.h>

struct sd_entry {
    struct sd_entry* prev;
    struct sd_entry* next;

    uuid_t  uuid;
    size_t  push_idx;
    mq_id_t queues[(4080 - sizeof(uuid_t)
                         - sizeof(size_t)
                         - (sizeof(struct sd_entry*) * 2)
                   ) / sizeof(mq_id_t)];
};

struct sd {
    struct sd_entry* entry_shortcut[256];

    struct sd_entry* tail;
    struct sd_entry* head;
};

static struct sd sd_global_data;

void init_sd(void) {
    memset(sd_global_data.entry_shortcut, 0, sizeof(sd_global_data.entry_shortcut));
}

static struct sd_entry* new_sd_entry(const uuid_t* uuid, mq_id_t queue) {
    struct sd_entry* entry = (struct sd_entry*)kernel_alloc.alloc(&kernel_alloc, (sizeof(struct sd_entry)));
    memset(entry, 0, sizeof(struct sd_entry));
    memcpy(&entry->uuid, uuid, sizeof(uuid_t));
    entry->queues[entry->push_idx++] = queue;

    return entry;
}

uint64_t sd_register(const uuid_t* uuid, mq_id_t svc_queue) {
    char uuid_s[38];
    uuid_fmt(uuid_s, sizeof(uuid_s), uuid);

    logd("sd", "Registering queue %u/%u for service %s", scheduler_current_process, svc_queue, uuid_s);

    uuid_key_t key         = uuid_key(uuid);
    struct sd_entry* entry = sd_global_data.entry_shortcut[key];

    if(!entry) {
        entry = new_sd_entry(uuid, svc_queue);

        if(sd_global_data.tail) {
            sd_global_data.tail->next = entry;
            sd_global_data.tail = entry;
        }
        else {
            sd_global_data.head = entry;
            sd_global_data.tail = entry;
        }

        sd_global_data.entry_shortcut[key] = entry;
    }
    else if(uuid_cmp(&entry->uuid, uuid) != 0) {
        struct sd_entry* cur = entry;

        if(uuid_cmp(&entry->uuid, uuid) < 0) {
            cur = entry->prev;
        }
        else {
            while(cur->next && uuid_key(&cur->next->uuid) == key && uuid_cmp(&cur->next->uuid, uuid) > 0) {
                cur = cur->next;
            }
        }

        entry = new_sd_entry(uuid, svc_queue);

        if(cur->prev) {
            cur->prev->next = entry;
        }

        entry->next = cur;
        entry->prev = cur->prev;
        cur->prev   = entry;

        sd_global_data.entry_shortcut[key] = entry;
    }
    else {
        for(size_t i = 0; i < entry->push_idx; ++i) {
            if(entry->queues[i] == svc_queue) {
                logw("sd", "Tried to register queue for service %s a second time!", uuid_s);
            }
        }

        if(entry->push_idx >= sizeof(entry->queues) / sizeof(mq_id_t)) {
            return ENOMEM;
        }

        entry->queues[entry->push_idx++] = svc_queue;
    }

    return 0;
}

int64_t sd_send(const uuid_t* uuid, struct Message* msg) {
    uuid_key_t key         = uuid_key(uuid);
    struct sd_entry* entry = sd_global_data.entry_shortcut[key];

    while(entry && uuid_cmp(&entry->uuid, uuid) != 0) {
        entry = entry->next;
    }

    if(!entry) {
        return -ENOENT;
    }

    unsigned success = 0;

    for(size_t i = 0; i < entry->push_idx; ++i) {
        if(mq_push(entry->queues[i], msg) == 0) {
            ++success;
        }
    }

    return success;
}
