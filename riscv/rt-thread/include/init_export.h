extern int finsh_system_init(void);
INIT_APP_EXPORT(finsh_system_init);

extern int rt_i2c_core_init();
INIT_COMPONENT_EXPORT(rt_i2c_core_init);

extern int ulog_init();
INIT_BOARD_EXPORT(ulog_init);

extern int ulog_console_backend_init();
INIT_PREV_EXPORT(ulog_console_backend_init);

extern int dfs_init();
INIT_PREV_EXPORT(dfs_init);

extern int dfs_ramfs_init();
INIT_COMPONENT_EXPORT(dfs_ramfs_init);

extern int rt_hw_systick_init();
INIT_BOARD_EXPORT(rt_hw_systick_init);
