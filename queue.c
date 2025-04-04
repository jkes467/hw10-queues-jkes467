#include <stdlib.h>
#include <stdbool.h>
#include "queue.h"
#include "tile_game.h"

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
    size_t encoded = serialize(state);
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
    // Clear any leftover queue data
    while (!queue_is_empty()) {
        dequeue(NULL);  // discard
    }

    bool *visited = calloc((1UL << 28), sizeof(bool));
    int  *depth   = calloc((1UL << 28), sizeof(int));
    if (!visited || !depth) return -1;

    size_t start_serial = serialize(start);
    enqueue(NULL, start);
    visited[start_serial] = true;
    depth[start_serial] = 0;

    while (!queue_is_empty()) {
        struct game_state cur = dequeue(NULL);
        size_t cur_serial = serialize(cur);
        int cur_depth = depth[cur_serial];

        if (is_goal_state(&cur)) {
            free(visited);
            free(depth);
            return cur_depth;
        }

        const int dr[4] = {-1, 1, 0, 0};
        const int dc[4] = {0, 0, -1, 1};

        for (int dir = 0; dir < 4; dir++) {
            int r = cur.empty_row + dr[dir];
            int c = cur.empty_col + dc[dir];

            if (r >= 0 && r < 4 && c >= 0 && c < 4) {
                struct game_state next = cur;

                next.tiles[cur.empty_row][cur.empty_col] = cur.tiles[r][c];
                next.tiles[r][c] = 0;
                next.empty_row = r;
                next.empty_col = c;

                size_t next_serial = serialize(next);
                if (!visited[next_serial]) {
                    visited[next_serial] = true;
                    depth[next_serial] = cur_depth + 1;
                    enqueue(NULL, next);
                }
            }
        }
    }

    free(visited);
    free(depth);
    return -1;
}