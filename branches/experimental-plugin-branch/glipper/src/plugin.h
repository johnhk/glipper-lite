
typedef struct {
	char* name;
	char* descr;
	//Maybe options??
} plugin_info;

//Common functions for the plugin system:

int get_plugin_info(char* module, plugin_info* info);

void start_plugin(char* module);

void stop_plugin(char* module);

//Sending Events to the plugins:

void plugins_newItem();