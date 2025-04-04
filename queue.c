#include <stdlib.h>
#include <stdbool.h>
#include "queue.h"
#include "tile_game.h"
#include "linked_list.h"

#define MAX_STATES (1UL << 28)  // 256 million entries, ~256MB

static struct linked_list internal_list = { .head = NULL };

bool is_goal_state(struct game_state *state) {
    int expected = 1;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            if (row == 3 && col == 3) {
                if (state->tiles[row][col] != 0)
                    return false;
            } else {
                if (state->tiles[row][col] != expected++)
                    return false;
            }
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
    while (!queue_is_empty()) {
        dequeue(NULL);  // Clear stale queue
    }

    #define MAX_STATES (1UL << 28)
    bool *visited = calloc(MAX_STATES, sizeof(bool));
    if (!visited) return -1;

    uint64_t start_serial = serialize((struct game_state){ .tiles = {{0}}, .empty_row = start.empty_row, .empty_col = start.empty_col, .num_steps = 0 });
    size_t start_idx = start_serial % MAX_STATES;

    start.num_steps = 0;
    visited[start_idx] = true;
    enqueue(NULL, start);

    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    while (!queue_is_empty()) {
        struct game_state cur = dequeue(NULL);

        if (is_goal_state(&cur)) {
            free(visited);
            return cur.num_steps;
        }

        for (int dir = 0; dir < 4; dir++) {
            int r = cur.empty_row + dr[dir];
            int c = cur.empty_col + dc[dir];

            if (r >= 0 && r < 4 && c >= 0 && c < 4) {
                struct game_state next = cur;

                next.tiles[cur.empty_row][cur.empty_col] = cur.tiles[r][c];
                next.tiles[r][c] = 0;
                next.empty_row = r;
                next.empty_col = c;
                next.num_steps = cur.num_steps + 1;

                struct game_state tmp = next;
                tmp.num_steps = 0;  // Ensure we serialize only layout
                uint64_t next_serial = serialize(tmp);
                size_t next_idx = next_serial % MAX_STATES;

                if (!visited[next_idx]) {
                    visited[next_idx] = true;
                    enqueue(NULL, next);
                }
            }
        }
    }

    free(visited);
    return -1;
}