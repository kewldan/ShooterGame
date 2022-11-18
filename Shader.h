#ifndef OPENGL_SHADER_H
#define OPENGL_SHADER_H

#include "glad/gl.h"
#include "Camera.h"
#include "Model.h"
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <plog/Log.h>

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
	unsigned int vertex, fragment, geometry, program;
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

	void unbind();

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
};


#endif //OPENGL_SHADER_H
