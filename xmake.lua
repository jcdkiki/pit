target("pit")
	set_kind("binary")
	add_files("src/*.c")
	add_includedirs("src")

	add_links("OpenGL32", "GLu32", "user32", "kernel32", "gdi32")