

struct simple_mesh_data {

	std::string texture;
	std::string obj;

};

struct complex_mesh_data {

	std::string tex1;
	std::string tex2;
	std::string obj;
	std::string shader_vs;
	std::string shader_fs;

};


std::map<std::string name, simple_mesh_data> simple_mesh_map {

	{ "cube",   { "cube_tex",  "cube.obj"}   },
	{ "goomba", { "oomba_tex", "goomba.obj"} },
	{ "roomba", { "oomba_tex", "roomba.obj"} },
	{ "bench",  { "bench_tex", "bench.obj"}  },
	{ "sword",  { "texture",   "sword.obj"}  },

};



