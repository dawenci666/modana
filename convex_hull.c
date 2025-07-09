#include <cairo.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    float x, y;
} cairo_point_t;

// Point comparison for sorting (by x, then y)
int compare_points(const void* a, const void* b) {
    cairo_point_t* p1 = (cairo_point_t*)a;
    cairo_point_t* p2 = (cairo_point_t*)b;
    if (p1->x < p2->x) return -1;
    if (p1->x > p2->x) return 1;
    if (p1->y < p2->y) return -1;
    if (p1->y > p2->y) return 1;
    return 0;
}

float cross(cairo_point_t O, cairo_point_t A, cairo_point_t B) {
    return (A.x - O.x)*(B.y - O.y) - (A.y - O.y)*(B.x - O.x);
}

// Andrew's monotone chain convex hull algorithm
// hull array should be at least size n*2
// returns size of hull
int convex_hull(cairo_point_t* points, int n, cairo_point_t* hull) {
    if (n < 3) {
        for (int i = 0; i < n; i++) hull[i] = points[i];
        return n;
    }

    qsort(points, n, sizeof(cairo_point_t), compare_points);

    int k = 0;

    // Lower hull
    for (int i = 0; i < n; i++) {
        while (k >= 2 && cross(hull[k-2], hull[k-1], points[i]) <= 0) k--;
        hull[k++] = points[i];
    }

    // Upper hull
    int t = k + 1;
    for (int i = n-2; i >= 0; i--) {
        while (k >= t && cross(hull[k-2], hull[k-1], points[i]) <= 0) k--;
        hull[k++] = points[i];
    }

    return k - 1;  // last point is same as first
}

// Vector operations
static cairo_point_t point_sub(cairo_point_t a, cairo_point_t b) {
    return (cairo_point_t){a.x - b.x, a.y - b.y};
}
static cairo_point_t point_add(cairo_point_t a, cairo_point_t b) {
    return (cairo_point_t){a.x + b.x, a.y + b.y};
}
static cairo_point_t point_scale(cairo_point_t p, float s) {
    return (cairo_point_t){p.x * s, p.y * s};
}

// Draw smooth closed curve through points using cubic Bezier curves
void draw_smooth_closed_curve(cairo_t* cr, cairo_point_t* hull, int n, float tension) {
    if (n < 3) return;

    if (tension < 0) tension = 0.3f;
    if (tension > 1) tension = 1.0f;

    cairo_move_to(cr, hull[0].x, hull[0].y);

    for (int i = 0; i < n; i++) {
        cairo_point_t p0 = hull[i];
        cairo_point_t p1 = hull[(i + 1) % n];
        cairo_point_t p_1 = hull[(i - 1 + n) % n];
        cairo_point_t p2 = hull[(i + 2) % n];

        cairo_point_t cp1 = point_add(p0, point_scale(point_sub(p1, p_1), tension));
        cairo_point_t cp2 = point_sub(p1, point_scale(point_sub(p2, p0), tension));

        cairo_curve_to(cr, cp1.x, cp1.y, cp2.x, cp2.y, p1.x, p1.y);
    }
    cairo_close_path(cr);

    // Fill and stroke
    cairo_set_source_rgba(cr, 0.2, 0.5, 0.8, 0.5);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 2);
    cairo_stroke(cr);
}

int main() {
    // Example input points
    cairo_point_t points[] = {
        {100, 100}, {120, 130}, {130, 180}, {140, 150},
        {160, 120}, {180, 140}, {150, 200}, {200, 200},
        {220, 180}, {210, 120}, {170, 90},  {140, 80}
    };
    int n_points = sizeof(points) / sizeof(points[0]);

    // Compute convex hull
    cairo_point_t* hull = malloc(sizeof(cairo_point_t) * n_points * 2); // enough size
    int hull_size = convex_hull(points, n_points, hull);

    printf("Hull size: %d\n", hull_size);
    for (int i = 0; i < hull_size; i++) {
        printf("Hull[%d]: (%.2f, %.2f)\n", i, hull[i].x, hull[i].y);
    }

    // Create cairo surface and context
    int width = 400, height = 300;
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);

    // White background
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // Draw input points
    cairo_set_source_rgb(cr, 1, 0, 0);
    for (int i = 0; i < n_points; i++) {
        cairo_arc(cr, points[i].x, points[i].y, 4, 0, 2*M_PI);
        cairo_fill(cr);
    }

    // Draw smooth hull curve with tension 0.4
    draw_smooth_closed_curve(cr, hull, hull_size, 0.4f);

    // Write output image
    cairo_surface_write_to_png(surface, "smooth_convex_hull.png");

    // Clean up
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    free(hull);

    printf("Saved image to smooth_convex_hull.png\n");

    return 0;
}
