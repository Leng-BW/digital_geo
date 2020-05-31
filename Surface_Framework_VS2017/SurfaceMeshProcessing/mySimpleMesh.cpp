#include "mySimplemesh.h"

/**************************************************
@brief   : ���캯��
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
SimpleMesh::SimpleMesh(Mesh *m) {
	mesh = m;
}

/**************************************************
@brief   : ��������
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
SimpleMesh::~SimpleMesh() {}



/**************************************************
@brief   : �趨mesh
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
void SimpleMesh::setMesh(Mesh *m){
	mesh = m;
	heap.clear();
}

/**************************************************
@brief   : ���������� v^{T} Q v
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
double SimpleMesh::quadric(const Point3d& v, const Eigen::MatrixXd &M_) {
	return M_(0, 0) * v.getX() * v.getX() +
		2.f*M_(0, 1) * v.getX() * v.getY() +
		2.f*M_(0, 2) * v.getX() * v.getZ() +
		2.f*M_(0, 3) * v.getX() +
		M_(1, 1)*v.getY() * v.getY() +
		2.f*M_(1, 2)*v.getY() * v.getZ() +
		2.f*M_(1, 3)*v.getY() +
		M_(2, 2)*v.getZ() * v.getZ() +
		2.f*M_(2, 3)*v.getZ() +
		M_(3, 3);
}
double SimpleMesh::quadric(const Mesh::Point& v, const Eigen::MatrixXd &M_)
{
	return M_(0, 0)*v[0] * v[0] +
		2.f*M_(0, 1)*v[0] * v[1] +
		2.f*M_(0, 2)*v[0] * v[2] +
		2.f*M_(0, 3)*v[0] +
		M_(1, 1)*v[1] * v[1] +
		2.f*M_(1, 2)*v[1] * v[2] +
		2.f*M_(1, 3)*v[1] +
		M_(2, 2)*v[2] * v[2] +
		2.f*M_(2, 3)*v[2] +
		M_(3, 3);
}


/**************************************************
@brief   : ����Q����
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
void SimpleMesh::computeQ() {
	for (Mesh::VertexIter v_it = mesh->vertices_begin(); v_it != mesh->vertices_end(); v_it++) {// �������е�
		mesh->data(*v_it).Ksum.resize(4, 4);
		std::vector<Mesh::Point> v;
		for (Mesh::VertexVertexIter vv_it = mesh->vv_iter(*v_it); vv_it.is_valid(); vv_it++) { // ĳ�����һ�����
			v.push_back(mesh->point(*vv_it));
		}
		Mesh::Point P(mesh->point(*v_it)); // ���ĵ�
		for (int i = 0; i < v.size(); i++) {
			Mesh::Point pre = (i == 0 ? v[v.size() - 1] : v[i - 1]); // ��һ����
			Mesh::Point pi = v[i]; // ��ǰ��
			double a, b, c, d;
			// �������������γɵ�ƽ��ϵ��
			myMeshAlgorithm.calABCD(Point3d(P[0], P[1], P[2]), Point3d(pi[0], pi[1], pi[2]), Point3d(pre[0], pre[1], pre[2]), a, b, c, d); 			
			Eigen::MatrixXd Kp;
			Kp.resize(4, 4);
			Kp(0, 0) = a * a;  Kp(0, 1) = a * b;  Kp(0, 2) = a * c;  Kp(0, 3) = a * d;
			Kp(1, 0) = a * b;  Kp(1, 1) = b * b;  Kp(1, 2) = b * c;  Kp(1, 3) = b * d;
			Kp(2, 0) = a * c;  Kp(2, 1) = c * b;  Kp(2, 2) = c * c;  Kp(2, 3) = c * d;
			Kp(3, 0) = a * d;  Kp(3, 1) = d * b;  Kp(3, 2) = d * c;  Kp(3, 3) = d * d;
			mesh->data(*v_it).Ksum += Kp;
			mesh->data(*v_it).deleted = false;// ��Ĭ��û�б�ɾ��
		}
	}// ÿ�������Kpֵ�Ѿ��������
	// ����
	for (Mesh::VertexIter v_it = mesh->vertices_begin(); v_it != mesh->vertices_end(); v_it++) {
		for (Mesh::VertexVertexIter vv_it = mesh->vv_iter(*v_it); vv_it.is_valid(); vv_it++) {
			Eigen::MatrixXd bar_Q;
			bar_Q = mesh->data(*v_it).Ksum + mesh->data(*vv_it).Ksum;
			if (v_it->idx() < vv_it->idx()) {
				double error = 0;
				Eigen::MatrixXd ba_Q_back = bar_Q;
				ba_Q_back(3, 0) = 0; ba_Q_back(3, 1) = 0;
				ba_Q_back(3, 2) = 0; ba_Q_back(3, 3) = 1;
				if (ba_Q_back.determinant() != 0) {// ����ʽ��ֵ �����Ϊ0�Ļ�˵��������ʽ�ǿ����
					//����ʽ���� �����µ������
					Eigen::MatrixXd b;
					b << 0,
						 0,
						 0,
						 1;
					Eigen::MatrixXd v_ = ba_Q_back.inverse() * b; // �����е�(1) ��ʽ
					Mesh::Point v_bar(v_(0, 0), v_(1, 0), v_(2, 0));// �õ��µĵ�
					error = quadric(v_bar, bar_Q);
					MyPair tmp(v_it, vv_it, error);
					tmp.optimal_pos = v_bar;
					tmp.optimal_nrm = mesh->normal(*v_it);
					heap.insert(tmp);
				}
				else {
					Mesh::Point verticesMid = 0.5f*(mesh->point(*v_it) + mesh->point(*vv_it));
					error = quadric(verticesMid, bar_Q);

					MyPair tmp(v_it, vv_it, error);
					tmp.optimal_pos = verticesMid;
					tmp.optimal_nrm = (mesh->normal(*v_it) + mesh->normal(*vv_it)) / norm(mesh->normal(*v_it) + mesh->normal(*vv_it));
					float e = quadric(mesh->point(*v_it), bar_Q);
					if (e < tmp.error) {
						tmp.error = e;
						tmp.optimal_pos = mesh->point(*v_it);
						tmp.optimal_nrm = mesh->normal(*v_it);
					}
					e = quadric(mesh->point(*vv_it), bar_Q);
					if (e < tmp.error) {
						tmp.error = e;
						tmp.optimal_pos = mesh->point(*vv_it);
						tmp.optimal_nrm = mesh->normal(*vv_it);
					}
					heap.insert(tmp);
				}
			}
		}
	}
}


/**************************************************
@brief   : ����������
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
double SimpleMesh::norm(const Mesh::Point &point) {
	Mesh::Point p = point * point;
	return std::sqrt(p[0] + p[1] + p[2]);
}



/**************************************************
@brief   : ����� �㷨��ʼ����
@author  : lee
@input   ��none
@output  ��none
@time    : none
**************************************************/
bool SimpleMesh::simpleMesh(QString str) {
	std::string strStr = str.toStdString();
	std::stringstream ss;
	ss << strStr;
	int times;
	ss >> times;
	std::cout << "[DEBUG]" << strStr << std::endl;
	std::cout << "[DEBUG] updatePoint start\n";
	while (!heap.empty() && times) {
		MyPair minCostPair = *(heap.begin());
		std::cout << "[DEBUG] error" << minCostPair.error << std::endl;
		std::cout << "[DEBUG] valid" << minCostPair.v_it->is_valid() << " " << minCostPair.vv_it->is_valid() << std::endl;
		mesh->set_point(*minCostPair.v_it, minCostPair.optimal_pos);
		mesh->set_normal(*minCostPair.v_it, minCostPair.optimal_nrm);
		mesh->data(*minCostPair.v_it).Ksum += mesh->data(*minCostPair.vv_it).Ksum;
		// ɾ����ĳ������Ӧ���γ��µ��߼��Է����ջ���������й�vv_it&v_it���߼���Ӧ�ö�ɾ��
		mesh->data(*minCostPair.vv_it).deleted = true;// ��Ĭ��û�б�ɾ��
		int deletePointIdx1 = minCostPair.vv_it->idx();
		int deletePointIdx2 = minCostPair.v_it->idx();
		heap.erase(heap.begin());
		// ɾ��vv_it v_itΪ�׵Ķ�������
		for (auto hit = heap.begin(); hit != heap.end() && !heap.empty();) { // ������Щ��vv_it�ǿյ�
			if(hit->v_it->is_valid() && hit->vv_it->is_valid())
				if (
					hit->v_it->idx() == deletePointIdx1 || hit->vv_it->idx() == deletePointIdx2
					|| hit->v_it->idx() == deletePointIdx2 || hit->vv_it->idx() == deletePointIdx1) {
					hit=heap.erase(hit); // ɾ��������
				}
				else {
					hit++;
				}
			else
				hit = heap.erase(hit);
		}
		// �����ӵ��� �����߼���
		for (Mesh::VertexVertexIter vv_it = mesh->vv_iter(*minCostPair.v_it); vv_it.is_valid(); vv_it++) { // ��v_it��1��������ж� ���Ƿ��ܼ����ջ��
			if (mesh->data(*vv_it).deleted == true)
				continue;
			Eigen::MatrixXd bar_Q;
			bar_Q = mesh->data(*minCostPair.v_it).Ksum + mesh->data(*vv_it).Ksum;
			if (minCostPair.v_it->idx() < vv_it->idx()) { // ���ִ����ǰ��
				double error = 0;
				Eigen::MatrixXd ba_Q_back = bar_Q;
				ba_Q_back(3, 0) = 0; ba_Q_back(3, 1) = 0;
				ba_Q_back(3, 2) = 0; ba_Q_back(3, 3) = 1;
				if (ba_Q_back.determinant() != 0) {// ����ʽ��ֵ �����Ϊ0�Ļ�˵��������ʽ�ǿ����
					//����ʽ���� �����µ������
					Eigen::MatrixXd b;
					b << 0,
						 0,
						 0,
						 1;
					Eigen::MatrixXd v_ = ba_Q_back.inverse() * b; // �����е�(1) ��ʽ
					Mesh::Point v_bar(v_(0, 0), v_(1, 0), v_(2, 0));// �õ��µĵ�
					error = quadric(v_bar, bar_Q);
					MyPair tmp(minCostPair.v_it, vv_it, error);
					tmp.optimal_pos = v_bar;
					tmp.optimal_nrm = mesh->normal(*minCostPair.v_it);
					heap.insert(tmp);
				}
				else {
					Mesh::Point verticesMid = 0.5f*(mesh->point(*minCostPair.v_it) + mesh->point(*vv_it));
					error = quadric(verticesMid, bar_Q);

					MyPair tmp(minCostPair.v_it, vv_it, error);
					tmp.optimal_pos = verticesMid;
					tmp.optimal_nrm = (mesh->normal(*minCostPair.v_it) + mesh->normal(*vv_it)) / norm(mesh->normal(*minCostPair.v_it) + mesh->normal(*vv_it));
					float e = quadric(mesh->point(*minCostPair.v_it), bar_Q);
					if (e < tmp.error) {
						tmp.error = e;
						tmp.optimal_pos = mesh->point(*minCostPair.v_it);
						tmp.optimal_nrm = mesh->normal(*minCostPair.v_it);
					}
					e = quadric(mesh->point(*vv_it), bar_Q);
					if (e < tmp.error) {
						tmp.error = e;
						tmp.optimal_pos = mesh->point(*vv_it);
						tmp.optimal_nrm = mesh->normal(*vv_it);
					}
					heap.insert(tmp);
				}
			}
			else {
				double error = 0;
				Eigen::MatrixXd ba_Q_back = bar_Q;
				ba_Q_back(3, 0) = 0; ba_Q_back(3, 1) = 0;
				ba_Q_back(3, 2) = 0; ba_Q_back(3, 3) = 1;
				if (ba_Q_back.determinant() != 0) {// ����ʽ��ֵ �����Ϊ0�Ļ�˵��������ʽ�ǿ����
					//����ʽ���� �����µ������
					Eigen::MatrixXd b;
					b << 0,
						0,
						0,
						1;
					Eigen::MatrixXd v_ = ba_Q_back.inverse() * b; // �����е�(1) ��ʽ
					Mesh::Point v_bar(v_(0, 0), v_(1, 0), v_(2, 0));// �õ��µĵ�
					error = quadric(v_bar, bar_Q);
					MyPair tmp(vv_it, minCostPair.v_it, error);
					tmp.optimal_pos = v_bar;
					tmp.optimal_nrm = mesh->normal(*vv_it);
					heap.insert(tmp);
				}
				else {
					Mesh::Point verticesMid = 0.5f*(mesh->point(*minCostPair.v_it) + mesh->point(*vv_it));
					error = quadric(verticesMid, bar_Q);

					MyPair tmp(vv_it, minCostPair.v_it, error);
					tmp.optimal_pos = verticesMid;
					tmp.optimal_nrm = (mesh->normal(*minCostPair.v_it) + mesh->normal(*vv_it)) / norm(mesh->normal(*minCostPair.v_it) + mesh->normal(*vv_it));
					float e = quadric(mesh->point(*minCostPair.v_it), bar_Q);
					if (e < tmp.error) {
						tmp.error = e;
						tmp.optimal_pos = mesh->point(*minCostPair.v_it);
						tmp.optimal_nrm = mesh->normal(*minCostPair.v_it);
					}
					e = quadric(mesh->point(*vv_it), bar_Q);
					if (e < tmp.error) {
						tmp.error = e;
						tmp.optimal_pos = mesh->point(*vv_it);
						tmp.optimal_nrm = mesh->normal(*vv_it);
					}
					heap.insert(tmp);
				}
			}
		}
		// ɾ��һ����
		//deleteOnePoint(minCostPair.vv_it);
		chooseOneToDel(*minCostPair.vv_it, *minCostPair.v_it);
		times--;
		std::cout << " --- \n";
	}
	// ɾ������Ϊtrue�ĵ�

	//�����е��غϵĵ�ɾ��
	//std::map<Mesh::Point, std::vector<Mesh::VertexHandle> > mp;
	//for (auto v_it = mesh->vertices_begin(); v_it != mesh->vertices_end(); v_it++) {
	//	mp[mesh->point(v_it)].push_back(v_it.handle());
	//}
	//int count = 0;
	//for (auto it = mp.begin(); it != mp.end(); it++) {
	//	std::cout << "[DEBUG] point size " << it->second.size() << std::endl;
	//	if (it->second.size() > 1) {
	//		count += it->second.size() - 1;
	//	}
	//}
	//for (int i = 0; i < count; i++) {
	//	std::map<Mesh::Point, std::vector<Mesh::VertexHandle> > mp;
	//	for (auto v_it = mesh->vertices_begin(); v_it != mesh->vertices_end(); v_it++) {
	//		mp[mesh->point(v_it)].push_back(v_it.handle());
	//		if (mp[mesh->point(v_it)].size() > 1) {
	//			chooseOneToDel(mp[mesh->point(v_it)][1], mp[mesh->point(v_it)][0]);
	//			break;
	//		}
	//	}
	//}
	std::cout << "[DEBUG] OVER" << std::endl;
	if (!mesh->has_vertex_status())  mesh->request_vertex_status();
	if (!mesh->has_face_status())    mesh->request_face_status();
	if (!mesh->has_edge_status())    mesh->request_edge_status();
	mesh->garbage_collection();
	if (mesh->has_vertex_status())  mesh->release_vertex_status();
	if (mesh->has_face_status())    mesh->release_face_status();
	if (mesh->has_edge_status())    mesh->release_edge_status();
	return true;
}


/**************************************************
@brief   : ɾ������
@author  : lee
@input   ��delete_vh Ҫɾ���Ķ��������
@output  ��none
@time    : none
**************************************************/
bool SimpleMesh::deleteOnePoint(Mesh::VertexHandle v_it) {
	//if (!mesh->has_vertex_status())  mesh->request_vertex_status();
	//if (!mesh->has_face_status())    mesh->request_face_status();
	//if (!mesh->has_edge_status())    mesh->request_edge_status();

	//for (auto vit = mesh->vertices_begin(); vit != mesh->vertices_end(); vit++)
	{
		//if (vit == v_it) {
			mesh->delete_vertex(v_it, true);//�Ƿ�Թ�������֮��ִ��ɾ����true�Զ���false�ֶ�ɾ��
		//}
	}

	//mesh->garbage_collection();//��������
	//if (mesh->has_vertex_status())  mesh->release_vertex_status();
	//if (mesh->has_face_status())    mesh->release_face_status();
	//if (mesh->has_edge_status())    mesh->release_edge_status();
	return true;
}


void SimpleMesh::chooseOneToDel(Mesh::VertexHandle v_l, Mesh::VertexHandle v_r)
{
	//�õ����ҵ��һ�������
	int l_n = mesh->valence(v_l);//�ж��ٸ��ڽӵ�
	std::vector<Mesh::VertexHandle> vec_l;
	for (auto vv_it = mesh->vv_iter(v_l); vv_it.is_valid(); vv_it++) {
		vec_l.push_back(*vv_it);
	}

	deleteOnePoint(v_l);
	int j = 0;
	//std::cout<<"one ring:"<<vec_l.size()<<std::endl;
	for (int i = 0; i < vec_l.size(); i++) {
		if (i == (vec_l.size() - 1)) {
			j = 0;
		}
		else {
			j = i + 1;
		}
		if (vec_l[i] != v_r && vec_l[j] != v_r) {
			mesh->add_face(vec_l[i], v_r, vec_l[j]);
		}
	}
}
