#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define ROWS 15
#define COLS 15
#define INF 1000000

// Forward declarations for timeout callback functions
static gboolean generation_step(gpointer data);
static gboolean solving_step(gpointer data);
static gboolean dfs_solving_step(gpointer data);
static gboolean dijkstra_solving_step(gpointer data);
static gboolean astar_solving_step(gpointer data);

// Data structures
typedef struct {
    int r;
    int c;
} Position;

typedef struct {
    bool wall_north;
    bool wall_south;
    bool wall_east;
    bool wall_west;
    bool visited;
    bool is_path;
} Cell;

typedef struct {
    Position pos;
    int dist;
} PQNode;

// Global variables
Cell maze[ROWS][COLS];
Position stack[ROWS * COLS];
int stack_top = 0;
Position queue[ROWS * COLS];
int front = 0, rear = 0;
Position parent[ROWS][COLS];
PQNode pq[ROWS * COLS];
int pq_size = 0;
int distance[ROWS][COLS];
GtkWidget *drawing_area;

// Direction offsets for north, east, south, west
int drow[] = {-1, 0, 1, 0};
int dcol[] = {0, 1, 0, -1};

// Initialize the maze with all walls up
void init_maze() {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            maze[r][c].wall_north = true;
            maze[r][c].wall_south = true;
            maze[r][c].wall_east = true;
            maze[r][c].wall_west = true;
            maze[r][c].visited = false;
            maze[r][c].is_path = false;
        }
    }
}

// Draw the maze using Cairo
static gboolean draw_maze(GtkWidget *widget, cairo_t *cr, gpointer data) {
    (void)data;  // Mark 'data' as unused
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    double cell_width = (double)width / COLS;
    double cell_height = (double)height / ROWS;

    // Draw each cell
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            double x = c * cell_width;
            double y = r * cell_height;

            if (maze[r][c].is_path) {
                cairo_set_source_rgb(cr, 0, 0.6, 0);  // Green for path
            } else if (maze[r][c].visited) {
                cairo_set_source_rgb(cr, 1, 0.5, 0);  // Dark orange for visited
            } else {
                cairo_set_source_rgb(cr, 0, 0, 0);  // White for unvisited
            }
            cairo_rectangle(cr, x, y, cell_width, cell_height);
            cairo_fill(cr);

            cairo_set_source_rgb(cr, 1, 1, 1);  // Black for walls
            if (maze[r][c].wall_north) {
                cairo_move_to(cr, x, y);
                cairo_line_to(cr, x + cell_width, y);
            }
            if (maze[r][c].wall_east) {
                cairo_move_to(cr, x + cell_width, y);
                cairo_line_to(cr, x + cell_width, y + cell_height);
            }
            if (maze[r][c].wall_south) {
                cairo_move_to(cr, x, y + cell_height);
                cairo_line_to(cr, x + cell_width, y + cell_height);
            }
            if (maze[r][c].wall_west) {
                cairo_move_to(cr, x, y);
                cairo_line_to(cr, x, y + cell_height);
            }
            cairo_stroke(cr);
        }
    }

    // Draw start (blue circle)
    double radius = cell_width / 4;
    cairo_set_source_rgb(cr, 0, 0, 1);
    cairo_arc(cr, cell_width / 2, cell_height / 2, radius, 0, 2 * G_PI);
    cairo_fill(cr);

    // Draw end (red circle)
    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_arc(cr, (COLS - 0.5) * cell_width, (ROWS - 0.5) * cell_height, radius, 0, 2 * G_PI);
    cairo_fill(cr);

    return FALSE;
}

// Maze generation step (recursive backtracking via timeout)
static gboolean generation_step(gpointer data) {
    (void)data;  // Mark 'data' as unused
    if (stack_top > 0) {
        Position current = stack[--stack_top];
        int neighbors[4];
        int count = 0;

        for (int dir = 0; dir < 4; dir++) {
            int nr = current.r + drow[dir];
            int nc = current.c + dcol[dir];
            if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS && !maze[nr][nc].visited) {
                neighbors[count++] = dir;
            }
        }

        if (count > 0) {
            stack[stack_top++] = current;
            int dir = neighbors[rand() % count];
            int nr = current.r + drow[dir];
            int nc = current.c + dcol[dir];

            if (dir == 0) { // North
                maze[current.r][current.c].wall_north = false;
                maze[nr][nc].wall_south = false;
            } else if (dir == 1) { // East
                maze[current.r][current.c].wall_east = false;
                maze[nr][nc].wall_west = false;
            } else if (dir == 2) { // South
                maze[current.r][current.c].wall_south = false;
                maze[nr][nc].wall_north = false;
            } else { // West
                maze[current.r][current.c].wall_west = false;
                maze[nr][nc].wall_east = false;
            }

            maze[nr][nc].visited = true;
            stack[stack_top++] = (Position){nr, nc};
        }
        gtk_widget_queue_draw(drawing_area);
        return TRUE;
    }
    return FALSE;
}

// Start maze generation
static void start_generation(GtkWidget *widget, gpointer data) {
    (void)widget;  // Mark 'widget' as unused
    (void)data;    // Mark 'data' as unused
    init_maze();
    stack_top = 0;
    stack[stack_top++] = (Position){0, 0};
    maze[0][0].visited = true;
    g_timeout_add(50, generation_step, NULL);
}

// BFS solving step
static gboolean solving_step(gpointer data) {
    (void)data;  // Mark 'data' as unused
    if (front < rear) {
        Position current = queue[front++];
        if (current.r == ROWS - 1 && current.c == COLS - 1) {
            Position p = current;
            while (p.r != -1) {
                maze[p.r][p.c].is_path = true;
                p = parent[p.r][p.c];
            }
            gtk_widget_queue_draw(drawing_area);
            return FALSE;
        }

        for (int dir = 0; dir < 4; dir++) {
            int nr = current.r + drow[dir];
            int nc = current.c + dcol[dir];
            if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS && !maze[nr][nc].visited) {
                if ((dir == 0 && !maze[current.r][current.c].wall_north) ||
                    (dir == 1 && !maze[current.r][current.c].wall_east) ||
                    (dir == 2 && !maze[current.r][current.c].wall_south) ||
                    (dir == 3 && !maze[current.r][current.c].wall_west)) {
                    queue[rear++] = (Position){nr, nc};
                    parent[nr][nc] = current;
                    maze[nr][nc].visited = true;
                }
            }
        }
        gtk_widget_queue_draw(drawing_area);
        return TRUE;
    }
    return FALSE;
}

// Start BFS solving
static void start_solving(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            maze[r][c].visited = false;
            maze[r][c].is_path = false;
        }
    }
    front = 0;
    rear = 0;
    queue[rear++] = (Position){0, 0};
    parent[0][0] = (Position){-1, -1};
    maze[0][0].visited = true;
    g_timeout_add(50, solving_step, NULL);
}

// DFS solving step
static gboolean dfs_solving_step(gpointer data) {
    (void)data;
    if (stack_top > 0) {
        Position current = stack[--stack_top];
        if (current.r == ROWS - 1 && current.c == COLS - 1) {
            Position p = current;
            while (p.r != -1) {
                maze[p.r][p.c].is_path = true;
                p = parent[p.r][p.c];
            }
            gtk_widget_queue_draw(drawing_area);
            return FALSE;
        }

        for (int dir = 0; dir < 4; dir++) {
            int nr = current.r + drow[dir];
            int nc = current.c + dcol[dir];
            if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS && !maze[nr][nc].visited) {
                if ((dir == 0 && !maze[current.r][current.c].wall_north) ||
                    (dir == 1 && !maze[current.r][current.c].wall_east) ||
                    (dir == 2 && !maze[current.r][current.c].wall_south) ||
                    (dir == 3 && !maze[current.r][current.c].wall_west)) {
                    stack[stack_top++] = (Position){nr, nc};
                    parent[nr][nc] = current;
                    maze[nr][nc].visited = true;
                }
            }
        }
        gtk_widget_queue_draw(drawing_area);
        return TRUE;
    }
    return FALSE;
}

// Start DFS solving
static void start_dfs_solving(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            maze[r][c].visited = false;
            maze[r][c].is_path = false;
        }
    }
    stack_top = 0;
    stack[stack_top++] = (Position){0, 0};
    maze[0][0].visited = true;
    parent[0][0] = (Position){-1, -1};
    g_timeout_add(50, dfs_solving_step, NULL);
}

// Dijkstra solving step
static gboolean dijkstra_solving_step(gpointer data) {
    (void)data;
    if (pq_size > 0) {
        int min_idx = 0;
        for (int i = 1; i < pq_size; i++) {
            if (pq[i].dist < pq[min_idx].dist)
                min_idx = i;
        }
        PQNode min_node = pq[min_idx];
        pq[min_idx] = pq[--pq_size];
        Position current = min_node.pos;

        if (maze[current.r][current.c].visited)
            return TRUE;
        maze[current.r][current.c].visited = true;

        if (current.r == ROWS - 1 && current.c == COLS - 1) {
            Position p = current;
            while (p.r != -1) {
                maze[p.r][p.c].is_path = true;
                p = parent[p.r][p.c];
            }
            gtk_widget_queue_draw(drawing_area);
            return FALSE;
        }

        for (int dir = 0; dir < 4; dir++) {
            int nr = current.r + drow[dir];
            int nc = current.c + dcol[dir];
            if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS && !maze[nr][nc].visited) {
                if ((dir == 0 && !maze[current.r][current.c].wall_north) ||
                    (dir == 1 && !maze[current.r][current.c].wall_east) ||
                    (dir == 2 && !maze[current.r][current.c].wall_south) ||
                    (dir == 3 && !maze[current.r][current.c].wall_west)) {
                    int new_dist = distance[current.r][current.c] + 1;
                    if (new_dist < distance[nr][nc]) {
                        distance[nr][nc] = new_dist;
                        parent[nr][nc] = current;
                        pq[pq_size++] = (PQNode){(Position){nr, nc}, new_dist};
                    }
                }
            }
        }
        gtk_widget_queue_draw(drawing_area);
        return TRUE;
    }
    return FALSE;
}

// Heuristic function for A* (Manhattan distance)
static int heuristic(int r, int c) {
    return abs(r - (ROWS - 1)) + abs(c - (COLS - 1));
}

// A* solving step
static gboolean astar_solving_step(gpointer data) {
    (void)data;
    if (pq_size > 0) {
        int min_idx = 0;
        for (int i = 1; i < pq_size; i++) {
            if (pq[i].dist < pq[min_idx].dist)
                min_idx = i;
        }
        PQNode min_node = pq[min_idx];
        pq[min_idx] = pq[--pq_size];
        Position current = min_node.pos;

        if (maze[current.r][current.c].visited)
            return TRUE;
        maze[current.r][current.c].visited = true;

        if (current.r == ROWS - 1 && current.c == COLS - 1) {
            Position p = current;
            while (p.r != -1) {
                maze[p.r][p.c].is_path = true;
                p = parent[p.r][p.c];
            }
            gtk_widget_queue_draw(drawing_area);
            return FALSE;
        }

        for (int dir = 0; dir < 4; dir++) {
            int nr = current.r + drow[dir];
            int nc = current.c + dcol[dir];
            if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS && !maze[nr][nc].visited) {
                if ((dir == 0 && !maze[current.r][current.c].wall_north) ||
                    (dir == 1 && !maze[current.r][current.c].wall_east) ||
                    (dir == 2 && !maze[current.r][current.c].wall_south) ||
                    (dir == 3 && !maze[current.r][current.c].wall_west)) {
                    int new_g = distance[current.r][current.c] + 1;
                    if (new_g < distance[nr][nc]) {
                        distance[nr][nc] = new_g;
                        int h = heuristic(nr, nc);
                        int priority = new_g + h;
                        pq[pq_size++] = (PQNode){(Position){nr, nc}, priority};
                        parent[nr][nc] = current;
                    }
                }
            }
        }
        gtk_widget_queue_draw(drawing_area);
        return TRUE;
    }
    return FALSE;
}

// Start Dijkstra solving
static void start_dijkstra_solving(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            maze[r][c].visited = false;
            maze[r][c].is_path = false;
            distance[r][c] = INF;
        }
    }
    pq_size = 0;
    distance[0][0] = 0;
    pq[pq_size++] = (PQNode){(Position){0, 0}, 0};
    parent[0][0] = (Position){-1, -1};
    g_timeout_add(50, dijkstra_solving_step, NULL);
}

// Start A* solving
static void start_astar_solving(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            maze[r][c].visited = false;
            maze[r][c].is_path = false;
            distance[r][c] = INF;
        }
    }
    pq_size = 0;
    distance[0][0] = 0;
    int h = heuristic(0, 0);
    pq[pq_size++] = (PQNode){(Position){0, 0}, distance[0][0] + h};
    parent[0][0] = (Position){-1, -1};
    g_timeout_add(50, astar_solving_step, NULL);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Maze Generator and Solver");
    gtk_window_set_default_size(GTK_WINDOW(window), COLS * 20, ROWS * 20 + 50);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    drawing_area = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(box), drawing_area, TRUE, TRUE, 0);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(draw_maze), NULL);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(box), button_box, FALSE, FALSE, 0);

    GtkWidget *generate_button = gtk_button_new_with_label("Generate Maze");
    gtk_box_pack_start(GTK_BOX(button_box), generate_button, TRUE, TRUE, 0);
    g_signal_connect(generate_button, "clicked", G_CALLBACK(start_generation), NULL);

    GtkWidget *bfs_button = gtk_button_new_with_label("Solve with BFS");
    gtk_box_pack_start(GTK_BOX(button_box), bfs_button, TRUE, TRUE, 0);
    g_signal_connect(bfs_button, "clicked", G_CALLBACK(start_solving), NULL);

    GtkWidget *dfs_button = gtk_button_new_with_label("Solve with DFS");
    gtk_box_pack_start(GTK_BOX(button_box), dfs_button, TRUE, TRUE, 0);
    g_signal_connect(dfs_button, "clicked", G_CALLBACK(start_dfs_solving), NULL);

    GtkWidget *dijkstra_button = gtk_button_new_with_label("Solve with Dijkstra");
    gtk_box_pack_start(GTK_BOX(button_box), dijkstra_button, TRUE, TRUE, 0);
    g_signal_connect(dijkstra_button, "clicked", G_CALLBACK(start_dijkstra_solving), NULL);

    GtkWidget *astar_button = gtk_button_new_with_label("Solve with A*");
    gtk_box_pack_start(GTK_BOX(button_box), astar_button, TRUE, TRUE, 0);
    g_signal_connect(astar_button, "clicked", G_CALLBACK(start_astar_solving), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}


