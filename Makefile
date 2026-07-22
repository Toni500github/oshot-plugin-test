all:
	cc -I/home/toni/stupid_projects/oshot/include/libs/ -I/home/toni/stupid_projects/oshot/src/plugins \
		-shared -fPIC uppercase_plugin.c -o libuppercase_plugin.so \
		-L/home/toni/stupid_projects/oshot/build/release/ -Wl,-rpath,/home/toni/stupid_projects/oshot/build/release/ -lcimgui -limgui -loshot_plugin -Wl,-undefined,dynamic_lookup -Wl,--export-dynamic
