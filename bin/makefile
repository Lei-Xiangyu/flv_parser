mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))
src_dir := $(mkfile_dir)../src

flv_parser: $(src_dir)/main.cc $(src_dir)/format_demux_flv.cc $(src_dir)/format_demux_flv.h
	g++ -std=c++11 -g -o flv_parser $(src_dir)/format_demux_flv.cc $(src_dir)/main.cc

clean:
	rm $(mkfile_dir)/flv_parser core.*