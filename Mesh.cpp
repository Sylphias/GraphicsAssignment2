#include "Mesh.h"

using namespace std;

// method to split strings
std::vector<std::string> splitString(std::string s, std::string delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	size_t pos = 0;
	while ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		tokens.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	tokens.push_back(s);
	return tokens;
}
void Mesh::load( const char* filename )
{
	// 2.1.1. load() should populate bindVertices, currentVertices, and faces

	// Add your code here.
	// load the OBJ file here
	ifstream readFile;
	std::string line;
	readFile.open(filename);

	if (readFile.is_open()) {
		while (getline(readFile, line)) {
			// split the string
			std::vector<std::string> values = splitString(line, " ");
			if (values.empty()) {
				continue;
			}
			char state = ' ';
			float x = 0.0f;
			float y = 0.0f;
			float z = 0.0f;

			if (values[0].compare("v") == 0) {
				std::stringstream(values[1]) >> x;
				std::stringstream(values[2]) >> y;
				std::stringstream(values[3]) >> z;
				bindVertices.push_back(Vector3f(x, y, z));
			}
			else if (values[0].compare("f") == 0) {
				Tuple3u face(stoul(values[1]), stoul(values[2]), stoul(values[3]));
				faces.push_back(face);
			}

		}
	}
	
	// make a copy of the bind vertices as the current vertices
	currentVertices = bindVertices;

}

void Mesh::draw()
{
	// Since these meshes don't have normals
	// be sure to generate a normal per triangle.
	// Notice that since we have per-triangle normals
	// rather than the analytical normals from
	// assignment 1, the appearance is "faceted".

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glEnable(GL_LIGHTING);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);


	glBegin(GL_TRIANGLES);
	for (unsigned i = 0; i<faces.size(); i++)
	{
		Vector3f U = currentVertices[faces[i][0] - 1] - currentVertices[faces[i][1] - 1];
		Vector3f V = currentVertices[faces[i][2] - 1] - currentVertices[faces[i][1] - 1];
		glNormal3fv(-1*Vector3f::cross(U, V).normalized());
		glVertex3fv(currentVertices[faces[i][0]-1]);
		glVertex3fv(currentVertices[faces[i][1]-1]);
		glVertex3fv(currentVertices[faces[i][2]-1]);

	}
	glEnd();

	glPopAttrib();

}

void Mesh::loadAttachments( const char* filename, int numJoints )
{
	// 2.2. Implement this method to load the per-vertex attachment weights
	// this method should update m_mesh.attachments
	ifstream readFile;
	std::string line;
	readFile.open(filename);
	if (readFile.is_open()) {
		while (getline(readFile, line)) {
			// split the string
			std::vector<std::string> values = splitString(line, " ");
			std::vector<float> f;
			for (unsigned i = 0; i < values.size(); i++) {
				if (!values[i].empty()) {
					f.push_back(stof(values[i]));
				}
			}
			attachments.push_back(f);
		}
	}
}
