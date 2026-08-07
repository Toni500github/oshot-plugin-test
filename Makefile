all:
	cc -shared -fPIC uppercase_plugin.c -o libuppercase_plugin.so -lcimgui -limgui -loshot_common -Wl,-undefined,dynamic_lookup -Wl,--export-dynamic
