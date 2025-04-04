#include <stdlib.h>
#include <stdbool.h>
#include "queue.h"
#include "tile_game.h"
#include "linked_list.h"

#define MAX_STATES (1UL << 28)  // 256 million entries, ~256MB

static struct linked_list internal_list = { .head = NULL };

bool is_goal_state(struct game_state *state) {
    int val = 1;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            if (row == 3 && col == 3) return state->tiles[row][col] == 0;
            if (state->tiles[row][col] != val++) return false;
        }
    }
    return true;
}

void enqueue(struct queue *q, struct game_state state) {
    (void)q;  // suppress unused parameter warning
    uint64_t encoded = serialize(state);
    insert_at_tail(&internal_list, encoded);
}

struct game_state dequeue(struct queue *q) {
    (void)q;
    size_t encoded = remove_from_head(&internal_list);
    return deserialize(encoded);
}

bool queue_is_empty() {
    return internal_list.head == NULL;
}

int number_of_moves(struct game_state start) {
    // Clear leftover queue data
    while (!queue_is_empty()) {
        dequeue(NULL);
    }

    #define MAX_STATES (1UL << 28)  // 256 million entries
    bool *visited = calloc(MAX_STATES, sizeof(bool));
    int  *depth   = calloc(MAX_STATES, sizeof(int));
    if (!visited || !depth) {
        free(visited);
        free(depth);
        return -1;
    }

    start.num_steps = 0;  // Ensure clean serialization
    uint64_t start_serial = serialize(start);
    size_t start_idx = start_serial % MAX_STATES;

    enqueue(NULL, start);
    visited[start_idx] = true;
    depth[start_idx] = 0;

    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    while (!queue_is_empty()) {
        struct game_state cur = dequeue(NULL);
        uint64_t cur_serial = serialize(cur);
        size_t cur_idx = cur_serial % MAX_STATES;
        int cur_depth = depth[cur_idx];

        if (is_goal_state(&cur)) {
            free(visited);
            free(depth);
            return cur_depth;
        }

        for (int dir = 0; dir < 4; dir++) {
            int r = cur.empty_row + dr[dir];
            int c = cur.empty_col + dc[dir];

            if (r >= 0 && r < 4 && c >= 0 && c < 4) {
                struct game_state next = cur;

                // Apply the move
                next.tiles[cur.empty_row][cur.empty_col] = cur.tiles[r][c];
                next.tiles[r][c] = 0;
                next.empty_row = r;
                next.empty_col = c;

                // Step 1: Clear num_steps before serialization (so visited tracks only layout)
                next.num_steps = 0;
                uint64_t next_serial = serialize(next);
                size_t next_idx = next_serial % MAX_STATES;

                // Step 2: Only enqueue if not visited
                if (!visited[next_idx]) {
                    visited[next_idx] = true;
                    depth[next_idx] = cur_depth + 1;

                    // Step 3: Set num_steps for BFS tracking
                    next.num_steps = cur_depth + 1;
                    enqueue(NULL, next);
                }
            }
        }
    }

    free(visited);
    free(depth);
    return -1;
}

