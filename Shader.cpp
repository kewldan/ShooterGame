#include "Shader.h"

#define SHADER_PART_VERTEX 1
#define SHADER_PART_GEOMETRY 2
#define SHADER_PART_FRAGMENT 4

Shader::Shader(const char* filename) {
	this->filename = filename;
	shaderParts = 0;
	program = glCreateProgram();

	uniforms = new Hashtable();

	vertex = -1;
	geometry = -1;
	fragment = -1;
	blockIndex = 0;
	
	char* path = new char[128];
	strcpy(path, filename);
	strcat(path, ".vert");
	if (std::filesystem::exists(path)) {
		std::ifstream in(path);
		std::string contents((std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());
		vertex = glCreateShader(GL_VERTEX_SHADER);
		const char* shader_source = contents.c_str();
		glShaderSource(vertex, 1, &shader_source, nullptr);
		glCompileShader(vertex);

		int length = 0;
		glGetShaderiv(vertex, GL_INFO_LOG_LENGTH, &length);
		if (length > 0) {
			char* log = new char[length];
			glGetShaderInfoLog(vertex, length, nullptr, log);
			PLOG_WARNING << "Vertex shader log:\n" << log;
		}

		int success = 0;
		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (success == GL_TRUE) {
			glObjectLabelBuild(GL_SHADER, vertex, "Shader (V)", filename);
			glAttachShader(program, vertex);
			shaderParts += SHADER_PART_VERTEX;
		}
		else {
			PLOG_WARNING << "Vertex shader found, but not attached";
		}
	}
	path[strlen(path) - 5] = 0;
	strcat(path, ".frag");
	if (std::filesystem::exists(path)) {
		std::ifstream in(path);
		std::string contents((std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		const char* shader_source = contents.c_str();
		glShaderSource(fragment, 1, &shader_source, nullptr);
		glCompileShader(fragment);

		int length = 0;
		glGetShaderiv(fragment, GL_INFO_LOG_LENGTH, &length);
		if (length > 0) {
			char* log = new char[length];
			glGetShaderInfoLog(fragment, length, nullptr, log);
			PLOG_WARNING << "Fragment shader log:\n" << log;
		}

		int success = 0;
		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (success == GL_TRUE) {
			glObjectLabelBuild(GL_SHADER, fragment, "Shader (F)", filename);
			glAttachShader(program, fragment);
			shaderParts += SHADER_PART_FRAGMENT;
		}
		else {
			PLOG_WARNING << "Fragment shader found, but not attached";
		}
	}
	path[strlen(path) - 5] = 0;
	strcat(path, ".geom");
	if (std::filesystem::exists(path)) {
		std::ifstream in(path);
		std::string contents((std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());
		geometry = glCreateShader(GL_GEOMETRY_SHADER);
		const char* shader_source = contents.c_str();
		glShaderSource(geometry, 1, &shader_source, nullptr);
		glCompileShader(geometry);

		int length = 0;
		glGetShaderiv(geometry, GL_INFO_LOG_LENGTH, &length);
		if (length > 0) {
			char* log = new char[length];
			glGetShaderInfoLog(geometry, length, nullptr, log);
			PLOG_WARNING << "Geometry shader log:\n" << log;
		}

		int success = 0;
		glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
		if (success == GL_TRUE) {
			glObjectLabelBuild(GL_SHADER, geometry, "Shader (G)", filename);
			glAttachShader(program, geometry);
			shaderParts += SHADER_PART_GEOMETRY;
		}
		else {
			PLOG_WARNING << "Geometry shader found, but not attached";
		}
	}

	glLinkProgram(program);
	glObjectLabelBuild(GL_PROGRAM, program, "Program", filename);
	if(shaderParts == 0){
		PLOGW << "Empty shader [" << filename << "] linked";
	}
	else {
		PLOGI << "Shader [" << filename << "] linked ["
			<< ((shaderParts & SHADER_PART_VERTEX) != 0 ? 'V' : '\0')
			<< ((shaderParts & SHADER_PART_GEOMETRY) != 0 ? 'G' : '\0')
			<< ((shaderParts & SHADER_PART_FRAGMENT) != 0 ? 'F' : '\0')
			<< ']';
	}
}

GLuint Shader::getProgramId() const {
	return program;
}

GLint Shader::getAttribLocation(const char* name) const {
	GLint value = glGetAttribLocation(program, name);
	if (value == -1) {
		PLOGE << "Attrib location in shader not found > " << name;
	}
	return value;
}

GLint Shader::getUniformLocation(const char* name) const {
	if (uniforms->has(name)) {
		return uniforms->get(name);
	}
	GLint value = glGetUniformLocation(program, name);
	if (value == -1) {
		PLOGE << "Uniform location in shader [" << filename << "] not found > " << name;
	}
	uniforms->put(name, value);
	return value;
}

void Shader::bind() {
	glUseProgram(program);
}

Shader::~Shader() {
	if ((shaderParts & SHADER_PART_VERTEX) != 0) {
		glDetachShader(program, vertex);
	}
	if ((shaderParts & SHADER_PART_FRAGMENT) != 0) {
		glDetachShader(program, fragment);
	}
	if ((shaderParts & SHADER_PART_GEOMETRY) != 0) {
		glDetachShader(program, geometry);
	}
	glDeleteProgram(program);
}

void Shader::upload(const  char* name, int value) const {
	glUniform1i(getUniformLocation(name), value);
}

void Shader::upload(const  char* name, float value) const {
	glUniform1f(getUniformLocation(name), value);
}

void Shader::upload(const  char* name, glm::vec2 value) const {
	glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::upload(const  char* name, glm::vec3 value) const {
	glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::upload(const  char* name, glm::vec4 value) const {
	glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::upload(const  char* name, glm::mat4 value) const {
	glUniformMatrix4fv(getUniformLocation(name), 1, false, glm::value_ptr(value));
}

void Shader::uploadMat4(const char* name, float* value) const
{
	glUniformMatrix4fv(getUniformLocation(name), 1, false, value);
}

void Shader::draw(Model* model) const {
	uploadMat4("mvp", model->getMVP());
	for (int i = 0; i < model->nbMeshes; i++) {
		if (model->meshes[i].hasTexture()) {
			model->meshes[i].texture->bind();
			upload("hasTexture", 1);
		}
		else {
			upload("hasTexture", 0);
		}
		model->meshes[i].draw();
	}
}

void Shader::bindUniformBlock(const char* name)
{
	unsigned int bindingPoint = glGetUniformBlockIndex(program, name);
	glUniformBlockBinding(program, bindingPoint, blockIndex++);
}

void Shader::upload(const  char* name, float x, float y) const {
	glUniform2f(getUniformLocation(name), x, y);
}

void Shader::upload(const  char* name, float x, float y, float z) const {
	glUniform3f(getUniformLocation(name), x, y, z);
}

void Shader::upload(const  char* name, float x, float y, float z, float w) const {
	glUniform4f(getUniformLocation(name), x, y, z, w);
}

UniformBlock::UniformBlock(unsigned int size)
{
	glGenBuffers(1, &block);
	glBindBuffer(GL_UNIFORM_BUFFER, block);
	glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, block, 0, size);
	glBindBuffer(GL_UNIFORM_BUFFER, block);
	offset = 0;
}

void UniformBlock::add(unsigned int size, void* value)
{
	glBufferSubData(GL_UNIFORM_BUFFER, offset, size, value);
	offset += size;
}
