#ifndef OPENGL_SHADER_H
#define OPENGL_SHADER_H

#include "glad/glad.h"
#include "Camera.h"
#include "Model.h"
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <plog/Log.h>
#ifdef RMT_PROFILER
#include "Remotery.h"
#else
#define rmt_ScopedCPUSample(n, n1)
#endif

class UniformBlock {
	unsigned int offset;
public:
	unsigned int block;
	UniformBlock(unsigned int size);
	void add(unsigned int size, void* value);
};

class Hashtable {
	std::unordered_map<const char*, int> htmap;

public:
	void put(const char* key, int value) {
		htmap[key] = value;
	}

	const int get(const char* key) {
		return htmap[key];
	}

	bool has(const char* key) {
		return htmap.contains(key);
	}

};

class Shader {
	unsigned int vertex, fragment, geometry, program, blockIndex;
	Hashtable* uniforms;
	int8_t shaderParts;
	const char* filename;

	int getUniformLocation(const char* name) const;

	int getAttribLocation(const char* name) const;

public:
	Shader(const char* filename);
	~Shader();

	unsigned int getProgramId() const;

	void bind();

	void upload(const char* name, int value) const;

	void upload(const char* name, float value) const;

	void upload(const char* name, glm::vec2 value) const;

	void upload(const char* name, float x, float y) const;

	void upload(const char* name, glm::vec3 value) const;

	void upload(const char* name, float x, float y, float z) const;

	void upload(const char* name, glm::vec4 value) const;

	void upload(const char* name, float x, float y, float z, float w) const;

	void upload(const char* name, glm::mat4 value) const;

	void uploadMat4(const char* name, float* value) const;

	void draw(Model* model) const;

	void bindUniformBlock(const char* name);
};


#endif //OPENGL_SHADER_H
