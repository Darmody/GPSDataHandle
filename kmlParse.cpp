/**
* kmlParse.cpp : 实现操作方法
* Description：利用Tiny解析kml文件，及与实时GPS数据的处理
* Lead author：蔡寰宇
* Deputy author：陈宽
* Deputy author：李聚升
* Version：3.0 2013.8.24 14:55
*/
	
//头文件
#include"kmlParse.h"

//坐标结构体，保存经纬度信息
typedef struct coordinate {

	double longitude;		//经度
	
	double latitude;			//纬度

	struct coordinate* next;		//指向下一个节点

} COORDINATE;

//路段对象，代表KML里一个lineString类型的placemark
typedef struct road {

	bool isExist;		//标识该段路是否存在

	string code;		//该路前方可执行代码

	double disToNextCrossing;		//距离下一个路口的距离
    
	double disToLastCrossing;

	double roadLength;					//路段长度

	int coordinateCount;					//坐标数量

	COORDINATE* coordinates;		//指向坐标头指针

	COORDINATE* coordinatesNail;		//指向坐标尾指针

} ROAD;

//路点对象，代表kml里Point类型的Placemark
typedef struct way_point {

	bool isExist;		//代表该路点是否存在

	COORDINATE coordinate;		//该路点的坐标

	int roadCount;							//该路点关联的路段数量

} WAY_POINT;

//路网对象，将路点对象，路段对象封装成无向图结构，以邻接矩阵的方式存储
typedef struct road_network {

	WAY_POINT* wayPoints;		//存储路点的数组

	ROAD** roads;						//以二维数组存储所有路段

	int pointNum;						//路点数量

	int roadNum;						//道路数量

} ROAD_NETWORK;

/**
*全局变量
*/

TiXmlDocument* document;		//解析kml文件所需的document元素
TiXmlElement* folder;				//解析kml文件所需的根元素

ROAD_NETWORK* roadnetwork;		//路网对象
WAY_POINT* waypoints;					//路点对象
ROAD* roads;									//路段对象

int length = 0;							//placemark的数量
const double MIN_DIS = 7;		//定位车辆位置时的最小误差距离（单位为米）

/**
* 加载kml文件
* @param fileName	文件名
*/
void loadFile(string fileName) {
	//初始化document元素
	document = new TiXmlDocument();
	//加载文件
	document->LoadFile(fileName.c_str());//c_str()
}

/**
* 初始化操作，获取解析文件入口
*/
void xmlInitail() {
	//加载KML文件
	loadFile("车道.KML");
	//获取根元素
	TiXmlElement* root = document->RootElement();
	//获取folder节点
	folder = root->FirstChildElement();
}

/**
* 解析坐标，将坐标信息转换为double值
* @param str 坐标文本信息
* @param x 转换后的double型坐标
*/
void parseCoordinate(string str, double &x) {
	//通过字符串流转换数据类型
	stringstream buffer;
	//写入内存
	buffer<<str;
	//从内存写入double
	buffer>>x;
	//释放内存
	buffer.clear();
}

//将KML中的Point类型placemakr转换成路点对象
bool transPointFromKML(TiXmlElement* placeMark, WAY_POINT &wayPointNode) {

	//获取Point属性
	if(placeMark->FirstChildElement("Point") != NULL) {
		//新建路点
		wayPointNode = WAY_POINT();
		//初始化roadCount值
		wayPointNode.roadCount = 0;
		//初始化isExist值
		wayPointNode.isExist = 1;

		//获取坐标节点
		TiXmlElement* coordinatesElem = placeMark->FirstChildElement("Point")->FirstChildElement("coordinates");
		//获取到坐标的文本值
		string coordinates = coordinatesElem->GetText();

		int index;	
		//初始化坐标
		wayPointNode.coordinate =  COORDINATE(); //COORDINATE() 无参构造函数，为新创结构体赋初始值

		//通过分割 '逗号' 将经纬度解析出来，index为解析出第一个 '逗号' 的下标
		index = coordinates.find_first_of(',');
		//将文本的经度值转换为double存入结构体
		parseCoordinate(coordinates.substr(0, index), wayPointNode.coordinate.longitude);
		//去除已经使用过的字符串
		coordinates = coordinates.substr(index + 1, coordinates.size());
		//继续解析
		index = coordinates.find_first_of(',');
		//将文本的纬度值转换为double存入结构体
		parseCoordinate(coordinates.substr(0, index), wayPointNode.coordinate.latitude);	

		return true;
	}

	return false;
}

/**
* 解析kml文件，将placemark存入本地结构体
* @param placeMark kml文件中placemark节点
* @param placemarkNode 指向当前的placemark节点
*/
bool transRoadFromKML(TiXmlElement* placeMark, ROAD* roadNode) {

	//获取LineString属性
	if(placeMark->FirstChildElement("LineString") != NULL) {

		//获取坐标节点
		TiXmlElement* coordinatesElem = placeMark->FirstChildElement("LineString")->FirstChildElement("coordinates");
		//获取属性值
		string coordinates = coordinatesElem->GetText();

		int index;
		//初始化大小
		roadNode->coordinateCount = 1;
		//初始化disToNextCrossing值
		roadNode->disToNextCrossing = 100000;
		//初始化坐标
		roadNode->coordinates = new COORDINATE;
		//声明一个travel执行遍历操作
		COORDINATE* travel = roadNode->coordinates;


		//遍历所有坐标，通过分割 '空格' 解析坐标
		while((index = coordinates.find_first_of(' ')) != string::npos) {
			//获取坐标
			string coordinateStr = coordinates.substr(0, index);
			//新建坐标节点
			COORDINATE* coordinateNode = new COORDINATE;

			int split;
			//通过分割 '逗号' 解析经纬度
			split = coordinateStr.find_first_of(',');
			//转换经度
			parseCoordinate(coordinateStr.substr(0, split), coordinateNode->longitude);

			coordinateStr = coordinateStr.substr(split + 1, coordinates.size());

			split = coordinateStr.find_first_of(',');
			//转换纬度
			parseCoordinate(coordinateStr.substr(0, split), coordinateNode->latitude);	

			coordinates = coordinates.substr(index + 1, coordinates.size());
			//将新的坐标节点接入travel
			travel->next = coordinateNode;

			//travel向后移动
			travel = travel->next;
			//长度自增
			roadNode->coordinateCount ++;
		}
		//获取了所有 '空格' 前的坐标，处理最后剩下的一个坐标
		int split;
		//新建一个坐标节点
		COORDINATE* coordinateNode = new COORDINATE;

		//通过 '逗号' 解析经纬度
		split = coordinates.find_first_of(',');
		//解析经度
		parseCoordinate(coordinates.substr(0, split), coordinateNode->longitude);
	
		coordinates = coordinates.substr(split + 1, coordinates.size());

		split = coordinates.find_first_of(',');
		//解析纬度
		parseCoordinate(coordinates.substr(0, split), coordinateNode->latitude);	
		//将节点接入尾部	
		coordinateNode->next = NULL;

		travel->next = coordinateNode;

		//初始化尾指针
		roadNode->coordinatesNail = new COORDINATE;
		//将尾指针指向最后一个节点
		roadNode->coordinatesNail->next = coordinateNode;


		//解析name属性
		if(placeMark->FirstChildElement("name") != NULL) {

			roadNode->code = placeMark->FirstChildElement("name")->GetText();
		}
		//解析description属性
		if(placeMark->FirstChildElement("description") != NULL) {

			string description = placeMark->FirstChildElement("description")->GetText();
			//获取LENGTH属性位置
			string attrName = "<tr><td>LENGTH</td><td>";

			int pos = description.find(attrName);
	
			stringstream buffer;
			//获取LENGTH属性对应值的位置
			pos += attrName.size();
			//LENGTH对应值为double型，进行字符判断后将文本值写入内存
			while((description[pos] >= '0' && description[pos] <= '9') || description[pos] == '.') {
					
				buffer<<description[pos++];
			}
			//转换成double存入结构体属性distToNext
			buffer>>roadNode->roadLength;
	
			buffer.clear();
		}

		return true;
	}

	return false;
} 

/**
* 数据重构，将KML的数据转换为我们需要的数据
*/
void dataRestruct() {
	//新建路网对象
	roadnetwork = new ROAD_NETWORK;
	//新建100个路点
	roadnetwork->pointNum = 100;
	//动态分配内存
	roadnetwork->wayPoints = (WAY_POINT*)malloc(sizeof(WAY_POINT) * 100);
	//给每个路点对应的路段分配内存
	roadnetwork->roads = new ROAD*[100];

	for(int i = 0; i < 100; i++) {

		roadnetwork->roads[i] = new ROAD[100];
	}

	//获取Placemark节点
	TiXmlElement* placeMark = folder->FirstChildElement("Placemark");
	
	int i = 0;
	//遍历kml中的placemark节点
	while(placeMark != NULL) {
		//将point类型的placemark转换为WAY_POINT对象
		if(transPointFromKML(placeMark, roadnetwork->wayPoints[i])) {
			//新加了一个路点，所以下标后移
			i++;
		
		}
		//placemark节点后移
		placeMark = placeMark->NextSiblingElement();
	}
	//记录总路点数
	roadnetwork->pointNum = i;

	//获取Placemark节点
	placeMark = folder->FirstChildElement("Placemark");
	//再次遍历
	while(placeMark != NULL) {
		//动态分配路段内存
		ROAD* roadNode = new ROAD;
		//将kml中LineString类型的placemark转换为Road对象
		if(transRoadFromKML(placeMark, roadNode)) {

			for(int i = 0; i < roadnetwork->pointNum; i++) {

				for(int j = 0; j < roadnetwork->pointNum; j++) {
					//查找该路段的起始点与终止点所对应的路网中路点的位置
					if(roadnetwork->wayPoints[i].coordinate.longitude == roadNode->coordinates->next->longitude
						&& roadnetwork->wayPoints[i].coordinate.latitude == roadNode->coordinates->next->latitude
						&& roadnetwork->wayPoints[j].coordinate.longitude == roadNode->coordinatesNail->next->longitude
						&& roadnetwork->wayPoints[j].coordinate.latitude == roadNode->coordinatesNail->next->latitude) {
						//将这两个路点的roadCount加1
						roadnetwork->wayPoints[i].roadCount ++;
					
						roadnetwork->wayPoints[j].roadCount ++;
						//将该路段赋值给 roads[i][j] 及 roads[j][i]，表示与i,j下标对应的路点的一条无向边
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
		//placemark节点后移
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
* 根据车辆所处位置动态加载当前位置附近所需的部分地图信息
* @param locateRoad 车辆当前所处路段
* @param startPointIndex 该路段起始点下标
* @param endPointIndex 该路段终止点下标
*/
void dynamicLoadMapInfo(ROAD* locateRoad, int startPointIndex, int endPointIndex) {
	//将该路段的起始路点的关联路段数量置为1
	roadnetwork->wayPoints[startPointIndex].roadCount = 1;
	//将该路段的终止路点的关联路段数量置为1
	roadnetwork->wayPoints[endPointIndex].roadCount = 1;
	//遍历所有路点
	for(int i = 0; i < roadnetwork->pointNum; i++) {
		//如果路点已存在且为当前的起始点或终止点，则不处理
		if( roadnetwork->wayPoints[i].isExist 
			&& ((roadnetwork->wayPoints[i].coordinate.longitude == roadnetwork->wayPoints[endPointIndex].coordinate.longitude
			&& roadnetwork->wayPoints[i].coordinate.latitude == roadnetwork->wayPoints[endPointIndex].coordinate.latitude)
			|| (roadnetwork->wayPoints[i].coordinate.longitude == roadnetwork->wayPoints[startPointIndex].coordinate.longitude
			&& roadnetwork->wayPoints[i].coordinate.latitude == roadnetwork->wayPoints[startPointIndex].coordinate.latitude))
			) 
		{
				continue;
		}
		//将非当前起始点及终止点的路段全部置为不存在的状态
		roadnetwork->wayPoints[i].isExist = 0;
	}

	for(int j = 0; j < roadnetwork->pointNum; j++) {

		for(int k = 0; k < roadnetwork->pointNum; k++) {
			//遍历将所有非当前locateRoad的所有路段置为不存在的状态
			if(roadnetwork->roads[j][k].isExist   
				&& (j != startPointIndex && k != startPointIndex) || j != endPointIndex && k != endPointIndex) {

				roadnetwork->roads[j][k].isExist = 0;
			}
		}
	}
	//获取KML中的placemark节点	
	TiXmlElement* kmlPlacemark = folder->FirstChildElement("Placemark");
	//存放起始坐标的缓冲区
	stringstream startBuffer;
	//存放终止坐标的缓冲区
	stringstream endBuffer;
	//设置double精度
	startBuffer.setf(ios::fixed);

	startBuffer.precision(6);//缓存区浮点型数据保留至小数点后第六位
	//将起始坐标输入缓冲区
	startBuffer<<roadnetwork->wayPoints[startPointIndex].coordinate.longitude<<","<<roadnetwork->wayPoints[startPointIndex].coordinate.latitude<<",0";

	endBuffer.setf(ios::fixed);

	endBuffer.precision(6);
	//将终点坐标输入缓冲区
	endBuffer<<roadnetwork->wayPoints[endPointIndex].coordinate.longitude<<","<<roadnetwork->wayPoints[endPointIndex].coordinate.latitude<<",0";
	//当前可用路点的下标
	int pointIndex = 0;

	while(kmlPlacemark != NULL) {
		//查找可用路点
		for(; pointIndex < roadnetwork->pointNum; pointIndex++) {

			if(roadnetwork->wayPoints[pointIndex].isExist != 1) {

				break;
			}
		}
		//如果当前placemark为LineString类型，表示为路段对象
		if(kmlPlacemark->FirstChildElement("LineString") != NULL) {

			//获取坐标节点
			TiXmlElement* coordinatesElem = kmlPlacemark->FirstChildElement("LineString")->FirstChildElement("coordinates");
			//获取该LineString的坐标值
			string coordinateStr = coordinatesElem->GetText();

			int index = coordinateStr.find_first_of(' ');
			//取第一个坐标
			string firstCoordinateStr = coordinateStr.substr(0, index);

			index = coordinateStr.find_last_of(' ');
			//取最后一个坐标
			string lastCoordinateStr = coordinateStr.substr(index + 1, coordinateStr.size());
			//如果两个坐标都与起始坐标不同，而两个坐标中有一个与终止坐标相同，表明找到了终止坐标的一条关联路段
			if(firstCoordinateStr.compare(startBuffer.str()) != 0 && lastCoordinateStr.compare(startBuffer.str()) != 0
				&& (firstCoordinateStr.compare(endBuffer.str()) == 0 || lastCoordinateStr.compare(endBuffer.str()) == 0)) {

				ROAD* roadNode = new ROAD;
				//将该LineString转换为Road对象
				transRoadFromKML(kmlPlacemark, roadNode);
				//将该road存入路点关联的位置
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
				//如果是第一个坐标与终止坐标相等，读取最后一个坐标
				if(firstCoordinateStr.compare(endBuffer.str()) == 0) {

					index = coordinateStr.find_last_of(' ');

					otherCoordinateStr = coordinateStr.substr(index + 1, coordinateStr.size());
				//如果是最后一个坐标与终止坐标相等，读取第一个坐标
				} else {

					index = coordinateStr.find_first_of(' ');

					otherCoordinateStr = coordinateStr.substr(0, index);
				} 
				//清空缓冲区
				startBuffer.clear();

				endBuffer.clear();

				TiXmlElement* kmlPoint = folder->FirstChildElement("Placemark");
				//再次遍历，查找Point节点
				while(kmlPoint != NULL) {

					if(kmlPoint->FirstChildElement("Point") != NULL) {

						//获取坐标节点
						TiXmlElement* coordinatesElem = kmlPoint->FirstChildElement("Point")->FirstChildElement("coordinates");
						//获取点的坐标
						string coordinatePointStr = coordinatesElem->GetText();
						//如果点坐标与搜索到路段的另一端坐标相同，表明找到这路段的另一个关联路点
						if(coordinatePointStr.compare(otherCoordinateStr) == 0) {
							//将该路点转换成WAY_POINT
							transPointFromKML(kmlPoint, roadnetwork->wayPoints[pointIndex]);

							roadnetwork->wayPoints[pointIndex].roadCount ++;

							break;
						}
					}
					//后移
					kmlPoint = kmlPoint->NextSiblingElement();
				}
			}
		}
		//后移
		kmlPlacemark = kmlPlacemark->NextSiblingElement();
	}
}

/**
* 计算一条路段两端的关键点分别到下一路口和上一路口的距离
* 将这两段距离分别存入到该路段结构体中的roadnetwork->roads[i][j].disToNextCrossing和roadnetwork->roads[i][j].disToLastCrossing成员上
*/
void caculateDistanceToCross()
{  
	//结构体COORDINATE型指针变量指向当前路段的上一相邻路段的末端点
	COORDINATE* lastRoadCoordinateNail;


	////指向当前路段的下一相邻路段的始端点
	//COORDINATE* nextRoadCoordinateHead;

	for(int i=0; i < roadnetwork->pointNum; i++)
	{   
		for(int j=0; j < roadnetwork->pointNum; j++)
		{
			//当前路段始端点到下一路口的距离
			double disToNextCross = 0;
			//当前路段末端点到上一路口的距离
			double disToLastCross = 0;
			//当前端点存在，寻找可执行操作相同且相邻的路段
			if(roadnetwork->roads[i][j].isExist) 
			{   
				//将当前路段的长度加入disToNextCross中
				disToNextCross += roadnetwork->roads[i][j].roadLength;
				//将当前路段的末端点作为上一路段的末端点
				lastRoadCoordinateNail = roadnetwork->roads[i][j].coordinatesNail->next;
				for(int x = j; x < roadnetwork->pointNum; x++)
				{
					for(int y= 0; y < roadnetwork->pointNum; y++)
					{
						//遇到和当前路段首末端点相同或相反的路段跳出本轮循环
						if(i == x && j == y /*|| i == y && j == x */)
						{
						   continue;
						}
						//如果下一路段存在
						if(roadnetwork->roads[x][y].isExist)
						{
							//下一路段的首端点和上一路段的末端点相同
							if( roadnetwork->roads[x][y].coordinates->next->latitude == lastRoadCoordinateNail->latitude &&
								roadnetwork->roads[x][y].coordinates->next->longitude == lastRoadCoordinateNail->longitude )
							{  
								//两路段的可执行操作相同
								if(roadnetwork->roads[i][j].code.compare(roadnetwork->roads[x][y].code) == 0) //compare() 字符串比较函数 前后路段的code内存储的可执行操作相同为0
								{
									//将下一路段的长累加入disToNextCross
									disToNextCross += roadnetwork->roads[x][y].roadLength;
									//记录的上一路段尾端点下移至本次的下一路段尾段点并跳出循环
									lastRoadCoordinateNail->longitude = roadnetwork->roads[x][y].coordinatesNail->next->longitude;
									lastRoadCoordinateNail->latitude = roadnetwork->roads[x][y].coordinatesNail->next->latitude;
									break;
								}
							}
							//否则，下一路段的尾端点与上一路段的末端点相同
							else if( roadnetwork->roads[x][y].coordinatesNail->next->latitude == lastRoadCoordinateNail->latitude &&
								     roadnetwork->roads[x][y].coordinatesNail->next->longitude == lastRoadCoordinateNail->longitude )
							{
								//同上
								if(roadnetwork->roads[i][j].code.find(roadnetwork->roads[x][y].code) != string::npos) //compare() 字符串比较函数 前后路段的code内存储的可执行操作相同为0
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
			//将计算的当前路段距离路口的距离存入该路段结构体相应成员中
			roadnetwork->roads[i][j].disToNextCrossing = disToNextCross * 1000; //单位米
			//roadnetwork->roads[i][j].disToLastCrossing = disToLastCross * 1000;

		}
	}
}

/*
* 计算两个经纬度坐标之间的距离，公式为：
* MLatA A的纬度，MLonA A的经度，MlatB B的纬度，MlonB B的经度
* C = sin(MLatA)*sin(MLatB)*cos(MLonA-MLonB) + cos(MLatA)*cos(MLatB)
* Distance = R*Arccos(C)*Pi/180
* @param latA A点纬度
* @param lonA A点经度
* @param latB B点纬度
* @param lonB B点经度
* return 返回距离值
*/
double distBetweenPoints(double latA, double lonA, double latB, double lonB) {
	//定义pi
	const double PI = 3.1415926535897932384626433832795;
	//定义地球半径，单位为米
	const double R = 6371004;
	//公式
	double C = sin(latA) * sin(latB) * cos(lonA - lonB) + cos(latA) * cos(latB);

	double distance = R * acos(C) * PI / 180;

	return distance;
}

/**
* 根据所得到的实时GPS信息，定位车辆所处地图坐标点及所处路段
* gps_coordinate GPS定位得到的点
* map_coordinate GPS坐标点与地图中的坐标匹配后得到的点，用指针返回其结构体存储地址
* locateRoad GPS坐标点与地图中的坐标匹配后得到的坐标所在路段，用指针返回其结构体存储地址
*/
bool matchGPSCoordinateFromMap(COORDINATE gps_coordinate ,COORDINATE* &map_coordinate ,ROAD* &locateRoad)
{
	double max = 7;//两点之间的最大间隔，单位米  
	double min_distance = 1000;//地图上的点到 gps点的距离
	double dis = 1000;//地图上的点和gps点之间的距离（用于和min_distance比较出最小的）
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
					   )  //如果GPS点坐标经纬度在某一路段两端的坐标范围内（经度在内或者是纬度在内），执行比对
					{
						while(roadnetwork->roads[i][j].coordinates->next != NULL)//挨个比对
						{
							dis = distBetweenPoints(roadnetwork->roads[i][j].coordinates->next->latitude,roadnetwork->roads[i][j].coordinates->next->longitude, gps_coordinate.latitude,gps_coordinate.longitude);
							if(min_distance>dis)//找出最小距离
							{			
								min_distance = dis;
								//获取所匹配到的路段及坐标
								map_coordinate =  roadnetwork->roads[i][j].coordinates->next;
								locateRoad = roadnetwork->roads[i] + j; //将roadnetwork->roads[i][j]地地址赋给指针变量locateRoad
							}
							roadnetwork->roads[i][j].coordinates->next = roadnetwork->roads[i][j].coordinates->next->next;
						}
					}
				}	
			}
		}
	if(min_distance < max)
		return true;//返回模糊匹配成功，返回真
	else
		return false;//返回空说明偏离误差过大

}

/**
*查询前方道路信息，并返回该信息
* @param gps_latitude 实时GPS纬度
* @param gps_longitude 实时GPS经度
* @param road 经定位得到的该点在地图上所处的路段
* @param locateCoordinate 经定位得到的地图上的最邻近的点
* @param disToNextCross 距离下一个路口的距离，返回
* @param disToLastCross 距离上一个路口的距离，返回
* @param next_lane_name 下一路口的可执行操作，返回
* @paramlast_lane_name 上一路口的可执行操作，返回
*/

void searchAheadRoadInfo(double gps_latitude, double gps_longitude, ROAD* road, COORDINATE* locateCoordinate,
				int &disToNextCross/*, int &disToLastCross*/, char &next_lane_name/*, string &last_lane_name*/)
{   
	//存储当前车辆的坐标点
	double lat = gps_latitude;
	double lon = gps_longitude;
	//距离路口的距离
	double next_dist = 0; 
	//double last_dist = 0; 
	//两点之间的距离
	double dist = 0; 
	//获取当前坐标点的前驱
	COORDINATE* coordinateToNext = locateCoordinate->next;
	while(coordinateToNext->next  != NULL)
	{
			//计算两点之间的距离
			dist = distBetweenPoints(lat, lon, coordinateToNext ->latitude, coordinateToNext ->longitude);
			//累积距离
			next_dist += dist;
			//将上一次的末点的经纬度设置为下一次的始点经纬度
			lat = coordinateToNext ->latitude;
			lon = coordinateToNext ->longitude;
			//下移得到下一次的末点经纬度
			coordinateToNext  = coordinateToNext ->next;
	}
	//获得此刻距需执行name操作处的距离
	next_dist = road->disToNextCrossing - road->roadLength*1000 + next_dist;
	//强制转换为int型
	disToNextCross = (int) next_dist;
	//前方所要执行的操作
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
	
	//初始化xml文件
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