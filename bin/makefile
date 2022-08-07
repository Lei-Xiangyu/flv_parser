mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))
src_dir := $(mkfile_dir)../src

lib_flv_parser.so: $(src_dir)/format_demux_flv.cc $(src_dir)/format_demux_flv.h
	g++ -shared -fPIC -std=c++11 -o lib_flv_parser.so $(src_dir)/format_demux_flv.cc