#include "../Include/Common.h"

//For GLUT to handle 
#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define MENU_ANIME_SUPER 4
#define MENU_ANIME_STOP 5

#define ANIME_TIME 51

//GLUT timer variable
float timer_cnt = 0;
float anime_timer_cnt = 0;
bool timer_enabled = true;
bool anime_timer_enabled = false;
unsigned int timer_speed = 16;
unsigned int anime_timer_speed = 16;


using namespace glm;
using namespace std;

mat4 view(1.0f);			// V of MVP, viewing matrix
mat4 projection(1.0f);		// P of MVP, projection matrix
mat4 model(1.0f);			// M of MVP, model matrix
vec3 temp = vec3();			// a 3 dimension vector which represents how far did the ladybug should move
float key_rotate_angle_y = 0;
float key_rotate_angle_z = 0;
vec2 pre_pos = vec2();
vec2 mouse_move_dir = vec2();
float mouse_move_dist;
bool mouse_rotating;

GLint um4p;
GLint um4mv;

GLuint program;			// shader program id

typedef struct
{
	GLuint vao;			// vertex array object
	GLuint vbo;			// vertex buffer object

	int materialId;
	int vertexCount;
	GLuint m_texture;
} Shape;

Shape cube_shape;
Shape sphere_shape;
Shape cylinder_shape;
Shape plane_shape;
Shape capsule_shape;

// Load shader file to program
char** loadShaderSource(const char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *src = new char[sz + 1];
	fread(src, sizeof(char), sz, fp);
	src[sz] = '\0';
	char **srcp = new char*[1];
	srcp[0] = src;
	return srcp;
}

// Free shader file
void freeShaderSource(char** srcp)
{
	delete srcp[0];
	delete srcp;
}

// Load .obj model
void My_LoadModels(Shape &part_shape, string part)
{
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string warn;
	string err;
	bool ret;
	if (part == "cube")
		ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "Cube.obj");
	else if (part == "sphere")
		ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "Sphere.obj");
	else if (part == "plane")
		ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "Plane.obj");
	else if (part == "cylinder") 
		ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "Cylinder.obj");
	else if (part == "capsule")
		ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "Capsule.obj");
	if (!warn.empty()) 
		cout << warn << endl;
	if (!err.empty()) 
		cout << err << endl;
	if (!ret) 
		exit(1);

	vector<float> vertices, texcoords, normals;  // if OBJ preserves vertex order, you can use element array buffer for memory efficiency
	for (int s = 0; s < shapes.size(); ++s) {  // for 'ladybug.obj', there is only one object
		int index_offset = 0;
		for (int f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			for (int v = 0; v < fv; ++v) {
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
				texcoords.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
				texcoords.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 2]);
			}
			index_offset += fv;
			part_shape.vertexCount += fv;
		}
	}
	glGenVertexArrays(1, &part_shape.vao);
	glBindVertexArray(part_shape.vao);

	glGenBuffers(1, &part_shape.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, part_shape.vbo);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + texcoords.size() * sizeof(float) + normals.size() * sizeof(float), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), texcoords.size() * sizeof(float), texcoords.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + texcoords.size() * sizeof(float), normals.size() * sizeof(float), normals.data());

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float) + texcoords.size() * sizeof(float)));
	glEnableVertexAttribArray(2);

	shapes.clear();
	shapes.shrink_to_fit();
	materials.clear();
	materials.shrink_to_fit();
	vertices.clear();
	vertices.shrink_to_fit();
	texcoords.clear();
	texcoords.shrink_to_fit();
	normals.clear();
	normals.shrink_to_fit();

	cout << "Load " << part_shape.vertexCount << " vertices" << endl;
	
	texture_data tdata = loadImg("Texture-01.jpg");

	glGenTextures(1, &part_shape.m_texture);
	glBindTexture(GL_TEXTURE_2D, part_shape.m_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	delete tdata.data;
}

// OpenGL initialization
void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Create Shader Program
	program = glCreateProgram();

	// Create customize shader by tell openGL specify shader type 
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load shader file
	char** vertexShaderSource = loadShaderSource("vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("fragment.fs.glsl");

	// Assign content of these shader files to those shaders we created before
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);

	// Free the shader file string(won't be used any more)
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);

	// Compile these shaders
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	// Logging
	shaderLog(vertexShader);
	shaderLog(fragmentShader);

	// Assign the program we created before with these shaders
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	// Get the id of inner variable 'um4p' and 'um4mv' in shader programs
	um4p = glGetUniformLocation(program, "um4p");
	um4mv = glGetUniformLocation(program, "um4mv");

	// Tell OpenGL to use this shader program now
	glUseProgram(program);

	My_LoadModels(cube_shape, "cube");
	My_LoadModels(sphere_shape, "sphere");
	My_LoadModels(cylinder_shape, "cylinder");
	My_LoadModels(plane_shape, "plane");
	My_LoadModels(capsule_shape, "capsule");
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	// Clear display buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Tell openGL to use the shader program we created before
	glUseProgram(program);
	mat4 translation_matrix;
	mat4 scale_matrix;
	mat4 rotation_matrix;
	vec3 rotate_axis;
	mat4 anime_rotation_matrix;
	vec3 anime_rotate_axis;

	/// draw body
	glBindVertexArray(sphere_shape.vao);
	translation_matrix = translate(mat4(1.0f), vec3(2.0, 3.0-1.8*anime_timer_cnt/ANIME_TIME, 0.0-3.7*anime_timer_cnt/ANIME_TIME) + temp);
	scale_matrix = scale(mat4(1.0f), vec3(1.5, 1.5, 3.6));
	rotate_axis = vec3(0.0, 0.0, 1.0);
	rotation_matrix = rotate(mat4(1.0f), radians(key_rotate_angle_z), rotate_axis);
	rotate_axis = vec3(0.0, 1.0, 0.0);
	rotation_matrix *= rotate(mat4(1.0f), radians(key_rotate_angle_y), rotate_axis);
	anime_rotate_axis = vec3(-1.0, 0.0, 0.0);
	anime_rotation_matrix = rotate(mat4(1.0f), radians(45 * anime_timer_cnt / ANIME_TIME), anime_rotate_axis);
	mat4 chest =  translation_matrix * rotation_matrix * anime_rotation_matrix;
	model = chest * scale_matrix;
	// Transfer value of (view*model) to both shader's inner variable 'um4mv'; 
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));
	// Transfer value of projection to both shader's inner variable 'um4p';
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	// Tell openGL to draw the vertex array we had binded before
	glDrawArrays(GL_TRIANGLES, 0, sphere_shape.vertexCount);
	
	glBindVertexArray(cylinder_shape.vao);
	translation_matrix = translate(mat4(1.0f), vec3(0.0, -1.5, 0.0));
	scale_matrix = scale(mat4(1.0f), vec3(1.5, 1.8, 2.8));
	//vec3 rotate_axis = vec3(0.0, 1.0, 0.0);
	//rotation_matrix = rotate(mat4(1.0f), radians(timer_cnt), rotate_axis);
	mat4 belly = chest * translation_matrix;
	model = belly * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, sphere_shape.vertexCount);

	/// draw head
	glBindVertexArray(sphere_shape.vao);
	translation_matrix = translate(mat4(1.0f), vec3(-1, 0.6, 0));
	scale_matrix = scale(mat4(1.0f), vec3(1, 1.2, 1));
	mat4 head = chest * translation_matrix;
	model = head * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, sphere_shape.vertexCount);
	
	/// draw shoulder
	translation_matrix = translate(mat4(1.0f), vec3(0, -0.6, 2.7));
	scale_matrix = scale(mat4(1.0f), vec3(3, 3, 3));
	anime_rotate_axis = vec3(-1.0, 0.0, 0.0);
	anime_rotation_matrix = rotate(mat4(1.0f), radians(190 * anime_timer_cnt / ANIME_TIME), anime_rotate_axis);
	mat4 left_shoulder_joint = chest * translation_matrix * anime_rotation_matrix;
	model = chest * translation_matrix * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, sphere_shape.vertexCount);

	translation_matrix = translate(mat4(1.0f), vec3(0, -0.6, -2.7));
	scale_matrix = scale(mat4(1.0f), vec3(3, 3, 3));
	anime_rotate_axis = vec3(1.0, 0.0, 0.0);
	anime_rotation_matrix = rotate(mat4(1.0f), radians(190 * anime_timer_cnt / ANIME_TIME), anime_rotate_axis);
	mat4 right_shoulder_joint = chest * translation_matrix * anime_rotation_matrix;
	model = chest * translation_matrix * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, sphere_shape.vertexCount);

	///draw arm1
	glBindVertexArray(cylinder_shape.vao);
	translation_matrix = translate(mat4(1.0f), vec3(0, -1.2, 0.6));
	scale_matrix = scale(mat4(1.0f), vec3(0.8, 0.8, 0.8));
	rotate_axis = vec3(1.0, 0.0, 0.0);
	rotation_matrix = rotate(mat4(1.0f), radians(-30.0f), rotate_axis);
	mat4 left_arm1 = left_shoulder_joint * translation_matrix;
	model = left_arm1 * rotation_matrix * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, cylinder_shape.vertexCount);

	translation_matrix = translate(mat4(1.0f), vec3(0, -1.2, -0.6));
	scale_matrix = scale(mat4(1.0f), vec3(0.8, 0.8, 0.8));
	rotate_axis = vec3(1.0, 0.0, 0.0);
	rotation_matrix = rotate(mat4(1.0f), radians(30.0f), rotate_axis);
	mat4 right_arm1 = right_shoulder_joint * translation_matrix;
	model = right_arm1 * rotation_matrix * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, cylinder_shape.vertexCount);

	///draw elbow
	translation_matrix = translate(mat4(1.0f), vec3(0, -1.2, 0.2));
	scale_matrix = scale(mat4(1.0f), vec3(1.3, 0.39, 1.3));
	rotate_axis = vec3(0.0, 0.0, 1.0);
	rotation_matrix = rotate(mat4(1.0f), radians(90.0f), rotate_axis);
	anime_rotate_axis = vec3(1.0, 0.0, 0.0);
	anime_rotation_matrix = rotate(mat4(1.0f), radians(10 * anime_timer_cnt / ANIME_TIME), anime_rotate_axis);
	mat4 left_elbow = left_arm1 * translation_matrix * anime_rotation_matrix;
	model = left_elbow * rotation_matrix * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, cylinder_shape.vertexCount);

	translation_matrix = translate(mat4(1.0f), vec3(0, -1.2, -0.2));
	scale_matrix = scale(mat4(1.0f), vec3(1.3, 0.39, 1.3));
	rotate_axis = vec3(0.0, 0.0, -1.0);
	rotation_matrix = rotate(mat4(1.0f), radians(90.0f), rotate_axis);
	anime_rotate_axis = vec3(-1.0, 0.0, 0.0);
	anime_rotation_matrix = rotate(mat4(1.0f), radians(10 * anime_timer_cnt / ANIME_TIME), anime_rotate_axis);
	mat4 right_elbow = right_arm1 * translation_matrix * anime_rotation_matrix;
	model = right_elbow * rotation_matrix * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, cylinder_shape.vertexCount);

	/// draw arm2
	glBindVertexArray(cube_shape.vao);
	translation_matrix = translate(mat4(1.0f), vec3(0, -1.2, 0));
	scale_matrix = scale(mat4(1.0f), vec3(2, 2.4, 2));
	mat4 left_arm2 = left_elbow * translation_matrix;
	model = left_arm2 * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, cube_shape.vertexCount);

	translation_matrix = translate(mat4(1.0f), vec3(0, -1.2, 0));
	scale_matrix = scale(mat4(1.0f), vec3(2, 2.4, 2));
	mat4 right_arm2 = right_elbow * translation_matrix;
	model = right_arm2 * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, cube_shape.vertexCount);

	/// draw hand
	glBindVertexArray(sphere_shape.vao);
	translation_matrix = translate(mat4(1.0f), vec3(0, -1.5, 0));
	scale_matrix = scale(mat4(1.0f), vec3(1.5, 1.5, 1.5));
	mat4 left_hand = left_arm2 * translation_matrix;
	model = left_hand * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, sphere_shape.vertexCount);

	translation_matrix = translate(mat4(1.0f), vec3(0, -1.5, 0));
	scale_matrix = scale(mat4(1.0f), vec3(1.5, 1.5, 1.5));
	mat4 right_hand = right_arm2 * translation_matrix;
	model = right_hand * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, sphere_shape.vertexCount);

	/// draw hip
	glBindVertexArray(sphere_shape.vao);
	translation_matrix = translate(mat4(1.0f), vec3(0, -2, 0.7));
	scale_matrix = scale(mat4(1.0f), vec3(0.7, 0.7, 0.7));
	anime_rotate_axis = vec3(1.0, 0.0, 0.0);
	anime_rotation_matrix = rotate(mat4(1.0f), radians(10 * anime_timer_cnt / ANIME_TIME), anime_rotate_axis);
	mat4 left_hip = belly * translation_matrix * anime_rotation_matrix;
	model = left_hip * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, sphere_shape.vertexCount);

	translation_matrix = translate(mat4(1.0f), vec3(0, -2, -0.7));
	scale_matrix = scale(mat4(1.0f), vec3(0.7, 0.7, 0.7));
	anime_rotate_axis = vec3(1.0, 0.0, 0.0);
	if (anime_timer_cnt*3/2 <= ANIME_TIME)
		anime_rotation_matrix = rotate(mat4(1.0f), radians(75 * anime_timer_cnt / ANIME_TIME*3/2), anime_rotate_axis);
	else
		anime_rotation_matrix = rotate(mat4(1.0f), radians(75.0f), anime_rotate_axis);
	mat4 right_hip = belly * translation_matrix * anime_rotation_matrix;
	model = right_hip * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, sphere_shape.vertexCount);

	/// draw thigh
	glBindVertexArray(capsule_shape.vao);
	translation_matrix = translate(mat4(1.0f), vec3(0, -1, 0));
	scale_matrix = scale(mat4(1.0f), vec3(0.7, 1, 0.7));
	mat4 left_thigh = left_hip * translation_matrix;
	model = left_thigh * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, capsule_shape.vertexCount);

	translation_matrix = translate(mat4(1.0f), vec3(0, -1, 0));
	scale_matrix = scale(mat4(1.0f), vec3(0.7, 1, 0.7));
	mat4 right_thigh = right_hip * translation_matrix;
	model = right_thigh * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, capsule_shape.vertexCount);

	/// draw knee
	glBindVertexArray(sphere_shape.vao);
	translation_matrix = translate(mat4(1.0f), vec3(0, -1, 0));
	scale_matrix = scale(mat4(1.0f), vec3(0.5, 0.5, 0.5));
	mat4 left_knee = left_thigh * translation_matrix;
	model = left_knee * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, sphere_shape.vertexCount);

	translation_matrix = translate(mat4(1.0f), vec3(0, -1, 0));
	scale_matrix = scale(mat4(1.0f), vec3(0.5, 0.5, 0.5));
	anime_rotate_axis = vec3(-1.0, 0.0, 0.0);
	if (anime_timer_cnt * 2 <= ANIME_TIME)
		anime_rotation_matrix = rotate(mat4(1.0f), radians(90 * anime_timer_cnt / ANIME_TIME*2), anime_rotate_axis);
	else 
		anime_rotation_matrix = rotate(mat4(1.0f), radians(90.0f), anime_rotate_axis);
	mat4 right_knee = right_thigh * translation_matrix * anime_rotation_matrix;
	model = right_knee * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, sphere_shape.vertexCount);

	/// draw calf
	glBindVertexArray(capsule_shape.vao);
	translation_matrix = translate(mat4(1.0f), vec3(0, -1, 0));
	scale_matrix = scale(mat4(1.0f), vec3(0.4, 1, 0.4));
	mat4 left_calf = left_knee * translation_matrix;
	model = left_calf * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, capsule_shape.vertexCount);

	translation_matrix = translate(mat4(1.0f), vec3(0, -1, 0));
	scale_matrix = scale(mat4(1.0f), vec3(0.4, 1, 0.4));
	mat4 right_calf = right_knee * translation_matrix;
	model = right_calf * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, capsule_shape.vertexCount);

	/// draw foot
	glBindVertexArray(cylinder_shape.vao);
	translation_matrix = translate(mat4(1.0f), vec3(-0.2, -1, 0));
	scale_matrix = scale(mat4(1.0f), vec3(1, 0.1, 0.4));
	mat4 left_foot = left_calf * translation_matrix;
	model = left_foot * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, cylinder_shape.vertexCount);

	translation_matrix = translate(mat4(1.0f), vec3(-0.2, -1, 0));
	scale_matrix = scale(mat4(1.0f), vec3(1, 0.1, 0.4));
	mat4 right_foot = right_calf * translation_matrix;
	model = right_foot * scale_matrix;
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view* model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, cylinder_shape.vertexCount);



	// Change current display buffer to another one (refresh frame) 
	glutSwapBuffers();
}

// Setting up viewing matrix
void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	float viewportAspect = (float)width / (float)height;

	// perspective(fov, aspect_ratio, near_plane_distance, far_plane_distance)
	// ps. fov = field of view, it represent how much range(degree) is this camera could see 
	projection = perspective(radians(60.0f), viewportAspect, 0.1f, 1000.0f);

	// lookAt(camera_position, camera_viewing_vector, up_vector)
	// up_vector represent the vector which define the direction of 'up'
	view = lookAt(vec3(-10.0f, 5.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
}

void My_Timer(int val)
{
	timer_cnt += 1.0f;
	glutPostRedisplay();
	if (timer_enabled)
	{
		glutTimerFunc(timer_speed, My_Timer, val);
	}
}

void Anime_Timer(int val) {
	anime_timer_cnt += 1.0f;
	glutPostRedisplay();
	if (anime_timer_enabled) {
		glutTimerFunc(anime_timer_speed, Anime_Timer, val);
	}
	if (anime_timer_cnt == ANIME_TIME) {
		anime_timer_enabled = false;
	}
}

void My_Keyboard(unsigned char key, int x, int y)
{
	printf("Key %c is pressed at (%d, %d)\n", key, x, y);
	if (key == 'd')
	{
		temp = temp + vec3(0, 0, 1);
	}
	else if (key == 'a')
	{
		temp = temp - vec3(0, 0, 1);
	}
	else if (key == 'w')
	{
		temp = temp + vec3(1, 0, 0);
	}
	else if (key == 's')
	{
		temp = temp - vec3(1, 0, 0);
	}
}

void My_SpecialKeys(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_UP:
		key_rotate_angle_z -= 10;
		printf("Up arrow is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_DOWN:
		key_rotate_angle_z += 10;
		printf("Down arrow is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		key_rotate_angle_y -= 10;
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_RIGHT:
		key_rotate_angle_y += 10;
		printf("Right arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	switch (id)
	{
	case MENU_TIMER_START:
		if (!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	case MENU_ANIME_SUPER:
		// do something
		if (!anime_timer_enabled)
		{
			anime_timer_cnt = 0;
			anime_timer_enabled = true;
			glutTimerFunc(anime_timer_speed, Anime_Timer, 0);
		}
		break;
	case MENU_ANIME_STOP:
		// do something
		anime_timer_cnt = 0;
		break;
	default:
		break;
	}
}

void My_Mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			mouse_rotating = true;
			pre_pos = vec2(x, y);
			printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
		}
		else if (state == GLUT_UP)
		{
			mouse_move_dir = vec2(x, y) - pre_pos;
			mouse_move_dist = sqrt(pow(x - pre_pos.x, 2) + pow(y - pre_pos.y, 2))/10;
			printf("Mouse %d is released at (%d, %d)\n", button, x, y);
		}
	}
}

void My_newMenu(int id)
{

}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
	// Change working directory to source code path
	chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(600, 600);
	glutCreateWindow("Practice"); // You cannot use OpenGL functions before this line;
								  // The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	dumpInfo();
	My_Init();

	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);
	int menu_anime = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddSubMenu("Anime", menu_anime);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_anime);						// Tell GLUT to design the menu which id==menu_new now
	glutAddMenuEntry("Super!", MENU_ANIME_SUPER);		// Add submenu "Hello" in "New"(a menu which index is equal to menu_new)
	glutAddMenuEntry("Normal", MENU_ANIME_STOP);		// Add submenu "Hello" in "New"(a menu which index is equal to menu_new)

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0);
	// Todo
	// Practice 1 : Register new GLUT event listner here
	// ex. glutXXXXX(my_Func);
	// Remind : you have to implement my_Func
	glutMouseFunc(My_Mouse);

	// Enter main event loop.
	glutMainLoop();

	return 0;
}