#ifndef __LAPLACIANDEFORMATION_H__
#define __LAPLACIANDEFORMATION_H__

#include <Eigen/Dense>
#include "MeshDefinition.h"
#include <map>
#include <QFileDialog>
#include <fstream>
#include <iostream>
#include "point3d.h"

class LaplacianDeformation{
public:
	LaplacianDeformation(Mesh *m);
	~LaplacianDeformation();

	void GenerateLaplacianMatrix();
	//void GenerateLaplacianMatrix2();
	void delta(Mesh::VertexIter vertexIndex, Point3d &position);
	void GenerateAnchorUnitMatrixAndAnchorPosition();
	void calcNewPoint();
	void updatePoint();
	bool ReadAnchorPoints();
	void setMeshInfo(Mesh *m);
	void smoothFunc();

private:
	double lamda;
	Mesh *mesh;
	Eigen::MatrixXd laplacianMatrix;//������˹����
	Eigen::MatrixXd AnchorUnitMatrix;//ѡ��ĵ��01
	Eigen::MatrixXd AnchorPosition;//ѡԼ���ĵ������
	Eigen::MatrixXd V_dNew;//�µ����������
	Eigen::MatrixXd A;//�����е�A����
	Eigen::MatrixXd b;//�����е�b����
	std::map<int, OpenMesh::Vec3d> mapVec;//���� idx ��Ӧ������
	std::vector<int> fixPointIdx; // �̶��� bumpy
	int editPointIdx; // �ƶ��� Ĭ��Ϊ 240 idx
	int anchorVerticesNum;
	int verticesNum;// ��������ӵ�еĵ�ĸ���

	//�����µ�������˹����
	Eigen::MatrixXd AL;
	Eigen::MatrixXd IL;
	Eigen::MatrixXd DL;
};

#endif
