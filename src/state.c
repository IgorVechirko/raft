#include "assert.h"
#include "configuration.h"
#include "election.h"
#include "log.h"
#include "queue.h"
#include "convert.h"

int raft_state(struct raft *r)
{
    return r->state;
}

void raft_leader(struct raft *r, raft_id *id, const char **address)
{
    switch (r->state) {
        case RAFT_UNAVAILABLE:
        case RAFT_CANDIDATE:
            *id = 0;
            *address = NULL;
            return;
        case RAFT_FOLLOWER:
            *id = r->follower_state.current_leader.id;
            *address = r->follower_state.current_leader.address;
            return;
        case RAFT_LEADER:
            if (r->transfer != NULL) {
                *id = 0;
                *address = NULL;
                return;
            }
            *id = r->id;
            *address = r->address;
            return;
    }
}

int prove_leadership(struct raft *r)
{
    if(r->state != RAFT_LEADER)
        return RAFT_BADROLE;

    unsigned int contacts = 0;
    for (unsigned int i = 0; i < r->configuration.n; i++) {
        struct raft_server *server = &r->configuration.servers[i];
        
        if (server->role == RAFT_VOTER && r->io->server_active(r->io, server->id, server->address)) {
            contacts++;
        }
    }

    bool isStillLeader = contacts > configurationVoterCount(&r->configuration) / 2;

    return isStillLeader ? 0 : RAFT_NOTLEADER;
}

RAFT_API int resign_leadership(struct raft *r)
{
    if(r->state != RAFT_LEADER)
        return RAFT_BADROLE;

    convertToFollower(r);

    return 0;
}
RAFT_API const struct raft_configuration* raft_get_server_config(struct raft* r)
{
    return &r->configuration;
}
RAFT_API int can_be_leader(struct raft *r)
{
    return r->io->can_be_leader(r->io) ? 0 : RAFT_INVALID;
}

raft_index raft_last_index(struct raft *r)
{
    return logLastIndex(&r->log);
}

raft_index raft_last_applied(struct raft *r)
{
    return r->last_applied;
}
