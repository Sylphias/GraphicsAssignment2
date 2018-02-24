#include "SkeletalModel.h"

#include <FL/Fl.H>

using namespace std;

// method to split strings
static std::vector<std::string> splitString(std::string s, std::string delimiter)
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

void SkeletalModel::load(const char *skeletonFile, const char *meshFile, const char *attachmentsFile)
{
	loadSkeleton(skeletonFile);

	m_mesh.load(meshFile);
	m_mesh.loadAttachments(attachmentsFile, m_joints.size());

	computeBindWorldToJointTransforms();
	updateCurrentJointToWorldTransforms();
}

void SkeletalModel::draw(Matrix4f cameraMatrix, bool skeletonVisible)
{
	// draw() gets called whenever a redraw is required
	// (after an update() occurs, when the camera moves, the window is resized, etc)

	m_matrixStack.clear();
	m_matrixStack.push(cameraMatrix);

	if( skeletonVisible )
	{
		drawJoints();

		drawSkeleton();
	}
	else
	{
		// Clear out any weird matrix we may have been using for drawing the bones and revert to the camera matrix.
		glLoadMatrixf(m_matrixStack.top());

		// Tell the mesh to draw itself.
		m_mesh.draw();
	}
}

void SkeletalModel::loadSkeleton( const char* filename )
{
	// Load the skeleton from file here.

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
			Joint *m_joint = new Joint;
			Matrix4f trans = Matrix4f::identity();
			trans.setCol(3, Vector4f(stof(values[0]), stof(values[1]), stof(values[2]), 1.0f));
			m_joint->transform = trans;
			m_joints.push_back(m_joint);
			// skip if root node and set root node variable
			if (stoi(values[3]) == -1) {
				m_rootJoint = m_joint;
				continue;
			}
			// Insert itself to the list of children of its parent
			m_joints[stoi(values[3])]->children.push_back(m_joint);
		}
	}
}

void drawSkeletonCubes(MatrixStack ms , Joint *J) {

	Vector3f parOff = J->transform.getCol(3).xyz();
	Vector3f rnd(0.0f, 0.0f, 1.0f);
	Vector3f z = parOff.normalized();
	Matrix4f scalingMat(0.05f, 0.0f, 0.0f, 0.0f
		, 0.0f, 0.05f, 0.0f, 0.0f
		, 0.0f, 0.0f, parOff.abs(), 0.0f
		, 0.0f, 0.0f, 0.0f, 1);
	Matrix4f translateMat(1.0f, 0.0f, 0.0f, 0.0f
		, 0.0f, 1.0f, 0.0f, 0.0f
		, 0.0f, 0.0f,1.0f,0.5f
		, 0.0f, 0.0f, 0.0f, 1);
	Vector3f yRot = Vector3f::cross(z, rnd).normalized();
	Vector3f xRot = Vector3f::cross(yRot, z).normalized();
	Matrix4f rotationMat(Vector4f(xRot,0.0f),Vector4f(yRot,0.0f),-1*Vector4f(z,0),Vector4f(0,0,0,1));
	//Apply a local transformation
	Matrix3f id = Matrix3f::identity();
	Matrix4f localTransform(Vector4f(id.getCol(0), 0), Vector4f(id.getCol(1), 0), Vector4f(id.getCol(2), 0), Vector4f(parOff, 1));
	ms.push(localTransform);
	ms.push(rotationMat);
	ms.push(scalingMat);
	ms.push(translateMat);
	// Draw the cube with transform
	glLoadMatrixf(ms.top());
	glutSolidCube(1.0f);
	ms.pop();
	ms.pop();
	ms.pop();
	ms.pop();
	// Apply relative transform before drawing all the children
	ms.push(J->transform);
	for (unsigned i = 0; i < J->children.size(); i++) {
		drawSkeletonCubes(ms, J->children[i]);
	}
	ms.pop();


}

void drawJointSpheres(MatrixStack ms, Joint *J) {
	// Draw the current sphere and move it


	ms.push(J->transform);
	glLoadMatrixf(ms.top());
	glutSolidSphere(0.025f, 12, 12);

	// go into the children and draw each sphere
	for (unsigned i = 0; i < J->children.size(); i++) {
		drawJointSpheres(ms, J->children[i]);
	}
	ms.pop();
}


void SkeletalModel::drawJoints( )
{
	// Draw a sphere at each joint. You will need to add a recursive helper function to traverse the joint hierarchy.
	drawJointSpheres(m_matrixStack,m_rootJoint);
	//
	// We recommend using glutSolidSphere( 0.025f, 12, 12 )
	// to draw a sphere of reasonable size.
	//
	// You are *not* permitted to use the OpenGL matrix stack commands
	// (glPushMatrix, glPopMatrix, glMultMatrix).
	// You should use your MatrixStack class
	// and use glLoadMatrix() before your drawing call.

	
}

void SkeletalModel::drawSkeleton( )
{
	// Draw boxes between the joints. You will need to add a recursive helper function to traverse the joint hierarchy.
	// Dont draw root node
	m_matrixStack.push(m_rootJoint->transform);
	for (int i =0; i < m_rootJoint->children.size(); i++) {
		drawSkeletonCubes(m_matrixStack, m_rootJoint->children[i]);
	}
	m_matrixStack.pop();
}

void SkeletalModel::setJointTransform(int jointIndex, float rX, float rY, float rZ)
{
	// Set the rotation part of the joint's transformation matrix based on the passed in Euler angles.
	Matrix4f rotation = Matrix4f::rotateZ(rZ)* Matrix4f::rotateY(rY) * Matrix4f::rotateX(rX);
	Matrix4f rotMat(rotation.getCol(0), rotation.getCol(1), rotation.getCol(2), m_joints[jointIndex]->transform.getCol(3));
	m_joints[jointIndex]->transform = rotMat;
}


void updateBindWTJTransform(MatrixStack ms,Joint *j) {
	ms.push(j->transform);
	j->bindWorldToJointTransform = ms.top().inverse();
	for (int i = 0; i <j->children.size(); i++) {
		updateBindWTJTransform(ms, j->children[i]);
	}
	ms.pop();
}

void SkeletalModel::computeBindWorldToJointTransforms()
{
	// 2.3.1. Implement this method to compute a per-joint transform from
	// world-space to joint space in the BIND POSE.
	//
	// Note that this needs to be computed only once since there is only
	// a single bind pose.
	//
	// This method should update each joint's bindWorldToJointTransform.
	// You will need to add a recursive helper function to traverse the joint hierarchy.
	MatrixStack ms;
	updateBindWTJTransform(ms,m_rootJoint);

}

void updateCurrentJTWTransform(MatrixStack ms, Joint *j) {
	ms.push(j->transform);
	j->currentJointToWorldTransform = ms.top();
	for (int i = 0; i <j->children.size(); i++) {
		updateCurrentJTWTransform(ms, j->children[i]);
	}
	ms.pop();
}

void SkeletalModel::updateCurrentJointToWorldTransforms()
{
	// 2.3.2. Implement this method to compute a per-joint transform from
	// joint space to world space in the CURRENT POSE.
	//
	// The current pose is defined by the rotations you've applied to the
	// joints and hence needs to be *updated* every time the joint angles change.
	//
	// This method should update each joint's bindWorldToJointTransform.
	// You will need to add a recursive helper function to traverse the joint hierarchy.
	MatrixStack ms;
	updateCurrentJTWTransform(ms, m_rootJoint);
}

void SkeletalModel::updateMesh()
{
	// 2.3.2. This is the core of SSD.
	// Implement this method to update the vertices of the mesh
	// given the current state of the skeleton.
	// You will need both the bind pose world --> joint transforms.
	// and the current joint --> world transforms.

	// For all vertex we will calculate the changes in each joint and update the currentVert
	for (unsigned vertIdx = 0; vertIdx < m_mesh.bindVertices.size(); vertIdx++) {
		Vector4f sumVert(0,0,0,0);
		for (int jointIdx = 1; jointIdx < m_joints.size(); jointIdx++) {
			float weight = m_mesh.attachments[vertIdx][jointIdx-1];
			if ( weight == 0) {continue;} // skip if weights are 0, reduce computation time
			sumVert= sumVert + (weight*(m_joints[jointIdx]->currentJointToWorldTransform*m_joints[jointIdx]->bindWorldToJointTransform*Vector4f(m_mesh.bindVertices[vertIdx], 1)));
		}
		m_mesh.currentVertices[vertIdx] = sumVert.xyz();
	}
}

