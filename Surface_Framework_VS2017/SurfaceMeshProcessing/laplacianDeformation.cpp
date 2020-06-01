#include "laplacianDeformation.h"


/**************************************************
@brief   : ���캯��
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
LaplacianDeformation::LaplacianDeformation(Mesh *m) {
	mesh = m;
	lamda = 0.01;//�����ĳ���
	for (auto it = mesh->vertices_begin(); it != mesh->vertices_end(); it++) {
		mapVec[it->idx()] = mesh->point(*it);
	}
	verticesNum = mesh->n_vertices();
}


/**************************************************
@brief   : ���������Ϣ ���ƹ��캯��
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
void LaplacianDeformation::setMeshInfo(Mesh *m) {
	mesh = m;
	lamda = 0.01;//�����ĳ���
	mapVec.clear();
	for (auto it = mesh->vertices_begin(); it != mesh->vertices_end(); it++) {
		mapVec[it->idx()] = mesh->point(*it);
	}
	verticesNum = mesh->n_vertices();
}


/**************************************************
@brief   : ��������
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
LaplacianDeformation::~LaplacianDeformation() {}

/**************************************************
@brief   : ��json�ļ��ж�ȡ�̶���  δ������ϣ� �༭�㣿
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
bool LaplacianDeformation::ReadAnchorPoints() {
	QString fileName = QFileDialog::getOpenFileName(NULL,
		QObject::tr("Open fixPoint file"),
		QObject::tr(""),
		QObject::tr("txt Files (*.txt);;"
			"All Files (*)"));
	std::string strFileName = fileName.toStdString();
	std::ifstream inFile;
	if (strFileName != "") {
		inFile.open(strFileName.c_str());
	}
	else {
		return false;
	}
	int tmp;
	while (inFile >> tmp) {
		fixPointIdx.push_back(tmp);
	}
	anchorVerticesNum = fixPointIdx.size() + 1; // �����˹̶��� & �༭��
	editPointIdx = 240; // Ĭ�ϱ༭��
	return true;
}

/**************************************************
@brief   : �����㷨��
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
void LaplacianDeformation::smoothFunc() {
	//if (!ReadAnchorPoints()) {
	//	std::cout << "Error not choose fix point" << std::endl;
	//	exit(0);
	//}
	GenerateLaplacianMatrix();
	GenerateAnchorUnitMatrixAndAnchorPosition();
	calcNewPoint();
	updatePoint();
}

/**************************************************
@brief   : ����������˹����
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
void LaplacianDeformation::GenerateLaplacianMatrix() {
	laplacianMatrix.resize(verticesNum, verticesNum);//�������Ŵ�С
	for (Mesh::VertexIter vertexIter = mesh->vertices_begin(); vertexIter != mesh->vertices_end(); vertexIter++) {
		for (Mesh::VertexIter vertexIter1 = mesh->vertices_begin(); vertexIter1 != mesh->vertices_end(); vertexIter1++) {
			if (vertexIter->idx() == vertexIter1->idx()) {
				laplacianMatrix(vertexIter->idx(), vertexIter1->idx()) = 1;
			}
			else {
				bool connected = false;
				int count = 0;
				for (Mesh::VertexVertexIter vv_it = mesh->vv_iter(*vertexIter); vv_it.is_valid(); vv_it++) {
					if (vv_it->idx() == vertexIter1->idx()) {//˵��������ͨ
						connected = true;
					}
					count++;//�ڽӵ�ĸ���
				}
				if (connected) {
					laplacianMatrix(vertexIter->idx(), vertexIter1->idx()) = -1.0 / (double)count;
				}
				else {
					laplacianMatrix(vertexIter->idx(), vertexIter1->idx()) = 0;
				}
			}
		}
	}
	std::cout << "GenerateLaplacianMatrix finish\n";
}

/**************************************************
@brief   : ����ê�㣨�̶��㣩  ���е㶼��ê����� linux �ϵĳ���ѡ���ĵ������
		   �󲿷ֵĵ㶼Ӧ���ǹ̶��㣬ֻ���ٲ��ֵĵ���ǿ��Ա��εĵ�
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
void LaplacianDeformation::GenerateAnchorUnitMatrixAndAnchorPosition() {
	// ѡȡ ����̶���
	int count = 0;
	int index = 0;
	editPointIdx = 240; // Ĭ�ϱ༭��
	Mesh::Point editPoint = mapVec[240];// �õ��༭�������
	// �������༭��֮��ľ��룬����������0.3 ��ô���ǹ̶���
	std::vector<Mesh::Point> v;
	std::vector<int> cc;// ��¼idx
	for (Mesh::VertexIter vertexIter = mesh->vertices_begin(); vertexIter != mesh->vertices_end(); vertexIter++, count++) {
		if ((editPoint - mesh->point(*vertexIter)).length() > 2) {
			v.push_back(mesh->point(*vertexIter));
			cc.push_back(vertexIter->idx());
		}
	}
	anchorVerticesNum = cc.size() + 1;
	AnchorPosition.resize(cc.size() + 1, 3);
	AnchorUnitMatrix.resize(cc.size() + 1, verticesNum);
	for (int i = 0; i < (cc.size() + 1); i++) {
		for (int j = 0; j < verticesNum; j++) {
			AnchorUnitMatrix(i, j) = 0;//����ȫ����ʼ��Ϊ0
		}
	}
	for (int i = 0; i < cc.size(); i++) {
		Mesh::Point tmpP = v[i];
		AnchorUnitMatrix(i, cc[i]) = 1;
		AnchorPosition(i, 0) = tmpP[0];
		AnchorPosition(i, 1) = tmpP[1];
		AnchorPosition(i, 2) = tmpP[2];
	}
	// �༭��
	AnchorUnitMatrix(cc.size(), editPointIdx) = 1;
	AnchorPosition(cc.size(), 0) = editPoint[0];
	AnchorPosition(cc.size(), 1) = editPoint[1];
	AnchorPosition(cc.size(), 2) = editPoint[2] + 5; // �α�
	std::cout << "GenerateAnchorUnitMatrixAndAnchorPosition finish\n";
}

/**************************************************
@brief   : �����µĵ�
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
void LaplacianDeformation::calcNewPoint() {
	A.resize(verticesNum + anchorVerticesNum, verticesNum);
	for (int i = 0; i < verticesNum; i++) {
		A.row(i) = laplacianMatrix.row(i);
	}
	for (int i = 0; i < anchorVerticesNum; i++) {
		A.row(i + verticesNum) = AnchorUnitMatrix.row(i);
	}
	b.resize(verticesNum + anchorVerticesNum, 3);
	int index = 0;
	for (auto v_it = mesh->vertices_begin(); v_it != mesh->vertices_end(); v_it++, index++) {
		Point3d position(0, 0, 0);
		delta(v_it, position);
		b(index, 0) = position.getX();
		b(index, 1) = position.getY();
		b(index, 2) = position.getZ();
	}
	for (int i = 0; i < anchorVerticesNum; i++) {
		b(verticesNum + i, 0) = AnchorPosition(i, 0);
		b(verticesNum + i, 1) = AnchorPosition(i, 1);
		b(verticesNum + i, 2) = AnchorPosition(i, 2);
	}
	//��ʼ���������������
	V_dNew.resize(verticesNum, 3);
	V_dNew = (A.transpose()*A).llt().solve(A.transpose()*b);
	std::cout << "calcNewPoint finish\n";
}



/**************************************************
@brief   : ���µ�����
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
void LaplacianDeformation::updatePoint() {
	int count = 0;
	for (Mesh::VertexIter vertexIter = mesh->vertices_begin(); vertexIter != mesh->vertices_end(); vertexIter++, count++) {
		OpenMesh::Vec3d point(V_dNew(count, 0), V_dNew(count, 1), V_dNew(count, 2));
		mesh->set_point(*vertexIter, point);
	}
	std::cout << "updatePoint finish\n";
}

/**************************************************
@brief   : ����΢������
		   Ĭ��Ȩ��Ϊ1
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
void LaplacianDeformation::delta(Mesh::VertexIter vertexIndex, Point3d &position) {
	int adjacentPointNum = 0;
	position.x = 0;
	position.y = 0;
	position.z = 0;
	Point3d rlt(0, 0, 0);
	//�ҵ������������ĸ���
	for (Mesh::VertexVertexIter vv_it = mesh->vv_iter(*vertexIndex); vv_it.is_valid(); ++vv_it) {//������������еİ�ߵ�����
		OpenMesh::Vec3d tmp0 = mesh->point(*vertexIndex);
		OpenMesh::Vec3d tmp1 = mesh->point(*vv_it);
		rlt.x += tmp0[0] - tmp1[0];
		rlt.y += tmp0[1] - tmp1[1];
		rlt.z += tmp0[2] - tmp1[2];
		adjacentPointNum++;
	}
	position.x = rlt.x / adjacentPointNum;
	position.y = rlt.y / adjacentPointNum;
	position.z = rlt.z / adjacentPointNum;
}
