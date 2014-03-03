/**
* kmlParse.cpp : ʵ�ֲ�������
* Description������Tiny����kml�ļ�������ʵʱGPS���ݵĴ���
* Lead author�������
* Deputy author���¿�
* Deputy author�������
* Version��3.0 2013.8.24 14:55
*/
	
//ͷ�ļ�
#include"kmlParse.h"

//����ṹ�壬���澭γ����Ϣ
typedef struct coordinate {

	double longitude;		//����
	
	double latitude;			//γ��

	struct coordinate* next;		//ָ����һ���ڵ�

} COORDINATE;

//·�ζ��󣬴���KML��һ��lineString���͵�placemark
typedef struct road {

	bool isExist;		//��ʶ�ö�·�Ƿ����

	string code;		//��·ǰ����ִ�д���

	double disToNextCrossing;		//������һ��·�ڵľ���
    
	double disToLastCrossing;

	double roadLength;					//·�γ���

	int coordinateCount;					//��������

	COORDINATE* coordinates;		//ָ������ͷָ��

	COORDINATE* coordinatesNail;		//ָ������βָ��

} ROAD;

//·����󣬴���kml��Point���͵�Placemark
typedef struct way_point {

	bool isExist;		//�����·���Ƿ����

	COORDINATE coordinate;		//��·�������

	int roadCount;							//��·�������·������

} WAY_POINT;

//·�����󣬽�·�����·�ζ����װ������ͼ�ṹ�����ڽӾ���ķ�ʽ�洢
typedef struct road_network {

	WAY_POINT* wayPoints;		//�洢·�������

	ROAD** roads;						//�Զ�ά����洢����·��

	int pointNum;						//·������

	int roadNum;						//��·����

} ROAD_NETWORK;

/**
*ȫ�ֱ���
*/

TiXmlDocument* document;		//����kml�ļ������documentԪ��
TiXmlElement* folder;				//����kml�ļ�����ĸ�Ԫ��

ROAD_NETWORK* roadnetwork;		//·������
WAY_POINT* waypoints;					//·�����
ROAD* roads;									//·�ζ���

int length = 0;							//placemark������
const double MIN_DIS = 7;		//��λ����λ��ʱ����С�����루��λΪ�ף�

/**
* ����kml�ļ�
* @param fileName	�ļ���
*/
void loadFile(string fileName) {
	//��ʼ��documentԪ��
	document = new TiXmlDocument();
	//�����ļ�
	document->LoadFile(fileName.c_str());//c_str()
}

/**
* ��ʼ����������ȡ�����ļ����
*/
void xmlInitail() {
	//����KML�ļ�
	loadFile("����.KML");
	//��ȡ��Ԫ��
	TiXmlElement* root = document->RootElement();
	//��ȡfolder�ڵ�
	folder = root->FirstChildElement();
}

/**
* �������꣬��������Ϣת��Ϊdoubleֵ
* @param str �����ı���Ϣ
* @param x ת�����double������
*/
void parseCoordinate(string str, double &x) {
	//ͨ���ַ�����ת����������
	stringstream buffer;
	//д���ڴ�
	buffer<<str;
	//���ڴ�д��double
	buffer>>x;
	//�ͷ��ڴ�
	buffer.clear();
}

//��KML�е�Point����placemakrת����·�����
bool transPointFromKML(TiXmlElement* placeMark, WAY_POINT &wayPointNode) {

	//��ȡPoint����
	if(placeMark->FirstChildElement("Point") != NULL) {
		//�½�·��
		wayPointNode = WAY_POINT();
		//��ʼ��roadCountֵ
		wayPointNode.roadCount = 0;
		//��ʼ��isExistֵ
		wayPointNode.isExist = 1;

		//��ȡ����ڵ�
		TiXmlElement* coordinatesElem = placeMark->FirstChildElement("Point")->FirstChildElement("coordinates");
		//��ȡ��������ı�ֵ
		string coordinates = coordinatesElem->GetText();

		int index;	
		//��ʼ������
		wayPointNode.coordinate =  COORDINATE(); //COORDINATE() �޲ι��캯����Ϊ�´��ṹ�帳��ʼֵ

		//ͨ���ָ� '����' ����γ�Ƚ���������indexΪ��������һ�� '����' ���±�
		index = coordinates.find_first_of(',');
		//���ı��ľ���ֵת��Ϊdouble����ṹ��
		parseCoordinate(coordinates.substr(0, index), wayPointNode.coordinate.longitude);
		//ȥ���Ѿ�ʹ�ù����ַ���
		coordinates = coordinates.substr(index + 1, coordinates.size());
		//��������
		index = coordinates.find_first_of(',');
		//���ı���γ��ֵת��Ϊdouble����ṹ��
		parseCoordinate(coordinates.substr(0, index), wayPointNode.coordinate.latitude);	

		return true;
	}

	return false;
}

/**
* ����kml�ļ�����placemark���뱾�ؽṹ��
* @param placeMark kml�ļ���placemark�ڵ�
* @param placemarkNode ָ��ǰ��placemark�ڵ�
*/
bool transRoadFromKML(TiXmlElement* placeMark, ROAD* roadNode) {

	//��ȡLineString����
	if(placeMark->FirstChildElement("LineString") != NULL) {

		//��ȡ����ڵ�
		TiXmlElement* coordinatesElem = placeMark->FirstChildElement("LineString")->FirstChildElement("coordinates");
		//��ȡ����ֵ
		string coordinates = coordinatesElem->GetText();

		int index;
		//��ʼ����С
		roadNode->coordinateCount = 1;
		//��ʼ��disToNextCrossingֵ
		roadNode->disToNextCrossing = 100000;
		//��ʼ������
		roadNode->coordinates = new COORDINATE;
		//����һ��travelִ�б�������
		COORDINATE* travel = roadNode->coordinates;


		//�����������꣬ͨ���ָ� '�ո�' ��������
		while((index = coordinates.find_first_of(' ')) != string::npos) {
			//��ȡ����
			string coordinateStr = coordinates.substr(0, index);
			//�½�����ڵ�
			COORDINATE* coordinateNode = new COORDINATE;

			int split;
			//ͨ���ָ� '����' ������γ��
			split = coordinateStr.find_first_of(',');
			//ת������
			parseCoordinate(coordinateStr.substr(0, split), coordinateNode->longitude);

			coordinateStr = coordinateStr.substr(split + 1, coordinates.size());

			split = coordinateStr.find_first_of(',');
			//ת��γ��
			parseCoordinate(coordinateStr.substr(0, split), coordinateNode->latitude);	

			coordinates = coordinates.substr(index + 1, coordinates.size());
			//���µ�����ڵ����travel
			travel->next = coordinateNode;

			//travel����ƶ�
			travel = travel->next;
			//��������
			roadNode->coordinateCount ++;
		}
		//��ȡ������ '�ո�' ǰ�����꣬�������ʣ�µ�һ������
		int split;
		//�½�һ������ڵ�
		COORDINATE* coordinateNode = new COORDINATE;

		//ͨ�� '����' ������γ��
		split = coordinates.find_first_of(',');
		//��������
		parseCoordinate(coordinates.substr(0, split), coordinateNode->longitude);
	
		coordinates = coordinates.substr(split + 1, coordinates.size());

		split = coordinates.find_first_of(',');
		//����γ��
		parseCoordinate(coordinates.substr(0, split), coordinateNode->latitude);	
		//���ڵ����β��	
		coordinateNode->next = NULL;

		travel->next = coordinateNode;

		//��ʼ��βָ��
		roadNode->coordinatesNail = new COORDINATE;
		//��βָ��ָ�����һ���ڵ�
		roadNode->coordinatesNail->next = coordinateNode;


		//����name����
		if(placeMark->FirstChildElement("name") != NULL) {

			roadNode->code = placeMark->FirstChildElement("name")->GetText();
		}
		//����description����
		if(placeMark->FirstChildElement("description") != NULL) {

			string description = placeMark->FirstChildElement("description")->GetText();
			//��ȡLENGTH����λ��
			string attrName = "<tr><td>LENGTH</td><td>";

			int pos = description.find(attrName);
	
			stringstream buffer;
			//��ȡLENGTH���Զ�Ӧֵ��λ��
			pos += attrName.size();
			//LENGTH��ӦֵΪdouble�ͣ������ַ��жϺ��ı�ֵд���ڴ�
			while((description[pos] >= '0' && description[pos] <= '9') || description[pos] == '.') {
					
				buffer<<description[pos++];
			}
			//ת����double����ṹ������distToNext
			buffer>>roadNode->roadLength;
	
			buffer.clear();
		}

		return true;
	}

	return false;
} 

/**
* �����ع�����KML������ת��Ϊ������Ҫ������
*/
void dataRestruct() {
	//�½�·������
	roadnetwork = new ROAD_NETWORK;
	//�½�100��·��
	roadnetwork->pointNum = 100;
	//��̬�����ڴ�
	roadnetwork->wayPoints = (WAY_POINT*)malloc(sizeof(WAY_POINT) * 100);
	//��ÿ��·���Ӧ��·�η����ڴ�
	roadnetwork->roads = new ROAD*[100];

	for(int i = 0; i < 100; i++) {

		roadnetwork->roads[i] = new ROAD[100];
	}

	//��ȡPlacemark�ڵ�
	TiXmlElement* placeMark = folder->FirstChildElement("Placemark");
	
	int i = 0;
	//����kml�е�placemark�ڵ�
	while(placeMark != NULL) {
		//��point���͵�placemarkת��ΪWAY_POINT����
		if(transPointFromKML(placeMark, roadnetwork->wayPoints[i])) {
			//�¼���һ��·�㣬�����±����
			i++;
		
		}
		//placemark�ڵ����
		placeMark = placeMark->NextSiblingElement();
	}
	//��¼��·����
	roadnetwork->pointNum = i;

	//��ȡPlacemark�ڵ�
	placeMark = folder->FirstChildElement("Placemark");
	//�ٴα���
	while(placeMark != NULL) {
		//��̬����·���ڴ�
		ROAD* roadNode = new ROAD;
		//��kml��LineString���͵�placemarkת��ΪRoad����
		if(transRoadFromKML(placeMark, roadNode)) {

			for(int i = 0; i < roadnetwork->pointNum; i++) {

				for(int j = 0; j < roadnetwork->pointNum; j++) {
					//���Ҹ�·�ε���ʼ������ֹ������Ӧ��·����·���λ��
					if(roadnetwork->wayPoints[i].coordinate.longitude == roadNode->coordinates->next->longitude
						&& roadnetwork->wayPoints[i].coordinate.latitude == roadNode->coordinates->next->latitude
						&& roadnetwork->wayPoints[j].coordinate.longitude == roadNode->coordinatesNail->next->longitude
						&& roadnetwork->wayPoints[j].coordinate.latitude == roadNode->coordinatesNail->next->latitude) {
						//��������·���roadCount��1
						roadnetwork->wayPoints[i].roadCount ++;
					
						roadnetwork->wayPoints[j].roadCount ++;
						//����·�θ�ֵ�� roads[i][j] �� roads[j][i]����ʾ��i,j�±��Ӧ��·���һ�������
						roadnetwork->roads[i][j].code = roadNode->code;
						roadnetwork->roads[i][j].coordinateCount = roadNode->coordinateCount;
						roadnetwork->roads[i][j].coordinates = roadNode->coordinates;
						roadnetwork->roads[i][j].coordinatesNail = roadNode->coordinatesNail;
						roadnetwork->roads[i][j].disToNextCrossing = roadNode->disToNextCrossing;
						roadnetwork->roads[i][j].isExist = 1;
						roadnetwork->roads[i][j].roadLength = roadNode->roadLength;
						
						roadnetwork->roads[j][i].code = roadNode->code;
						roadnetwork->roads[j][i].coordinateCount = roadNode->coordinateCount;
						roadnetwork->roads[j][i].coordinates = roadNode->coordinates;
						roadnetwork->roads[j][i].coordinatesNail = roadNode->coordinatesNail;
						roadnetwork->roads[j][i].disToNextCrossing = roadNode->disToNextCrossing;
						roadnetwork->roads[j][i].isExist = 1;
						roadnetwork->roads[j][i].roadLength = roadNode->roadLength;
						
					}
				}
			}
		}
		//placemark�ڵ����
		placeMark = placeMark->NextSiblingElement();
	}
}
/*
void saveDataToFile() {

	ofstream ocout;

	ocout.open("mapInfo.data");

	ocout.setf(ios::fixed);

	ocout.precision(6);

	stringstream buffer;

	buffer.setf(ios::fixed);

	buffer.precision(6);

	buffer<<roadnetwork->pointNum<<endl;

//	ocout<<roadnetwork->wayPoints[0].coordinate.longitude<<endl;

	for(int i = 0; i < roadnetwork->pointNum; i++) {

		buffer<<roadnetwork->wayPoints[i].coordinate.longitude<<","<<roadnetwork->wayPoints[i].coordinate.latitude<<";";
		buffer<<roadnetwork->wayPoints[i].roadCount<<";";

		for(int j = 0; j < roadnetwork->pointNum; j++) {

			if(roadnetwork->roads[i][j].isExist == 1) {

				buffer<<j<<" "<<roadnetwork->roads[i][j].code<<" "<<roadnetwork->roads[i][j].roadLength<<" "<<roadnetwork->roads[i][j].coordinateCount<<";";

				COORDINATE* coordinateNode = roadnetwork->roads[i][j].coordinates->next;

				while(coordinateNode != NULL) {

					buffer<<coordinateNode->longitude<<","<<coordinateNode->latitude<<" ";

					coordinateNode = coordinateNode->next;
				}
			}
		}

		buffer<<endl;
	}

	ocout<<buffer.str();

	buffer.clear();

	ocout.close();
}
*/
/*
void parseDataFromFile(WAY_POINT wayPoint) {

	ifstream ocin;

	ocin.open("mapInfo.data");

	ocin.setf(ios::fixed);

	ocin.precision(6);

	ocin.getline();

	string dataLine;

	while((ocin.getline(dataLine) != "") {

		int i = dataLine.find_first_of(';');

		string coordinateStr = dataLine.substr(0, i);

		int dot = coordinateStr.find_first_of(',');
	}
}
*/

/**
* ���ݳ�������λ�ö�̬���ص�ǰλ�ø�������Ĳ��ֵ�ͼ��Ϣ
* @param locateRoad ������ǰ����·��
* @param startPointIndex ��·����ʼ���±�
* @param endPointIndex ��·����ֹ���±�
*/
void dynamicLoadMapInfo(ROAD* locateRoad, int startPointIndex, int endPointIndex) {
	//����·�ε���ʼ·��Ĺ���·��������Ϊ1
	roadnetwork->wayPoints[startPointIndex].roadCount = 1;
	//����·�ε���ֹ·��Ĺ���·��������Ϊ1
	roadnetwork->wayPoints[endPointIndex].roadCount = 1;
	//��������·��
	for(int i = 0; i < roadnetwork->pointNum; i++) {
		//���·���Ѵ�����Ϊ��ǰ����ʼ�����ֹ�㣬�򲻴���
		if( roadnetwork->wayPoints[i].isExist 
			&& ((roadnetwork->wayPoints[i].coordinate.longitude == roadnetwork->wayPoints[endPointIndex].coordinate.longitude
			&& roadnetwork->wayPoints[i].coordinate.latitude == roadnetwork->wayPoints[endPointIndex].coordinate.latitude)
			|| (roadnetwork->wayPoints[i].coordinate.longitude == roadnetwork->wayPoints[startPointIndex].coordinate.longitude
			&& roadnetwork->wayPoints[i].coordinate.latitude == roadnetwork->wayPoints[startPointIndex].coordinate.latitude))
			) 
		{
				continue;
		}
		//���ǵ�ǰ��ʼ�㼰��ֹ���·��ȫ����Ϊ�����ڵ�״̬
		roadnetwork->wayPoints[i].isExist = 0;
	}

	for(int j = 0; j < roadnetwork->pointNum; j++) {

		for(int k = 0; k < roadnetwork->pointNum; k++) {
			//���������зǵ�ǰlocateRoad������·����Ϊ�����ڵ�״̬
			if(roadnetwork->roads[j][k].isExist   
				&& (j != startPointIndex && k != startPointIndex) || j != endPointIndex && k != endPointIndex) {

				roadnetwork->roads[j][k].isExist = 0;
			}
		}
	}
	//��ȡKML�е�placemark�ڵ�	
	TiXmlElement* kmlPlacemark = folder->FirstChildElement("Placemark");
	//�����ʼ����Ļ�����
	stringstream startBuffer;
	//�����ֹ����Ļ�����
	stringstream endBuffer;
	//����double����
	startBuffer.setf(ios::fixed);

	startBuffer.precision(6);//���������������ݱ�����С��������λ
	//����ʼ�������뻺����
	startBuffer<<roadnetwork->wayPoints[startPointIndex].coordinate.longitude<<","<<roadnetwork->wayPoints[startPointIndex].coordinate.latitude<<",0";

	endBuffer.setf(ios::fixed);

	endBuffer.precision(6);
	//���յ��������뻺����
	endBuffer<<roadnetwork->wayPoints[endPointIndex].coordinate.longitude<<","<<roadnetwork->wayPoints[endPointIndex].coordinate.latitude<<",0";
	//��ǰ����·����±�
	int pointIndex = 0;

	while(kmlPlacemark != NULL) {
		//���ҿ���·��
		for(; pointIndex < roadnetwork->pointNum; pointIndex++) {

			if(roadnetwork->wayPoints[pointIndex].isExist != 1) {

				break;
			}
		}
		//�����ǰplacemarkΪLineString���ͣ���ʾΪ·�ζ���
		if(kmlPlacemark->FirstChildElement("LineString") != NULL) {

			//��ȡ����ڵ�
			TiXmlElement* coordinatesElem = kmlPlacemark->FirstChildElement("LineString")->FirstChildElement("coordinates");
			//��ȡ��LineString������ֵ
			string coordinateStr = coordinatesElem->GetText();

			int index = coordinateStr.find_first_of(' ');
			//ȡ��һ������
			string firstCoordinateStr = coordinateStr.substr(0, index);

			index = coordinateStr.find_last_of(' ');
			//ȡ���һ������
			string lastCoordinateStr = coordinateStr.substr(index + 1, coordinateStr.size());
			//����������궼����ʼ���겻ͬ����������������һ������ֹ������ͬ�������ҵ�����ֹ�����һ������·��
			if(firstCoordinateStr.compare(startBuffer.str()) != 0 && lastCoordinateStr.compare(startBuffer.str()) != 0
				&& (firstCoordinateStr.compare(endBuffer.str()) == 0 || lastCoordinateStr.compare(endBuffer.str()) == 0)) {

				ROAD* roadNode = new ROAD;
				//����LineStringת��ΪRoad����
				transRoadFromKML(kmlPlacemark, roadNode);
				//����road����·�������λ��
				roadnetwork->wayPoints[endPointIndex].roadCount ++;
				roadnetwork->wayPoints[pointIndex].roadCount ++;

				roadnetwork->roads[endPointIndex][pointIndex].code = roadNode->code;
				roadnetwork->roads[endPointIndex][pointIndex].coordinateCount = roadNode->coordinateCount;
				roadnetwork->roads[endPointIndex][pointIndex].coordinates = roadNode->coordinates;
				roadnetwork->roads[endPointIndex][pointIndex].coordinatesNail = roadNode->coordinatesNail;
				roadnetwork->roads[endPointIndex][pointIndex].disToNextCrossing = roadNode->disToNextCrossing;
				roadnetwork->roads[endPointIndex][pointIndex].isExist = 1;
				roadnetwork->roads[endPointIndex][pointIndex].roadLength = roadNode->roadLength;
						
				roadnetwork->roads[pointIndex][endPointIndex].code = roadNode->code;
				roadnetwork->roads[pointIndex][endPointIndex].coordinateCount = roadNode->coordinateCount;
				roadnetwork->roads[pointIndex][endPointIndex].coordinates = roadNode->coordinates;
				roadnetwork->roads[pointIndex][endPointIndex].coordinatesNail = roadNode->coordinatesNail;
				roadnetwork->roads[pointIndex][endPointIndex].disToNextCrossing = roadNode->disToNextCrossing;
				roadnetwork->roads[pointIndex][endPointIndex].isExist = 1;
				roadnetwork->roads[pointIndex][endPointIndex].roadLength = roadNode->roadLength;

				string otherCoordinateStr;
				//����ǵ�һ����������ֹ������ȣ���ȡ���һ������
				if(firstCoordinateStr.compare(endBuffer.str()) == 0) {

					index = coordinateStr.find_last_of(' ');

					otherCoordinateStr = coordinateStr.substr(index + 1, coordinateStr.size());
				//��������һ����������ֹ������ȣ���ȡ��һ������
				} else {

					index = coordinateStr.find_first_of(' ');

					otherCoordinateStr = coordinateStr.substr(0, index);
				} 
				//��ջ�����
				startBuffer.clear();

				endBuffer.clear();

				TiXmlElement* kmlPoint = folder->FirstChildElement("Placemark");
				//�ٴα���������Point�ڵ�
				while(kmlPoint != NULL) {

					if(kmlPoint->FirstChildElement("Point") != NULL) {

						//��ȡ����ڵ�
						TiXmlElement* coordinatesElem = kmlPoint->FirstChildElement("Point")->FirstChildElement("coordinates");
						//��ȡ�������
						string coordinatePointStr = coordinatesElem->GetText();
						//�����������������·�ε���һ��������ͬ�������ҵ���·�ε���һ������·��
						if(coordinatePointStr.compare(otherCoordinateStr) == 0) {
							//����·��ת����WAY_POINT
							transPointFromKML(kmlPoint, roadnetwork->wayPoints[pointIndex]);

							roadnetwork->wayPoints[pointIndex].roadCount ++;

							break;
						}
					}
					//����
					kmlPoint = kmlPoint->NextSiblingElement();
				}
			}
		}
		//����
		kmlPlacemark = kmlPlacemark->NextSiblingElement();
	}
}

/**
* ����һ��·�����˵Ĺؼ���ֱ���һ·�ں���һ·�ڵľ���
* �������ξ���ֱ���뵽��·�νṹ���е�roadnetwork->roads[i][j].disToNextCrossing��roadnetwork->roads[i][j].disToLastCrossing��Ա��
*/
void caculateDistanceToCross()
{  
	//�ṹ��COORDINATE��ָ�����ָ��ǰ·�ε���һ����·�ε�ĩ�˵�
	COORDINATE* lastRoadCoordinateNail;


	////ָ��ǰ·�ε���һ����·�ε�ʼ�˵�
	//COORDINATE* nextRoadCoordinateHead;

	for(int i=0; i < roadnetwork->pointNum; i++)
	{   
		for(int j=0; j < roadnetwork->pointNum; j++)
		{
			//��ǰ·��ʼ�˵㵽��һ·�ڵľ���
			double disToNextCross = 0;
			//��ǰ·��ĩ�˵㵽��һ·�ڵľ���
			double disToLastCross = 0;
			//��ǰ�˵���ڣ�Ѱ�ҿ�ִ�в�����ͬ�����ڵ�·��
			if(roadnetwork->roads[i][j].isExist) 
			{   
				//����ǰ·�εĳ��ȼ���disToNextCross��
				disToNextCross += roadnetwork->roads[i][j].roadLength;
				//����ǰ·�ε�ĩ�˵���Ϊ��һ·�ε�ĩ�˵�
				lastRoadCoordinateNail = roadnetwork->roads[i][j].coordinatesNail->next;
				for(int x = j; x < roadnetwork->pointNum; x++)
				{
					for(int y= 0; y < roadnetwork->pointNum; y++)
					{
						//�����͵�ǰ·����ĩ�˵���ͬ���෴��·����������ѭ��
						if(i == x && j == y /*|| i == y && j == x */)
						{
						   continue;
						}
						//�����һ·�δ���
						if(roadnetwork->roads[x][y].isExist)
						{
							//��һ·�ε��׶˵����һ·�ε�ĩ�˵���ͬ
							if( roadnetwork->roads[x][y].coordinates->next->latitude == lastRoadCoordinateNail->latitude &&
								roadnetwork->roads[x][y].coordinates->next->longitude == lastRoadCoordinateNail->longitude )
							{  
								//��·�εĿ�ִ�в�����ͬ
								if(roadnetwork->roads[i][j].code.compare(roadnetwork->roads[x][y].code) == 0) //compare() �ַ����ȽϺ��� ǰ��·�ε�code�ڴ洢�Ŀ�ִ�в�����ͬΪ0
								{
									//����һ·�εĳ��ۼ���disToNextCross
									disToNextCross += roadnetwork->roads[x][y].roadLength;
									//��¼����һ·��β�˵����������ε���һ·��β�ε㲢����ѭ��
									lastRoadCoordinateNail->longitude = roadnetwork->roads[x][y].coordinatesNail->next->longitude;
									lastRoadCoordinateNail->latitude = roadnetwork->roads[x][y].coordinatesNail->next->latitude;
									break;
								}
							}
							//������һ·�ε�β�˵�����һ·�ε�ĩ�˵���ͬ
							else if( roadnetwork->roads[x][y].coordinatesNail->next->latitude == lastRoadCoordinateNail->latitude &&
								     roadnetwork->roads[x][y].coordinatesNail->next->longitude == lastRoadCoordinateNail->longitude )
							{
								//ͬ��
								if(roadnetwork->roads[i][j].code.find(roadnetwork->roads[x][y].code) != string::npos) //compare() �ַ����ȽϺ��� ǰ��·�ε�code�ڴ洢�Ŀ�ִ�в�����ͬΪ0
								{
									disToNextCross += roadnetwork->roads[x][y].roadLength;
									lastRoadCoordinateNail->longitude = roadnetwork->roads[x][y].coordinates->next->longitude;
									lastRoadCoordinateNail->latitude = roadnetwork->roads[x][y].coordinates->next->latitude;
									break;
								}
							}
						 }
					 }
				 }     
			}
			//������ĵ�ǰ·�ξ���·�ڵľ�������·�νṹ����Ӧ��Ա��
			roadnetwork->roads[i][j].disToNextCrossing = disToNextCross * 1000; //��λ��
			//roadnetwork->roads[i][j].disToLastCrossing = disToLastCross * 1000;

		}
	}
}

/*
* ����������γ������֮��ľ��룬��ʽΪ��
* MLatA A��γ�ȣ�MLonA A�ľ��ȣ�MlatB B��γ�ȣ�MlonB B�ľ���
* C = sin(MLatA)*sin(MLatB)*cos(MLonA-MLonB) + cos(MLatA)*cos(MLatB)
* Distance = R*Arccos(C)*Pi/180
* @param latA A��γ��
* @param lonA A�㾭��
* @param latB B��γ��
* @param lonB B�㾭��
* return ���ؾ���ֵ
*/
double distBetweenPoints(double latA, double lonA, double latB, double lonB) {
	//����pi
	const double PI = 3.1415926535897932384626433832795;
	//�������뾶����λΪ��
	const double R = 6371004;
	//��ʽ
	double C = sin(latA) * sin(latB) * cos(lonA - lonB) + cos(latA) * cos(latB);

	double distance = R * acos(C) * PI / 180;

	return distance;
}

/**
* �������õ���ʵʱGPS��Ϣ����λ����������ͼ����㼰����·��
* gps_coordinate GPS��λ�õ��ĵ�
* map_coordinate GPS��������ͼ�е�����ƥ���õ��ĵ㣬��ָ�뷵����ṹ��洢��ַ
* locateRoad GPS��������ͼ�е�����ƥ���õ�����������·�Σ���ָ�뷵����ṹ��洢��ַ
*/
bool matchGPSCoordinateFromMap(COORDINATE gps_coordinate ,COORDINATE* &map_coordinate ,ROAD* &locateRoad)
{
	double max = 7;//����֮������������λ��  
	double min_distance = 1000;//��ͼ�ϵĵ㵽 gps��ľ���
	double dis = 1000;//��ͼ�ϵĵ��gps��֮��ľ��루���ں�min_distance�Ƚϳ���С�ģ�
	for(int i = 0;i<roadnetwork->pointNum;i++)
		for(int j = 0;j<roadnetwork->pointNum;j++)
		{
			if(roadnetwork->roads[i][j].isExist == 1)
			{
				if(roadnetwork->wayPoints[i].isExist && roadnetwork->wayPoints[j].isExist)
				{
					if( roadnetwork->wayPoints[i].coordinate.latitude>gps_coordinate.latitude && roadnetwork->wayPoints[j].coordinate.latitude<gps_coordinate.latitude
						|| roadnetwork->wayPoints[i].coordinate.latitude<gps_coordinate.latitude && roadnetwork->wayPoints[j].coordinate.latitude>gps_coordinate.latitude
						|| roadnetwork->wayPoints[i].coordinate.longitude>gps_coordinate.longitude && roadnetwork->wayPoints[j].coordinate.longitude<gps_coordinate.longitude
						|| roadnetwork->wayPoints[i].coordinate.longitude<gps_coordinate.longitude && roadnetwork->wayPoints[j].coordinate.longitude>gps_coordinate.longitude
					   )  //���GPS�����꾭γ����ĳһ·�����˵����귶Χ�ڣ��������ڻ�����γ�����ڣ���ִ�бȶ�
					{
						while(roadnetwork->roads[i][j].coordinates->next != NULL)//�����ȶ�
						{
							dis = distBetweenPoints(roadnetwork->roads[i][j].coordinates->next->latitude,roadnetwork->roads[i][j].coordinates->next->longitude, gps_coordinate.latitude,gps_coordinate.longitude);
							if(min_distance>dis)//�ҳ���С����
							{			
								min_distance = dis;
								//��ȡ��ƥ�䵽��·�μ�����
								map_coordinate =  roadnetwork->roads[i][j].coordinates->next;
								locateRoad = roadnetwork->roads[i] + j; //��roadnetwork->roads[i][j]�ص�ַ����ָ�����locateRoad
							}
							roadnetwork->roads[i][j].coordinates->next = roadnetwork->roads[i][j].coordinates->next->next;
						}
					}
				}	
			}
		}
	if(min_distance < max)
		return true;//����ģ��ƥ��ɹ���������
	else
		return false;//���ؿ�˵��ƫ��������

}

/**
*��ѯǰ����·��Ϣ�������ظ���Ϣ
* @param gps_latitude ʵʱGPSγ��
* @param gps_longitude ʵʱGPS����
* @param road ����λ�õ��ĸõ��ڵ�ͼ��������·��
* @param locateCoordinate ����λ�õ��ĵ�ͼ�ϵ����ڽ��ĵ�
* @param disToNextCross ������һ��·�ڵľ��룬����
* @param disToLastCross ������һ��·�ڵľ��룬����
* @param next_lane_name ��һ·�ڵĿ�ִ�в���������
* @paramlast_lane_name ��һ·�ڵĿ�ִ�в���������
*/

void searchAheadRoadInfo(double gps_latitude, double gps_longitude, ROAD* road, COORDINATE* locateCoordinate,
				int &disToNextCross/*, int &disToLastCross*/, char &next_lane_name/*, string &last_lane_name*/)
{   
	//�洢��ǰ�����������
	double lat = gps_latitude;
	double lon = gps_longitude;
	//����·�ڵľ���
	double next_dist = 0; 
	//double last_dist = 0; 
	//����֮��ľ���
	double dist = 0; 
	//��ȡ��ǰ������ǰ��
	COORDINATE* coordinateToNext = locateCoordinate->next;
	while(coordinateToNext->next  != NULL)
	{
			//��������֮��ľ���
			dist = distBetweenPoints(lat, lon, coordinateToNext ->latitude, coordinateToNext ->longitude);
			//�ۻ�����
			next_dist += dist;
			//����һ�ε�ĩ��ľ�γ������Ϊ��һ�ε�ʼ�㾭γ��
			lat = coordinateToNext ->latitude;
			lon = coordinateToNext ->longitude;
			//���Ƶõ���һ�ε�ĩ�㾭γ��
			coordinateToNext  = coordinateToNext ->next;
	}
	//��ô˿̾���ִ��name�������ľ���
	next_dist = road->disToNextCrossing - road->roadLength*1000 + next_dist;
	//ǿ��ת��Ϊint��
	disToNextCross = (int) next_dist;
	//ǰ����Ҫִ�еĲ���
	if(road->code.find("4") != string::npos) 
	{
		next_lane_name =  '4';
	}
	else if(road->code.find("a") != string::npos)
	{
		next_lane_name =  'a';
	}
}

int main() {
	
	cout.setf(ios::fixed);
	
	//��ʼ��xml�ļ�
	xmlInitail();
	
	dataRestruct();

//	saveDataToFile();

	for(int i = 0; i < roadnetwork->pointNum; i++) {

		for(int j = 0; j < roadnetwork->pointNum; j++) {

			if(roadnetwork->roads[i][j].isExist == 1) {

				cout<<"road_form_a_to_b "<<i<<" "<<j<<endl;
			}
		}
	}

	dynamicLoadMapInfo(&roadnetwork->roads[1][21], 1, 21);

	for(int i = 0; i < roadnetwork->pointNum; i++) {

		if(roadnetwork->wayPoints[i].isExist == 1) {

			cout<<"coordinate<x,y> "<<roadnetwork->wayPoints[i].coordinate.longitude<<" "<<roadnetwork->wayPoints[i].coordinate.latitude<<endl;

			cout<<"road_count "<<roadnetwork->wayPoints[i].roadCount<<endl;

			for(int j = 0; j < roadnetwork->pointNum; j++) {

				if(roadnetwork->roads[i][j].isExist == 1) {

					cout<<"road_length "<<roadnetwork->roads[i][j].roadLength<<endl;
					
//				COORDINATE* coordinateNode = roadnetwork->roads[i][j].coordinatesNail->next;

//				while(coordinateNode != NULL) {

//					cout<<coordinateNode->longitude<<" "<<coordinateNode->latitude<<endl;
							
//					coordinateNode = coordinateNode->pre;
					}
				}
			}
		
//		ROAD* roadnode = roadnetwork->wayPoints[i].roads->next;

		}

		cout<<endl;
//	}

    caculateDistanceToCross();

	COORDINATE* map_coordinate = NULL;
	COORDINATE gps_coordinate;
	gps_coordinate.latitude = 39.906010;
	gps_coordinate.longitude = 116.295921;
	ROAD* locateRoad = NULL;
	if(matchGPSCoordinateFromMap(gps_coordinate,map_coordinate,locateRoad))
	{
		cout<<endl<<"match_successful"<<endl;
		cout<<"map_coordinate<x,y> "<<map_coordinate->latitude<<"	"<<map_coordinate->longitude<<endl;
		cout<<"disToNextCrossing "<<locateRoad->disToNextCrossing<<"  name "<<locateRoad->code<<endl<<endl;
		cout<<"nail_coordinate<x,y>"<<locateRoad->coordinatesNail->next->longitude<<"   "<<locateRoad->coordinatesNail->next->latitude<<endl;

		int distonextcross = 0;
		char next_lane_name = ' ';
		searchAheadRoadInfo(gps_coordinate.latitude , gps_coordinate.longitude, locateRoad , map_coordinate ,
							distonextcross, next_lane_name);
		cout<<"distance_to_next_cross_and_name "<<distonextcross<<"  "<<next_lane_name<<endl;
	}
	else
		cout<<endl<<"match_failed"<<endl;

	system("pause");

		return 0;
}