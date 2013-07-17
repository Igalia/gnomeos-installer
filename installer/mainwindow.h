#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MAIN_TYPE_WINDOW            (main_window_get_type())
#define MAIN_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MAIN_TYPE_WINDOW, MainWindow))
#define MAIN_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  MAIN_TYPE_WINDOW, MainWindowClass))
#define MAIN_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MAIN_TYPE_WINDOW))
#define MAIN_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  MAIN_TYPE_WINDOW))
#define MAIN_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  MAIN_TYPE_WINDOW, MainWindowClass))

typedef struct _MainWindow         MainWindow;
typedef struct _MainWindowClass    MainWindowClass;

GType main_window_get_type(void);

GtkWidget* main_window_new();

G_END_DECLS

#endif
