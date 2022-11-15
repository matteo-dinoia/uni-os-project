Issues will also be used as a TODO list and will be identified with a TODO tag.

Pull requests will be used, using fork.

File name as <word>-<word>. For naming convention, will be using GTK+, summarized as follows:
  1. All **macros and constants** in caps: ```MAX_BUFFER_SIZE, TRACKING_ID_PREFIX```.
  2. **Struct** names and typedef's in camelcase: ```GtkWidget, TrackingOrder```.
  3. **Functions** that operate **on structs**: classic C style: ```gtk_widget_show(), tracking_order_process()```.
  4. **Pointers**: nothing fancy here: ```GtkWidget *foo, TrackingOrder *bar```.
  5. **Global variables**: just don't use global variables. They are evil.
  6. **Functions** that are there, but **shouldn't be called directly**:one or more underscores at the beginning: ```_refrobnicate_data_tables()```

