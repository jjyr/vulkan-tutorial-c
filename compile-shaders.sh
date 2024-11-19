#! /bin/sh

glslangValidator -V shaders/shader.vert -o shaders/vert.spv
glslangValidator -V shaders/shader.frag -o shaders/frag.spv
